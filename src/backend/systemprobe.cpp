// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemprobe.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include <algorithm>

namespace
{
constexpr int ProcessTimeoutMs = 3000;
constexpr qsizetype MaximumProbeOutput = 256 * 1024;

using CameraType = SystemStateSnapshot::CameraType;
using CapabilityStatus = SystemStateSnapshot::CapabilityStatus;
using DaemonStatus = SystemStateSnapshot::DaemonStatus;
using EngineStatus = SystemStateSnapshot::EngineStatus;
using PamStatus = SystemStateSnapshot::PamStatus;
using ProfileStatus = SystemStateSnapshot::ProfileStatus;
using SecurityTier = SystemStateSnapshot::SecurityTier;
using SecureBootStatus = SystemStateSnapshot::SecureBootStatus;

struct CommandResult
{
    bool started = false;
    bool finished = false;
    int exitCode = -1;
    QString standardOutput;
};

QString translate(const char *text)
{
    return QCoreApplication::translate("SystemProbe", text);
}

QByteArray readBoundedFile(const QString &path, qsizetype maximumBytes = MaximumProbeOutput)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return file.read(maximumBytes);
}

CommandResult runReadOnlyCommand(const QString &program, const QStringList &arguments)
{
    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    process.setProcessEnvironment(environment);
    process.setProgram(program);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(QIODevice::ReadOnly);

    CommandResult result;
    result.started = process.waitForStarted(ProcessTimeoutMs);
    if (!result.started)
    {
        return result;
    }
    result.finished = process.waitForFinished(ProcessTimeoutMs);
    if (!result.finished)
    {
        process.kill();
        process.waitForFinished(1000);
        return result;
    }
    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput().left(MaximumProbeOutput));
    return result;
}

QString packageVersion(const QString &package)
{
    const QString rpm = QStringLiteral("/usr/bin/rpm");
    if (!QFileInfo(rpm).isExecutable())
    {
        return {};
    }
    const CommandResult result =
        runReadOnlyCommand(rpm, {QStringLiteral("-q"), QStringLiteral("--qf"), QStringLiteral("%{VERSION}"), package});
    if (!result.finished || result.exitCode != 0)
    {
        return {};
    }

    const QString version = result.standardOutput.trimmed();
    static const QRegularExpression safeVersion(QStringLiteral(R"(\A[A-Za-z0-9._+~:-]{1,64}\z)"));
    return safeVersion.match(version).hasMatch() ? version : QString();
}

QString normalizedVersion(const QString &output)
{
    static const QRegularExpression versionPattern(
        QStringLiteral(R"((?:^|\s)(\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?)(?:\s|$))"));
    const auto match = versionPattern.match(output.trimmed());
    return match.hasMatch() ? match.captured(1) : QString();
}

bool supportedIrlumeVersion(const QString &version)
{
    static const QRegularExpression supported(QStringLiteral(R"(\A0\.6\.\d+(?:-[0-9A-Za-z.-]+)?\z)"));
    return supported.match(version).hasMatch();
}

QString displayManagerLabel(const QString &target)
{
    const QString service = QFileInfo(target).fileName().toLower();
    if (service.contains(QStringLiteral("plasmalogin")) || service.contains(QStringLiteral("plasma-login-manager")))
    {
        return QStringLiteral("Plasma Login Manager");
    }
    if (service.contains(QStringLiteral("sddm")))
    {
        return QStringLiteral("SDDM");
    }
    if (service.isEmpty())
    {
        return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unsupported (%1)").arg(service.left(64));
}

PamStatus pamStatusFromOutput(const QString &output, const QString &displayManager)
{
    QStringList serviceNames;
    if (displayManager == QStringLiteral("Plasma Login Manager"))
    {
        serviceNames = {QStringLiteral("plasmalogin"), QStringLiteral("/kde")};
    }
    else if (displayManager == QStringLiteral("SDDM"))
    {
        serviceNames = {QStringLiteral("/sddm"), QStringLiteral("/kde")};
    }
    else
    {
        return PamStatus::Unknown;
    }

    int present = 0;
    int wired = 0;
    for (const QString &line : output.split(QLatin1Char('\n')))
    {
        const bool relevant = std::any_of(serviceNames.cbegin(), serviceNames.cend(),
                                          [&line](const QString &serviceName) { return line.contains(serviceName); });
        if (!relevant)
        {
            continue;
        }
        ++present;
        if (line.contains(QStringLiteral("● wired")))
        {
            ++wired;
        }
    }

    if (present == 0)
    {
        return PamStatus::Unknown;
    }
    if (wired == 0)
    {
        return PamStatus::NotConfigured;
    }
    return wired == present ? PamStatus::Clean : PamStatus::Drift;
}

SecureBootStatus secureBootStatus(const SystemProbeInputs &inputs)
{
    if (!inputs.secureBootVariablePresent || inputs.secureBootVariable.size() < 5)
    {
        return SecureBootStatus::Unknown;
    }
    return inputs.secureBootVariable.at(4) == '\x01' ? SecureBootStatus::Enabled : SecureBootStatus::Disabled;
}

QString enumToken(SecurityTier tier)
{
    switch (tier)
    {
    case SecurityTier::Secure:
        return QStringLiteral("secure");
    case SecurityTier::Convenience:
        return QStringLiteral("convenience");
    case SecurityTier::Unsupported:
        return QStringLiteral("unsupported");
    }
    return QStringLiteral("unsupported");
}

QString statusToken(CapabilityStatus status)
{
    switch (status)
    {
    case CapabilityStatus::Available:
        return QStringLiteral("available");
    case CapabilityStatus::Unavailable:
        return QStringLiteral("unavailable");
    case CapabilityStatus::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

QString secureBootToken(SecureBootStatus status)
{
    switch (status)
    {
    case SecureBootStatus::Enabled:
        return QStringLiteral("enabled");
    case SecureBootStatus::Disabled:
        return QStringLiteral("disabled");
    case SecureBootStatus::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

QString reportValue(const QString &value)
{
    QString sanitized = value.left(96);
    sanitized.replace(QRegularExpression(QStringLiteral(R"([\r\n\x00-\x1f])")), QStringLiteral(" "));
    sanitized.replace(QRegularExpression(QStringLiteral(R"((/home/|/dev/|/etc/|[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+))")),
                      QStringLiteral("[redacted]"));
    return sanitized.isEmpty() ? QStringLiteral("unknown") : sanitized;
}

QString makeSupportReport(const SystemStateSnapshot &state)
{
    return QStringLiteral("# plasma-irlume support report\n\n"
                          "- Data source: live-local-probe\n"
                          "- Fedora: %1\n"
                          "- Plasma: %2\n"
                          "- Display manager: %3\n"
                          "- irlume: %4\n"
                          "- Security tier: %5\n"
                          "- TPM hardware: %6\n"
                          "- Template protection: %7\n"
                          "- Secure Boot: %8\n"
                          "- Diagnostic code: %9\n")
        .arg(reportValue(state.fedoraVersion), reportValue(state.plasmaVersion),
             reportValue(state.activeDisplayManager), reportValue(state.engineVersion), enumToken(state.securityTier),
             statusToken(state.tpmStatus), statusToken(state.templateProtectionStatus),
             secureBootToken(state.secureBootStatus),
             reportValue(state.issueCode.isEmpty() ? QStringLiteral("none") : state.issueCode));
}
} // namespace

SystemStateSnapshot SystemProbe::probe() const
{
    SystemProbeInputs inputs;
    inputs.osRelease = readBoundedFile(QStringLiteral("/etc/os-release"));
    inputs.plasmaVersion = packageVersion(QStringLiteral("plasma-workspace"));

    const QFileInfo displayManager(QStringLiteral("/etc/systemd/system/display-manager.service"));
    inputs.displayManagerTarget =
        displayManager.isSymLink() ? displayManager.symLinkTarget() : displayManager.canonicalFilePath();

    const QDir efivars(QStringLiteral("/sys/firmware/efi/efivars"));
    const QStringList secureBootVariables =
        efivars.entryList({QStringLiteral("SecureBoot-*")}, QDir::Files | QDir::Readable, QDir::Name);
    if (!secureBootVariables.isEmpty())
    {
        inputs.secureBootVariablePresent = true;
        inputs.secureBootVariable = readBoundedFile(efivars.filePath(secureBootVariables.constFirst()), 8);
    }

    inputs.tpmPresent = QDir(QStringLiteral("/sys/class/tpm")).exists() &&
                        !QDir(QStringLiteral("/sys/class/tpm"))
                             .entryList({QStringLiteral("tpm*")}, QDir::Dirs | QDir::NoDotAndDotDot)
                             .isEmpty();

    const QString irlume = QStringLiteral("/usr/bin/irlume");
    inputs.irlumePresent = QFileInfo(irlume).isExecutable();
    if (inputs.irlumePresent)
    {
        inputs.irlumeVersionOutput = runReadOnlyCommand(irlume, {QStringLiteral("--version")}).standardOutput;
        inputs.irlumeStatusOutput = runReadOnlyCommand(irlume, {QStringLiteral("status")}).standardOutput;
        inputs.irlumeDoctorOutput = runReadOnlyCommand(irlume, {QStringLiteral("doctor")}).standardOutput;
        inputs.irlumeLoginStatusOutput =
            runReadOnlyCommand(irlume, {QStringLiteral("login"), QStringLiteral("status")}).standardOutput;
    }

    return evaluate(inputs);
}

SystemStateSnapshot SystemProbe::evaluate(const SystemProbeInputs &inputs)
{
    SystemStateSnapshot state;
    state.scenarioId = QStringLiteral("live-system");
    state.dataSource = translate("Live local system");
    state.liveData = true;
    state.fedoraVersion = parseOsReleaseValue(inputs.osRelease, QStringLiteral("VERSION_ID"));
    const QString distribution = parseOsReleaseValue(inputs.osRelease, QStringLiteral("ID")).toLower();
    state.plasmaVersion = inputs.plasmaVersion;
    state.activeDisplayManager = displayManagerLabel(inputs.displayManagerTarget);
    state.secureBootStatus = secureBootStatus(inputs);
    state.tpmStatus = inputs.tpmPresent ? CapabilityStatus::Available : CapabilityStatus::Unavailable;
    state.engineVersion = normalizedVersion(inputs.irlumeVersionOutput);

    if (!inputs.irlumePresent)
    {
        state.headline = translate("irlume is not installed");
        state.summary = translate("Install irlume 0.6.x to run live face-authentication readiness checks.");
        state.issueCode = QStringLiteral("engine-missing");
        state.engineStatus = EngineStatus::Missing;
        state.daemonStatus = DaemonStatus::Missing;
        state.pamStatus = PamStatus::NotConfigured;
        state.supportReport = makeSupportReport(state);
        return state;
    }

    if (!supportedIrlumeVersion(state.engineVersion))
    {
        state.headline = translate("This irlume version is not supported");
        state.summary = translate("plasma-irlume supports the read-only diagnostic output of irlume 0.6.x.");
        state.issueCode = QStringLiteral("engine-version-unsupported");
        state.engineStatus = EngineStatus::UnsupportedVersion;
        state.supportReport = makeSupportReport(state);
        return state;
    }

    state.engineStatus = EngineStatus::Ready;
    const QString status = inputs.irlumeStatusOutput;
    state.daemonStatus =
        status.contains(QStringLiteral("daemon        : running"))
            ? DaemonStatus::Running
            : (status.contains(QStringLiteral("daemon        : NOT reachable")) ? DaemonStatus::Broken
                                                                                : DaemonStatus::Unknown);

    if (status.contains(QRegularExpression(QStringLiteral(R"(enrollment\s+:\s+\d+ profile\(s\))"))))
    {
        state.profileStatus = ProfileStatus::Enrolled;
    }
    else if (status.contains(QRegularExpression(QStringLiteral(R"(enrollment\s+:\s+none)"))))
    {
        state.profileStatus = ProfileStatus::NotEnrolled;
    }
    if (status.contains(QRegularExpression(QStringLiteral(R"(templates\s+:\s+encrypted at rest)"))))
    {
        state.templateProtectionStatus = CapabilityStatus::Available;
    }
    else if (status.contains(QRegularExpression(QStringLiteral(R"(templates\s+:\s+plaintext)"))))
    {
        state.templateProtectionStatus = CapabilityStatus::Unavailable;
    }

    const QRegularExpression cameraLine(QStringLiteral(R"(cameras\s+:\s+rgb=([^\s]+)\s+ir=([^\s]+))"));
    const auto cameraMatch = cameraLine.match(status);
    if (cameraMatch.hasMatch())
    {
        const bool rgb = cameraMatch.captured(1) != QStringLiteral("none");
        const bool infrared = cameraMatch.captured(2) != QStringLiteral("none");
        state.cameraType = infrared ? CameraType::Infrared : (rgb ? CameraType::Rgb : CameraType::None);
        state.emitterStatus = infrared ? CapabilityStatus::Unknown : CapabilityStatus::Unavailable;
        state.livenessStatus =
            infrared ? CapabilityStatus::Available : (rgb ? CapabilityStatus::Unavailable : CapabilityStatus::Unknown);
    }

    const QString doctor = inputs.irlumeDoctorOutput;
    if (doctor.contains(QStringLiteral("[doctor] TPM 2.0: none")))
    {
        state.tpmStatus = CapabilityStatus::Unavailable;
    }
    else if (doctor.contains(QRegularExpression(QStringLiteral(R"(\[doctor\] TPM 2\.0: .+ [✓\x{2705}])"))))
    {
        state.tpmStatus = CapabilityStatus::Available;
    }
    if (doctor.contains(QStringLiteral("[doctor] Secure Boot: enabled")))
    {
        state.secureBootStatus = SecureBootStatus::Enabled;
    }
    else if (doctor.contains(QStringLiteral("[doctor] Secure Boot: disabled")) ||
             doctor.contains(QStringLiteral("[doctor] Secure Boot: SETUP MODE")))
    {
        state.secureBootStatus = SecureBootStatus::Disabled;
    }

    state.pamStatus = pamStatusFromOutput(inputs.irlumeLoginStatusOutput, state.activeDisplayManager);

    if (distribution != QStringLiteral("fedora") || state.fedoraVersion != QStringLiteral("44"))
    {
        state.securityTier = SecurityTier::Unsupported;
        state.headline = translate("This operating system is not supported");
        state.summary = translate("Version 1.0 supports Fedora 44 on mutable DNF-based installations.");
        state.issueCode = QStringLiteral("platform-unsupported");
    }
    else if (state.activeDisplayManager != QStringLiteral("Plasma Login Manager") &&
             state.activeDisplayManager != QStringLiteral("SDDM"))
    {
        state.securityTier = SecurityTier::Unsupported;
        state.headline = translate("The active display manager is not supported");
        state.summary = translate("Face login supports Plasma Login Manager and SDDM on Fedora 44.");
        state.issueCode = QStringLiteral("display-manager-unsupported");
    }
    else if (state.daemonStatus != DaemonStatus::Running)
    {
        state.securityTier = SecurityTier::Unsupported;
        state.headline = translate("The irlume service needs attention");
        state.summary = translate("The engine is installed, but its background service is not reachable.");
        state.issueCode = QStringLiteral("daemon-unhealthy");
    }
    else if (state.cameraType == CameraType::Infrared && state.livenessStatus == CapabilityStatus::Available)
    {
        state.securityTier = SecurityTier::Secure;
        state.headline = translate("Secure face login hardware is available");
        state.summary = translate("irlume reports infrared hardware and required liveness capability.");
    }
    else if (state.cameraType == CameraType::Rgb)
    {
        state.securityTier = SecurityTier::Convenience;
        state.headline = translate("Face unlock is limited to convenience use");
        state.summary = translate("RGB-only hardware is restricted to lock-screen use.");
        state.issueCode = QStringLiteral("rgb-convenience-only");
    }
    else
    {
        state.securityTier = SecurityTier::Unsupported;
        state.headline = translate("No compatible face-login camera was found");
        state.summary = translate("irlume did not report usable RGB or infrared camera hardware.");
        state.issueCode = QStringLiteral("camera-unavailable");
    }

    state.supportReport = makeSupportReport(state);
    return state;
}

QString SystemProbe::parseOsReleaseValue(const QByteArray &contents, const QString &key)
{
    const QString prefix = key + QLatin1Char('=');
    const QString text = QString::fromUtf8(contents);
    for (const QString &line : text.split(QLatin1Char('\n')))
    {
        if (!line.startsWith(prefix))
        {
            continue;
        }
        QString value = line.sliced(prefix.size()).trimmed();
        if (value.size() >= 2 && value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
        {
            value = value.sliced(1, value.size() - 2);
        }
        static const QRegularExpression safeValue(QStringLiteral(R"(\A[A-Za-z0-9._+~ -]{1,96}\z)"));
        return safeValue.match(value).hasMatch() ? value : QString();
    }
    return {};
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemprobe.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QtConcurrentRun>

#include <algorithm>

namespace
{
constexpr qsizetype MaximumProbeOutput = 256 * 1024;

using CameraType = SystemStateSnapshot::CameraType;
using CapabilityStatus = SystemStateSnapshot::CapabilityStatus;
using DaemonStatus = SystemStateSnapshot::DaemonStatus;
using EngineStatus = SystemStateSnapshot::EngineStatus;
using PamStatus = SystemStateSnapshot::PamStatus;
using ProfileStatus = SystemStateSnapshot::ProfileStatus;
using SecurityTier = SystemStateSnapshot::SecurityTier;
using SecureBootStatus = SystemStateSnapshot::SecureBootStatus;

QString translate(const char *text)
{
    return QCoreApplication::translate("SystemProbe", text);
}

QByteArray readBoundedFile(const QString &path, qsizetype maximumBytes = MaximumProbeOutput)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.read(maximumBytes) : QByteArray();
}

QString displayManagerLabel(const QString &target)
{
    const QString service = QFileInfo(target).fileName().toLower();
    if (service.contains(QStringLiteral("plasmalogin")) || service.contains(QStringLiteral("plasma-login-manager")))
        return QStringLiteral("Plasma Login Manager");
    if (service.contains(QStringLiteral("sddm")))
        return QStringLiteral("SDDM");
    return service.isEmpty() ? QStringLiteral("Unknown") : QStringLiteral("Unsupported (%1)").arg(service.left(64));
}

SecureBootStatus secureBootStatus(const SystemProbeInputs &inputs)
{
    if (!inputs.secureBootVariablePresent || inputs.secureBootVariable.size() < 5)
        return SecureBootStatus::Unknown;
    return inputs.secureBootVariable.at(4) == '\x01' ? SecureBootStatus::Enabled : SecureBootStatus::Disabled;
}

CapabilityStatus doctorCapability(const EngineSnapshot &engine, const QString &id)
{
    if (!engine.doctor.data)
        return CapabilityStatus::Unknown;
    const auto match = std::find_if(engine.doctor.data->cbegin(), engine.doctor.data->cend(),
                                    [&id](const EngineDoctorCheck &check) { return check.id == id; });
    if (match == engine.doctor.data->cend() || match->state == EngineDoctorCheck::State::Unknown ||
        match->state == EngineDoctorCheck::State::Info)
        return CapabilityStatus::Unknown;
    return match->state == EngineDoctorCheck::State::Pass ? CapabilityStatus::Available : CapabilityStatus::Unavailable;
}

PamStatus pamStatus(const EngineLoginSnapshot &login, const QString &localDisplayManager, bool *migration)
{
    *migration = false;
    if (!login.loginManagerKnown || !login.loginManagerRecognized)
        return PamStatus::Unknown;

    const QString expectedName = localDisplayManager == QLatin1String("Plasma Login Manager")
                                     ? QStringLiteral("plasmalogin")
                                 : localDisplayManager == QLatin1String("SDDM") ? QStringLiteral("sddm")
                                                                                : QString();
    if (expectedName.isEmpty())
        return PamStatus::Unknown;
    if (login.loginManagerName != expectedName)
    {
        *migration = true;
        return PamStatus::Drift;
    }

    int relevant = 0;
    int wired = 0;
    for (const EngineLoginSurface &surface : login.surfaces)
    {
        const bool currentLoginSurface = login.loginManagerServices.contains(surface.id);
        const bool lockSurface = surface.id == QLatin1String("kde") && surface.role == QLatin1String("lock-screen");
        if (surface.role == QLatin1String("login-screen") && surface.wired && !currentLoginSurface)
            *migration = true;
        if (!currentLoginSurface && !lockSurface)
            continue;
        if (!surface.present)
            continue;
        ++relevant;
        if (surface.wired)
            ++wired;
    }
    if (*migration)
        return PamStatus::Drift;
    if (relevant == 0)
        return PamStatus::Unknown;
    if (wired == 0)
        return PamStatus::NotConfigured;
    return wired == relevant ? PamStatus::Clean : PamStatus::Drift;
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
                          "- Backend version: %4\n"
                          "- Diagnostic code: %5\n")
        .arg(reportValue(state.fedoraVersion), reportValue(state.plasmaVersion),
             reportValue(state.activeDisplayManager), reportValue(state.engineVersion),
             reportValue(state.issueCode.isEmpty() ? QStringLiteral("none") : state.issueCode));
}
} // namespace

SystemProbe::SystemProbe(QObject *parent) : QObject(parent) {}

void SystemProbe::requestProbe(quint64 generation, const EngineSnapshot &engine)
{
    m_latestGeneration = generation;
    auto *watcher = new QFutureWatcher<SystemStateSnapshot>(this);
    connect(watcher, &QFutureWatcher<SystemStateSnapshot>::finished, this,
            [this, watcher, generation]()
            {
                if (generation == m_latestGeneration)
                    Q_EMIT probeCompleted(generation, watcher->result());
                watcher->deleteLater();
            });
    watcher->setFuture(QtConcurrent::run([engine]() { return SystemProbe().probe(engine); }));
}

SystemStateSnapshot SystemProbe::probe(const EngineSnapshot &engine) const
{
    SystemProbeInputs inputs;
    inputs.osRelease = readBoundedFile(QStringLiteral("/etc/os-release"));
    // Avoid package-manager subprocesses on the GUI thread. Plasma version remains
    // unknown until a future non-blocking platform information provider supplies it.
    inputs.plasmaVersion.clear();
    const QFileInfo displayManager(QStringLiteral("/etc/systemd/system/display-manager.service"));
    inputs.displayManagerTarget =
        displayManager.isSymLink() ? displayManager.symLinkTarget() : displayManager.canonicalFilePath();

    const QDir efivars(QStringLiteral("/sys/firmware/efi/efivars"));
    const QStringList variables =
        efivars.entryList({QStringLiteral("SecureBoot-*")}, QDir::Files | QDir::Readable, QDir::Name);
    if (!variables.isEmpty())
    {
        inputs.secureBootVariablePresent = true;
        inputs.secureBootVariable = readBoundedFile(efivars.filePath(variables.constFirst()), 8);
    }
    const QDir tpm(QStringLiteral("/sys/class/tpm"));
    inputs.tpmPresent =
        tpm.exists() && !tpm.entryList({QStringLiteral("tpm*")}, QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
    inputs.engine = engine;
    return evaluate(inputs);
}

SystemStateSnapshot SystemProbe::evaluate(const SystemProbeInputs &inputs)
{
    SystemStateSnapshot state;
    state.scenarioId = QStringLiteral("live-system");
    state.dataSource = translate("Live local system");
    state.liveData = true;
    state.fedoraVersion = parseOsReleaseValue(inputs.osRelease, QStringLiteral("VERSION_ID"));
    state.plasmaVersion = inputs.plasmaVersion;
    state.activeDisplayManager = displayManagerLabel(inputs.displayManagerTarget);
    state.secureBootStatus = secureBootStatus(inputs);
    state.tpmStatus = inputs.tpmPresent ? CapabilityStatus::Available : CapabilityStatus::Unavailable;
    state.engineVersion = inputs.engine.engineVersion();
    state.passwordFallbackStatus = SystemStateSnapshot::PasswordFallbackStatus::Unknown;

    if (!inputs.engine.executablePresent)
    {
        state.headline = translate("The face-authentication backend is not installed");
        state.summary = translate("Install irlume 0.7 or newer to run Contract 1 readiness checks.");
        state.issueCode = QStringLiteral("engine-missing");
        state.engineStatus = EngineStatus::Missing;
        state.daemonStatus = DaemonStatus::Missing;
        state.supportReport = makeSupportReport(state);
        return state;
    }
    if (!inputs.engine.contractAvailable())
    {
        state.headline = translate("The machine contract is unavailable");
        state.summary = translate("The installed backend did not complete a valid Contract 1 handshake.");
        state.issueCode = inputs.engine.handshake.error ? inputs.engine.handshake.error->code
                                                        : QStringLiteral("machine-contract-unavailable");
        state.engineStatus = state.issueCode == QLatin1String("unsupported-contract")
                                 ? EngineStatus::UnsupportedContract
                                 : EngineStatus::Unavailable;
        state.supportReport = makeSupportReport(state);
        return state;
    }

    if (inputs.engine.capabilities.recognizedReadCount() == 0)
    {
        state.headline = translate("No compatible read capabilities are available");
        state.summary =
            translate("The backend completed Contract 1 negotiation but advertised no supported diagnostics.");
        state.issueCode = QStringLiteral("no-compatible-read-capabilities");
        state.engineStatus = EngineStatus::NoCompatibleCapabilities;
        state.supportReport = makeSupportReport(state);
        return state;
    }

    state.engineStatus = inputs.engine.partialDiagnostics() ? EngineStatus::PartialDiagnostics : EngineStatus::Ready;
    if (inputs.engine.status.data)
    {
        const EngineStatusSnapshot &status = *inputs.engine.status.data;
        if (status.daemon == EngineStatusSnapshot::Daemon::Running)
            state.daemonStatus = DaemonStatus::Running;
        else
            state.daemonStatus = DaemonStatus::Broken;
        if (status.enrollmentKnown)
            state.profileStatus =
                status.profileCount.value_or(0) > 0 ? ProfileStatus::Enrolled : ProfileStatus::NotEnrolled;
        if (status.templates == EngineStatusSnapshot::TemplateProtection::Encrypted)
            state.templateProtectionStatus = CapabilityStatus::Available;
        else if (status.templates == EngineStatusSnapshot::TemplateProtection::Plaintext)
            state.templateProtectionStatus = CapabilityStatus::Unavailable;
        if (status.irCamera)
        {
            state.cameraType = CameraType::Infrared;
        }
        else if (status.rgbCamera)
        {
            state.cameraType = CameraType::Rgb;
        }
        else
        {
            state.cameraType = CameraType::None;
        }
    }

    const CapabilityStatus doctorTpm = doctorCapability(inputs.engine, QStringLiteral("tpm"));
    if (doctorTpm != CapabilityStatus::Unknown)
        state.tpmStatus = doctorTpm;
    state.livenessStatus = CapabilityStatus::Unknown;
    state.emitterStatus = CapabilityStatus::Unknown;

    bool migration = false;
    if (inputs.engine.loginStatus.data)
        state.pamStatus = pamStatus(*inputs.engine.loginStatus.data, state.activeDisplayManager, &migration);
    if (migration)
    {
        state.issueCode = QStringLiteral("display-manager-migration");
        state.securityTier = SecurityTier::Unsupported;
    }
    else if (inputs.engine.status.error)
    {
        state.issueCode = inputs.engine.status.error->code;
    }
    else if (inputs.engine.doctor.error)
    {
        state.issueCode = inputs.engine.doctor.error->code;
    }
    else if (state.daemonStatus == DaemonStatus::Broken)
    {
        state.issueCode = QStringLiteral("daemon-unhealthy");
    }

    const QString distribution = parseOsReleaseValue(inputs.osRelease, QStringLiteral("ID")).toLower();
    if (distribution != QLatin1String("fedora") || state.fedoraVersion != QLatin1String("44"))
    {
        state.issueCode = QStringLiteral("platform-unsupported");
        state.securityTier = SecurityTier::Unsupported;
    }

    if (state.issueCode.isEmpty() && state.engineStatus == EngineStatus::Ready)
    {
        state.headline = translate("Read-only face-authentication status is available");
        state.summary =
            translate("The backend supports Contract 1 diagnostics. Configuration changes remain disabled.");
    }
    else
    {
        state.headline = translate("Face Login needs attention");
        state.summary = translate("Some read-only backend state is unavailable or inconsistent.");
    }
    state.supportReport = makeSupportReport(state);
    return state;
}

QString SystemProbe::parseOsReleaseValue(const QByteArray &contents, const QString &key)
{
    for (const QByteArray &line : contents.split('\n'))
    {
        const int separator = line.indexOf('=');
        if (separator <= 0 || QString::fromLatin1(line.left(separator)) != key)
            continue;
        QString value = QString::fromUtf8(line.mid(separator + 1)).trimmed();
        if (value.size() >= 2 && ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))) ||
                                  (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\'')))))
            value = value.mid(1, value.size() - 2);
        static const QRegularExpression safeValue(QStringLiteral(R"(\A[A-Za-z0-9._+~ -]{1,96}\z)"));
        return safeValue.match(value).hasMatch() ? value : QString();
    }
    return {};
}

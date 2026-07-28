// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemprobe.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QtConcurrentRun>

namespace
{
constexpr qsizetype MaximumProbeOutput = 256 * 1024;

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
    return service.isEmpty() ? QStringLiteral("Unknown") : QStringLiteral("Other");
}

SystemStateSnapshot::SecureBootStatus secureBootStatus(const SystemProbeInputs &inputs)
{
    if (!inputs.secureBootVariablePresent || inputs.secureBootVariable.size() < 5)
        return SystemStateSnapshot::SecureBootStatus::Unknown;
    return inputs.secureBootVariable.at(4) == '\x01' ? SystemStateSnapshot::SecureBootStatus::Enabled
                                                     : SystemStateSnapshot::SecureBootStatus::Disabled;
}

SystemStateSnapshot::CapabilityStatus capability(OperationSupport support)
{
    return support == OperationSupport::Supported ? SystemStateSnapshot::CapabilityStatus::Supported
                                                  : SystemStateSnapshot::CapabilityStatus::Unsupported;
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
    inputs.engine = engine;
    return evaluate(inputs);
}

SystemStateSnapshot SystemProbe::evaluate(const SystemProbeInputs &inputs)
{
    SystemStateSnapshot state;
    state.scenarioId = QStringLiteral("native-milestone-1");
    state.dataSource = translate("Live local system");
    state.liveData = true;
    state.fedoraVersion = parseOsReleaseValue(inputs.osRelease, QStringLiteral("VERSION_ID"));
    state.plasmaVersion = inputs.plasmaVersion;
    state.activeDisplayManager = displayManagerLabel(inputs.displayManagerTarget);
    state.secureBootStatus = secureBootStatus(inputs);
    state.engineVersion = inputs.engine.engineVersion();
    state.visionStatus = capability(inputs.engine.capabilities.vision);
    state.enrollmentStatus = capability(inputs.engine.capabilities.enrollment);
    state.authenticationStatus = capability(inputs.engine.capabilities.authentication);
    state.pamStatus = capability(inputs.engine.capabilities.pamConfiguration);
    state.templatePersistenceStatus = capability(inputs.engine.capabilities.templatePersistence);

    if (!inputs.engine.engineAvailable)
    {
        state.headline = translate("Native engine unavailable");
        state.summary =
            translate("The KCM remains usable for camera checks. Biometric and PAM operations are not implemented.");
        state.issueCode = QStringLiteral("native-engine-unavailable");
        state.engineStatus = SystemStateSnapshot::EngineStatus::Unavailable;
        return state;
    }

    if (!inputs.engine.protocolAvailable())
    {
        state.headline = translate("Native protocol unavailable");
        state.summary = translate("The local engine did not provide the versioned Milestone 1 status protocol.");
        state.issueCode = QStringLiteral("native-protocol-unavailable");
        state.engineStatus = SystemStateSnapshot::EngineStatus::ProtocolError;
        return state;
    }

    state.headline = translate("Native engine skeleton available");
    state.summary =
        translate("Status reporting is available. Recognition, enrollment, template storage, and PAM remain disabled.");
    state.engineStatus = SystemStateSnapshot::EngineStatus::SkeletonAvailable;
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

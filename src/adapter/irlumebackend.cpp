// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumebackend.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

namespace
{
constexpr qsizetype MaximumOutputBytes = 256 * 1024;
constexpr int ProcessTimeoutMs = 3000;

bool exactInteger(const QJsonValue &value, int minimum, int *result)
{
    if (!value.isDouble())
    {
        return false;
    }
    const double number = value.toDouble();
    const int integer = value.toInt(minimum - 1);
    if (number != static_cast<double>(integer) || integer < minimum)
    {
        return false;
    }
    *result = integer;
    return true;
}

bool knownObject(const QJsonValue &value, QJsonObject *object)
{
    if (!value.isObject())
    {
        return false;
    }
    *object = value.toObject();
    return object->value(QStringLiteral("known")).isBool();
}

} // namespace

IrlumeBackend::IrlumeBackend(QObject *parent) : IrlumeBackend(QStringLiteral("/usr/bin/irlume"), parent) {}

IrlumeBackend::IrlumeBackend(QString executable, QObject *parent)
    : FaceAuthBackend(parent), m_executable(std::move(executable))
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this,
            [this]()
            {
                if (!m_process || m_processHandled)
                    return;
                m_timedOut = true;
                m_process->kill();
            });
}

IrlumeBackend::IrlumeBackend(Executor testExecutor, QObject *parent)
    : FaceAuthBackend(parent), m_testExecutor(std::move(testExecutor))
{
}

IrlumeBackend::~IrlumeBackend()
{
    m_timeout.stop();
    if (!m_process)
        return;
    disconnect(m_process, nullptr, this, nullptr);
    if (m_process->state() != QProcess::NotRunning)
    {
        QObject *reaper = QCoreApplication::instance();
        m_process->setParent(reaper);
        connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), m_process, &QObject::deleteLater);
        m_process->kill();
    }
    else
    {
        delete m_process;
    }
    m_process = nullptr;
}

QString IrlumeBackend::commandName(Command command)
{
    switch (command)
    {
    case Command::Version:
        return QStringLiteral("version");
    case Command::Status:
        return QStringLiteral("status");
    case Command::Doctor:
        return QStringLiteral("doctor");
    case Command::ProfilesList:
        return QStringLiteral("profiles.list");
    case Command::LoginStatus:
        return QStringLiteral("login.status");
    }
    return {};
}

QString IrlumeBackend::capabilityName(Command command)
{
    switch (command)
    {
    case Command::Version:
        return QStringLiteral("version-json");
    case Command::Status:
        return QStringLiteral("status-json");
    case Command::Doctor:
        return QStringLiteral("doctor-json");
    case Command::ProfilesList:
        return QStringLiteral("profiles-list-json");
    case Command::LoginStatus:
        return QStringLiteral("login-status-json");
    }
    return {};
}

QStringList IrlumeBackend::arguments(Command command)
{
    switch (command)
    {
    case Command::Version:
        return {QStringLiteral("version"), QStringLiteral("--json")};
    case Command::Status:
        return {QStringLiteral("status"), QStringLiteral("--json"), QStringLiteral("--contract"), QStringLiteral("1")};
    case Command::Doctor:
        return {QStringLiteral("doctor"), QStringLiteral("--json"), QStringLiteral("--contract"), QStringLiteral("1")};
    case Command::ProfilesList:
        return {QStringLiteral("profiles"), QStringLiteral("list"), QStringLiteral("--json"),
                QStringLiteral("--contract"), QStringLiteral("1")};
    case Command::LoginStatus:
        return {QStringLiteral("login"), QStringLiteral("status"), QStringLiteral("--json"),
                QStringLiteral("--contract"), QStringLiteral("1")};
    }
    return {};
}

EngineOperation IrlumeBackend::operationFor(Command command)
{
    switch (command)
    {
    case Command::Version:
        return EngineOperation::Handshake;
    case Command::Status:
        return EngineOperation::Status;
    case Command::Doctor:
        return EngineOperation::Doctor;
    case Command::ProfilesList:
        return EngineOperation::Profiles;
    case Command::LoginStatus:
        return EngineOperation::LoginStatus;
    }
    return EngineOperation::Handshake;
}

IrlumeBackend::ProcessResult IrlumeBackend::executeForTest(Command command) const
{
    return m_testExecutor ? m_testExecutor(command) : ProcessResult{};
}

EngineError IrlumeBackend::processError(const ProcessResult &result, EngineOperation operation)
{
    if (!result.started)
    {
        return {operation, QStringLiteral("engine-not-installed"), false};
    }
    if (result.outputTooLarge)
    {
        return {operation, QStringLiteral("engine-output-too-large"), false};
    }
    if (result.timedOut || !result.finished)
    {
        return {operation, QStringLiteral("engine-timeout"), true};
    }
    return {operation, QStringLiteral("engine-process-failed"), false};
}

std::optional<IrlumeBackend::Envelope> IrlumeBackend::parseEnvelope(const ProcessResult &result, Command command,
                                                                    EngineError *parseError)
{
    if (!result.started || !result.finished || result.outputTooLarge || result.timedOut)
    {
        *parseError = processError(result, operationFor(command));
        return std::nullopt;
    }

    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(result.standardOutput, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isObject())
    {
        *parseError = {operationFor(command), QStringLiteral("invalid-json-document"), false};
        return std::nullopt;
    }
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("contract_version")).toInt(-1) != 1 ||
        !object.value(QStringLiteral("engine_version")).isString() ||
        object.value(QStringLiteral("engine_version")).toString().isEmpty() ||
        object.value(QStringLiteral("command")).toString() != commandName(command) ||
        !object.value(QStringLiteral("ok")).isBool())
    {
        *parseError = {operationFor(command), QStringLiteral("invalid-document-contract"), false};
        return std::nullopt;
    }

    Envelope envelope;
    envelope.ok = object.value(QStringLiteral("ok")).toBool();
    envelope.engineVersion = object.value(QStringLiteral("engine_version")).toString();
    const bool hasData = object.contains(QStringLiteral("data"));
    const bool hasError = object.contains(QStringLiteral("error"));
    if (envelope.ok)
    {
        if (result.exitCode != 0 || !hasData || hasError || !object.value(QStringLiteral("data")).isObject())
        {
            *parseError = {operationFor(command), QStringLiteral("engine-exit-mismatch"), false};
            return std::nullopt;
        }
        envelope.data = object.value(QStringLiteral("data")).toObject();
    }
    else
    {
        if (result.exitCode == 0 || hasData || !hasError || !object.value(QStringLiteral("error")).isObject())
        {
            *parseError = {operationFor(command), QStringLiteral("engine-exit-mismatch"), false};
            return std::nullopt;
        }
        const QJsonObject error = object.value(QStringLiteral("error")).toObject();
        if (!error.value(QStringLiteral("code")).isString() ||
            error.value(QStringLiteral("code")).toString().isEmpty() ||
            !error.value(QStringLiteral("retryable")).isBool())
        {
            *parseError = {operationFor(command), QStringLiteral("invalid-structured-error"), false};
            return std::nullopt;
        }
        envelope.error = {operationFor(command), error.value(QStringLiteral("code")).toString(),
                          error.value(QStringLiteral("retryable")).toBool()};
    }
    return envelope;
}

bool IrlumeBackend::parseVersion(const QJsonObject &data, EngineSnapshot *snapshot, EngineError *error)
{
    if (!data.value(QStringLiteral("capabilities")).isArray() ||
        !data.value(QStringLiteral("contract_versions")).isObject() || !data.value(QStringLiteral("limits")).isObject())
    {
        *error = {QStringLiteral("invalid-handshake"), false};
        return false;
    }

    const QJsonArray capabilityValues = data.value(QStringLiteral("capabilities")).toArray();
    QSet<QString> capabilities;
    for (const QJsonValue &value : capabilityValues)
    {
        if (!value.isString() || value.toString().isEmpty() || capabilities.contains(value.toString()))
        {
            *error = {QStringLiteral("invalid-handshake"), false};
            return false;
        }
        capabilities.insert(value.toString());
    }

    const QJsonObject versions = data.value(QStringLiteral("contract_versions")).toObject();
    int minimum = 0;
    int maximum = 0;
    const QJsonObject limits = data.value(QStringLiteral("limits")).toObject();
    int maxProfiles = 0;
    if (!exactInteger(versions.value(QStringLiteral("min")), 1, &minimum) ||
        !exactInteger(versions.value(QStringLiteral("max")), 1, &maximum) || minimum > maximum ||
        !exactInteger(limits.value(QStringLiteral("max_profiles")), 1, &maxProfiles))
    {
        *error = {QStringLiteral("invalid-handshake"), false};
        return false;
    }
    if (minimum > 1 || maximum < 1)
    {
        *error = {QStringLiteral("unsupported-contract"), false};
        return false;
    }

    snapshot->capabilities.features = EngineFeature::None;
    if (capabilities.contains(QStringLiteral("status-json")))
        snapshot->capabilities.features |= EngineFeature::StatusRead;
    if (capabilities.contains(QStringLiteral("doctor-json")))
        snapshot->capabilities.features |= EngineFeature::DoctorRead;
    if (capabilities.contains(QStringLiteral("profiles-list-json")))
        snapshot->capabilities.features |= EngineFeature::ProfilesRead;
    if (capabilities.contains(QStringLiteral("login-status-json")))
        snapshot->capabilities.features |= EngineFeature::LoginStatusRead;
    snapshot->capabilities.maxProfiles = maxProfiles;
    snapshot->capabilities.advertised = QStringList(capabilities.cbegin(), capabilities.cend());
    snapshot->capabilities.advertised.sort();
    return true;
}

std::optional<EngineStatusSnapshot> IrlumeBackend::parseStatus(const QJsonObject &data)
{
    static const QSet<QString> authMethods = {
        QStringLiteral("auto"),
        QStringLiteral("face"),
        QStringLiteral("fingerprint"),
        QStringLiteral("both"),
    };
    if (!data.value(QStringLiteral("daemon")).isString() ||
        !authMethods.contains(data.value(QStringLiteral("auth_method")).toString()) ||
        !data.value(QStringLiteral("face_disabled")).isBool() || !data.value(QStringLiteral("enrollment")).isObject() ||
        !data.value(QStringLiteral("templates")).isString() || !data.value(QStringLiteral("keyring")).isObject() ||
        !data.value(QStringLiteral("recovery")).isObject() || !data.value(QStringLiteral("camera")).isObject() ||
        !data.value(QStringLiteral("fingerprint")).isBool())
    {
        return std::nullopt;
    }

    EngineStatusSnapshot status;
    const QString daemon = data.value(QStringLiteral("daemon")).toString();
    if (daemon == QLatin1String("running"))
        status.daemon = EngineStatusSnapshot::Daemon::Running;
    else if (daemon == QLatin1String("access-denied"))
        status.daemon = EngineStatusSnapshot::Daemon::AccessDenied;
    else if (daemon == QLatin1String("unreachable"))
        status.daemon = EngineStatusSnapshot::Daemon::Unreachable;
    else
        return std::nullopt;

    const QString templates = data.value(QStringLiteral("templates")).toString();
    if (templates == QLatin1String("encrypted"))
        status.templates = EngineStatusSnapshot::TemplateProtection::Encrypted;
    else if (templates == QLatin1String("plaintext"))
        status.templates = EngineStatusSnapshot::TemplateProtection::Plaintext;
    else if (templates != QLatin1String("unknown"))
        return std::nullopt;

    const QJsonObject enrollment = data.value(QStringLiteral("enrollment")).toObject();
    if (!enrollment.value(QStringLiteral("known")).isBool())
        return std::nullopt;
    status.enrollmentKnown = enrollment.value(QStringLiteral("known")).toBool();
    if (status.enrollmentKnown)
    {
        int profileCount = 0;
        int scanCount = 0;
        if (!exactInteger(enrollment.value(QStringLiteral("profiles")), 0, &profileCount) ||
            !exactInteger(enrollment.value(QStringLiteral("scans")), 0, &scanCount))
            return std::nullopt;
        status.profileCount = profileCount;
        status.scanCount = scanCount;
    }
    else if (enrollment.contains(QStringLiteral("profiles")) || enrollment.contains(QStringLiteral("scans")))
        return std::nullopt;

    QJsonObject keyring;
    QJsonObject recovery;
    if (!knownObject(data.value(QStringLiteral("keyring")), &keyring) ||
        !knownObject(data.value(QStringLiteral("recovery")), &recovery))
        return std::nullopt;
    status.keyringKnown = keyring.value(QStringLiteral("known")).toBool();
    status.recoveryKnown = recovery.value(QStringLiteral("known")).toBool();
    if (status.keyringKnown)
    {
        if (!keyring.value(QStringLiteral("armed")).isBool() ||
            !(keyring.value(QStringLiteral("policy")).isString() || keyring.value(QStringLiteral("policy")).isNull()))
            return std::nullopt;
        status.keyringArmed = keyring.value(QStringLiteral("armed")).toBool();
    }
    if (status.recoveryKnown)
    {
        if (!recovery.value(QStringLiteral("passphrase_set")).isBool())
            return std::nullopt;
        status.recoveryPassphraseSet = recovery.value(QStringLiteral("passphrase_set")).toBool();
    }

    const QJsonObject camera = data.value(QStringLiteral("camera")).toObject();
    if (!camera.value(QStringLiteral("rgb")).isBool() || !camera.value(QStringLiteral("ir")).isBool())
        return std::nullopt;
    status.rgbCamera = camera.value(QStringLiteral("rgb")).toBool();
    status.irCamera = camera.value(QStringLiteral("ir")).toBool();
    status.fingerprintPresent = data.value(QStringLiteral("fingerprint")).toBool();
    status.faceDisabled = data.value(QStringLiteral("face_disabled")).toBool();
    status.authMethod = data.value(QStringLiteral("auth_method")).toString();
    return status;
}

std::optional<QVector<EngineDoctorCheck>> IrlumeBackend::parseDoctor(const QJsonObject &data)
{
    if (!data.value(QStringLiteral("checks")).isArray())
        return std::nullopt;
    QVector<EngineDoctorCheck> checks;
    QSet<QString> ids;
    for (const QJsonValue &value : data.value(QStringLiteral("checks")).toArray())
    {
        if (!value.isObject())
            return std::nullopt;
        const QJsonObject object = value.toObject();
        const QString id = object.value(QStringLiteral("id")).toString();
        const QString state = object.value(QStringLiteral("state")).toString();
        if (id.isEmpty() || ids.contains(id) || !object.value(QStringLiteral("id")).isString() ||
            !object.value(QStringLiteral("state")).isString() ||
            (object.contains(QStringLiteral("detail")) && !object.value(QStringLiteral("detail")).isString()))
            return std::nullopt;
        ids.insert(id);
        EngineDoctorCheck::State typedState;
        if (state == QLatin1String("pass"))
            typedState = EngineDoctorCheck::State::Pass;
        else if (state == QLatin1String("warn"))
            typedState = EngineDoctorCheck::State::Warn;
        else if (state == QLatin1String("fail"))
            typedState = EngineDoctorCheck::State::Fail;
        else if (state == QLatin1String("unknown"))
            typedState = EngineDoctorCheck::State::Unknown;
        else if (state == QLatin1String("info"))
            typedState = EngineDoctorCheck::State::Info;
        else
            return std::nullopt;
        checks.push_back({id, typedState});
    }
    return checks;
}

std::optional<EngineProfileSnapshot> IrlumeBackend::parseProfiles(const QJsonObject &data, int maxProfiles)
{
    if (!data.value(QStringLiteral("profiles")).isArray() ||
        !data.value(QStringLiteral("require_eyes_open")).isBool() ||
        !data.value(QStringLiteral("require_challenge")).isBool())
        return std::nullopt;
    const QJsonArray profiles = data.value(QStringLiteral("profiles")).toArray();
    if (profiles.size() > maxProfiles)
        return std::nullopt;

    EngineProfileSnapshot snapshot;
    snapshot.requireEyesOpen = data.value(QStringLiteral("require_eyes_open")).toBool();
    snapshot.requireChallenge = data.value(QStringLiteral("require_challenge")).toBool();
    for (const QJsonValue &value : profiles)
    {
        if (!value.isObject())
            return std::nullopt;
        const QJsonObject profile = value.toObject();
        if (!profile.value(QStringLiteral("display_name")).isString() ||
            !profile.value(QStringLiteral("scans")).isArray())
            return std::nullopt;
        EngineProfile typedProfile;
        typedProfile.displayName = profile.value(QStringLiteral("display_name")).toString();
        for (const QJsonValue &scanValue : profile.value(QStringLiteral("scans")).toArray())
        {
            if (!scanValue.isObject() || !scanValue.toObject().value(QStringLiteral("display_name")).isString())
                return std::nullopt;
            typedProfile.scanDisplayNames.push_back(
                scanValue.toObject().value(QStringLiteral("display_name")).toString());
        }
        snapshot.profiles.push_back(std::move(typedProfile));
    }
    return snapshot;
}

std::optional<EngineLoginSnapshot> IrlumeBackend::parseLogin(const QJsonObject &data)
{
    if (!data.value(QStringLiteral("login_manager")).isObject() || !data.value(QStringLiteral("surfaces")).isArray() ||
        !data.value(QStringLiteral("selinux_module")).isString())
        return std::nullopt;
    EngineLoginSnapshot snapshot;
    const QJsonObject manager = data.value(QStringLiteral("login_manager")).toObject();
    if (!manager.value(QStringLiteral("known")).isBool())
        return std::nullopt;
    snapshot.loginManagerKnown = manager.value(QStringLiteral("known")).toBool();
    if (snapshot.loginManagerKnown)
    {
        if (!manager.value(QStringLiteral("name")).isString() ||
            manager.value(QStringLiteral("name")).toString().isEmpty() ||
            !manager.value(QStringLiteral("recognized")).isBool() ||
            !manager.value(QStringLiteral("services")).isArray())
            return std::nullopt;
        snapshot.loginManagerName = manager.value(QStringLiteral("name")).toString();
        snapshot.loginManagerRecognized = manager.value(QStringLiteral("recognized")).toBool();
        for (const QJsonValue &service : manager.value(QStringLiteral("services")).toArray())
        {
            if (!service.isString() || service.toString().isEmpty())
                return std::nullopt;
            snapshot.loginManagerServices.push_back(service.toString());
        }
    }
    else if (manager.contains(QStringLiteral("name")))
        return std::nullopt;

    QSet<QString> surfaceIds;
    for (const QJsonValue &value : data.value(QStringLiteral("surfaces")).toArray())
    {
        if (!value.isObject())
            return std::nullopt;
        const QJsonObject surface = value.toObject();
        const QString id = surface.value(QStringLiteral("id")).toString();
        const QString role = surface.value(QStringLiteral("role")).toString();
        if (id.isEmpty() || role.isEmpty() || surfaceIds.contains(id) ||
            !surface.value(QStringLiteral("present")).isBool() || !surface.value(QStringLiteral("wired")).isBool())
            return std::nullopt;
        const bool present = surface.value(QStringLiteral("present")).toBool();
        const bool wired = surface.value(QStringLiteral("wired")).toBool();
        if ((!present && wired) || (wired && !surface.value(QStringLiteral("mode")).isString()) ||
            (!wired && surface.contains(QStringLiteral("mode"))))
            return std::nullopt;
        surfaceIds.insert(id);
        snapshot.surfaces.push_back(
            {id, role, present, wired, wired ? surface.value(QStringLiteral("mode")).toString() : QString()});
    }

    const QString selinux = data.value(QStringLiteral("selinux_module")).toString();
    if (selinux == QLatin1String("loaded"))
        snapshot.selinuxModule = EngineLoginSnapshot::SelinuxModule::Loaded;
    else if (selinux == QLatin1String("not-loaded"))
        snapshot.selinuxModule = EngineLoginSnapshot::SelinuxModule::NotLoaded;
    else if (selinux != QLatin1String("unknown"))
        return std::nullopt;
    return snapshot;
}

void IrlumeBackend::requestRefresh(quint64 generation)
{
    if (m_process || m_cancelling)
    {
        m_pendingGeneration = generation;
        cancelRefresh();
        return;
    }
    beginRefresh(generation);
}

void IrlumeBackend::cancelRefresh()
{
    if (!m_process)
    {
        if (m_generation != 0)
            Q_EMIT refreshCancelled(m_generation);
        startPendingRefresh();
        return;
    }
    m_cancelling = true;
    m_timeout.stop();
    m_process->kill();
}

void IrlumeBackend::beginRefresh(quint64 generation)
{
    m_generation = generation;
    m_snapshot = {};
    m_snapshot.executablePresent = m_testExecutor || QFileInfo(m_executable).isExecutable();
    m_snapshot.handshake.state = ResultState::Loading;
    m_snapshot.status.state = ResultState::Pending;
    m_snapshot.doctor.state = ResultState::Pending;
    m_snapshot.profiles.state = ResultState::Pending;
    m_snapshot.loginStatus.state = ResultState::Pending;
    m_pendingCommands.clear();
    m_cancelling = false;
    emitProgress();

    if (!m_snapshot.executablePresent)
    {
        failHandshake({EngineOperation::Handshake, QStringLiteral("engine-not-installed"), false});
        return;
    }

    if (m_testExecutor)
    {
        QTimer::singleShot(0, this,
                           [this, generation]()
                           {
                               if (generation != m_generation || m_cancelling)
                                   return;
                               m_snapshot = refreshForTest();
                               Q_EMIT refreshProgress(generation, m_snapshot);
                               Q_EMIT refreshCompleted(generation, m_snapshot);
                           });
        return;
    }
    startCommand(Command::Version);
}

void IrlumeBackend::startCommand(Command command)
{
    cleanupProcess();
    m_currentCommand = command;
    m_standardOutput.clear();
    m_standardError.clear();
    m_outputTooLarge = false;
    m_timedOut = false;
    m_processHandled = false;
    setOperationState(command, ResultState::Loading);
    emitProgress();

    m_process = new QProcess(this);
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    const uid_t uid = geteuid();
    if (const passwd *account = getpwuid(uid))
    {
        environment.insert(QStringLiteral("USER"), QString::fromLocal8Bit(account->pw_name));
        environment.insert(QStringLiteral("LOGNAME"), QString::fromLocal8Bit(account->pw_name));
        environment.insert(QStringLiteral("HOME"), QString::fromLocal8Bit(account->pw_dir));
    }
    const QString runtimeDirectory = QStringLiteral("/run/user/%1").arg(uid);
    environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), runtimeDirectory);
    if (QFileInfo(runtimeDirectory + QStringLiteral("/bus")).exists())
        environment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
                           QStringLiteral("unix:path=%1/bus").arg(runtimeDirectory));

    m_process->setProcessEnvironment(environment);
    m_process->setProgram(m_executable);
    m_process->setArguments(arguments(command));
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &IrlumeBackend::drainProcess);
    connect(m_process, &QProcess::readyReadStandardError, this, &IrlumeBackend::drainProcess);
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error)
            {
                if (error != QProcess::FailedToStart || m_processHandled)
                    return;
                m_processHandled = true;
                m_timeout.stop();
                ProcessResult result;
                finishProcess(result);
            });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus)
            {
                if (m_processHandled)
                    return;
                m_processHandled = true;
                m_timeout.stop();
                drainProcess();
                ProcessResult result;
                result.started = true;
                result.finished = !m_outputTooLarge && !m_timedOut;
                result.outputTooLarge = m_outputTooLarge;
                result.timedOut = m_timedOut;
                result.exitCode = exitCode;
                result.standardOutput = m_standardOutput;
                result.standardError = m_standardError;
                finishProcess(result);
            });
    m_process->start(QIODevice::ReadOnly);
    m_timeout.start(ProcessTimeoutMs);
}

void IrlumeBackend::drainProcess()
{
    if (!m_process || m_outputTooLarge)
        return;
    const QByteArray outputChunk = m_process->readAllStandardOutput();
    const QByteArray errorChunk = m_process->readAllStandardError();
    if (outputChunk.size() > MaximumOutputBytes - m_standardOutput.size() ||
        errorChunk.size() > MaximumOutputBytes - m_standardError.size())
    {
        m_outputTooLarge = true;
        m_process->kill();
        return;
    }
    m_standardOutput.append(outputChunk);
    m_standardError.append(errorChunk);
}

void IrlumeBackend::finishProcess(const ProcessResult &result)
{
    const Command command = m_currentCommand;
    cleanupProcess();
    if (m_cancelling)
    {
        const quint64 cancelledGeneration = m_generation;
        m_cancelling = false;
        Q_EMIT refreshCancelled(cancelledGeneration);
        startPendingRefresh();
        return;
    }
    finishCommand(command, result);
}

void IrlumeBackend::finishCommand(Command command, const ProcessResult &result)
{
    EngineError error;
    const auto envelope = parseEnvelope(result, command, &error);
    if (!envelope)
    {
        if (command == Command::Version)
            failHandshake(error);
        else
        {
            setOperationError(command, error);
            emitProgress();
            startNextCommand();
        }
        return;
    }
    if (!envelope->ok)
    {
        if (command == Command::Version)
            failHandshake(envelope->error);
        else
        {
            setOperationError(command, envelope->error);
            emitProgress();
            startNextCommand();
        }
        return;
    }

    if (command == Command::Version)
    {
        if (!parseVersion(envelope->data, &m_snapshot, &error))
        {
            failHandshake(error);
            return;
        }
        m_snapshot.handshake.state = ResultState::Available;
        m_snapshot.handshake.data = EngineHandshakeSnapshot{1, envelope->engineVersion};
        m_snapshot.handshake.error.reset();
        const auto queueIfSupported = [this](EngineFeature feature, Command readCommand)
        {
            if (m_snapshot.capabilities.supports(feature))
            {
                setOperationState(readCommand, ResultState::Pending);
                m_pendingCommands.push_back(readCommand);
            }
            else
            {
                setOperationState(readCommand, ResultState::NotAdvertised);
            }
        };
        queueIfSupported(EngineFeature::StatusRead, Command::Status);
        queueIfSupported(EngineFeature::DoctorRead, Command::Doctor);
        queueIfSupported(EngineFeature::ProfilesRead, Command::ProfilesList);
        queueIfSupported(EngineFeature::LoginStatusRead, Command::LoginStatus);
        emitProgress();
        startNextCommand();
        return;
    }

    bool parsed = false;
    if (command == Command::Status)
    {
        auto data = parseStatus(envelope->data);
        parsed = data.has_value();
        if (data)
            m_snapshot.status.data = std::move(data);
    }
    else if (command == Command::Doctor)
    {
        auto data = parseDoctor(envelope->data);
        parsed = data.has_value();
        if (data)
            m_snapshot.doctor.data = std::move(data);
    }
    else if (command == Command::ProfilesList)
    {
        auto data = parseProfiles(envelope->data, m_snapshot.capabilities.maxProfiles);
        parsed = data.has_value();
        if (data)
            m_snapshot.profiles.data = std::move(data);
    }
    else if (command == Command::LoginStatus)
    {
        auto data = parseLogin(envelope->data);
        parsed = data.has_value();
        if (data)
            m_snapshot.loginStatus.data = std::move(data);
    }

    if (!parsed)
    {
        setOperationError(command,
                          {operationFor(command),
                           QStringLiteral("invalid-") + commandName(command) + QStringLiteral("-data"), false});
    }
    else
    {
        setOperationState(command, ResultState::Available);
    }
    emitProgress();
    startNextCommand();
}

void IrlumeBackend::startNextCommand()
{
    if (m_pendingCommands.isEmpty())
    {
        completeRefresh();
        return;
    }
    const Command next = m_pendingCommands.takeFirst();
    startCommand(next);
}

void IrlumeBackend::completeRefresh()
{
    Q_EMIT refreshCompleted(m_generation, m_snapshot);
    startPendingRefresh();
}

void IrlumeBackend::failHandshake(const EngineError &error)
{
    m_snapshot.handshake.state = ResultState::Failed;
    m_snapshot.handshake.data.reset();
    m_snapshot.handshake.error = error;
    m_snapshot.status.state = ResultState::NotAdvertised;
    m_snapshot.doctor.state = ResultState::NotAdvertised;
    m_snapshot.profiles.state = ResultState::NotAdvertised;
    m_snapshot.loginStatus.state = ResultState::NotAdvertised;
    emitProgress();
    completeRefresh();
}

void IrlumeBackend::setOperationState(Command command, ResultState state)
{
    if (command == Command::Version)
        m_snapshot.handshake.state = state;
    else if (command == Command::Status)
        m_snapshot.status.state = state;
    else if (command == Command::Doctor)
        m_snapshot.doctor.state = state;
    else if (command == Command::ProfilesList)
        m_snapshot.profiles.state = state;
    else if (command == Command::LoginStatus)
        m_snapshot.loginStatus.state = state;
}

void IrlumeBackend::setOperationError(Command command, const EngineError &error)
{
    setOperationState(command, ResultState::Failed);
    if (command == Command::Status)
    {
        m_snapshot.status.data.reset();
        m_snapshot.status.error = error;
    }
    else if (command == Command::Doctor)
    {
        m_snapshot.doctor.data.reset();
        m_snapshot.doctor.error = error;
    }
    else if (command == Command::ProfilesList)
    {
        m_snapshot.profiles.data.reset();
        m_snapshot.profiles.error = error;
    }
    else if (command == Command::LoginStatus)
    {
        m_snapshot.loginStatus.data.reset();
        m_snapshot.loginStatus.error = error;
    }
}

void IrlumeBackend::emitProgress()
{
    Q_EMIT refreshProgress(m_generation, m_snapshot);
}

void IrlumeBackend::cleanupProcess()
{
    if (!m_process)
        return;
    m_process->deleteLater();
    m_process = nullptr;
}

void IrlumeBackend::startPendingRefresh()
{
    if (!m_pendingGeneration)
        return;
    const quint64 next = *m_pendingGeneration;
    m_pendingGeneration.reset();
    beginRefresh(next);
}

EngineSnapshot IrlumeBackend::refreshForTest()
{
    EngineSnapshot snapshot;
    snapshot.executablePresent = static_cast<bool>(m_testExecutor);
    snapshot.handshake.state = ResultState::Loading;
    if (!snapshot.executablePresent)
    {
        snapshot.handshake.state = ResultState::Failed;
        snapshot.handshake.error =
            EngineError{EngineOperation::Handshake, QStringLiteral("engine-not-installed"), false};
        return snapshot;
    }

    EngineError error;
    const auto version = parseEnvelope(executeForTest(Command::Version), Command::Version, &error);
    if (!version || !version->ok || !parseVersion(version ? version->data : QJsonObject{}, &snapshot, &error))
    {
        snapshot.handshake.state = ResultState::Failed;
        snapshot.handshake.error = version && !version->ok ? version->error : error;
        return snapshot;
    }
    snapshot.handshake.state = ResultState::Available;
    snapshot.handshake.data = EngineHandshakeSnapshot{1, version->engineVersion};

    const auto runReadCommand = [this, &snapshot](Command command, auto parser, auto *destination)
    {
        EngineError commandError;
        const auto envelope = parseEnvelope(executeForTest(command), command, &commandError);
        if (!envelope || !envelope->ok)
        {
            destination->state = ResultState::Failed;
            destination->error = envelope ? envelope->error : commandError;
            return;
        }
        auto parsed = parser(envelope->data);
        if (!parsed)
        {
            destination->state = ResultState::Failed;
            destination->error =
                EngineError{operationFor(command),
                            QStringLiteral("invalid-") + commandName(command) + QStringLiteral("-data"), false};
            return;
        }
        destination->state = ResultState::Available;
        destination->data = std::move(parsed);
    };

    if (snapshot.capabilities.supports(EngineFeature::StatusRead))
        runReadCommand(Command::Status, parseStatus, &snapshot.status);
    if (snapshot.capabilities.supports(EngineFeature::DoctorRead))
        runReadCommand(Command::Doctor, parseDoctor, &snapshot.doctor);
    if (snapshot.capabilities.supports(EngineFeature::ProfilesRead))
        runReadCommand(
            Command::ProfilesList, [&snapshot](const QJsonObject &data)
            { return parseProfiles(data, snapshot.capabilities.maxProfiles); }, &snapshot.profiles);
    if (snapshot.capabilities.supports(EngineFeature::LoginStatusRead))
        runReadCommand(Command::LoginStatus, parseLogin, &snapshot.loginStatus);
    return snapshot;
}

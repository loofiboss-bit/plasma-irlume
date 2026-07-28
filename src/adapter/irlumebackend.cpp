// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumebackend.h"

#include <QElapsedTimer>
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

#include <algorithm>
#include <utility>

namespace
{
constexpr qsizetype MaximumOutputBytes = 256 * 1024;
constexpr int ProcessTimeoutMs = 3000;
constexpr int ProcessStartTimeoutMs = 1000;

const QSet<QString> SensitiveFields = {
    QStringLiteral("credential"), QStringLiteral("credentials"), QStringLiteral("device_path"),
    QStringLiteral("embedding"),  QStringLiteral("embeddings"),  QStringLiteral("frame"),
    QStringLiteral("frames"),     QStringLiteral("image"),       QStringLiteral("images"),
    QStringLiteral("password"),   QStringLiteral("path"),        QStringLiteral("user"),
    QStringLiteral("username"),
};

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

void drainProcess(QProcess *process, QByteArray *standardOutput, QByteArray *standardError, bool *tooLarge)
{
    const QByteArray outputChunk = process->readAllStandardOutput();
    const QByteArray errorChunk = process->readAllStandardError();
    if (outputChunk.size() > MaximumOutputBytes - standardOutput->size() ||
        errorChunk.size() > MaximumOutputBytes - standardError->size())
    {
        *tooLarge = true;
        return;
    }
    standardOutput->append(outputChunk);
    standardError->append(errorChunk);
}
} // namespace

IrlumeBackend::IrlumeBackend(QString executable) : m_executable(std::move(executable)) {}

IrlumeBackend::IrlumeBackend(Executor executor) : m_executor(std::move(executor)) {}

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

IrlumeBackend::ProcessResult IrlumeBackend::execute(Command command) const
{
    if (m_executor)
    {
        return m_executor(command);
    }

    QProcess process;
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));

    const uid_t uid = geteuid();
    if (const passwd *account = getpwuid(uid))
    {
        const QString user = QString::fromLocal8Bit(account->pw_name);
        const QString home = QString::fromLocal8Bit(account->pw_dir);
        environment.insert(QStringLiteral("USER"), user);
        environment.insert(QStringLiteral("LOGNAME"), user);
        environment.insert(QStringLiteral("HOME"), home);
    }
    const QString runtimeDirectory = QStringLiteral("/run/user/%1").arg(uid);
    environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), runtimeDirectory);
    if (QFileInfo(runtimeDirectory + QStringLiteral("/bus")).exists())
    {
        environment.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
                           QStringLiteral("unix:path=%1/bus").arg(runtimeDirectory));
    }

    process.setProcessEnvironment(environment);
    process.setProgram(m_executable);
    process.setArguments(arguments(command));
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(QIODevice::ReadOnly);

    ProcessResult result;
    result.started = process.waitForStarted(ProcessStartTimeoutMs);
    if (!result.started)
    {
        return result;
    }

    QElapsedTimer timer;
    timer.start();
    while (process.state() != QProcess::NotRunning && timer.elapsed() < ProcessTimeoutMs)
    {
        process.waitForReadyRead(std::min(50, ProcessTimeoutMs - static_cast<int>(timer.elapsed())));
        drainProcess(&process, &result.standardOutput, &result.standardError, &result.outputTooLarge);
        if (result.outputTooLarge)
        {
            process.kill();
            process.waitForFinished(1000);
            return result;
        }
    }
    if (process.state() != QProcess::NotRunning)
    {
        result.timedOut = true;
        process.kill();
        process.waitForFinished(1000);
        return result;
    }

    drainProcess(&process, &result.standardOutput, &result.standardError, &result.outputTooLarge);
    result.finished = !result.outputTooLarge;
    result.exitCode = process.exitCode();
    return result;
}

EngineError IrlumeBackend::processError(const ProcessResult &result)
{
    if (!result.started)
    {
        return {QStringLiteral("engine-not-installed"), false};
    }
    if (result.outputTooLarge)
    {
        return {QStringLiteral("engine-output-too-large"), false};
    }
    if (result.timedOut || !result.finished)
    {
        return {QStringLiteral("engine-timeout"), true};
    }
    return {QStringLiteral("engine-process-failed"), false};
}

bool IrlumeBackend::containsUnexpectedSensitiveField(const QJsonObject &object)
{
    for (auto it = object.constBegin(); it != object.constEnd(); ++it)
    {
        if (SensitiveFields.contains(it.key().toLower()))
        {
            return true;
        }
        if (it.value().isObject() && containsUnexpectedSensitiveField(it.value().toObject()))
        {
            return true;
        }
        if (it.value().isArray())
        {
            for (const QJsonValue &entry : it.value().toArray())
            {
                if (entry.isObject() && containsUnexpectedSensitiveField(entry.toObject()))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::optional<IrlumeBackend::Envelope> IrlumeBackend::parseEnvelope(const ProcessResult &result, Command command,
                                                                    EngineError *parseError)
{
    if (!result.started || !result.finished || result.outputTooLarge || result.timedOut)
    {
        *parseError = processError(result);
        return std::nullopt;
    }

    QJsonParseError jsonError;
    const QJsonDocument document = QJsonDocument::fromJson(result.standardOutput, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isObject())
    {
        *parseError = {QStringLiteral("invalid-json-document"), false};
        return std::nullopt;
    }
    const QJsonObject object = document.object();
    if (containsUnexpectedSensitiveField(object) || object.value(QStringLiteral("contract_version")).toInt(-1) != 1 ||
        !object.value(QStringLiteral("engine_version")).isString() ||
        object.value(QStringLiteral("engine_version")).toString().isEmpty() ||
        object.value(QStringLiteral("command")).toString() != commandName(command) ||
        !object.value(QStringLiteral("ok")).isBool())
    {
        *parseError = {QStringLiteral("invalid-document-contract"), false};
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
            *parseError = {QStringLiteral("engine-exit-mismatch"), false};
            return std::nullopt;
        }
        envelope.data = object.value(QStringLiteral("data")).toObject();
    }
    else
    {
        if (result.exitCode == 0 || hasData || !hasError || !object.value(QStringLiteral("error")).isObject())
        {
            *parseError = {QStringLiteral("engine-exit-mismatch"), false};
            return std::nullopt;
        }
        const QJsonObject error = object.value(QStringLiteral("error")).toObject();
        if (!error.value(QStringLiteral("code")).isString() ||
            error.value(QStringLiteral("code")).toString().isEmpty() ||
            !error.value(QStringLiteral("retryable")).isBool())
        {
            *parseError = {QStringLiteral("invalid-structured-error"), false};
            return std::nullopt;
        }
        envelope.error = {error.value(QStringLiteral("code")).toString(),
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

    snapshot->contractAvailable = true;
    snapshot->contractVersion = 1;
    snapshot->capabilities.statusRead = capabilities.contains(QStringLiteral("status-json"));
    snapshot->capabilities.doctorRead = capabilities.contains(QStringLiteral("doctor-json"));
    snapshot->capabilities.profilesRead = capabilities.contains(QStringLiteral("profiles-list-json"));
    snapshot->capabilities.loginStatusRead = capabilities.contains(QStringLiteral("login-status-json"));
    snapshot->capabilities.mutationSupported = false;
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

EngineSnapshot IrlumeBackend::refresh()
{
    EngineSnapshot snapshot;
    snapshot.executablePresent = m_executor || QFileInfo(m_executable).isExecutable();
    if (!snapshot.executablePresent)
    {
        snapshot.errors.push_back({QStringLiteral("engine-not-installed"), false});
        return snapshot;
    }

    EngineError error;
    const auto version = parseEnvelope(execute(Command::Version), Command::Version, &error);
    if (!version)
    {
        snapshot.errors.push_back(error);
        return snapshot;
    }
    snapshot.engineVersion = version->engineVersion;
    if (!version->ok)
    {
        snapshot.errors.push_back(version->error);
        return snapshot;
    }
    if (!parseVersion(version->data, &snapshot, &error))
    {
        snapshot.errors.push_back(error);
        return snapshot;
    }

    const auto runReadCommand = [this, &snapshot](Command command, auto parser, auto *destination)
    {
        EngineError commandError;
        const auto envelope = parseEnvelope(execute(command), command, &commandError);
        if (!envelope)
        {
            snapshot.errors.push_back(commandError);
            return;
        }
        if (!envelope->ok)
        {
            snapshot.errors.push_back(envelope->error);
            return;
        }
        auto parsed = parser(envelope->data);
        if (!parsed)
        {
            snapshot.errors.push_back(
                {QStringLiteral("invalid-") + commandName(command) + QStringLiteral("-data"), false});
            return;
        }
        *destination = std::move(parsed);
    };

    if (snapshot.capabilities.statusRead)
        runReadCommand(Command::Status, parseStatus, &snapshot.status);
    if (snapshot.capabilities.doctorRead)
        runReadCommand(Command::Doctor, parseDoctor, &snapshot.doctorChecks);
    if (snapshot.capabilities.profilesRead)
    {
        runReadCommand(
            Command::ProfilesList, [&snapshot](const QJsonObject &data)
            { return parseProfiles(data, snapshot.capabilities.maxProfiles); }, &snapshot.profiles);
    }
    if (snapshot.capabilities.loginStatusRead)
        runReadCommand(Command::LoginStatus, parseLogin, &snapshot.login);
    return snapshot;
}

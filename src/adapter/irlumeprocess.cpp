// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumeprocess.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <utility>

namespace
{
constexpr qsizetype MaximumOutputBytes = 256 * 1024;
constexpr int OperationTimeoutMs = 120000;
constexpr int CancellationGraceMs = 2000;

const QSet<QString> SensitiveFields = {
    QStringLiteral("credential"), QStringLiteral("credentials"), QStringLiteral("device_path"),
    QStringLiteral("embedding"),  QStringLiteral("embeddings"),  QStringLiteral("frame"),
    QStringLiteral("frames"),     QStringLiteral("image"),       QStringLiteral("images"),
    QStringLiteral("password"),   QStringLiteral("path"),        QStringLiteral("template"),
    QStringLiteral("templates"),  QStringLiteral("user"),        QStringLiteral("username"),
};

bool hasSafeEngineVersion(const QJsonObject &object)
{
    static const QRegularExpression versionPattern(QStringLiteral(R"(\A\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?\z)"));
    return versionPattern.match(object.value(QStringLiteral("engine_version")).toString()).hasMatch();
}

bool hasSupportedContractVersion(const QJsonObject &object)
{
    const int version = object.value(QStringLiteral("contract_version")).toInt(-1);
    return version == 1 || version == 2;
}

bool isTerminalType(const QString &type)
{
    return type == QLatin1String("completed") || type == QLatin1String("cancelled") || type == QLatin1String("failed");
}
} // namespace

IrlumeProcess::IrlumeProcess(QObject *parent) : IrlumeProcess(QStringLiteral("/usr/bin/irlume"), parent) {}

IrlumeProcess::IrlumeProcess(QString executable, QObject *parent) : QObject(parent), m_executable(std::move(executable))
{
    m_timeout.setSingleShot(true);
    m_cancelTimer.setSingleShot(true);

    connect(&m_process, &QProcess::readyReadStandardOutput, this, &IrlumeProcess::readStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this,
            [this]()
            {
                if (m_standardError.size() < MaximumOutputBytes)
                {
                    m_standardError +=
                        m_process.readAllStandardError().left(MaximumOutputBytes - m_standardError.size());
                }
                else
                {
                    m_process.readAllStandardError();
                }
            });
    connect(&m_process, &QProcess::finished, this, &IrlumeProcess::processFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &IrlumeProcess::processError);
    connect(&m_timeout, &QTimer::timeout, this, &IrlumeProcess::operationTimedOut);
    connect(&m_cancelTimer, &QTimer::timeout, this, &IrlumeProcess::forceCancellation);
}

IrlumeProcess::~IrlumeProcess()
{
    if (m_process.state() != QProcess::NotRunning)
    {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool IrlumeProcess::startOperation(Operation operation, const QString &profileId, const QString &scanId,
                                   const QString &newName)
{
    if (m_running)
    {
        return false;
    }
    if ((operation == Operation::AddScan || operation == Operation::DeleteProfile ||
         operation == Operation::DeleteScan || operation == Operation::RenameProfile ||
         operation == Operation::RenameScan) &&
        !isSafeOpaqueId(profileId))
    {
        Q_EMIT operationError(operation, QStringLiteral("invalid-profile-id"), false);
        return false;
    }
    if ((operation == Operation::DeleteScan || operation == Operation::RenameScan) && !isSafeOpaqueId(scanId))
    {
        Q_EMIT operationError(operation, QStringLiteral("invalid-scan-id"), false);
        return false;
    }
    if ((operation == Operation::RenameProfile || operation == Operation::RenameScan) && !isSafeDisplayName(newName))
    {
        Q_EMIT operationError(operation, QStringLiteral("invalid-display-name"), false);
        return false;
    }

    reset();
    m_operation = operation;
    m_running = true;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_executable);
    m_process.setArguments(argumentsForOperation(operation, profileId, scanId, newName));
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    m_process.start(QIODevice::ReadOnly);
    m_timeout.start(OperationTimeoutMs);
    return true;
}

void IrlumeProcess::cancel()
{
    if (!m_running || m_cancelRequested)
    {
        return;
    }
    m_cancelRequested = true;
    m_process.terminate();
    m_cancelTimer.start(CancellationGraceMs);
}

bool IrlumeProcess::isRunning() const
{
    return m_running;
}

QString IrlumeProcess::commandName(Operation operation)
{
    switch (operation)
    {
    case Operation::Capabilities:
        return QStringLiteral("version");
    case Operation::ListProfiles:
        return QStringLiteral("profiles.list");
    case Operation::Enroll:
        return QStringLiteral("enroll");
    case Operation::AuthTest:
        return QStringLiteral("auth.test");
    case Operation::AddScan:
        return QStringLiteral("profiles.add-scan");
    case Operation::DeleteProfile:
    case Operation::DeleteScan:
        return QStringLiteral("profiles.delete");
    case Operation::RenameProfile:
    case Operation::RenameScan:
        return QStringLiteral("profiles.rename");
    case Operation::ListCameras:
        return QStringLiteral("cameras.list");
    case Operation::TestEmitter:
        return QStringLiteral("cameras.emitter-test");
    }
    return {};
}

QStringList IrlumeProcess::argumentsForOperation(Operation operation, const QString &profileId, const QString &scanId,
                                                 const QString &newName)
{
    switch (operation)
    {
    case Operation::Capabilities:
        return {QStringLiteral("version"), QStringLiteral("--json")};
    case Operation::ListProfiles:
        return {QStringLiteral("profiles"), QStringLiteral("list"), QStringLiteral("--json")};
    case Operation::Enroll:
        return {QStringLiteral("enroll"), QStringLiteral("--events=jsonl")};
    case Operation::AuthTest:
        return {QStringLiteral("auth"), QStringLiteral("test"), QStringLiteral("--events=jsonl")};
    case Operation::AddScan:
        return {QStringLiteral("profiles"), QStringLiteral("add-scan"), QStringLiteral("--profile-id"), profileId,
                QStringLiteral("--events=jsonl")};
    case Operation::DeleteProfile:
        return {QStringLiteral("profiles"), QStringLiteral("delete"), QStringLiteral("--profile-id"), profileId,
                QStringLiteral("--json")};
    case Operation::DeleteScan:
        return {QStringLiteral("profiles"),     QStringLiteral("delete"),
                QStringLiteral("--profile-id"), profileId,
                QStringLiteral("--scan-id"),    scanId,
                QStringLiteral("--json")};
    case Operation::RenameProfile:
        return {QStringLiteral("profiles"),     QStringLiteral("rename"),
                QStringLiteral("--profile-id"), profileId,
                QStringLiteral("--name"),       newName,
                QStringLiteral("--json")};
    case Operation::RenameScan:
        return {QStringLiteral("profiles"),     QStringLiteral("rename"),
                QStringLiteral("--profile-id"), profileId,
                QStringLiteral("--scan-id"),    scanId,
                QStringLiteral("--name"),       newName,
                QStringLiteral("--json")};
    case Operation::ListCameras:
        return {QStringLiteral("cameras"), QStringLiteral("list"), QStringLiteral("--json")};
    case Operation::TestEmitter:
        return {QStringLiteral("cameras"), QStringLiteral("emitter-test"), QStringLiteral("--json")};
    }
    return {};
}

bool IrlumeProcess::isSafeOpaqueId(const QString &value)
{
    static const QRegularExpression idPattern(QStringLiteral(R"(\A[A-Za-z0-9][A-Za-z0-9._:-]{0,127}\z)"));
    return idPattern.match(value).hasMatch();
}

bool IrlumeProcess::isSafeDisplayName(const QString &value)
{
    if (value.isEmpty() || value.size() > 80 || value != value.trimmed())
    {
        return false;
    }
    return std::all_of(value.cbegin(), value.cend(), [](QChar character) { return character.isPrint(); });
}

IrlumeProcess::ParseResult IrlumeProcess::parseStreamEvent(const QJsonObject &object, Operation operation,
                                                           int expectedSequence, const QString &expectedOperationId)
{
    ParseResult result;
    result.event.operation = operation;

    if (containsSensitiveField(object) || !hasSupportedContractVersion(object) || !hasSafeEngineVersion(object) ||
        object.value(QStringLiteral("command")).toString() != commandName(operation) ||
        !object.value(QStringLiteral("sequence")).isDouble() || !object.value(QStringLiteral("terminal")).isBool())
    {
        result.errorCode = QStringLiteral("invalid-event-contract");
        return result;
    }

    const QString operationId = object.value(QStringLiteral("operation_id")).toString();
    const int sequence = object.value(QStringLiteral("sequence")).toInt(-1);
    const QString type = object.value(QStringLiteral("event")).toString();
    const bool terminal = object.value(QStringLiteral("terminal")).toBool();
    static const QSet<QString> eventTypes = {
        QStringLiteral("started"),   QStringLiteral("stage"),     QStringLiteral("progress"), QStringLiteral("preview"),
        QStringLiteral("completed"), QStringLiteral("cancelled"), QStringLiteral("failed"),
    };

    if (!isSafeOpaqueId(operationId) || sequence != expectedSequence ||
        (!expectedOperationId.isEmpty() && operationId != expectedOperationId) || !eventTypes.contains(type) ||
        (expectedSequence == 0 && type != QLatin1String("started")) ||
        (expectedSequence > 0 && type == QLatin1String("started")) || terminal != isTerminalType(type))
    {
        result.errorCode = QStringLiteral("invalid-event-sequence");
        return result;
    }

    if (object.contains(QStringLiteral("data")) && !object.value(QStringLiteral("data")).isObject())
    {
        result.errorCode = QStringLiteral("invalid-event-data");
        return result;
    }
    if (object.contains(QStringLiteral("error")) && !object.value(QStringLiteral("error")).isObject())
    {
        result.errorCode = QStringLiteral("invalid-event-error");
        return result;
    }

    const QJsonObject error = object.value(QStringLiteral("error")).toObject();
    result.event.command = commandName(operation);
    result.event.operationId = operationId;
    result.event.sequence = sequence;
    result.event.type = type;
    result.event.terminal = terminal;
    result.event.data = object.value(QStringLiteral("data")).toObject();
    result.event.errorCode = error.value(QStringLiteral("code")).toString();
    result.event.retryable = error.value(QStringLiteral("retryable")).toBool(false);
    result.ok = true;
    return result;
}

IrlumeProcess::ParseResult IrlumeProcess::parseDocument(const QJsonObject &object, Operation operation)
{
    ParseResult result;
    result.event.operation = operation;

    if (containsSensitiveField(object) || !hasSupportedContractVersion(object) || !hasSafeEngineVersion(object) ||
        object.value(QStringLiteral("command")).toString() != commandName(operation) ||
        !object.value(QStringLiteral("ok")).isBool())
    {
        result.errorCode = QStringLiteral("invalid-document-contract");
        return result;
    }

    const bool ok = object.value(QStringLiteral("ok")).toBool();
    if (ok && !object.value(QStringLiteral("data")).isObject())
    {
        result.errorCode = QStringLiteral("invalid-document-data");
        return result;
    }

    const QJsonObject error = object.value(QStringLiteral("error")).toObject();
    result.event.command = commandName(operation);
    result.event.sequence = 0;
    result.event.type = ok ? QStringLiteral("completed") : QStringLiteral("failed");
    result.event.terminal = true;
    result.event.data = object.value(QStringLiteral("data")).toObject();
    result.event.errorCode = error.value(QStringLiteral("code")).toString();
    result.event.retryable = error.value(QStringLiteral("retryable")).toBool(false);
    result.ok = true;
    return result;
}

void IrlumeProcess::readStandardOutput()
{
    const QByteArray chunk = m_process.readAllStandardOutput();
    if (m_failureEmitted)
    {
        return;
    }
    if (chunk.size() > MaximumOutputBytes - m_totalOutputBytes)
    {
        fail(QStringLiteral("engine-output-too-large"));
        m_process.kill();
        return;
    }
    m_totalOutputBytes += chunk.size();
    m_standardOutput += chunk;
    if (isStreamingOperation(m_operation) && !m_failureEmitted)
    {
        consumeStreamLines(false);
    }
}

void IrlumeProcess::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_running)
    {
        return;
    }

    readStandardOutput();
    m_timeout.stop();
    m_cancelTimer.stop();

    if (isStreamingOperation(m_operation))
    {
        consumeStreamLines(true);
        if (!m_terminalReceived && !m_failureEmitted)
        {
            fail(m_cancelRequested ? QStringLiteral("cancellation-unconfirmed")
                                   : QStringLiteral("missing-terminal-event"));
        }
    }
    else if (!m_failureEmitted)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(m_standardOutput, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            fail(m_cancelRequested ? QStringLiteral("cancellation-unconfirmed")
                                   : QStringLiteral("invalid-json-document"));
        }
        else
        {
            const ParseResult parsed = parseDocument(document.object(), m_operation);
            if (!parsed.ok)
            {
                fail(parsed.errorCode);
            }
            else
            {
                m_terminalReceived = true;
                m_pendingTerminalEvent = parsed.event;
            }
        }
    }

    if (!m_terminalReceived && !m_failureEmitted && (exitStatus != QProcess::NormalExit || exitCode != 0))
    {
        fail(QStringLiteral("engine-process-failed"));
    }
    if (m_pendingTerminalEvent && !m_failureEmitted)
    {
        const bool successfulExit = exitStatus == QProcess::NormalExit && exitCode == 0;
        const bool completedEvent = m_pendingTerminalEvent->type == QLatin1String("completed");
        if (successfulExit != completedEvent)
        {
            fail(QStringLiteral("engine-exit-mismatch"));
        }
    }
    const Operation completedOperation = m_operation;
    const QString errorCode = m_pendingErrorCode;
    const bool retryable = m_pendingErrorRetryable;
    const std::optional<Event> terminalEvent = m_pendingTerminalEvent;
    m_running = false;
    m_standardOutput.clear();
    m_standardError.clear();

    if (terminalEvent)
    {
        Q_EMIT eventReceived(*terminalEvent);
    }
    else if (!errorCode.isEmpty())
    {
        Q_EMIT operationError(completedOperation, errorCode, retryable);
    }
}

void IrlumeProcess::processError(QProcess::ProcessError error)
{
    if (!m_running || error == QProcess::Crashed)
    {
        return;
    }
    if (error == QProcess::FailedToStart)
    {
        fail(QStringLiteral("engine-not-installed"));
        m_running = false;
        m_timeout.stop();
        Q_EMIT operationError(m_operation, m_pendingErrorCode, m_pendingErrorRetryable);
    }
    else
    {
        fail(QStringLiteral("engine-process-error"));
    }
}

void IrlumeProcess::operationTimedOut()
{
    if (!m_running)
    {
        return;
    }
    fail(QStringLiteral("engine-timeout"), true);
    m_process.kill();
}

void IrlumeProcess::forceCancellation()
{
    if (m_running && m_process.state() != QProcess::NotRunning)
    {
        m_process.kill();
    }
}

bool IrlumeProcess::isStreamingOperation(Operation operation)
{
    return operation == Operation::Enroll || operation == Operation::AuthTest || operation == Operation::AddScan;
}

bool IrlumeProcess::containsSensitiveField(const QJsonValue &value)
{
    if (value.isArray())
    {
        const QJsonArray array = value.toArray();
        return std::any_of(array.cbegin(), array.cend(),
                           [](const QJsonValue &entry) { return containsSensitiveField(entry); });
    }
    if (!value.isObject())
    {
        return false;
    }

    const QJsonObject object = value.toObject();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it)
    {
        if (SensitiveFields.contains(it.key().toLower()) || containsSensitiveField(it.value()))
        {
            return true;
        }
    }
    return false;
}

void IrlumeProcess::consumeStreamLines(bool finalChunk)
{
    if (m_failureEmitted)
    {
        m_standardOutput.clear();
        return;
    }
    while (true)
    {
        const qsizetype newline = m_standardOutput.indexOf('\n');
        if (newline < 0)
        {
            break;
        }
        const QByteArray line = m_standardOutput.left(newline).trimmed();
        m_standardOutput.remove(0, newline + 1);
        if (line.isEmpty())
        {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            fail(QStringLiteral("invalid-jsonl-event"));
            m_process.kill();
            return;
        }

        const ParseResult parsed = parseStreamEvent(document.object(), m_operation, m_nextSequence, m_operationId);
        if (!parsed.ok || m_terminalReceived)
        {
            fail(parsed.ok ? QStringLiteral("event-after-terminal") : parsed.errorCode);
            m_process.kill();
            return;
        }

        if (m_operationId.isEmpty())
        {
            m_operationId = parsed.event.operationId;
        }
        ++m_nextSequence;
        m_terminalReceived = parsed.event.terminal;
        if (parsed.event.terminal)
        {
            m_pendingTerminalEvent = parsed.event;
        }
        else
        {
            Q_EMIT eventReceived(parsed.event);
        }
    }

    if (finalChunk && !m_standardOutput.trimmed().isEmpty() && !m_failureEmitted)
    {
        fail(QStringLiteral("truncated-jsonl-event"));
    }
}

void IrlumeProcess::fail(const QString &errorCode, bool retryable)
{
    if (m_failureEmitted)
    {
        return;
    }
    m_failureEmitted = true;
    m_pendingTerminalEvent.reset();
    m_pendingErrorCode = errorCode;
    m_pendingErrorRetryable = retryable;
}

void IrlumeProcess::reset()
{
    m_timeout.stop();
    m_cancelTimer.stop();
    m_standardOutput.clear();
    m_standardError.clear();
    m_totalOutputBytes = 0;
    m_operationId.clear();
    m_nextSequence = 0;
    m_terminalReceived = false;
    m_failureEmitted = false;
    m_pendingErrorCode.clear();
    m_pendingErrorRetryable = false;
    m_pendingTerminalEvent.reset();
    m_cancelRequested = false;
}

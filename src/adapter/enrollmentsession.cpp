// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollmentsession.h"

#include <QBuffer>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <algorithm>

#include <cmath>
#include <utility>

namespace
{
constexpr qsizetype MaximumBufferedBytes = 512 * 1024;
constexpr qsizetype MaximumEventBytes = 256 * 1024;
constexpr qsizetype MaximumJpegBytes = 128 * 1024;
constexpr int MaximumPreviewWidth = 640;
constexpr int MaximumPreviewHeight = 480;
constexpr int MinimumFrameIntervalMs = 125;
constexpr int OperationTimeoutMs = 120000;
constexpr int CancellationGraceMs = 2000;
constexpr int MaximumLandmarks = 478;

bool isSafeText(const QString &text, qsizetype maximumLength)
{
    if (text.size() > maximumLength || text != text.trimmed())
    {
        return false;
    }
    return std::all_of(text.cbegin(), text.cend(), [](QChar character) { return character.isPrint(); });
}

bool parseNormalizedPoint(const QJsonValue &value, QPointF &point)
{
    const QJsonArray coordinates = value.toArray();
    if (!value.isArray() || coordinates.size() != 2 || !coordinates.at(0).isDouble() || !coordinates.at(1).isDouble())
    {
        return false;
    }
    const double x = coordinates.at(0).toDouble();
    const double y = coordinates.at(1).toDouble();
    if (!std::isfinite(x) || !std::isfinite(y) || x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0)
    {
        return false;
    }
    point = QPointF(x, y);
    return true;
}

bool parseFaceBox(const QJsonValue &value, QRectF &box)
{
    const QJsonArray coordinates = value.toArray();
    if (!value.isArray() || coordinates.size() != 4)
    {
        return false;
    }
    for (const QJsonValue &coordinate : coordinates)
    {
        if (!coordinate.isDouble() || !std::isfinite(coordinate.toDouble()) || coordinate.toDouble() < 0.0 ||
            coordinate.toDouble() > 1.0)
        {
            return false;
        }
    }
    box = QRectF(coordinates.at(0).toDouble(), coordinates.at(1).toDouble(), coordinates.at(2).toDouble(),
                 coordinates.at(3).toDouble());
    return box.right() <= 1.0 && box.bottom() <= 1.0 && box.width() > 0.0 && box.height() > 0.0;
}

bool parsePosition(const QJsonValue &value, EnrollmentSession::Position &position)
{
    const QJsonObject object = value.toObject();
    const QStringList requiredBooleans = {
        QStringLiteral("face_detected"), QStringLiteral("centered"), QStringLiteral("facing_camera"),
        QStringLiteral("well_lit"),      QStringLiteral("ir_ready"), QStringLiteral("well_framed"),
    };
    if (!value.isObject())
    {
        return false;
    }
    for (const QString &key : requiredBooleans)
    {
        if (!object.value(key).isBool())
        {
            return false;
        }
    }
    if (!object.value(QStringLiteral("quality")).isDouble() || !object.value(QStringLiteral("countdown")).isDouble())
    {
        return false;
    }
    const int quality = object.value(QStringLiteral("quality")).toInt(-1);
    const int countdown = object.value(QStringLiteral("countdown")).toInt(-1);
    const QString guidance = object.value(QStringLiteral("guidance")).toString();
    if (quality < 0 || quality > 100 || countdown < 0 || countdown > 3 || !isSafeText(guidance, 160))
    {
        return false;
    }
    position.faceDetected = object.value(QStringLiteral("face_detected")).toBool();
    position.centered = object.value(QStringLiteral("centered")).toBool();
    position.facingCamera = object.value(QStringLiteral("facing_camera")).toBool();
    position.wellLit = object.value(QStringLiteral("well_lit")).toBool();
    position.irReady = object.value(QStringLiteral("ir_ready")).toBool();
    position.wellFramed = object.value(QStringLiteral("well_framed")).toBool();
    position.quality = quality;
    position.countdown = countdown;
    position.guidance = guidance;
    return true;
}

QString eventErrorCode(const QJsonObject &object)
{
    const QJsonValue error = object.value(QStringLiteral("error"));
    return error.isObject() ? error.toObject().value(QStringLiteral("code")).toString() : QString{};
}
} // namespace

EnrollmentSession::EnrollmentSession(QObject *parent) : EnrollmentSession(QStringLiteral("/usr/bin/irlume"), parent) {}

EnrollmentSession::EnrollmentSession(QString executable, QObject *parent)
    : QObject(parent), m_executable(std::move(executable))
{
    m_timeout.setSingleShot(true);
    m_cancelTimer.setSingleShot(true);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &EnrollmentSession::readStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this,
            [this]()
            {
                const qsizetype remaining = MaximumEventBytes - m_standardError.size();
                if (remaining > 0)
                {
                    m_standardError += m_process.readAllStandardError().left(remaining);
                }
                else
                {
                    m_process.readAllStandardError();
                }
            });
    connect(&m_process, &QProcess::finished, this, &EnrollmentSession::processFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &EnrollmentSession::processError);
    connect(&m_timeout, &QTimer::timeout, this, &EnrollmentSession::operationTimedOut);
    connect(&m_cancelTimer, &QTimer::timeout, this, &EnrollmentSession::forceCancellation);
}

EnrollmentSession::~EnrollmentSession()
{
    clearFrame();
    if (m_process.state() != QProcess::NotRunning)
    {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool EnrollmentSession::startOperation(IrlumeProcess::Operation operation, const QString &profileId)
{
    if (m_active || !isPreviewOperation(operation) ||
        ((operation == IrlumeProcess::Operation::AddScan) && !IrlumeProcess::isSafeOpaqueId(profileId)))
    {
        return false;
    }

    reset();
    m_operation = operation;
    m_active = true;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(m_executable);
    m_process.setArguments(argumentsForOperation(operation, profileId));
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    m_process.start(QIODevice::ReadOnly);
    m_timeout.start(OperationTimeoutMs);
    Q_EMIT stateChanged();
    return true;
}

void EnrollmentSession::cancel()
{
    if (!m_active || m_cancelRequested)
    {
        return;
    }
    m_cancelRequested = true;
    clearFrame();
    m_process.terminate();
    m_cancelTimer.start(CancellationGraceMs);
}

void EnrollmentSession::clearFrame()
{
    if (!m_frame.isNull())
    {
        m_frame.detach();
        m_frame.fill(0);
    }
    m_frame = {};
    m_landmarks.clear();
    m_faceBox = {};
    m_spectrum.clear();
    ++m_frameRevision;
    Q_EMIT frameChanged();
}

bool EnrollmentSession::active() const
{
    return m_active;
}

bool EnrollmentSession::frameAvailable() const
{
    return !m_frame.isNull();
}

QString EnrollmentSession::spectrum() const
{
    return m_spectrum;
}

QString EnrollmentSession::guidance() const
{
    return m_position.guidance;
}

int EnrollmentSession::quality() const
{
    return m_position.quality;
}

bool EnrollmentSession::faceDetected() const
{
    return m_position.faceDetected;
}

bool EnrollmentSession::centered() const
{
    return m_position.centered;
}

bool EnrollmentSession::facingCamera() const
{
    return m_position.facingCamera;
}

bool EnrollmentSession::wellLit() const
{
    return m_position.wellLit;
}

bool EnrollmentSession::irReady() const
{
    return m_position.irReady;
}

bool EnrollmentSession::wellFramed() const
{
    return m_position.wellFramed;
}

int EnrollmentSession::countdown() const
{
    return m_position.countdown;
}

int EnrollmentSession::droppedFrames() const
{
    return m_droppedFrames;
}

quint64 EnrollmentSession::frameRevision() const
{
    return m_frameRevision;
}

QVariantList EnrollmentSession::landmarksVariant() const
{
    QVariantList values;
    values.reserve(m_landmarks.size());
    for (const QPointF &point : m_landmarks)
    {
        values.push_back(point);
    }
    return values;
}

QVector<QPointF> EnrollmentSession::landmarks() const
{
    return m_landmarks;
}

QRectF EnrollmentSession::faceBox() const
{
    return m_faceBox;
}

QImage EnrollmentSession::frame() const
{
    return m_frame;
}

QStringList EnrollmentSession::argumentsForOperation(IrlumeProcess::Operation operation, const QString &profileId)
{
    QStringList arguments = IrlumeProcess::argumentsForOperation(operation, profileId);
    if (isPreviewOperation(operation))
    {
        arguments.push_back(QStringLiteral("--preview=ir-jpeg"));
        arguments.push_back(QStringLiteral("--preview-max-fps=8"));
        arguments.push_back(QStringLiteral("--preview-max-size=640x480"));
    }
    return arguments;
}

EnrollmentSession::ParseResult EnrollmentSession::parseEvent(const QJsonObject &object,
                                                             IrlumeProcess::Operation operation, int expectedSequence,
                                                             const QString &expectedOperationId,
                                                             const QString &expectedSessionId)
{
    ParseResult result;
    const QString sessionId = object.value(QStringLiteral("session_id")).toString();
    if (!IrlumeProcess::isSafeOpaqueId(sessionId) || (!expectedSessionId.isEmpty() && sessionId != expectedSessionId))
    {
        result.errorCode = QStringLiteral("invalid-preview-session");
        return result;
    }

    const bool preview = object.value(QStringLiteral("event")).toString() == QLatin1String("preview");
    QJsonObject sanitized = object;
    QJsonObject data = object.value(QStringLiteral("data")).toObject();
    const QString encodedFrame = data.take(QStringLiteral("frame_jpeg_base64")).toString();
    sanitized.insert(QStringLiteral("data"), data);

    const auto parsed = IrlumeProcess::parseStreamEvent(sanitized, operation, expectedSequence, expectedOperationId);
    if (!parsed.ok)
    {
        result.errorCode = parsed.errorCode;
        return result;
    }

    result.event = parsed.event;
    result.sessionId = sessionId;
    result.preview = preview;
    if (!preview)
    {
        result.ok = true;
        return result;
    }

    if (encodedFrame.isEmpty() || encodedFrame.size() > (MaximumJpegBytes * 4 / 3) + 8)
    {
        result.errorCode = QStringLiteral("invalid-preview-frame");
        return result;
    }
    const auto decoded =
        QByteArray::fromBase64Encoding(encodedFrame.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded || decoded.decoded.size() > MaximumJpegBytes || !decoded.decoded.startsWith("\xff\xd8"))
    {
        result.errorCode = QStringLiteral("invalid-preview-frame");
        return result;
    }

    QBuffer buffer;
    buffer.setData(decoded.decoded);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "JPEG");
    reader.setDecideFormatFromContent(true);
    const QSize declaredSize(data.value(QStringLiteral("width")).toInt(-1),
                             data.value(QStringLiteral("height")).toInt(-1));
    const QSize encodedSize = reader.size();
    if (!declaredSize.isValid() || declaredSize.width() > MaximumPreviewWidth ||
        declaredSize.height() > MaximumPreviewHeight || encodedSize != declaredSize)
    {
        result.errorCode = QStringLiteral("invalid-preview-dimensions");
        return result;
    }
    result.frame = reader.read();
    if (result.frame.isNull() || result.frame.size() != declaredSize)
    {
        result.errorCode = QStringLiteral("invalid-preview-frame");
        return result;
    }

    result.spectrum = data.value(QStringLiteral("spectrum")).toString();
    if (result.spectrum != QLatin1String("ir") && result.spectrum != QLatin1String("rgb"))
    {
        result.errorCode = QStringLiteral("invalid-preview-spectrum");
        return result;
    }

    const QJsonArray landmarkValues = data.value(QStringLiteral("landmarks")).toArray();
    if (!data.value(QStringLiteral("landmarks")).isArray() || landmarkValues.size() != MaximumLandmarks)
    {
        result.errorCode = QStringLiteral("invalid-preview-landmarks");
        return result;
    }
    result.landmarks.reserve(landmarkValues.size());
    for (const QJsonValue &value : landmarkValues)
    {
        QPointF point;
        if (!parseNormalizedPoint(value, point))
        {
            result.errorCode = QStringLiteral("invalid-preview-landmarks");
            return result;
        }
        result.landmarks.push_back(point);
    }
    if (!parseFaceBox(data.value(QStringLiteral("face_box")), result.faceBox) ||
        !parsePosition(data.value(QStringLiteral("position")), result.position))
    {
        result.errorCode = QStringLiteral("invalid-preview-position");
        return result;
    }
    result.ok = true;
    return result;
}

void EnrollmentSession::readStandardOutput()
{
    const QByteArray chunk = m_process.readAllStandardOutput();
    if (m_failureEmitted)
    {
        return;
    }
    if (m_output.size() + chunk.size() > MaximumBufferedBytes)
    {
        fail(QStringLiteral("preview-output-too-large"));
        m_process.kill();
        return;
    }
    m_output += chunk;
    consumeLines(false);
}

void EnrollmentSession::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_active)
    {
        return;
    }
    readStandardOutput();
    consumeLines(true);
    m_timeout.stop();
    m_cancelTimer.stop();

    if (!m_terminalReceived && !m_failureEmitted)
    {
        fail(m_cancelRequested ? QStringLiteral("cancellation-unconfirmed") : QStringLiteral("missing-terminal-event"));
    }
    if (m_pendingTerminalEvent && !m_failureEmitted)
    {
        const bool successfulExit = exitStatus == QProcess::NormalExit && exitCode == 0;
        const bool completed = m_pendingTerminalEvent->type == QLatin1String("completed");
        if (successfulExit != completed)
        {
            fail(QStringLiteral("engine-exit-mismatch"));
        }
    }

    const IrlumeProcess::Operation operation = m_operation;
    const QString errorCode = m_pendingErrorCode;
    const bool retryable = m_pendingErrorRetryable;
    const auto terminalEvent = m_pendingTerminalEvent;
    m_active = false;
    clearFrame();
    Q_EMIT stateChanged();
    if (terminalEvent && !m_failureEmitted)
    {
        Q_EMIT eventReceived(*terminalEvent);
    }
    else if (!errorCode.isEmpty())
    {
        Q_EMIT operationError(operation, errorCode, retryable);
    }
}

void EnrollmentSession::processError(QProcess::ProcessError error)
{
    if (!m_active || error == QProcess::Crashed)
    {
        return;
    }
    if (error == QProcess::FailedToStart)
    {
        fail(QStringLiteral("engine-not-installed"));
        m_active = false;
        m_timeout.stop();
        clearFrame();
        Q_EMIT stateChanged();
        Q_EMIT operationError(m_operation, m_pendingErrorCode, m_pendingErrorRetryable);
    }
    else
    {
        fail(QStringLiteral("engine-process-error"));
    }
}

void EnrollmentSession::operationTimedOut()
{
    if (!m_active)
    {
        return;
    }
    fail(QStringLiteral("engine-timeout"), true);
    clearFrame();
    m_process.kill();
}

void EnrollmentSession::forceCancellation()
{
    if (m_active && m_process.state() != QProcess::NotRunning)
    {
        m_process.kill();
    }
}

bool EnrollmentSession::isPreviewOperation(IrlumeProcess::Operation operation)
{
    return operation == IrlumeProcess::Operation::Enroll || operation == IrlumeProcess::Operation::AuthTest ||
           operation == IrlumeProcess::Operation::AddScan;
}

void EnrollmentSession::consumeLines(bool finalChunk)
{
    while (!m_failureEmitted)
    {
        const qsizetype newline = m_output.indexOf('\n');
        if (newline < 0)
        {
            break;
        }
        if (newline > MaximumEventBytes)
        {
            fail(QStringLiteral("preview-event-too-large"));
            m_process.kill();
            return;
        }
        QByteArray line = m_output.left(newline).trimmed();
        m_output.remove(0, newline + 1);
        if (line.isEmpty())
        {
            continue;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
        line.fill('\0');
        line.clear();
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            fail(QStringLiteral("invalid-jsonl-event"));
            m_process.kill();
            return;
        }
        ParseResult parsed = parseEvent(document.object(), m_operation, m_nextSequence, m_operationId, m_sessionId);
        if (!parsed.ok || m_terminalReceived)
        {
            fail(parsed.ok ? QStringLiteral("event-after-terminal") : parsed.errorCode,
                 eventErrorCode(document.object()) == QLatin1String("camera-busy"));
            m_process.kill();
            return;
        }
        if (m_operationId.isEmpty())
        {
            m_operationId = parsed.event.operationId;
            m_sessionId = parsed.sessionId;
        }
        ++m_nextSequence;
        m_terminalReceived = parsed.event.terminal;
        if (parsed.preview)
        {
            acceptPreview(std::move(parsed));
        }
        else if (parsed.event.terminal)
        {
            m_pendingTerminalEvent = parsed.event;
        }
        else
        {
            Q_EMIT eventReceived(parsed.event);
        }
    }

    if (finalChunk && !m_output.trimmed().isEmpty() && !m_failureEmitted)
    {
        fail(QStringLiteral("truncated-jsonl-event"));
    }
}

void EnrollmentSession::acceptPreview(ParseResult parsed)
{
    m_position = std::move(parsed.position);
    if (m_frameTimer.isValid() && m_frameTimer.elapsed() < MinimumFrameIntervalMs)
    {
        ++m_droppedFrames;
        Q_EMIT stateChanged();
        return;
    }
    m_frameTimer.restart();
    if (!m_frame.isNull())
    {
        m_frame.detach();
        m_frame.fill(0);
    }
    m_frame = std::move(parsed.frame);
    m_landmarks = std::move(parsed.landmarks);
    m_faceBox = parsed.faceBox;
    m_spectrum = std::move(parsed.spectrum);
    ++m_frameRevision;
    Q_EMIT frameChanged();
    Q_EMIT stateChanged();
}

void EnrollmentSession::fail(const QString &errorCode, bool retryable)
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

void EnrollmentSession::reset()
{
    m_timeout.stop();
    m_cancelTimer.stop();
    clearFrame();
    m_output.clear();
    m_standardError.fill('\0');
    m_standardError.clear();
    m_operationId.clear();
    m_sessionId.clear();
    m_nextSequence = 0;
    m_terminalReceived = false;
    m_failureEmitted = false;
    m_cancelRequested = false;
    m_pendingErrorCode.clear();
    m_pendingErrorRetryable = false;
    m_pendingTerminalEvent.reset();
    m_position = {};
    m_droppedFrames = 0;
    m_frameTimer.invalidate();
}

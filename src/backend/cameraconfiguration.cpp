// SPDX-License-Identifier: GPL-3.0-or-later

#include "cameraconfiguration.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QSet>

#include <algorithm>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("CameraConfiguration", text);
}

bool safeLabel(const QString &label)
{
    return !label.isEmpty() && label.size() <= 80 && label == label.trimmed() &&
           !label.contains(QStringLiteral("/dev/")) && !label.contains(QStringLiteral("\\\\.\\\\")) &&
           std::all_of(label.cbegin(), label.cend(), [](QChar character) { return character.isPrint(); });
}
} // namespace

CameraConfiguration::CameraConfiguration(IrlumeProcess *process, QObject *parent)
    : CameraConfiguration(process, new KAuthActionRunner, parent)
{
    m_runner->setParent(this);
}

CameraConfiguration::CameraConfiguration(IrlumeProcess *process, AuthActionRunner *runner, QObject *parent)
    : QObject(parent), m_process(process), m_runner(runner),
      m_statusText(translate("Check the reviewed camera pair and infrared emitter."))
{
    Q_ASSERT(m_process);
    Q_ASSERT(m_runner);
    connect(m_process, &IrlumeProcess::eventReceived, this, &CameraConfiguration::handleEvent);
    connect(m_process, &IrlumeProcess::operationError, this, &CameraConfiguration::handleProcessError);
    connect(m_runner, &AuthActionRunner::completed, this, &CameraConfiguration::handleActionCompleted);
}

bool CameraConfiguration::busy() const
{
    return m_busy;
}

bool CameraConfiguration::contractAvailable() const
{
    return m_contractAvailable;
}

bool CameraConfiguration::hasPairs() const
{
    return !m_pairIds.isEmpty();
}

bool CameraConfiguration::ready() const
{
    return m_contractAvailable && m_activeKnown && m_activePairIndex >= 0;
}

QStringList CameraConfiguration::pairLabels() const
{
    return m_pairLabels;
}

int CameraConfiguration::selectedPairIndex() const
{
    return m_selectedPairIndex;
}

void CameraConfiguration::setSelectedPairIndex(int index)
{
    if (index < 0 || index >= m_pairIds.size() || index == m_selectedPairIndex)
    {
        return;
    }
    m_selectedPairIndex = index;
    Q_EMIT stateChanged();
}

int CameraConfiguration::activePairIndex() const
{
    return m_activePairIndex;
}

bool CameraConfiguration::emitterTested() const
{
    return m_emitterTested;
}

bool CameraConfiguration::emitterAvailable() const
{
    return m_emitterAvailable;
}

int CameraConfiguration::emitterControlCount() const
{
    return m_emitterControlCount;
}

QString CameraConfiguration::captureMode() const
{
    return m_captureMode;
}

bool CameraConfiguration::tuneConclusive() const
{
    return m_tuneConclusive;
}

QString CameraConfiguration::statusText() const
{
    return m_statusText;
}

QString CameraConfiguration::errorCode() const
{
    return m_errorCode;
}

void CameraConfiguration::refresh()
{
    if (m_busy)
    {
        return;
    }
    m_contractAvailable = false;
    m_activeKnown = false;
    m_emitterTested = false;
    m_emitterAvailable = false;
    m_emitterControlCount = 0;
    m_pairIds.clear();
    m_pairLabels.clear();
    m_selectedPairIndex = -1;
    m_activePairIndex = -1;
    m_errorCode.clear();
    m_statusText = translate("Checking the camera configuration contract…");
    beginRead(ReadPhase::Capabilities, IrlumeProcess::Operation::Capabilities);
}

void CameraConfiguration::selectPair()
{
    if (m_busy || !m_contractAvailable || m_selectedPairIndex < 0 || m_selectedPairIndex >= m_pairIds.size())
    {
        finishError(QStringLiteral("camera-selection-not-ready"));
        return;
    }
    if (m_selectedPairIndex == m_activePairIndex && m_activeKnown)
    {
        m_statusText = translate("The selected camera pair is already active.");
        m_errorCode.clear();
        Q_EMIT stateChanged();
        return;
    }
    m_busy = true;
    m_errorCode.clear();
    m_statusText = translate("Selecting and verifying the secure camera pair…");
    Q_EMIT stateChanged();
    if (!m_runner->start(AuthAction::SelectCamera, {{QStringLiteral("pairId"), m_pairIds.at(m_selectedPairIndex)}}))
    {
        finishError(QStringLiteral("operation-start-failed"));
    }
}

void CameraConfiguration::setupEmitter()
{
    if (m_busy || !ready())
    {
        finishError(QStringLiteral("emitter-setup-not-ready"));
        return;
    }
    m_busy = true;
    m_errorCode.clear();
    m_statusText = translate("Configuring and verifying the infrared emitter…");
    Q_EMIT stateChanged();
    if (!m_runner->start(AuthAction::SetupEmitter))
    {
        finishError(QStringLiteral("operation-start-failed"));
    }
}

void CameraConfiguration::tuneCamera()
{
    if (m_busy || !ready())
    {
        finishError(QStringLiteral("camera-tuning-not-ready"));
        return;
    }
    m_busy = true;
    m_errorCode.clear();
    m_statusText = translate("Measuring and selecting the bounded capture mode…");
    Q_EMIT stateChanged();
    if (!m_runner->start(AuthAction::TuneCamera))
    {
        finishError(QStringLiteral("operation-start-failed"));
    }
}

void CameraConfiguration::handleEvent(const IrlumeProcess::Event &event)
{
    if (!m_busy)
    {
        return;
    }
    if (m_phase == ReadPhase::Capabilities && event.operation == IrlumeProcess::Operation::Capabilities)
    {
        const QJsonArray capabilities = event.data.value(QStringLiteral("capabilities")).toArray();
        const bool available = std::any_of(capabilities.cbegin(), capabilities.cend(), [](const QJsonValue &value)
                                           { return value.toString() == QLatin1String("camera-config-json"); });
        if (!available)
        {
            finishError(QStringLiteral("camera-contract-unavailable"));
            return;
        }
        m_contractAvailable = true;
        m_statusText = translate("Reading reviewed camera pairs…");
        beginRead(ReadPhase::Cameras, IrlumeProcess::Operation::ListCameras);
        return;
    }
    if (m_phase == ReadPhase::Cameras && event.operation == IrlumeProcess::Operation::ListCameras)
    {
        if (!parseCameras(event.data))
        {
            finishError(QStringLiteral("invalid-camera-list"));
            return;
        }
        m_statusText = translate("Testing infrared emitter controls without changing them…");
        beginRead(ReadPhase::Emitter, IrlumeProcess::Operation::TestEmitter);
        return;
    }
    if (m_phase == ReadPhase::Emitter && event.operation == IrlumeProcess::Operation::TestEmitter)
    {
        const int controlCount = event.data.value(QStringLiteral("control_count")).toInt(-1);
        if (!event.data.value(QStringLiteral("available")).isBool() ||
            event.data.value(QStringLiteral("mutated")).toBool(true) || controlCount < 0 || controlCount > 256)
        {
            finishError(QStringLiteral("invalid-emitter-result"));
            return;
        }
        m_emitterTested = true;
        m_emitterAvailable = event.data.value(QStringLiteral("available")).toBool();
        m_emitterControlCount = controlCount;
        m_phase = ReadPhase::Idle;
        m_busy = false;
        m_errorCode.clear();
        m_statusText = m_emitterAvailable
                           ? translate("The active camera and infrared emitter controls are ready.")
                           : translate("The camera pair is available, but no infrared emitter control was found.");
        Q_EMIT stateChanged();
        return;
    }
    finishError(QStringLiteral("unexpected-camera-response"));
}

void CameraConfiguration::handleProcessError(IrlumeProcess::Operation operation, const QString &errorCode, bool)
{
    if (m_busy && ((m_phase == ReadPhase::Capabilities && operation == IrlumeProcess::Operation::Capabilities) ||
                   (m_phase == ReadPhase::Cameras && operation == IrlumeProcess::Operation::ListCameras) ||
                   (m_phase == ReadPhase::Emitter && operation == IrlumeProcess::Operation::TestEmitter)))
    {
        finishError(errorCode);
    }
}

void CameraConfiguration::handleActionCompleted(AuthAction action, bool success, const QVariantMap &data,
                                                const QString &errorCode)
{
    if (action != AuthAction::SelectCamera && action != AuthAction::SetupEmitter && action != AuthAction::TuneCamera)
    {
        return;
    }
    m_busy = false;
    if (!success)
    {
        finishError(errorCode.isEmpty() ? QStringLiteral("camera-operation-failed") : errorCode);
        return;
    }

    m_errorCode.clear();
    if (action == AuthAction::TuneCamera)
    {
        m_captureMode = data.value(QStringLiteral("captureMode")).toString();
        m_tuneConclusive = data.value(QStringLiteral("conclusive")).toBool();
        m_statusText = m_tuneConclusive ? translate("Camera tuning completed with a conclusive capture mode.")
                                        : translate("Camera tuning completed; the measurement was not conclusive.");
        Q_EMIT stateChanged();
        Q_EMIT configurationChanged();
        return;
    }
    if (action == AuthAction::SetupEmitter)
    {
        m_emitterTested = true;
        m_emitterAvailable = data.value(QStringLiteral("verified")).toBool();
        m_emitterControlCount = data.value(QStringLiteral("controlCount")).toInt();
        m_statusText = translate("The infrared emitter was configured and verified.");
        Q_EMIT stateChanged();
        Q_EMIT configurationChanged();
        return;
    }

    m_statusText = translate("The camera pair was selected and independently verified.");
    Q_EMIT configurationChanged();
    refresh();
}

bool CameraConfiguration::parseCameras(const QJsonObject &data)
{
    const QJsonArray pairs = data.value(QStringLiteral("pairs")).toArray();
    if (!data.value(QStringLiteral("active_known")).isBool() ||
        !data.value(QStringLiteral("selection_requires_authorization")).toBool(false) || pairs.size() > 16)
    {
        return false;
    }

    QStringList ids;
    QStringList labels;
    QSet<QString> observedIds;
    int activeIndex = -1;
    for (const QJsonValue &value : pairs)
    {
        if (!value.isObject())
        {
            return false;
        }
        const QJsonObject pair = value.toObject();
        const QString id = pair.value(QStringLiteral("pair_id")).toString();
        const QString label = pair.value(QStringLiteral("display_name")).toString();
        if (!IrlumeProcess::isSafeOpaqueId(id) || !safeLabel(label) || observedIds.contains(id) ||
            pair.value(QStringLiteral("security_tier")).toString() != QLatin1String("secure") ||
            !pair.value(QStringLiteral("built_in")).isBool() || !pair.value(QStringLiteral("active")).isBool())
        {
            return false;
        }
        if (pair.value(QStringLiteral("active")).toBool())
        {
            if (activeIndex >= 0)
            {
                return false;
            }
            activeIndex = ids.size();
        }
        observedIds.insert(id);
        ids.push_back(id);
        labels.push_back(label);
    }

    m_pairIds = ids;
    m_pairLabels = labels;
    m_activeKnown = data.value(QStringLiteral("active_known")).toBool();
    m_activePairIndex = activeIndex;
    m_selectedPairIndex = activeIndex >= 0 ? activeIndex : (ids.isEmpty() ? -1 : 0);
    return !m_activeKnown || activeIndex >= 0;
}

void CameraConfiguration::finishError(const QString &errorCode)
{
    m_phase = ReadPhase::Idle;
    m_busy = false;
    m_errorCode = errorCode.isEmpty() ? QStringLiteral("camera-operation-failed") : errorCode;
    if (m_errorCode == QLatin1String("camera-contract-unavailable"))
    {
        m_contractAvailable = false;
        m_statusText = translate("The installed irlume release does not provide the reviewed camera contract.");
    }
    else if (m_errorCode == QLatin1String("authorization-cancelled"))
    {
        m_statusText = translate("Administrator authentication was cancelled. Nothing was changed.");
    }
    else
    {
        m_statusText = translate("The camera operation failed without a verified configuration change.");
    }
    Q_EMIT stateChanged();
}

void CameraConfiguration::beginRead(ReadPhase phase, IrlumeProcess::Operation operation)
{
    m_phase = phase;
    m_busy = true;
    Q_EMIT stateChanged();
    if (!m_process->startOperation(operation))
    {
        finishError(QStringLiteral("operation-start-failed"));
    }
}

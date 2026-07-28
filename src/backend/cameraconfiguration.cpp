// SPDX-License-Identifier: GPL-3.0-or-later

#include "cameraconfiguration.h"

#include <QCoreApplication>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("CameraConfiguration", text);
}
} // namespace

CameraConfiguration::CameraConfiguration(QObject *parent) : QObject(parent) {}

void CameraConfiguration::applySnapshot(const EngineSnapshot &snapshot)
{
    m_contractAvailable = snapshot.contractAvailable();
    m_resultState = snapshot.status.state;
    m_readOnlyAvailable = snapshot.status.state == ResultState::Available && snapshot.status.data.has_value();
    m_mutationSupported = snapshot.capabilities.supports(EngineFeature::CameraMutation);
    m_errorCode.clear();
    if (m_resultState == ResultState::Loading || m_resultState == ResultState::Pending)
        m_statusText = translate("Updating read-only camera capability…");
    else if (m_resultState == ResultState::Failed)
    {
        m_errorCode = snapshot.status.error ? snapshot.status.error->code : QStringLiteral("status-unavailable");
        m_statusText = translate("Camera capability is unknown because read-only status failed.");
    }
    else
        m_statusText =
            m_readOnlyAvailable
                ? translate("Camera capability is available read-only. Pair selection and tuning are disabled.")
                : translate("Camera capability is unknown because read-only status is unavailable.");
    Q_EMIT stateChanged();
}

bool CameraConfiguration::contractAvailable() const
{
    return m_contractAvailable;
}

bool CameraConfiguration::busy() const
{
    return m_resultState == ResultState::Loading || m_resultState == ResultState::Pending;
}

bool CameraConfiguration::readOnlyAvailable() const
{
    return m_readOnlyAvailable;
}

bool CameraConfiguration::mutationSupported() const
{
    return m_mutationSupported;
}

bool CameraConfiguration::hasPairs() const
{
    return false;
}

bool CameraConfiguration::ready() const
{
    return false;
}

QStringList CameraConfiguration::pairLabels() const
{
    return {};
}

int CameraConfiguration::selectedPairIndex() const
{
    return -1;
}

void CameraConfiguration::setSelectedPairIndex(int) {}

int CameraConfiguration::activePairIndex() const
{
    return -1;
}

bool CameraConfiguration::emitterTested() const
{
    return false;
}

bool CameraConfiguration::emitterAvailable() const
{
    return false;
}

int CameraConfiguration::emitterControlCount() const
{
    return 0;
}

QString CameraConfiguration::captureMode() const
{
    return {};
}

bool CameraConfiguration::tuneConclusive() const
{
    return false;
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
    Q_EMIT refreshRequested();
}

void CameraConfiguration::failCapability()
{
    m_errorCode = QStringLiteral("capability-unavailable");
    m_statusText =
        translate("The installed backend exposes read-only Contract 1 but not camera mutation capabilities.");
    Q_EMIT stateChanged();
}

void CameraConfiguration::selectPair()
{
    failCapability();
}

void CameraConfiguration::setupEmitter()
{
    failCapability();
}

void CameraConfiguration::tuneCamera()
{
    failCapability();
}

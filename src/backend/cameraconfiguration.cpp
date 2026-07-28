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
    m_readOnlyAvailable = snapshot.status.has_value();
    m_mutationSupported = snapshot.capabilities.mutationSupported;
    m_errorCode.clear();
    m_statusText = m_readOnlyAvailable
                       ? translate("Camera capability is available read-only. Pair selection and tuning are disabled.")
                       : translate("Camera capability is unknown because read-only status is unavailable.");
    Q_EMIT stateChanged();
}

bool CameraConfiguration::busy() const
{
    return false;
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

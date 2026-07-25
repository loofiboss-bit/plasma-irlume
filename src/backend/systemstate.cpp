// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemstate.h"

#include <utility>

SystemState::SystemState(QObject *parent) : QObject(parent) {}

void SystemState::apply(const SystemStateSnapshot &snapshot)
{
    m_snapshot = snapshot;
    Q_EMIT stateChanged();
}

QString SystemState::scenarioId() const
{
    return m_snapshot.scenarioId;
}

QString SystemState::headline() const
{
    return m_snapshot.headline;
}

QString SystemState::summary() const
{
    return m_snapshot.summary;
}

QString SystemState::issueCode() const
{
    return m_snapshot.issueCode;
}

QString SystemState::dataSource() const
{
    return m_snapshot.dataSource;
}

QString SystemState::fedoraVersion() const
{
    return m_snapshot.fedoraVersion;
}

QString SystemState::plasmaVersion() const
{
    return m_snapshot.plasmaVersion;
}

QString SystemState::engineVersion() const
{
    return m_snapshot.engineVersion;
}

QString SystemState::activeDisplayManager() const
{
    return m_snapshot.activeDisplayManager;
}

QString SystemState::supportReport() const
{
    return m_snapshot.supportReport;
}

SystemState::SecurityTier SystemState::securityTier() const
{
    return static_cast<SecurityTier>(m_snapshot.securityTier);
}

SystemState::CameraType SystemState::cameraType() const
{
    return static_cast<CameraType>(m_snapshot.cameraType);
}

SystemState::EngineStatus SystemState::engineStatus() const
{
    return static_cast<EngineStatus>(m_snapshot.engineStatus);
}

SystemState::DaemonStatus SystemState::daemonStatus() const
{
    return static_cast<DaemonStatus>(m_snapshot.daemonStatus);
}

SystemState::PamStatus SystemState::pamStatus() const
{
    return static_cast<PamStatus>(m_snapshot.pamStatus);
}

SystemState::CapabilityStatus SystemState::tpmStatus() const
{
    return static_cast<CapabilityStatus>(m_snapshot.tpmStatus);
}

SystemState::CapabilityStatus SystemState::templateProtectionStatus() const
{
    return static_cast<CapabilityStatus>(m_snapshot.templateProtectionStatus);
}

SystemState::CapabilityStatus SystemState::emitterStatus() const
{
    return static_cast<CapabilityStatus>(m_snapshot.emitterStatus);
}

SystemState::CapabilityStatus SystemState::livenessStatus() const
{
    return static_cast<CapabilityStatus>(m_snapshot.livenessStatus);
}

SystemState::ProfileStatus SystemState::profileStatus() const
{
    return static_cast<ProfileStatus>(m_snapshot.profileStatus);
}

SystemState::SecureBootStatus SystemState::secureBootStatus() const
{
    return static_cast<SecureBootStatus>(m_snapshot.secureBootStatus);
}

bool SystemState::passwordFallbackPreserved() const
{
    return m_snapshot.passwordFallbackPreserved;
}

bool SystemState::liveData() const
{
    return m_snapshot.liveData;
}

QString SystemState::securityTierLabel() const
{
    switch (m_snapshot.securityTier)
    {
    case SystemStateSnapshot::SecurityTier::Secure:
        return tr("Secure");
    case SystemStateSnapshot::SecurityTier::Convenience:
        return tr("Convenience");
    case SystemStateSnapshot::SecurityTier::Unsupported:
        return tr("Unsupported");
    }
    return tr("Unknown");
}

QString SystemState::cameraLabel() const
{
    switch (m_snapshot.cameraType)
    {
    case SystemStateSnapshot::CameraType::Infrared:
        return tr("Infrared camera");
    case SystemStateSnapshot::CameraType::Rgb:
        return tr("RGB camera");
    case SystemStateSnapshot::CameraType::None:
        return tr("No camera");
    case SystemStateSnapshot::CameraType::Unknown:
        return tr("Camera unknown");
    }
    return tr("Camera unknown");
}

QString SystemState::cameraStatusLabel() const
{
    switch (m_snapshot.cameraType)
    {
    case SystemStateSnapshot::CameraType::Infrared:
        return tr("Secure hardware");
    case SystemStateSnapshot::CameraType::Rgb:
        return tr("Convenience only");
    case SystemStateSnapshot::CameraType::None:
        return tr("Unavailable");
    case SystemStateSnapshot::CameraType::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::engineStatusLabel() const
{
    switch (m_snapshot.engineStatus)
    {
    case SystemStateSnapshot::EngineStatus::Ready:
        return tr("Ready");
    case SystemStateSnapshot::EngineStatus::Missing:
        return tr("Not installed");
    case SystemStateSnapshot::EngineStatus::UnsupportedVersion:
        return tr("Unsupported version");
    case SystemStateSnapshot::EngineStatus::Unavailable:
        return tr("Unavailable");
    }
    return tr("Unknown");
}

QString SystemState::daemonStatusLabel() const
{
    switch (m_snapshot.daemonStatus)
    {
    case SystemStateSnapshot::DaemonStatus::Running:
        return tr("Running");
    case SystemStateSnapshot::DaemonStatus::Missing:
        return tr("Not installed");
    case SystemStateSnapshot::DaemonStatus::Broken:
        return tr("Needs attention");
    case SystemStateSnapshot::DaemonStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::pamStatusLabel() const
{
    switch (m_snapshot.pamStatus)
    {
    case SystemStateSnapshot::PamStatus::Clean:
        return tr("Matches expected state");
    case SystemStateSnapshot::PamStatus::NotConfigured:
        return tr("Not configured");
    case SystemStateSnapshot::PamStatus::Drift:
        return tr("Configuration drift");
    case SystemStateSnapshot::PamStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::profileStatusLabel() const
{
    switch (m_snapshot.profileStatus)
    {
    case SystemStateSnapshot::ProfileStatus::Enrolled:
        return tr("Enrolled");
    case SystemStateSnapshot::ProfileStatus::NotEnrolled:
        return tr("Not enrolled");
    case SystemStateSnapshot::ProfileStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::tpmStatusLabel() const
{
    switch (m_snapshot.tpmStatus)
    {
    case SystemStateSnapshot::CapabilityStatus::Available:
        return tr("Hardware available; protection not verified");
    case SystemStateSnapshot::CapabilityStatus::Unavailable:
        return tr("Hardware unavailable");
    case SystemStateSnapshot::CapabilityStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::templateProtectionStatusLabel() const
{
    switch (m_snapshot.templateProtectionStatus)
    {
    case SystemStateSnapshot::CapabilityStatus::Available:
        return tr("Protected at rest");
    case SystemStateSnapshot::CapabilityStatus::Unavailable:
        return tr("Not protected at rest");
    case SystemStateSnapshot::CapabilityStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::emitterStatusLabel() const
{
    switch (m_snapshot.emitterStatus)
    {
    case SystemStateSnapshot::CapabilityStatus::Available:
        return tr("Available");
    case SystemStateSnapshot::CapabilityStatus::Unavailable:
        return tr("Unavailable");
    case SystemStateSnapshot::CapabilityStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::livenessStatusLabel() const
{
    switch (m_snapshot.livenessStatus)
    {
    case SystemStateSnapshot::CapabilityStatus::Available:
        return tr("Available");
    case SystemStateSnapshot::CapabilityStatus::Unavailable:
        return tr("Unavailable");
    case SystemStateSnapshot::CapabilityStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

QString SystemState::secureBootStatusLabel() const
{
    switch (m_snapshot.secureBootStatus)
    {
    case SystemStateSnapshot::SecureBootStatus::Enabled:
        return tr("Enabled");
    case SystemStateSnapshot::SecureBootStatus::Disabled:
        return tr("Disabled");
    case SystemStateSnapshot::SecureBootStatus::Unknown:
        return tr("Unknown");
    }
    return tr("Unknown");
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "fakeadapter.h"

#include <QCoreApplication>

namespace
{
using CameraType = SystemStateSnapshot::CameraType;
using CapabilityStatus = SystemStateSnapshot::CapabilityStatus;
using DaemonStatus = SystemStateSnapshot::DaemonStatus;
using EngineStatus = SystemStateSnapshot::EngineStatus;
using PamStatus = SystemStateSnapshot::PamStatus;
using ProfileStatus = SystemStateSnapshot::ProfileStatus;
using SecurityTier = SystemStateSnapshot::SecurityTier;
using SecureBootStatus = SystemStateSnapshot::SecureBootStatus;

SystemStateSnapshot baseline()
{
    SystemStateSnapshot state;
    state.scenarioId = QStringLiteral("secure-ir");
    state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "Secure face login is available");
    state.summary = QCoreApplication::translate(
        "FakeSystemStateAdapter", "Infrared hardware and the simulated irlume service pass every read-only "
                                  "readiness check.");
    state.dataSource = QCoreApplication::translate("FakeSystemStateAdapter", "Simulated preview");
    state.fedoraVersion = QStringLiteral("44");
    state.plasmaVersion = QStringLiteral("6.6.0");
    state.engineVersion = QStringLiteral("0.7.0-fixture");
    state.activeDisplayManager = QStringLiteral("Plasma Login Manager");
    state.securityTier = SecurityTier::Secure;
    state.cameraType = CameraType::Infrared;
    state.engineStatus = EngineStatus::Ready;
    state.daemonStatus = DaemonStatus::Running;
    state.pamStatus = PamStatus::Clean;
    state.tpmStatus = CapabilityStatus::Available;
    state.templateProtectionStatus = CapabilityStatus::Available;
    state.emitterStatus = CapabilityStatus::Available;
    state.livenessStatus = CapabilityStatus::Available;
    state.profileStatus = ProfileStatus::Enrolled;
    state.secureBootStatus = SecureBootStatus::Enabled;
    state.supportReport = QStringLiteral("# plasma-irlume support report\n\n"
                                         "- Data source: simulated-preview\n"
                                         "- Fedora: 44\n"
                                         "- Plasma: 6.6.0\n"
                                         "- Display manager: Plasma Login Manager\n"
                                         "- Security tier: secure\n"
                                         "- Diagnostic code: none\n");
    return state;
}
} // namespace

QStringList FakeSystemStateAdapter::scenarioNames() const
{
    return {
        QCoreApplication::translate("FakeSystemStateAdapter", "Secure IR hardware"),
        QCoreApplication::translate("FakeSystemStateAdapter", "RGB-only hardware"),
        QCoreApplication::translate("FakeSystemStateAdapter", "No camera"),
        QCoreApplication::translate("FakeSystemStateAdapter", "Missing irlume"),
        QCoreApplication::translate("FakeSystemStateAdapter", "Unsupported irlume version"),
        QCoreApplication::translate("FakeSystemStateAdapter", "Broken daemon"),
        QCoreApplication::translate("FakeSystemStateAdapter", "Existing PAM drift"),
    };
}

SystemStateSnapshot FakeSystemStateAdapter::stateForScenario(int index) const
{
    SystemStateSnapshot state = baseline();

    switch (index)
    {
    case SecureIr:
        return state;
    case RgbOnly:
        state.scenarioId = QStringLiteral("rgb-only");
        state.headline =
            QCoreApplication::translate("FakeSystemStateAdapter", "Face unlock is limited to convenience use");
        state.summary =
            QCoreApplication::translate("FakeSystemStateAdapter", "An RGB camera is available, but it does not "
                                                                  "meet the secure login-screen tier.");
        state.securityTier = SecurityTier::Convenience;
        state.cameraType = CameraType::Rgb;
        state.profileStatus = ProfileStatus::NotEnrolled;
        state.tpmStatus = CapabilityStatus::Unavailable;
        state.templateProtectionStatus = CapabilityStatus::Unavailable;
        return state;
    case NoCamera:
        state.scenarioId = QStringLiteral("no-camera");
        state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "No compatible camera was found");
        state.summary = QCoreApplication::translate("FakeSystemStateAdapter", "Face login remains unavailable until "
                                                                              "compatible camera hardware is present.");
        state.issueCode = QStringLiteral("camera-unavailable");
        state.securityTier = SecurityTier::Unsupported;
        state.cameraType = CameraType::None;
        state.profileStatus = ProfileStatus::NotEnrolled;
        state.tpmStatus = CapabilityStatus::Unavailable;
        state.templateProtectionStatus = CapabilityStatus::Unavailable;
        state.emitterStatus = CapabilityStatus::Unavailable;
        state.livenessStatus = CapabilityStatus::Unavailable;
        return state;
    case MissingIrlume:
        state.scenarioId = QStringLiteral("missing-irlume");
        state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "irlume is not installed");
        state.summary =
            QCoreApplication::translate("FakeSystemStateAdapter", "The face-authentication engine is required "
                                                                  "before readiness can be checked.");
        state.issueCode = QStringLiteral("engine-missing");
        state.engineVersion.clear();
        state.securityTier = SecurityTier::Unsupported;
        state.engineStatus = EngineStatus::Missing;
        state.daemonStatus = DaemonStatus::Missing;
        state.pamStatus = PamStatus::NotConfigured;
        state.profileStatus = ProfileStatus::Unknown;
        state.tpmStatus = CapabilityStatus::Unknown;
        state.templateProtectionStatus = CapabilityStatus::Unknown;
        state.emitterStatus = CapabilityStatus::Unknown;
        state.livenessStatus = CapabilityStatus::Unknown;
        return state;
    case UnsupportedIrlume:
        state.scenarioId = QStringLiteral("unsupported-irlume");
        state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "The machine contract is not supported");
        state.summary = QCoreApplication::translate("FakeSystemStateAdapter",
                                                    "The simulated backend does not advertise Machine API Contract "
                                                    "1. The engine version is informational.");
        state.issueCode = QStringLiteral("unsupported-contract");
        state.engineVersion = QStringLiteral("1.0.0");
        state.securityTier = SecurityTier::Unsupported;
        state.engineStatus = EngineStatus::UnsupportedContract;
        state.daemonStatus = DaemonStatus::Unknown;
        state.pamStatus = PamStatus::Unknown;
        state.profileStatus = ProfileStatus::Unknown;
        state.tpmStatus = CapabilityStatus::Unknown;
        state.templateProtectionStatus = CapabilityStatus::Unknown;
        state.emitterStatus = CapabilityStatus::Unknown;
        state.livenessStatus = CapabilityStatus::Unknown;
        return state;
    case BrokenDaemon:
        state.scenarioId = QStringLiteral("broken-daemon");
        state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "The irlume service needs attention");
        state.summary =
            QCoreApplication::translate("FakeSystemStateAdapter", "The simulated engine is compatible, but its "
                                                                  "background service is not healthy.");
        state.issueCode = QStringLiteral("daemon-unhealthy");
        state.securityTier = SecurityTier::Unsupported;
        state.engineStatus = EngineStatus::Ready;
        state.daemonStatus = DaemonStatus::Broken;
        state.pamStatus = PamStatus::Unknown;
        return state;
    case PamDrift:
        state.scenarioId = QStringLiteral("pam-drift");
        state.headline =
            QCoreApplication::translate("FakeSystemStateAdapter", "Authentication configuration has drifted");
        state.summary = QCoreApplication::translate(
            "FakeSystemStateAdapter", "The observed PAM state no longer matches the simulated engine plan. "
                                      "This read-only phase makes no changes.");
        state.issueCode = QStringLiteral("pam-drift");
        state.securityTier = SecurityTier::Unsupported;
        state.pamStatus = PamStatus::Drift;
        return state;
    default:
        state.scenarioId = QStringLiteral("invalid-scenario");
        state.headline = QCoreApplication::translate("FakeSystemStateAdapter", "Preview state is unavailable");
        state.summary = QCoreApplication::translate("FakeSystemStateAdapter",
                                                    "The requested fake-adapter scenario does not exist.");
        state.issueCode = QStringLiteral("invalid-scenario");
        state.securityTier = SecurityTier::Unsupported;
        state.cameraType = CameraType::Unknown;
        state.engineStatus = EngineStatus::Unavailable;
        state.daemonStatus = DaemonStatus::Unknown;
        state.pamStatus = PamStatus::Unknown;
        state.profileStatus = ProfileStatus::Unknown;
        state.tpmStatus = CapabilityStatus::Unknown;
        state.templateProtectionStatus = CapabilityStatus::Unknown;
        state.emitterStatus = CapabilityStatus::Unknown;
        state.livenessStatus = CapabilityStatus::Unknown;
        state.secureBootStatus = SecureBootStatus::Unknown;
        return state;
    }
}

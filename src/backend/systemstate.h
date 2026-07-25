// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

struct SystemStateSnapshot
{
    enum class SecurityTier
    {
        Secure,
        Convenience,
        Unsupported,
    };

    enum class CameraType
    {
        Infrared,
        Rgb,
        None,
        Unknown,
    };

    enum class EngineStatus
    {
        Ready,
        Missing,
        UnsupportedVersion,
        Unavailable,
    };

    enum class DaemonStatus
    {
        Running,
        Missing,
        Broken,
        Unknown,
    };

    enum class PamStatus
    {
        Clean,
        NotConfigured,
        Drift,
        Unknown,
    };

    enum class CapabilityStatus
    {
        Available,
        Unavailable,
        Unknown,
    };

    enum class ProfileStatus
    {
        Enrolled,
        NotEnrolled,
        Unknown,
    };

    enum class SecureBootStatus
    {
        Enabled,
        Disabled,
        Unknown,
    };

    QString scenarioId;
    QString headline;
    QString summary;
    QString issueCode;
    QString dataSource;
    QString fedoraVersion;
    QString plasmaVersion;
    QString engineVersion;
    QString activeDisplayManager;
    QString supportReport;
    SecurityTier securityTier = SecurityTier::Unsupported;
    CameraType cameraType = CameraType::Unknown;
    EngineStatus engineStatus = EngineStatus::Unavailable;
    DaemonStatus daemonStatus = DaemonStatus::Unknown;
    PamStatus pamStatus = PamStatus::Unknown;
    CapabilityStatus tpmStatus = CapabilityStatus::Unknown;
    CapabilityStatus templateProtectionStatus = CapabilityStatus::Unknown;
    CapabilityStatus emitterStatus = CapabilityStatus::Unknown;
    CapabilityStatus livenessStatus = CapabilityStatus::Unknown;
    ProfileStatus profileStatus = ProfileStatus::Unknown;
    SecureBootStatus secureBootStatus = SecureBootStatus::Unknown;
    bool passwordFallbackPreserved = true;
    bool liveData = false;
};

class SystemState final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString scenarioId READ scenarioId NOTIFY stateChanged)
    Q_PROPERTY(QString headline READ headline NOTIFY stateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)
    Q_PROPERTY(QString issueCode READ issueCode NOTIFY stateChanged)
    Q_PROPERTY(QString dataSource READ dataSource NOTIFY stateChanged)
    Q_PROPERTY(QString fedoraVersion READ fedoraVersion NOTIFY stateChanged)
    Q_PROPERTY(QString plasmaVersion READ plasmaVersion NOTIFY stateChanged)
    Q_PROPERTY(QString engineVersion READ engineVersion NOTIFY stateChanged)
    Q_PROPERTY(QString activeDisplayManager READ activeDisplayManager NOTIFY stateChanged)
    Q_PROPERTY(QString supportReport READ supportReport NOTIFY stateChanged)
    Q_PROPERTY(SecurityTier securityTier READ securityTier NOTIFY stateChanged)
    Q_PROPERTY(CameraType cameraType READ cameraType NOTIFY stateChanged)
    Q_PROPERTY(EngineStatus engineStatus READ engineStatus NOTIFY stateChanged)
    Q_PROPERTY(DaemonStatus daemonStatus READ daemonStatus NOTIFY stateChanged)
    Q_PROPERTY(PamStatus pamStatus READ pamStatus NOTIFY stateChanged)
    Q_PROPERTY(CapabilityStatus tpmStatus READ tpmStatus NOTIFY stateChanged)
    Q_PROPERTY(CapabilityStatus templateProtectionStatus READ templateProtectionStatus NOTIFY stateChanged)
    Q_PROPERTY(CapabilityStatus emitterStatus READ emitterStatus NOTIFY stateChanged)
    Q_PROPERTY(CapabilityStatus livenessStatus READ livenessStatus NOTIFY stateChanged)
    Q_PROPERTY(ProfileStatus profileStatus READ profileStatus NOTIFY stateChanged)
    Q_PROPERTY(SecureBootStatus secureBootStatus READ secureBootStatus NOTIFY stateChanged)
    Q_PROPERTY(bool passwordFallbackPreserved READ passwordFallbackPreserved NOTIFY stateChanged)
    Q_PROPERTY(bool liveData READ liveData NOTIFY stateChanged)
    Q_PROPERTY(QString securityTierLabel READ securityTierLabel NOTIFY stateChanged)
    Q_PROPERTY(QString cameraLabel READ cameraLabel NOTIFY stateChanged)
    Q_PROPERTY(QString cameraStatusLabel READ cameraStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString engineStatusLabel READ engineStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString daemonStatusLabel READ daemonStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString pamStatusLabel READ pamStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString profileStatusLabel READ profileStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString tpmStatusLabel READ tpmStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString templateProtectionStatusLabel READ templateProtectionStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString emitterStatusLabel READ emitterStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString livenessStatusLabel READ livenessStatusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString secureBootStatusLabel READ secureBootStatusLabel NOTIFY stateChanged)

  public:
    enum class SecurityTier
    {
        Secure,
        Convenience,
        Unsupported,
    };
    Q_ENUM(SecurityTier)

    enum class CameraType
    {
        Infrared,
        Rgb,
        None,
        Unknown,
    };
    Q_ENUM(CameraType)

    enum class EngineStatus
    {
        Ready,
        Missing,
        UnsupportedVersion,
        Unavailable,
    };
    Q_ENUM(EngineStatus)

    enum class DaemonStatus
    {
        Running,
        Missing,
        Broken,
        Unknown,
    };
    Q_ENUM(DaemonStatus)

    enum class PamStatus
    {
        Clean,
        NotConfigured,
        Drift,
        Unknown,
    };
    Q_ENUM(PamStatus)

    enum class CapabilityStatus
    {
        Available,
        Unavailable,
        Unknown,
    };
    Q_ENUM(CapabilityStatus)

    enum class ProfileStatus
    {
        Enrolled,
        NotEnrolled,
        Unknown,
    };
    Q_ENUM(ProfileStatus)

    enum class SecureBootStatus
    {
        Enabled,
        Disabled,
        Unknown,
    };
    Q_ENUM(SecureBootStatus)

    explicit SystemState(QObject *parent = nullptr);

    void apply(const SystemStateSnapshot &snapshot);

    [[nodiscard]] QString scenarioId() const;
    [[nodiscard]] QString headline() const;
    [[nodiscard]] QString summary() const;
    [[nodiscard]] QString issueCode() const;
    [[nodiscard]] QString dataSource() const;
    [[nodiscard]] QString fedoraVersion() const;
    [[nodiscard]] QString plasmaVersion() const;
    [[nodiscard]] QString engineVersion() const;
    [[nodiscard]] QString activeDisplayManager() const;
    [[nodiscard]] QString supportReport() const;
    [[nodiscard]] SecurityTier securityTier() const;
    [[nodiscard]] CameraType cameraType() const;
    [[nodiscard]] EngineStatus engineStatus() const;
    [[nodiscard]] DaemonStatus daemonStatus() const;
    [[nodiscard]] PamStatus pamStatus() const;
    [[nodiscard]] CapabilityStatus tpmStatus() const;
    [[nodiscard]] CapabilityStatus templateProtectionStatus() const;
    [[nodiscard]] CapabilityStatus emitterStatus() const;
    [[nodiscard]] CapabilityStatus livenessStatus() const;
    [[nodiscard]] ProfileStatus profileStatus() const;
    [[nodiscard]] SecureBootStatus secureBootStatus() const;
    [[nodiscard]] bool passwordFallbackPreserved() const;
    [[nodiscard]] bool liveData() const;

    [[nodiscard]] QString securityTierLabel() const;
    [[nodiscard]] QString cameraLabel() const;
    [[nodiscard]] QString cameraStatusLabel() const;
    [[nodiscard]] QString engineStatusLabel() const;
    [[nodiscard]] QString daemonStatusLabel() const;
    [[nodiscard]] QString pamStatusLabel() const;
    [[nodiscard]] QString profileStatusLabel() const;
    [[nodiscard]] QString tpmStatusLabel() const;
    [[nodiscard]] QString templateProtectionStatusLabel() const;
    [[nodiscard]] QString emitterStatusLabel() const;
    [[nodiscard]] QString livenessStatusLabel() const;
    [[nodiscard]] QString secureBootStatusLabel() const;

  Q_SIGNALS:
    void stateChanged();

  private:
    SystemStateSnapshot m_snapshot;
};

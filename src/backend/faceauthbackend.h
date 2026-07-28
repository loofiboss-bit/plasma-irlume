// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

struct EngineError
{
    QString code;
    bool retryable = false;
};

struct EngineCapabilities
{
    bool statusRead = false;
    bool doctorRead = false;
    bool profilesRead = false;
    bool loginStatusRead = false;
    bool mutationSupported = false;
    int maxProfiles = 0;
    QStringList advertised;
};

struct EngineStatusSnapshot
{
    enum class Daemon
    {
        Running,
        AccessDenied,
        Unreachable,
    };

    enum class TemplateProtection
    {
        Encrypted,
        Plaintext,
        Unknown,
    };

    Daemon daemon = Daemon::Unreachable;
    TemplateProtection templates = TemplateProtection::Unknown;
    bool enrollmentKnown = false;
    std::optional<int> profileCount;
    std::optional<int> scanCount;
    bool keyringKnown = false;
    bool keyringArmed = false;
    bool recoveryKnown = false;
    bool recoveryPassphraseSet = false;
    bool rgbCamera = false;
    bool irCamera = false;
    bool fingerprintPresent = false;
    bool faceDisabled = false;
    QString authMethod;
};

struct EngineDoctorCheck
{
    enum class State
    {
        Pass,
        Warn,
        Fail,
        Unknown,
        Info,
    };

    QString id;
    State state = State::Unknown;
};

struct EngineProfile
{
    std::optional<QString> stableId;
    QString displayName;
    QVector<QString> scanDisplayNames;
};

struct EngineProfileSnapshot
{
    QVector<EngineProfile> profiles;
    bool requireEyesOpen = false;
    bool requireChallenge = false;
};

struct EngineLoginSurface
{
    QString id;
    QString role;
    bool present = false;
    bool wired = false;
    QString mode;
};

struct EngineLoginSnapshot
{
    enum class SelinuxModule
    {
        Loaded,
        NotLoaded,
        Unknown,
    };

    bool loginManagerKnown = false;
    QString loginManagerName;
    bool loginManagerRecognized = false;
    QStringList loginManagerServices;
    QVector<EngineLoginSurface> surfaces;
    SelinuxModule selinuxModule = SelinuxModule::Unknown;
};

struct EngineSnapshot
{
    bool executablePresent = false;
    bool contractAvailable = false;
    int contractVersion = 0;
    QString engineVersion;
    EngineCapabilities capabilities;
    std::optional<EngineStatusSnapshot> status;
    std::optional<QVector<EngineDoctorCheck>> doctorChecks;
    std::optional<EngineProfileSnapshot> profiles;
    std::optional<EngineLoginSnapshot> login;
    QVector<EngineError> errors;
};

class FaceAuthBackend
{
  public:
    virtual ~FaceAuthBackend() = default;
    [[nodiscard]] virtual EngineSnapshot refresh() = 0;
};

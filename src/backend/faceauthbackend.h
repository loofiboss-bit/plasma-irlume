// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFlags>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>
#include <utility>

enum class EngineOperation
{
    Handshake,
    Status,
    Doctor,
    Profiles,
    LoginStatus,
};

enum class ResultState
{
    NotAdvertised,
    Pending,
    Loading,
    Available,
    Failed,
};

enum class EngineFeature : quint32
{
    None = 0,
    StatusRead = 1U << 0U,
    DoctorRead = 1U << 1U,
    ProfilesRead = 1U << 2U,
    LoginStatusRead = 1U << 3U,
    CameraDiscovery = 1U << 4U,
    Preview = 1U << 5U,
    Enrollment = 1U << 6U,
    ProfileMutation = 1U << 7U,
    CameraMutation = 1U << 8U,
    AuthenticationMutation = 1U << 9U,
};
Q_DECLARE_FLAGS(EngineFeatures, EngineFeature)
Q_DECLARE_OPERATORS_FOR_FLAGS(EngineFeatures)

struct EngineError
{
    EngineOperation operation = EngineOperation::Handshake;
    QString code;
    bool retryable = false;

    EngineError() = default;
    EngineError(EngineOperation source, QString stableCode, bool canRetry)
        : operation(source), code(std::move(stableCode)), retryable(canRetry)
    {
    }
    EngineError(QString stableCode, bool canRetry)
        : operation(EngineOperation::Handshake), code(std::move(stableCode)), retryable(canRetry)
    {
    }
};

template <typename T> struct OperationResult
{
    ResultState state = ResultState::NotAdvertised;
    std::optional<T> data;
    std::optional<EngineError> error;

    OperationResult &operator=(T value)
    {
        state = ResultState::Available;
        data = std::move(value);
        error.reset();
        return *this;
    }

    [[nodiscard]] bool has_value() const
    {
        return data.has_value();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return data.has_value();
    }

    [[nodiscard]] const T *operator->() const
    {
        return data ? &*data : nullptr;
    }

    [[nodiscard]] T *operator->()
    {
        return data ? &*data : nullptr;
    }
};

struct EngineHandshakeSnapshot
{
    int contractVersion = 0;
    QString engineVersion;
};

struct EngineCapabilities
{
    EngineFeatures features;
    int maxProfiles = 0;
    QStringList advertised;

    [[nodiscard]] bool supports(EngineFeature feature) const
    {
        return features.testFlag(feature);
    }

    [[nodiscard]] int recognizedReadCount() const
    {
        return static_cast<int>(supports(EngineFeature::StatusRead)) +
               static_cast<int>(supports(EngineFeature::DoctorRead)) +
               static_cast<int>(supports(EngineFeature::ProfilesRead)) +
               static_cast<int>(supports(EngineFeature::LoginStatusRead));
    }
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
    EngineCapabilities capabilities;
    OperationResult<EngineHandshakeSnapshot> handshake;
    OperationResult<EngineStatusSnapshot> status;
    OperationResult<QVector<EngineDoctorCheck>> doctor;
    OperationResult<EngineProfileSnapshot> profiles;
    OperationResult<EngineLoginSnapshot> loginStatus;

    [[nodiscard]] bool contractAvailable() const
    {
        return handshake.state == ResultState::Available && handshake.data.has_value();
    }

    [[nodiscard]] QString engineVersion() const
    {
        return handshake.data ? handshake.data->engineVersion : QString();
    }

    [[nodiscard]] bool partialDiagnostics() const
    {
        if (!contractAvailable())
            return false;
        const int readCount = capabilities.recognizedReadCount();
        if (readCount == 0)
            return false;
        if (readCount < 4)
            return true;
        return status.state != ResultState::Available || doctor.state != ResultState::Available ||
               profiles.state != ResultState::Available || loginStatus.state != ResultState::Available;
    }

    [[nodiscard]] bool retryable() const
    {
        const auto retryableResult = [](const auto &result) { return result.error && result.error->retryable; };
        return retryableResult(handshake) || retryableResult(status) || retryableResult(doctor) ||
               retryableResult(profiles) || retryableResult(loginStatus);
    }
};

class FaceAuthBackend : public QObject
{
    Q_OBJECT

  public:
    explicit FaceAuthBackend(QObject *parent = nullptr) : QObject(parent) {}
    ~FaceAuthBackend() override = default;

    virtual void requestRefresh(quint64 generation) = 0;
    virtual void cancelRefresh() = 0;

  Q_SIGNALS:
    void refreshProgress(quint64 generation, const EngineSnapshot &snapshot);
    void refreshCompleted(quint64 generation, const EngineSnapshot &snapshot);
    void refreshCancelled(quint64 generation);
};

Q_DECLARE_METATYPE(EngineOperation)
Q_DECLARE_METATYPE(ResultState)
Q_DECLARE_METATYPE(EngineFeature)
Q_DECLARE_METATYPE(EngineFeatures)
Q_DECLARE_METATYPE(EngineError)
Q_DECLARE_METATYPE(EngineSnapshot)

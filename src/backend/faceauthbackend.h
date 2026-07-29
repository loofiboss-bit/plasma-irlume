// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFlags>
#include <QObject>
#include <QString>

#include <optional>
#include <utility>

enum class EngineOperation
{
    Protocol,
    Status,
    Capabilities,
    Enrollment,
    EmbeddingExtraction,
    LocalVerification,
    ProfileDeletion,
    Authentication,
    PamConfiguration,
    TemplatePersistence,
};

enum class ResultState
{
    Pending,
    Loading,
    Available,
    Unsupported,
    Failed,
};

enum class EngineFeature : quint32
{
    None = 0,
    StatusRead = 1U << 0U,
    CapabilityRead = 1U << 1U,
    CameraDiscovery = 1U << 2U,
    Preview = 1U << 3U,
    DetectorAnalysis = 1U << 4U,
    EmbeddingExtraction = 1U << 5U,
    UserSessionEnrollment = 1U << 6U,
    EncryptedPersistence = 1U << 7U,
    LocalVerification = 1U << 8U,
    ProfileDeletion = 1U << 9U,
};
Q_DECLARE_FLAGS(EngineFeatures, EngineFeature)
Q_DECLARE_OPERATORS_FOR_FLAGS(EngineFeatures)

enum class OperationSupport
{
    Supported,
    Unsupported,
};

struct EngineError
{
    EngineOperation operation = EngineOperation::Protocol;
    QString code;
    bool retryable = false;

    EngineError() = default;
    EngineError(EngineOperation source, QString stableCode, bool canRetry)
        : operation(source), code(std::move(stableCode)), retryable(canRetry)
    {
    }
};

template <typename T> struct OperationResult
{
    ResultState state = ResultState::Unsupported;
    std::optional<T> data;
    std::optional<EngineError> error;

    OperationResult &operator=(T value)
    {
        state = ResultState::Available;
        data = std::move(value);
        error.reset();
        return *this;
    }
};

struct EngineProtocolSnapshot
{
    int protocolVersion = 0;
    QString engineVersion;
};

struct EngineCapabilities
{
    EngineFeatures features;
    OperationSupport detectorAnalysis = OperationSupport::Unsupported;
    OperationSupport embeddingExtraction = OperationSupport::Unsupported;
    OperationSupport enrollment = OperationSupport::Unsupported;
    OperationSupport localVerification = OperationSupport::Unsupported;
    OperationSupport profileDeletion = OperationSupport::Unsupported;
    OperationSupport authentication = OperationSupport::Unsupported;
    OperationSupport pamConfiguration = OperationSupport::Unsupported;
    OperationSupport encryptedPersistence = OperationSupport::Unsupported;

    [[nodiscard]] bool supports(EngineFeature feature) const
    {
        return features.testFlag(feature);
    }
};

struct EngineStatusSnapshot
{
    enum class State
    {
        Ready,
        Degraded,
    };

    enum class KeyProviderState
    {
        Available,
        Locked,
        Unavailable,
    };

    enum class VaultState
    {
        Unknown,
        Absent,
        Ready,
        Corrupt,
        ModelMismatch,
    };

    State state = State::Degraded;
    bool detectorModelAvailable = false;
    bool embeddingModelAvailable = false;
    KeyProviderState keyProviderState = KeyProviderState::Unavailable;
    VaultState vaultState = VaultState::Unknown;
    bool profileEnrolled = false;
    quint8 sampleCount = 0;
};

struct EngineSnapshot
{
    bool engineAvailable = false;
    EngineCapabilities capabilities;
    OperationResult<EngineProtocolSnapshot> protocol;
    OperationResult<EngineStatusSnapshot> status;

    [[nodiscard]] bool protocolAvailable() const
    {
        return protocol.state == ResultState::Available && protocol.data.has_value();
    }

    [[nodiscard]] QString engineVersion() const
    {
        return protocol.data ? protocol.data->engineVersion : QString();
    }

    [[nodiscard]] bool partialDiagnostics() const
    {
        return protocolAvailable() && capabilities.supports(EngineFeature::StatusRead) &&
               status.state != ResultState::Available;
    }

    [[nodiscard]] bool retryable() const
    {
        const auto retryableResult = [](const auto &result) { return result.error && result.error->retryable; };
        return retryableResult(protocol) || retryableResult(status);
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

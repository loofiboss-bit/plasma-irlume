// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QByteArrayView>
#include <QFutureWatcher>

#include <functional>
#include <memory>

class IdentityWorkerClient;
class KWalletKeyProvider;

class NativeFaceAuthBackend final : public FaceAuthBackend
{
    Q_OBJECT

  public:
    struct Availability
    {
        bool available = false;
        int protocolVersion = 0;
        QString engineVersion;
        bool detectorModelAvailable = false;
        bool embeddingModelAvailable = false;
        EngineStatusSnapshot::VaultState vaultState = EngineStatusSnapshot::VaultState::Unknown;
        bool profileEnrolled = false;
        quint8 sampleCount = 0;
        EngineStatusSnapshot::KeyProviderState keyProviderState = EngineStatusSnapshot::KeyProviderState::Unavailable;
    };

    using AvailabilityProbe = std::function<Availability()>;

    explicit NativeFaceAuthBackend(QObject *parent = nullptr);
    explicit NativeFaceAuthBackend(AvailabilityProbe probe, QObject *parent = nullptr);
    ~NativeFaceAuthBackend() override;

    void requestRefresh(quint64 generation) override;
    void cancelRefresh() override;

  private:
    [[nodiscard]] static EngineSnapshot loadingSnapshot();
    [[nodiscard]] static EngineSnapshot completedSnapshot(const Availability &availability);
    void requestProductionStatus(quint64 generation, Availability availability);
    void runStatusWorker(quint64 generation, Availability availability, const QByteArray &key);
    void handleStatusResponse(quint64 generation, Availability availability, QByteArrayView payload,
                              const QString &transportError);
    void completeActiveRefresh(quint64 generation, const Availability &availability);

    AvailabilityProbe m_probe;
    std::unique_ptr<KWalletKeyProvider> m_statusKeyProvider;
    std::unique_ptr<IdentityWorkerClient> m_statusWorker;
    QFutureWatcher<Availability> *m_watcher = nullptr;
    std::optional<quint64> m_activeGeneration;
    bool m_probeIncludesStatus = false;
};

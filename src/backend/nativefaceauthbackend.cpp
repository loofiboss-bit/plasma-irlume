// SPDX-License-Identifier: GPL-3.0-or-later

#include "nativefaceauthbackend.h"

#include "identityprotocol.h"
#include "identityworkerclient.h"
#include "kwalletkeyprovider.h"

#include <KWallet>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QtConcurrent>

#include <utility>

#ifndef KFACEAUTH_IDENTITY_WORKER_PATH
#define KFACEAUTH_IDENTITY_WORKER_PATH "/usr/libexec/kfaceauth-identity-worker"
#endif

#ifndef KFACEAUTH_MODEL_ROOT
#define KFACEAUTH_MODEL_ROOT "/usr/share/kfaceauth/models"
#endif

namespace
{
bool verifiedFile(const QString &path, qint64 expectedSize, const QByteArray &expectedSha256)
{
    QFile file(path);
    const QFileInfo info(file);
    if (!info.isFile() || info.isSymLink() || info.size() != expectedSize || !file.open(QIODevice::ReadOnly))
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
        return false;
    return hash.result().toHex() == expectedSha256;
}

NativeFaceAuthBackend::Availability productionAvailability()
{
    NativeFaceAuthBackend::Availability availability;
    const QFileInfo worker(QStringLiteral(KFACEAUTH_IDENTITY_WORKER_PATH));
    availability.detectorModelAvailable =
        verifiedFile(QStringLiteral(KFACEAUTH_MODEL_ROOT "/files/face_detection_yunet_2023mar.onnx"), 232589,
                     QByteArrayLiteral("8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4"));
    availability.embeddingModelAvailable =
        verifiedFile(QStringLiteral(KFACEAUTH_MODEL_ROOT "/files/face_recognition_sface_2021dec.onnx"), 38696353,
                     QByteArrayLiteral("0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79"));
    const bool workerUsable = worker.isFile() && worker.isExecutable() && !worker.isSymLink();
    availability.available =
        workerUsable && availability.detectorModelAvailable && availability.embeddingModelAvailable;
    availability.protocolVersion = 2;
    availability.engineVersion = QStringLiteral("0.1.0-local-identity");
    return availability;
}

EngineError unavailable(EngineOperation operation, const QString &code, bool retryable)
{
    return EngineError(operation, code, retryable);
}
} // namespace

NativeFaceAuthBackend::NativeFaceAuthBackend(QObject *parent)
    : FaceAuthBackend(parent), m_probe(productionAvailability),
      m_statusKeyProvider(std::make_unique<KWalletKeyProvider>()),
      m_statusWorker(std::make_unique<IdentityWorkerClient>())
{
}

NativeFaceAuthBackend::NativeFaceAuthBackend(AvailabilityProbe probe, QObject *parent)
    : FaceAuthBackend(parent), m_probe(std::move(probe)), m_probeIncludesStatus(true)
{
    Q_ASSERT(m_probe);
}

NativeFaceAuthBackend::~NativeFaceAuthBackend()
{
    cancelRefresh();
}

void NativeFaceAuthBackend::requestRefresh(quint64 generation)
{
    cancelRefresh();
    if (!m_probeIncludesStatus && m_statusWorker && m_statusWorker->busy())
        m_statusWorker = std::make_unique<IdentityWorkerClient>();
    m_activeGeneration = generation;
    Q_EMIT refreshProgress(generation, loadingSnapshot());

    auto *watcher = new QFutureWatcher<Availability>(this);
    m_watcher = watcher;
    connect(watcher, &QFutureWatcher<Availability>::finished, this,
            [this, watcher, generation]()
            {
                const Availability availability = watcher->result();
                watcher->deleteLater();
                if (m_watcher != watcher || !m_activeGeneration || *m_activeGeneration != generation)
                    return;
                m_watcher = nullptr;
                if (m_probeIncludesStatus || !availability.available)
                    completeActiveRefresh(generation, availability);
                else
                    requestProductionStatus(generation, availability);
            });
    watcher->setFuture(QtConcurrent::run(m_probe));
}

void NativeFaceAuthBackend::cancelRefresh()
{
    if (!m_activeGeneration)
        return;
    const quint64 generation = *m_activeGeneration;
    m_activeGeneration.reset();
    if (m_statusWorker)
        m_statusWorker->cancel();
    if (m_statusKeyProvider)
        m_statusKeyProvider->cancel();
    if (m_watcher)
    {
        m_watcher->disconnect(this);
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }
    Q_EMIT refreshCancelled(generation);
}

void NativeFaceAuthBackend::requestProductionStatus(quint64 generation, Availability availability)
{
    if (!m_activeGeneration || *m_activeGeneration != generation || !m_statusWorker || !m_statusKeyProvider)
        return;

    availability.keyProviderState = !KWallet::Wallet::isEnabled()
                                        ? EngineStatusSnapshot::KeyProviderState::Unavailable
                                        : (KWallet::Wallet::isOpen(KWallet::Wallet::NetworkWallet())
                                               ? EngineStatusSnapshot::KeyProviderState::Available
                                               : EngineStatusSnapshot::KeyProviderState::Locked);
    if (availability.keyProviderState != EngineStatusSnapshot::KeyProviderState::Available)
    {
        runStatusWorker(generation, std::move(availability), {});
        return;
    }

    m_statusKeyProvider->requestKey(
        [this, generation, availability = std::move(availability)](KWalletKeyProvider::Result result) mutable
        {
            if (!m_activeGeneration || *m_activeGeneration != generation)
            {
                result.clear();
                return;
            }
            if (result.state == KWalletKeyProvider::State::Locked ||
                result.state == KWalletKeyProvider::State::Cancelled)
                availability.keyProviderState = EngineStatusSnapshot::KeyProviderState::Locked;
            else if (result.state == KWalletKeyProvider::State::Unavailable)
                availability.keyProviderState = EngineStatusSnapshot::KeyProviderState::Unavailable;

            QByteArray key;
            if (result.state == KWalletKeyProvider::State::Available)
                key = std::move(result.key);
            result.clear();
            runStatusWorker(generation, std::move(availability), key);
            key.fill(0);
            key.clear();
        });
}

void NativeFaceAuthBackend::runStatusWorker(quint64 generation, Availability availability, const QByteArray &key)
{
    if (!m_activeGeneration || *m_activeGeneration != generation || !m_statusWorker)
        return;
    QByteArray request = IdentityProtocol::statusRequest(generation, key);
    if (request.isEmpty())
    {
        availability.available = false;
        completeActiveRefresh(generation, availability);
        return;
    }
    m_statusWorker->execute(generation, std::move(request),
                            [this, availability = std::move(availability)](quint64 completed, QByteArrayView payload,
                                                                           const QString &transportError) mutable
                            { handleStatusResponse(completed, std::move(availability), payload, transportError); });
}

void NativeFaceAuthBackend::handleStatusResponse(quint64 generation, Availability availability, QByteArrayView payload,
                                                 const QString &transportError)
{
    if (!m_activeGeneration || *m_activeGeneration != generation)
        return;
    IdentityProtocol::Response response;
    QString parseError;
    if (!transportError.isEmpty() || !IdentityProtocol::parseResponse(payload, generation, &response, &parseError) ||
        response.kind != IdentityProtocol::ResponseKind::Status || response.sensitivePayload.size() != 1)
    {
        response.clearSensitive();
        availability.available = false;
        completeActiveRefresh(generation, availability);
        return;
    }

    const quint8 sampleCount = static_cast<quint8>(response.sensitivePayload.at(0));
    availability.vaultState = response.code == 0   ? EngineStatusSnapshot::VaultState::Absent
                              : response.code == 1 ? EngineStatusSnapshot::VaultState::Ready
                              : response.code == 2 ? EngineStatusSnapshot::VaultState::Corrupt
                              : response.code == 3 ? EngineStatusSnapshot::VaultState::ModelMismatch
                                                   : EngineStatusSnapshot::VaultState::Unknown;
    availability.profileEnrolled = response.code == 1;
    availability.sampleCount = availability.profileEnrolled ? sampleCount : 0;
    if ((availability.profileEnrolled && (sampleCount < 3 || sampleCount > IdentityProtocol::MaximumSamples)) ||
        (!availability.profileEnrolled && sampleCount != 0))
        availability.available = false;
    response.clearSensitive();
    completeActiveRefresh(generation, availability);
}

EngineSnapshot NativeFaceAuthBackend::loadingSnapshot()
{
    EngineSnapshot snapshot;
    snapshot.protocol.state = ResultState::Loading;
    snapshot.status.state = ResultState::Loading;
    return snapshot;
}

EngineSnapshot NativeFaceAuthBackend::completedSnapshot(const Availability &availability)
{
    EngineSnapshot snapshot;
    snapshot.engineAvailable = availability.available;
    snapshot.capabilities.authentication = OperationSupport::Unsupported;
    snapshot.capabilities.pamConfiguration = OperationSupport::Unsupported;

    if (!availability.available)
    {
        snapshot.protocol.state = ResultState::Failed;
        snapshot.protocol.error =
            unavailable(EngineOperation::Protocol, QStringLiteral("native-engine-unavailable"), true);
        snapshot.status.state = ResultState::Failed;
        snapshot.status.error =
            unavailable(EngineOperation::Status, QStringLiteral("local-identity-components-unavailable"), true);
        return snapshot;
    }

    snapshot.protocol = EngineProtocolSnapshot{availability.protocolVersion, availability.engineVersion};
    snapshot.capabilities.features = EngineFeature::CapabilityRead | EngineFeature::StatusRead |
                                     EngineFeature::DetectorAnalysis | EngineFeature::EmbeddingExtraction |
                                     EngineFeature::UserSessionEnrollment | EngineFeature::EncryptedPersistence |
                                     EngineFeature::LocalVerification | EngineFeature::ProfileDeletion;
    snapshot.capabilities.detectorAnalysis = OperationSupport::Supported;
    snapshot.capabilities.embeddingExtraction = OperationSupport::Supported;
    snapshot.capabilities.enrollment = OperationSupport::Supported;
    snapshot.capabilities.localVerification = OperationSupport::Supported;
    snapshot.capabilities.profileDeletion = OperationSupport::Supported;
    snapshot.capabilities.encryptedPersistence = OperationSupport::Supported;
    EngineStatusSnapshot status;
    status.state = EngineStatusSnapshot::State::Ready;
    status.detectorModelAvailable = availability.detectorModelAvailable;
    status.embeddingModelAvailable = availability.embeddingModelAvailable;
    status.keyProviderState = availability.keyProviderState;
    status.vaultState = availability.vaultState;
    status.profileEnrolled = availability.profileEnrolled;
    status.sampleCount = availability.sampleCount;
    if (status.keyProviderState != EngineStatusSnapshot::KeyProviderState::Available ||
        status.vaultState == EngineStatusSnapshot::VaultState::Unknown ||
        status.vaultState == EngineStatusSnapshot::VaultState::Corrupt ||
        status.vaultState == EngineStatusSnapshot::VaultState::ModelMismatch)
        status.state = EngineStatusSnapshot::State::Degraded;
    snapshot.status = status;
    return snapshot;
}

void NativeFaceAuthBackend::completeActiveRefresh(quint64 generation, const Availability &availability)
{
    if (!m_activeGeneration || *m_activeGeneration != generation)
        return;
    m_activeGeneration.reset();
    Q_EMIT refreshCompleted(generation, completedSnapshot(availability));
}

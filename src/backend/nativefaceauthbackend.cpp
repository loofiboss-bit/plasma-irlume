// SPDX-License-Identifier: GPL-3.0-or-later

#include "nativefaceauthbackend.h"

#include <utility>

namespace
{
EngineError unsupported(EngineOperation operation)
{
    return EngineError(operation, QStringLiteral("unsupported-in-milestone-1"), false);
}
} // namespace

NativeFaceAuthBackend::NativeFaceAuthBackend(QObject *parent)
    : NativeFaceAuthBackend([]() { return Availability{}; }, parent)
{
}

NativeFaceAuthBackend::NativeFaceAuthBackend(AvailabilityProbe probe, QObject *parent)
    : FaceAuthBackend(parent), m_probe(std::move(probe))
{
    Q_ASSERT(m_probe);
    m_probeTimer.setSingleShot(true);
    m_probeTimer.setInterval(0);
    connect(&m_probeTimer, &QTimer::timeout, this, &NativeFaceAuthBackend::completeActiveRefresh);
}

void NativeFaceAuthBackend::requestRefresh(quint64 generation)
{
    if (m_activeGeneration)
    {
        const quint64 cancelledGeneration = *m_activeGeneration;
        m_probeTimer.stop();
        m_activeGeneration.reset();
        Q_EMIT refreshCancelled(cancelledGeneration);
    }

    m_activeGeneration = generation;
    Q_EMIT refreshProgress(generation, loadingSnapshot());
    m_probeTimer.start();
}

void NativeFaceAuthBackend::cancelRefresh()
{
    if (!m_activeGeneration)
        return;

    const quint64 cancelledGeneration = *m_activeGeneration;
    m_probeTimer.stop();
    m_activeGeneration.reset();
    Q_EMIT refreshCancelled(cancelledGeneration);
}

EngineSnapshot NativeFaceAuthBackend::loadingSnapshot()
{
    EngineSnapshot snapshot;
    snapshot.protocol.state = ResultState::Loading;
    snapshot.status.state = ResultState::Unsupported;
    snapshot.status.error = unsupported(EngineOperation::Status);
    return snapshot;
}

EngineSnapshot NativeFaceAuthBackend::completedSnapshot(const Availability &availability)
{
    EngineSnapshot snapshot;
    snapshot.engineAvailable = availability.available;
    snapshot.capabilities.enrollment = OperationSupport::Unsupported;
    snapshot.capabilities.authentication = OperationSupport::Unsupported;
    snapshot.capabilities.pamConfiguration = OperationSupport::Unsupported;
    snapshot.capabilities.templatePersistence = OperationSupport::Unsupported;

    if (!availability.available)
    {
        snapshot.protocol.state = ResultState::Failed;
        snapshot.protocol.error =
            EngineError(EngineOperation::Protocol, QStringLiteral("native-engine-unavailable"), true);
        snapshot.status.state = ResultState::Unsupported;
        snapshot.status.error = unsupported(EngineOperation::Status);
        return snapshot;
    }

    snapshot.protocol = EngineProtocolSnapshot{availability.protocolVersion, availability.engineVersion};
    snapshot.capabilities.features = EngineFeature::CapabilityRead | EngineFeature::StatusRead;
    snapshot.status = EngineStatusSnapshot{EngineStatusSnapshot::State::Skeleton};
    return snapshot;
}

void NativeFaceAuthBackend::completeActiveRefresh()
{
    if (!m_activeGeneration)
        return;

    const quint64 generation = *m_activeGeneration;
    const Availability availability = m_probe();
    if (!m_activeGeneration || *m_activeGeneration != generation)
        return;

    m_activeGeneration.reset();
    Q_EMIT refreshCompleted(generation, completedSnapshot(availability));
}

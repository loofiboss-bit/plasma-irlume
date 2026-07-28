// SPDX-License-Identifier: GPL-3.0-or-later

#include "refreshcoordinator.h"

RefreshCoordinator::RefreshCoordinator(std::unique_ptr<FaceAuthBackend> backend, QObject *parent)
    : QObject(parent), m_backend(std::move(backend))
{
    Q_ASSERT(m_backend);
    qRegisterMetaType<EngineSnapshot>();
    connect(m_backend.get(), &FaceAuthBackend::refreshProgress, this,
            [this](quint64 generation, const EngineSnapshot &snapshot)
            {
                if (generation != m_generation)
                    return;
                m_snapshot = snapshot;
                Q_EMIT snapshotChanged(m_snapshot);
                Q_EMIT stateChanged();
            });
    connect(m_backend.get(), &FaceAuthBackend::refreshCompleted, this,
            [this](quint64 generation, const EngineSnapshot &snapshot)
            {
                if (generation != m_generation)
                    return;
                m_snapshot = snapshot;
                m_refreshing = false;
                Q_EMIT snapshotChanged(m_snapshot);
                Q_EMIT stateChanged();
            });
    connect(m_backend.get(), &FaceAuthBackend::refreshCancelled, this,
            [this](quint64 generation)
            {
                if (generation != m_generation)
                    return;
                m_refreshing = false;
                Q_EMIT stateChanged();
            });
}

RefreshCoordinator::~RefreshCoordinator()
{
    disconnect(m_backend.get(), nullptr, this, nullptr);
    m_backend->cancelRefresh();
}

bool RefreshCoordinator::refreshing() const
{
    return m_refreshing;
}

bool RefreshCoordinator::partialDiagnostics() const
{
    return m_snapshot.partialDiagnostics();
}

bool RefreshCoordinator::retryAvailable() const
{
    return !m_refreshing && m_snapshot.retryable();
}

quint64 RefreshCoordinator::generation() const
{
    return m_generation;
}

void RefreshCoordinator::requestRefresh()
{
    ++m_generation;
    m_refreshing = true;
    Q_EMIT stateChanged();
    m_backend->requestRefresh(m_generation);
}

void RefreshCoordinator::cancelRefresh()
{
    if (!m_refreshing)
        return;
    ++m_generation;
    m_refreshing = false;
    m_backend->cancelRefresh();
    Q_EMIT stateChanged();
}

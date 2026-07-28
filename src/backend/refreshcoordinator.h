// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QObject>

#include <memory>

class RefreshCoordinator final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool refreshing READ refreshing NOTIFY stateChanged)
    Q_PROPERTY(bool partialDiagnostics READ partialDiagnostics NOTIFY stateChanged)
    Q_PROPERTY(bool retryAvailable READ retryAvailable NOTIFY stateChanged)

  public:
    explicit RefreshCoordinator(std::unique_ptr<FaceAuthBackend> backend, QObject *parent = nullptr);
    ~RefreshCoordinator() override;

    [[nodiscard]] bool refreshing() const;
    [[nodiscard]] bool partialDiagnostics() const;
    [[nodiscard]] bool retryAvailable() const;
    [[nodiscard]] quint64 generation() const;

    Q_INVOKABLE void requestRefresh();
    void cancelRefresh();

  Q_SIGNALS:
    void snapshotChanged(const EngineSnapshot &snapshot);
    void stateChanged();

  private:
    std::unique_ptr<FaceAuthBackend> m_backend;
    EngineSnapshot m_snapshot;
    quint64 m_generation = 0;
    bool m_refreshing = false;
};

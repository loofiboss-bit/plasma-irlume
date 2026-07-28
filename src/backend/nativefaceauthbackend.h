// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QTimer>

#include <functional>

class NativeFaceAuthBackend final : public FaceAuthBackend
{
    Q_OBJECT

  public:
    struct Availability
    {
        bool available = false;
        int protocolVersion = 0;
        QString engineVersion;
    };

    using AvailabilityProbe = std::function<Availability()>;

    explicit NativeFaceAuthBackend(QObject *parent = nullptr);
    explicit NativeFaceAuthBackend(AvailabilityProbe probe, QObject *parent = nullptr);

    void requestRefresh(quint64 generation) override;
    void cancelRefresh() override;

  private:
    [[nodiscard]] static EngineSnapshot loadingSnapshot();
    [[nodiscard]] static EngineSnapshot completedSnapshot(const Availability &availability);
    void completeActiveRefresh();

    AvailabilityProbe m_probe;
    QTimer m_probeTimer;
    std::optional<quint64> m_activeGeneration;
};

// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "authconfiguration.h"
#include "camerapreviewsession.h"
#include "faceauthbackend.h"
#include "profilemodel.h"
#include "refreshcoordinator.h"
#include "supportreport.h"
#include "systemprobe.h"
#include "systemstate.h"

#include <KQuickConfigModule>

#include <memory>

class IrlumeKcm final : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SystemState *systemState READ systemState CONSTANT)
    Q_PROPERTY(ProfileModel *profileModel READ profileModel CONSTANT)
    Q_PROPERTY(CameraPreviewSession *cameraPreviewSession READ cameraPreviewSession CONSTANT)
    Q_PROPERTY(AuthConfiguration *authConfiguration READ authConfiguration CONSTANT)
    Q_PROPERTY(SupportReport *supportReport READ supportReport CONSTANT)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshStateChanged)
    Q_PROPERTY(bool partialDiagnostics READ partialDiagnostics NOTIFY refreshStateChanged)
    Q_PROPERTY(bool retryAvailable READ retryAvailable NOTIFY refreshStateChanged)

  public:
    IrlumeKcm(QObject *parent, const KPluginMetaData &data);
    IrlumeKcm(QObject *parent, const KPluginMetaData &data, std::unique_ptr<FaceAuthBackend> backend);
    ~IrlumeKcm() override;

    [[nodiscard]] SystemState *systemState();
    [[nodiscard]] ProfileModel *profileModel();
    [[nodiscard]] CameraPreviewSession *cameraPreviewSession();
    [[nodiscard]] AuthConfiguration *authConfiguration();
    [[nodiscard]] SupportReport *supportReport();
    [[nodiscard]] bool refreshing() const;
    [[nodiscard]] bool partialDiagnostics() const;
    [[nodiscard]] bool retryAvailable() const;
    Q_INVOKABLE void refresh();

  Q_SIGNALS:
    void refreshStateChanged();

  private:
    SystemProbe m_probe;
    SystemState m_systemState;
    CameraPreviewSession m_cameraPreviewSession;
    ProfileModel m_profileModel;
    AuthConfiguration m_authConfiguration;
    SupportReport m_supportReport;
    RefreshCoordinator m_refreshCoordinator;
    quint64 m_probeGeneration = 0;
};

// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "authconfiguration.h"
#include "cameraconfiguration.h"
#include "enrollmentsession.h"
#include "irlumebackend.h"
#include "profilemodel.h"
#include "supportreport.h"
#include "systemprobe.h"
#include "systemstate.h"

#include <KQuickConfigModule>

class IrlumeKcm final : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(SystemState *systemState READ systemState CONSTANT)
    Q_PROPERTY(ProfileModel *profileModel READ profileModel CONSTANT)
    Q_PROPERTY(EnrollmentSession *enrollmentSession READ enrollmentSession CONSTANT)
    Q_PROPERTY(AuthConfiguration *authConfiguration READ authConfiguration CONSTANT)
    Q_PROPERTY(CameraConfiguration *cameraConfiguration READ cameraConfiguration CONSTANT)
    Q_PROPERTY(SupportReport *supportReport READ supportReport CONSTANT)

  public:
    IrlumeKcm(QObject *parent, const KPluginMetaData &data);

    [[nodiscard]] SystemState *systemState();
    [[nodiscard]] ProfileModel *profileModel();
    [[nodiscard]] EnrollmentSession *enrollmentSession();
    [[nodiscard]] AuthConfiguration *authConfiguration();
    [[nodiscard]] CameraConfiguration *cameraConfiguration();
    [[nodiscard]] SupportReport *supportReport();
    Q_INVOKABLE void refresh();

  private:
    IrlumeBackend m_backend;
    SystemProbe m_probe;
    SystemState m_systemState;
    EnrollmentSession m_enrollmentSession;
    ProfileModel m_profileModel;
    AuthConfiguration m_authConfiguration;
    CameraConfiguration m_cameraConfiguration;
    SupportReport m_supportReport;
};

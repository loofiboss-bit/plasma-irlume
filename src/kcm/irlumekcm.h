// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "authconfiguration.h"
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
    Q_PROPERTY(AuthConfiguration *authConfiguration READ authConfiguration CONSTANT)
    Q_PROPERTY(SupportReport *supportReport READ supportReport CONSTANT)

  public:
    IrlumeKcm(QObject *parent, const KPluginMetaData &data);

    [[nodiscard]] SystemState *systemState();
    [[nodiscard]] ProfileModel *profileModel();
    [[nodiscard]] AuthConfiguration *authConfiguration();
    [[nodiscard]] SupportReport *supportReport();
    Q_INVOKABLE void refresh();

  private:
    SystemProbe m_probe;
    SystemState m_systemState;
    ProfileModel m_profileModel;
    AuthConfiguration m_authConfiguration;
    SupportReport m_supportReport;
};

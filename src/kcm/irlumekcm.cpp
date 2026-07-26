// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumekcm.h"

#include "enrollmentpreviewitem.h"

#include <KPluginFactory>
#include <qqml.h>

IrlumeKcm::IrlumeKcm(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data), m_systemState(this), m_profileProcess(this), m_cameraProcess(this),
      m_enrollmentSession(this), m_profileModel(&m_profileProcess, &m_enrollmentSession, this),
      m_authConfiguration(&m_systemState, this), m_cameraConfiguration(&m_cameraProcess, this),
      m_supportReport(&m_systemState, &m_profileModel, &m_authConfiguration, this)
{
    qmlRegisterType<EnrollmentPreviewItem>("org.kde.plasma.irlume", 2, 0, "EnrollmentPreview");
    setButtons(NoAdditionalButton);
    connect(&m_profileModel, &ProfileModel::profilesChanged, this, &IrlumeKcm::refresh);
    connect(&m_authConfiguration, &AuthConfiguration::configurationChanged, this, &IrlumeKcm::refresh);
    connect(&m_cameraConfiguration, &CameraConfiguration::configurationChanged, this, &IrlumeKcm::refresh);
    refresh();
    m_profileModel.refresh();
    m_cameraConfiguration.refresh();
}

SystemState *IrlumeKcm::systemState()
{
    return &m_systemState;
}

ProfileModel *IrlumeKcm::profileModel()
{
    return &m_profileModel;
}

EnrollmentSession *IrlumeKcm::enrollmentSession()
{
    return &m_enrollmentSession;
}

AuthConfiguration *IrlumeKcm::authConfiguration()
{
    return &m_authConfiguration;
}

CameraConfiguration *IrlumeKcm::cameraConfiguration()
{
    return &m_cameraConfiguration;
}

SupportReport *IrlumeKcm::supportReport()
{
    return &m_supportReport;
}

void IrlumeKcm::refresh()
{
    m_systemState.apply(m_probe.probe());
}

K_PLUGIN_CLASS_WITH_JSON(IrlumeKcm, "kcm_irlume.json")

#include "irlumekcm.moc"

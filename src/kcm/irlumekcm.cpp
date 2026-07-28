// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumekcm.h"

#include "backendfactory.h"
#include "enrollmentpreviewitem.h"

#include <KPluginFactory>
#include <qqml.h>

IrlumeKcm::IrlumeKcm(QObject *parent, const KPluginMetaData &data)
    : IrlumeKcm(parent, data, createProductionFaceAuthBackend())
{
}

IrlumeKcm::IrlumeKcm(QObject *parent, const KPluginMetaData &data, std::unique_ptr<FaceAuthBackend> backend)
    : KQuickConfigModule(parent, data), m_probe(this), m_systemState(this), m_enrollmentSession(this),
      m_profileModel(this), m_authConfiguration(&m_systemState, this), m_cameraConfiguration(this),
      m_supportReport(&m_systemState, &m_profileModel, &m_authConfiguration, this),
      m_refreshCoordinator(std::move(backend), this)
{
    qmlRegisterType<EnrollmentPreviewItem>("org.kde.plasma.irlume", 2, 0, "EnrollmentPreview");
    setButtons(NoAdditionalButton);
    connect(&m_authConfiguration, &AuthConfiguration::configurationChanged, this, &IrlumeKcm::refresh);
    connect(&m_cameraConfiguration, &CameraConfiguration::configurationChanged, this, &IrlumeKcm::refresh);
    connect(&m_profileModel, &ProfileModel::refreshRequested, this, &IrlumeKcm::refresh);
    connect(&m_cameraConfiguration, &CameraConfiguration::refreshRequested, this, &IrlumeKcm::refresh);
    connect(&m_refreshCoordinator, &RefreshCoordinator::snapshotChanged, this,
            [this](const EngineSnapshot &snapshot)
            {
                const auto inProgress = [](const auto &result)
                { return result.state == ResultState::Pending || result.state == ResultState::Loading; };
                if (!inProgress(snapshot.handshake) && !inProgress(snapshot.status) && !inProgress(snapshot.doctor) &&
                    !inProgress(snapshot.profiles) && !inProgress(snapshot.loginStatus))
                    m_probe.requestProbe(++m_probeGeneration, snapshot);
                m_profileModel.applySnapshot(snapshot);
                m_cameraConfiguration.applySnapshot(snapshot);
                m_authConfiguration.applySnapshot(snapshot);
            });
    connect(&m_probe, &SystemProbe::probeCompleted, this,
            [this](quint64 generation, const SystemStateSnapshot &snapshot)
            {
                if (generation == m_probeGeneration)
                    m_systemState.apply(snapshot);
            });
    connect(&m_refreshCoordinator, &RefreshCoordinator::stateChanged, this, &IrlumeKcm::refreshStateChanged);
    refresh();
}

IrlumeKcm::~IrlumeKcm() = default;

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

bool IrlumeKcm::refreshing() const
{
    return m_refreshCoordinator.refreshing();
}

bool IrlumeKcm::partialDiagnostics() const
{
    return m_refreshCoordinator.partialDiagnostics();
}

bool IrlumeKcm::retryAvailable() const
{
    return m_refreshCoordinator.retryAvailable();
}

void IrlumeKcm::refresh()
{
    m_refreshCoordinator.requestRefresh();
}

K_PLUGIN_CLASS_WITH_JSON(IrlumeKcm, "kcm_irlume.json")

#include "irlumekcm.moc"

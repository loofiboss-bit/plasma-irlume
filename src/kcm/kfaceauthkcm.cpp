// SPDX-License-Identifier: GPL-3.0-or-later

#include "kfaceauthkcm.h"

#include "camerapreviewitem.h"
#include "nativefaceauthbackend.h"

#include <KPluginFactory>
#include <qqml.h>

KFaceAuthKcm::KFaceAuthKcm(QObject *parent, const KPluginMetaData &data)
    : KFaceAuthKcm(parent, data, std::make_unique<NativeFaceAuthBackend>())
{
}

KFaceAuthKcm::KFaceAuthKcm(QObject *parent, const KPluginMetaData &data, std::unique_ptr<FaceAuthBackend> backend)
    : KQuickConfigModule(parent, data), m_probe(this), m_systemState(this), m_cameraPreviewSession(this),
      m_visionAnalysisSession(&m_cameraPreviewSession, this),
      m_supportReport(&m_systemState, &m_cameraPreviewSession, this), m_refreshCoordinator(std::move(backend), this)
{
    qmlRegisterType<CameraPreviewItem>(KFACEAUTH_QML_URI, 4, 0, "CameraPreview");
    qmlRegisterUncreatableType<CameraPreviewSession>(KFACEAUTH_QML_URI, 4, 0, "CameraPreviewSession",
                                                     QStringLiteral("CameraPreviewSession is provided by the KCM"));
    qmlRegisterUncreatableType<VisionAnalysisSession>(KFACEAUTH_QML_URI, 4, 0, "VisionAnalysisSession",
                                                      QStringLiteral("VisionAnalysisSession is provided by the KCM"));
    setButtons(NoAdditionalButton);
    connect(&m_refreshCoordinator, &RefreshCoordinator::snapshotChanged, this,
            [this](const EngineSnapshot &snapshot)
            {
                const auto inProgress = [](const auto &result)
                { return result.state == ResultState::Pending || result.state == ResultState::Loading; };
                if (!inProgress(snapshot.protocol) && !inProgress(snapshot.status))
                    m_probe.requestProbe(++m_probeGeneration, snapshot);
            });
    connect(&m_probe, &SystemProbe::probeCompleted, this,
            [this](quint64 generation, const SystemStateSnapshot &snapshot)
            {
                if (generation == m_probeGeneration)
                    m_systemState.apply(snapshot);
            });
    connect(&m_refreshCoordinator, &RefreshCoordinator::stateChanged, this, &KFaceAuthKcm::refreshStateChanged);
    refresh();
}

KFaceAuthKcm::~KFaceAuthKcm() = default;

SystemState *KFaceAuthKcm::systemState()
{
    return &m_systemState;
}

CameraPreviewSession *KFaceAuthKcm::cameraPreviewSession()
{
    return &m_cameraPreviewSession;
}

VisionAnalysisSession *KFaceAuthKcm::visionAnalysisSession()
{
    return &m_visionAnalysisSession;
}

SupportReport *KFaceAuthKcm::supportReport()
{
    return &m_supportReport;
}

bool KFaceAuthKcm::refreshing() const
{
    return m_refreshCoordinator.refreshing();
}

bool KFaceAuthKcm::partialDiagnostics() const
{
    return m_refreshCoordinator.partialDiagnostics();
}

bool KFaceAuthKcm::retryAvailable() const
{
    return m_refreshCoordinator.retryAvailable();
}

void KFaceAuthKcm::refresh()
{
    m_refreshCoordinator.requestRefresh();
}

K_PLUGIN_CLASS_WITH_JSON(KFaceAuthKcm, KFACEAUTH_PLUGIN_METADATA)

#include "kfaceauthkcm.moc"

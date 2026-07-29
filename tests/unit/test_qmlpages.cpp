// SPDX-License-Identifier: GPL-3.0-or-later

#include "camerapreviewitem.h"
#include "camerapreviewsession.h"
#include "enrollmentsession.h"
#include "identityworkerclient.h"
#include "kwalletkeyprovider.h"
#include "localverificationsession.h"
#include "supportreport.h"
#include "systemstate.h"
#include "visionanalysissession.h"

#include <KLocalizedQmlContext>

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTest>
#include <qqml.h>

class QmlPagesTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void milestonePagesCreateForUnavailableEngine();
    void cameraPageStopsWhenHidden();
    void analysisCancelsWhenApplicationDeactivates();
};

namespace
{
std::unique_ptr<QObject> createPage(QQmlEngine &engine, const QString &fileName, const QVariantMap &properties)
{
    const QString path = QStringLiteral(KFACEAUTH_SOURCE_DIR "/src/kcm/ui/") + fileName;
    QQmlComponent component(&engine, QUrl::fromLocalFile(path));
    if (component.isError())
    {
        qWarning().noquote() << component.errorString();
        return {};
    }

    auto object = std::unique_ptr<QObject>(component.createWithInitialProperties(properties));
    if (!object)
        qWarning().noquote() << component.errorString();
    return object;
}
} // namespace

void QmlPagesTest::milestonePagesCreateForUnavailableEngine()
{
    SystemStateSnapshot snapshot;
    snapshot.headline = QStringLiteral("Native engine unavailable");
    snapshot.summary = QStringLiteral("Camera checks remain available.");
    snapshot.issueCode = QStringLiteral("native-engine-unavailable");
    SystemState state;
    state.apply(snapshot);
    CameraPreviewSession cameraPreviewSession(QStringLiteral("/nonexistent/preview-worker"), nullptr);
    VisionAnalysisSession visionAnalysisSession(&cameraPreviewSession, QStringLiteral("/nonexistent/vision-worker"),
                                                nullptr);
    KWalletKeyProvider keyProvider;
    IdentityWorkerClient identityWorker(QStringLiteral("/nonexistent/identity-worker"), {}, nullptr);
    EnrollmentSession enrollmentSession(&cameraPreviewSession, &identityWorker, &keyProvider);
    LocalVerificationSession localVerificationSession(&cameraPreviewSession, &identityWorker, &keyProvider);
    SupportReport supportReport(&state, &cameraPreviewSession);
    QQmlEngine engine;
    auto *localizedContext = KLocalization::setupLocalizedContext(&engine);
    localizedContext->setTranslationDomain(QStringLiteral("kcm_kfaceauth"));

    auto overview = createPage(engine, QStringLiteral("SetupStatusPage.qml"),
                               {
                                   {QStringLiteral("systemState"), QVariant::fromValue(&state)},
                                   {QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&cameraPreviewSession)},
                                   {QStringLiteral("refreshActive"), false},
                               });
    auto camera = createPage(engine, QStringLiteral("CameraCheckPage.qml"),
                             {
                                 {QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&cameraPreviewSession)},
                                 {QStringLiteral("visionAnalysisSession"), QVariant::fromValue(&visionAnalysisSession)},
                             });
    auto diagnostics = createPage(engine, QStringLiteral("DiagnosticsPage.qml"),
                                  {
                                      {QStringLiteral("systemState"), QVariant::fromValue(&state)},
                                      {QStringLiteral("supportReport"), QVariant::fromValue(&supportReport)},
                                      {QStringLiteral("refreshActive"), false},
                                  });
    auto profile = createPage(engine, QStringLiteral("FaceProfilePage.qml"),
                              {
                                  {QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&cameraPreviewSession)},
                                  {QStringLiteral("enrollmentSession"), QVariant::fromValue(&enrollmentSession)},
                              });
    auto recognition =
        createPage(engine, QStringLiteral("TestRecognitionPage.qml"),
                   {
                       {QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&cameraPreviewSession)},
                       {QStringLiteral("localVerificationSession"), QVariant::fromValue(&localVerificationSession)},
                   });
    QVERIFY(overview);
    QVERIFY(camera);
    QVERIFY(diagnostics);
    QVERIFY(profile);
    QVERIFY(recognition);

    for (QObject *page : {overview.get(), camera.get(), profile.get(), recognition.get(), diagnostics.get()})
    {
        auto *item = qobject_cast<QQuickItem *>(page);
        QVERIFY(item);
        for (const int width : {320, 480, 960})
        {
            item->setSize(QSizeF(width, 720));
            QCoreApplication::processEvents();
            QVERIFY(item->implicitHeight() > 0);
        }
    }

    for (QObject *control : {
             overview->findChild<QObject *>(QStringLiteral("overviewRefreshButton")),
             overview->findChild<QObject *>(QStringLiteral("openCameraButton")),
             camera->findChild<QObject *>(QStringLiteral("cameraDeviceSelector")),
             camera->findChild<QObject *>(QStringLiteral("cameraRefreshButton")),
             camera->findChild<QObject *>(QStringLiteral("cameraPreviewAction")),
             camera->findChild<QObject *>(QStringLiteral("visionAnalyzeAction")),
             profile->findChild<QObject *>(QStringLiteral("refreshStatusButton")),
             profile->findChild<QObject *>(QStringLiteral("deleteProfileButton")),
             profile->findChild<QObject *>(QStringLiteral("resetProfileButton")),
             profile->findChild<QObject *>(QStringLiteral("previewButton")),
             profile->findChild<QObject *>(QStringLiteral("startEnrollmentButton")),
             profile->findChild<QObject *>(QStringLiteral("captureButton")),
             profile->findChild<QObject *>(QStringLiteral("retrySampleButton")),
             profile->findChild<QObject *>(QStringLiteral("cancelEnrollmentButton")),
             profile->findChild<QObject *>(QStringLiteral("finishEnrollmentButton")),
             recognition->findChild<QObject *>(QStringLiteral("previewButton")),
             recognition->findChild<QObject *>(QStringLiteral("verifyButton")),
             recognition->findChild<QObject *>(QStringLiteral("clearVerificationButton")),
             diagnostics->findChild<QObject *>(QStringLiteral("diagnosticsRefreshButton")),
             diagnostics->findChild<QObject *>(QStringLiteral("copyReportButton")),
             diagnostics->findChild<QObject *>(QStringLiteral("exportReportButton")),
         })
    {
        QVERIFY(control);
        QVERIFY(control->property("activeFocusOnTab").toBool());
        const QString accessibleText = control->property("text").isValid()
                                           ? control->property("text").toString()
                                           : control->property("accessibilityLabel").toString();
        QVERIFY(!accessibleText.isEmpty());
    }

    for (QObject *dialog : {
             profile->findChild<QObject *>(QStringLiteral("deleteProfileConfirmation")),
             profile->findChild<QObject *>(QStringLiteral("resetProfileConfirmation")),
         })
    {
        QVERIFY(dialog);
        QVERIFY(dialog->property("modal").toBool());
        QVERIFY(!dialog->property("title").toString().isEmpty());
    }
}

void QmlPagesTest::cameraPageStopsWhenHidden()
{
    CameraPreviewSession session(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("KFACEAUTH_FAKE_VISION_MODE"), QStringLiteral("timeout"));
    VisionAnalysisSession analysis(&session, QStringLiteral(KFACEAUTH_FAKE_VISION_WORKER_PATH), environment, nullptr);
    QQmlEngine engine;
    auto *localizedContext = KLocalization::setupLocalizedContext(&engine);
    localizedContext->setTranslationDomain(QStringLiteral("kcm_kfaceauth"));
    auto page = createPage(engine, QStringLiteral("CameraCheckPage.qml"),
                           {
                               {QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&session)},
                               {QStringLiteral("visionAnalysisSession"), QVariant::fromValue(&analysis)},
                           });
    QVERIFY(page);
    auto *item = qobject_cast<QQuickItem *>(page.get());
    QVERIFY(item);
    session.refreshDevices();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);

    auto *selector = page->findChild<QObject *>(QStringLiteral("cameraDeviceSelector"));
    auto *refreshButton = page->findChild<QObject *>(QStringLiteral("cameraRefreshButton"));
    auto *previewAction = page->findChild<QObject *>(QStringLiteral("cameraPreviewAction"));
    auto *analyzeAction = page->findChild<QObject *>(QStringLiteral("visionAnalyzeAction"));
    QVERIFY(selector);
    QVERIFY(refreshButton);
    QVERIFY(previewAction);
    QVERIFY(analyzeAction);
    QVERIFY(selector->property("activeFocusOnTab").toBool());
    QVERIFY(refreshButton->property("activeFocusOnTab").toBool());
    QVERIFY(previewAction->property("activeFocusOnTab").toBool());
    QVERIFY(analyzeAction->property("activeFocusOnTab").toBool());
    QVERIFY(!selector->property("accessibilityLabel").toString().isEmpty());

    session.startPreview();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Streaming);
    QTRY_VERIFY(session.frameAvailable());
    QTRY_VERIFY(analyzeAction->property("enabled").toBool());
    analysis.analyzeCurrentFrame();
    QTRY_COMPARE(analysis.state(), VisionAnalysisSession::State::Analyzing);
    item->setVisible(false);
    QTRY_COMPARE(analysis.state(), VisionAnalysisSession::State::Idle);
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);
    QVERIFY(!session.frameAvailable());
    QVERIFY(!analysis.resultAvailable());
}

void QmlPagesTest::analysisCancelsWhenApplicationDeactivates()
{
    CameraPreviewSession session(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    QProcessEnvironment environment;
    environment.insert(QStringLiteral("KFACEAUTH_FAKE_VISION_MODE"), QStringLiteral("timeout"));
    VisionAnalysisSession analysis(&session, QStringLiteral(KFACEAUTH_FAKE_VISION_WORKER_PATH), environment, nullptr);
    session.refreshDevices();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);
    session.startPreview();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Streaming);
    QTRY_VERIFY(session.frameAvailable());
    analysis.analyzeCurrentFrame();
    QTRY_COMPARE(analysis.state(), VisionAnalysisSession::State::Analyzing);

    QMetaObject::invokeMethod(qGuiApp, "applicationStateChanged", Qt::DirectConnection,
                              Q_ARG(Qt::ApplicationState, Qt::ApplicationInactive));
    QTRY_COMPARE(analysis.state(), VisionAnalysisSession::State::Idle);
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);
    QVERIFY(!analysis.resultAvailable());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    qmlRegisterType<CameraPreviewItem>("org.kde.kfaceauth", 4, 0, "CameraPreview");
    qmlRegisterUncreatableType<CameraPreviewSession>("org.kde.kfaceauth", 4, 0, "CameraPreviewSession",
                                                     QStringLiteral("provided by test"));
    qmlRegisterUncreatableType<VisionAnalysisSession>("org.kde.kfaceauth", 4, 0, "VisionAnalysisSession",
                                                      QStringLiteral("provided by test"));
    qmlRegisterUncreatableType<EnrollmentSession>("org.kde.kfaceauth", 4, 0, "EnrollmentSession",
                                                  QStringLiteral("provided by test"));
    qmlRegisterUncreatableType<LocalVerificationSession>("org.kde.kfaceauth", 4, 0, "LocalVerificationSession",
                                                         QStringLiteral("provided by test"));
    QmlPagesTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlpages.moc"

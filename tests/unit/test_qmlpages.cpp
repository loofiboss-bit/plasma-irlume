// SPDX-License-Identifier: GPL-3.0-or-later

#include "camerapreviewitem.h"
#include "camerapreviewsession.h"
#include "supportreport.h"
#include "systemstate.h"

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
                             {{QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&cameraPreviewSession)}});
    auto diagnostics = createPage(engine, QStringLiteral("DiagnosticsPage.qml"),
                                  {
                                      {QStringLiteral("systemState"), QVariant::fromValue(&state)},
                                      {QStringLiteral("supportReport"), QVariant::fromValue(&supportReport)},
                                      {QStringLiteral("refreshActive"), false},
                                  });
    QVERIFY(overview);
    QVERIFY(camera);
    QVERIFY(diagnostics);

    for (QObject *page : {overview.get(), camera.get(), diagnostics.get()})
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
}

void QmlPagesTest::cameraPageStopsWhenHidden()
{
    CameraPreviewSession session(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    QQmlEngine engine;
    auto *localizedContext = KLocalization::setupLocalizedContext(&engine);
    localizedContext->setTranslationDomain(QStringLiteral("kcm_kfaceauth"));
    auto page = createPage(engine, QStringLiteral("CameraCheckPage.qml"),
                           {{QStringLiteral("cameraPreviewSession"), QVariant::fromValue(&session)}});
    QVERIFY(page);
    auto *item = qobject_cast<QQuickItem *>(page.get());
    QVERIFY(item);
    session.refreshDevices();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);

    auto *selector = page->findChild<QObject *>(QStringLiteral("cameraDeviceSelector"));
    auto *refreshButton = page->findChild<QObject *>(QStringLiteral("cameraRefreshButton"));
    auto *previewAction = page->findChild<QObject *>(QStringLiteral("cameraPreviewAction"));
    QVERIFY(selector);
    QVERIFY(refreshButton);
    QVERIFY(previewAction);
    QVERIFY(selector->property("activeFocusOnTab").toBool());
    QVERIFY(refreshButton->property("activeFocusOnTab").toBool());
    QVERIFY(previewAction->property("activeFocusOnTab").toBool());
    QVERIFY(!selector->property("accessibilityLabel").toString().isEmpty());

    session.startPreview();
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Streaming);
    QTRY_VERIFY(session.frameAvailable());
    item->setVisible(false);
    QTRY_COMPARE(session.state(), CameraPreviewSession::State::Ready);
    QVERIFY(!session.frameAvailable());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    qmlRegisterType<CameraPreviewItem>("org.kde.kfaceauth", 4, 0, "CameraPreview");
    qmlRegisterUncreatableType<CameraPreviewSession>("org.kde.kfaceauth", 4, 0, "CameraPreviewSession",
                                                     QStringLiteral("provided by test"));
    QmlPagesTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlpages.moc"

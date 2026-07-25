// SPDX-License-Identifier: GPL-3.0-or-later

#include "fakeadapter.h"
#include "profilemodel.h"
#include "systemstate.h"

#include <KLocalizedQmlContext>

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTest>

class QmlPagesTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void everyScenarioCreatesEveryPage();
};

namespace
{
std::unique_ptr<QObject> createPage(QQmlEngine &engine, const QString &fileName, const QVariantMap &properties)
{
    const QString path = QStringLiteral(IRLUME_SOURCE_DIR "/src/kcm/ui/") + fileName;
    QQmlComponent component(&engine, QUrl::fromLocalFile(path));
    if (component.isError())
    {
        qWarning().noquote() << component.errorString();
        return {};
    }

    auto object = std::unique_ptr<QObject>(component.createWithInitialProperties(properties));
    if (!object)
    {
        qWarning().noquote() << component.errorString();
    }
    return object;
}
} // namespace

void QmlPagesTest::everyScenarioCreatesEveryPage()
{
    FakeSystemStateAdapter adapter;
    ProfileModel profileModel;
    QQmlEngine engine;
    auto *localizedContext = KLocalization::setupLocalizedContext(&engine);
    localizedContext->setTranslationDomain(QStringLiteral("plasma_irlume"));

    for (int index = 0; index < adapter.scenarioNames().size(); ++index)
    {
        SystemState state;
        state.apply(adapter.stateForScenario(index));

        const QVariant stateValue = QVariant::fromValue(&state);
        auto overview =
            createPage(engine, QStringLiteral("OverviewPage.qml"), {{QStringLiteral("systemState"), stateValue}});
        QVERIFY2(overview, qPrintable(QStringLiteral("Overview failed for scenario %1").arg(index)));

        auto security =
            createPage(engine, QStringLiteral("SecurityPage.qml"), {{QStringLiteral("systemState"), stateValue}});
        QVERIFY2(security, qPrintable(QStringLiteral("Security failed for scenario %1").arg(index)));

        auto enrollment = createPage(engine, QStringLiteral("EnrollmentPage.qml"),
                                     {
                                         {QStringLiteral("systemState"), stateValue},
                                         {QStringLiteral("profileModel"), QVariant::fromValue(&profileModel)},
                                     });
        QVERIFY2(enrollment, qPrintable(QStringLiteral("Enrollment failed for scenario %1").arg(index)));

        auto diagnostics =
            createPage(engine, QStringLiteral("DiagnosticsPage.qml"), {{QStringLiteral("systemState"), stateValue}});
        QVERIFY2(diagnostics, qPrintable(QStringLiteral("Diagnostics failed for scenario %1").arg(index)));

        auto *overviewItem = qobject_cast<QQuickItem *>(overview.get());
        auto *securityItem = qobject_cast<QQuickItem *>(security.get());
        auto *enrollmentItem = qobject_cast<QQuickItem *>(enrollment.get());
        auto *diagnosticsItem = qobject_cast<QQuickItem *>(diagnostics.get());
        QVERIFY(overviewItem);
        QVERIFY(securityItem);
        QVERIFY(enrollmentItem);
        QVERIFY(diagnosticsItem);
        for (const int width : {320, 480, 960})
        {
            overviewItem->setSize(QSizeF(width, 720));
            securityItem->setSize(QSizeF(width, 720));
            enrollmentItem->setSize(QSizeF(width, 720));
            diagnosticsItem->setSize(QSizeF(width, 720));
            QCoreApplication::processEvents();
            QVERIFY(overviewItem->implicitHeight() > 0);
            QVERIFY(securityItem->implicitHeight() > 0);
            QVERIFY(enrollmentItem->implicitHeight() > 0);
            QVERIFY(diagnosticsItem->implicitHeight() > 0);
        }
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QmlPagesTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_qmlpages.moc"

// SPDX-License-Identifier: GPL-3.0-or-later

#include "authconfiguration.h"
#include "fakeadapter.h"
#include "profilemodel.h"
#include "supportreport.h"
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
class NullAuthActionRunner final : public AuthActionRunner
{
  public:
    using AuthActionRunner::AuthActionRunner;

    bool start(AuthAction, const QVariantMap &) override
    {
        return false;
    }
};

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
        NullAuthActionRunner authRunner;
        AuthConfiguration authConfiguration(&state, &authRunner);
        SupportReport supportReport(&state, &profileModel, &authConfiguration);

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

        auto authentication =
            createPage(engine, QStringLiteral("AuthenticationPage.qml"),
                       {
                           {QStringLiteral("systemState"), stateValue},
                           {QStringLiteral("authConfiguration"), QVariant::fromValue(&authConfiguration)},
                       });
        QVERIFY2(authentication, qPrintable(QStringLiteral("Authentication failed for scenario %1").arg(index)));

        auto diagnostics =
            createPage(engine, QStringLiteral("DiagnosticsPage.qml"),
                       {
                           {QStringLiteral("systemState"), stateValue},
                           {QStringLiteral("authConfiguration"), QVariant::fromValue(&authConfiguration)},
                           {QStringLiteral("supportReport"), QVariant::fromValue(&supportReport)},
                       });
        QVERIFY2(diagnostics, qPrintable(QStringLiteral("Diagnostics failed for scenario %1").arg(index)));

        auto *overviewItem = qobject_cast<QQuickItem *>(overview.get());
        auto *securityItem = qobject_cast<QQuickItem *>(security.get());
        auto *enrollmentItem = qobject_cast<QQuickItem *>(enrollment.get());
        auto *diagnosticsItem = qobject_cast<QQuickItem *>(diagnostics.get());
        auto *authenticationItem = qobject_cast<QQuickItem *>(authentication.get());
        QVERIFY(overviewItem);
        QVERIFY(securityItem);
        QVERIFY(enrollmentItem);
        QVERIFY(diagnosticsItem);
        QVERIFY(authenticationItem);
        for (const int width : {320, 480, 960})
        {
            overviewItem->setSize(QSizeF(width, 720));
            securityItem->setSize(QSizeF(width, 720));
            enrollmentItem->setSize(QSizeF(width, 720));
            diagnosticsItem->setSize(QSizeF(width, 720));
            authenticationItem->setSize(QSizeF(width, 720));
            QCoreApplication::processEvents();
            QVERIFY(overviewItem->implicitHeight() > 0);
            QVERIFY(securityItem->implicitHeight() > 0);
            QVERIFY(enrollmentItem->implicitHeight() > 0);
            QVERIFY(diagnosticsItem->implicitHeight() > 0);
            QVERIFY(authenticationItem->implicitHeight() > 0);
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

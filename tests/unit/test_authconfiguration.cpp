// SPDX-License-Identifier: GPL-3.0-or-later

#include "authconfiguration.h"
#include "fakeadapter.h"
#include "systemstate.h"

#include <QSignalSpy>
#include <QTest>

class FakeAuthActionRunner final : public AuthActionRunner
{
  public:
    using AuthActionRunner::AuthActionRunner;

    bool start(AuthAction action, const QVariantMap &arguments) override
    {
        calls.push_back({action, arguments});
        return starts;
    }

    void finish(AuthAction action, bool success, const QVariantMap &data = {}, const QString &errorCode = {})
    {
        Q_EMIT completed(action, success, data, errorCode);
    }

    struct Call
    {
        AuthAction action;
        QVariantMap arguments;
    };
    QList<Call> calls;
    bool starts = true;
};

class AuthConfigurationTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void secureStateEnablesBothScopes();
    void rgbStateNeverEnablesLoginScreen();
    void recoveryMustBeAcknowledged();
    void previewAndVerifiedMutationUpdateState();
    void automaticRollbackIsReported();
};

void AuthConfigurationTest::secureStateEnablesBothScopes()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);

    QVERIFY(configuration.canEnableLockScreen());
    QVERIFY(configuration.canEnableLoginScreen());
    QVERIFY(configuration.canDisable());
    QCOMPARE(configuration.recoveryCommand(), QStringLiteral("sudo irlume login disable --apply"));
}

void AuthConfigurationTest::rgbStateNeverEnablesLoginScreen()
{
    FakeSystemStateAdapter adapter;
    auto snapshot = adapter.stateForScenario(FakeSystemStateAdapter::RgbOnly);
    snapshot.profileStatus = SystemStateSnapshot::ProfileStatus::Enrolled;
    SystemState state;
    state.apply(snapshot);
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);

    QVERIFY(configuration.canEnableLockScreen());
    QVERIFY(!configuration.canEnableLoginScreen());
    configuration.previewLoginScreen();
    QVERIFY(runner.calls.isEmpty());
    QCOMPARE(configuration.errorCode(), QStringLiteral("secure-tier-required"));
}

void AuthConfigurationTest::recoveryMustBeAcknowledged()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);

    configuration.enableLockScreen();
    QVERIFY(runner.calls.isEmpty());
    QCOMPARE(configuration.errorCode(), QStringLiteral("recovery-not-acknowledged"));

    configuration.setRecoveryAcknowledged(true);
    configuration.enableLockScreen();
    QVERIFY(runner.calls.isEmpty());
    QCOMPARE(configuration.errorCode(), QStringLiteral("preview-required"));

    configuration.previewLockScreen();
    runner.finish(AuthAction::Preview, true,
                  {
                      {QStringLiteral("scope"), QStringLiteral("lock-screen")},
                      {QStringLiteral("changes"), QStringList{QStringLiteral("pam-service:kde")}},
                  });
    QVERIFY(configuration.canApplyLockScreen());
    configuration.enableLockScreen();
    QCOMPARE(runner.calls.size(), 2);
    QCOMPARE(runner.calls.last().action, AuthAction::EnableLockScreen);
}

void AuthConfigurationTest::previewAndVerifiedMutationUpdateState()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);
    QSignalSpy changedSpy(&configuration, &AuthConfiguration::configurationChanged);

    configuration.previewLockScreen();
    QCOMPARE(runner.calls.last().action, AuthAction::Preview);
    QCOMPARE(runner.calls.last().arguments.value(QStringLiteral("scope")).toString(), QStringLiteral("lock-screen"));
    runner.finish(AuthAction::Preview, true,
                  {{QStringLiteral("scope"), QStringLiteral("lock-screen")},
                   {QStringLiteral("changes"),
                    QStringList{QStringLiteral("pam-service:plasmalogin"), QStringLiteral("pam-service:kde")}}});
    QVERIFY(configuration.previewAvailable());
    QCOMPARE(configuration.previewChanges().size(), 2);

    configuration.setRecoveryAcknowledged(true);
    configuration.enableLockScreen();
    runner.finish(AuthAction::EnableLockScreen, true,
                  {
                      {QStringLiteral("transactionId"), QStringLiteral("transaction-example-001")},
                      {QStringLiteral("verified"), true},
                      {QStringLiteral("passwordFallbackPreserved"), true},
                  });
    QVERIFY(configuration.lockScreenEnabled());
    QVERIFY(!configuration.previewAvailable());
    QCOMPARE(changedSpy.count(), 1);
}

void AuthConfigurationTest::automaticRollbackIsReported()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);

    configuration.previewLoginScreen();
    runner.finish(AuthAction::Preview, true,
                  {
                      {QStringLiteral("scope"), QStringLiteral("login-screen")},
                      {QStringLiteral("changes"), QStringList{QStringLiteral("pam-service:plasmalogin")}},
                  });
    configuration.setRecoveryAcknowledged(true);
    configuration.enableLoginScreen();
    runner.finish(AuthAction::EnableLoginScreen, false, {{QStringLiteral("rollbackRestored"), true}},
                  QStringLiteral("post-apply-verification-failed"));

    QVERIFY(configuration.rollbackRestored());
    QVERIFY(!configuration.loginScreenEnabled());
    QVERIFY(configuration.statusText().contains(QStringLiteral("restored"), Qt::CaseInsensitive));
}

QTEST_GUILESS_MAIN(AuthConfigurationTest)

#include "test_authconfiguration.moc"

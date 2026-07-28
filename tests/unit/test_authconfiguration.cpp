// SPDX-License-Identifier: GPL-3.0-or-later

#include "authconfiguration.h"
#include "fakeadapter.h"
#include "systemstate.h"

#include <QTest>

class FakeAuthActionRunner final : public AuthActionRunner
{
  public:
    using AuthActionRunner::AuthActionRunner;

    bool start(AuthAction action, const QVariantMap &arguments) override
    {
        calls.push_back({action, arguments});
        return true;
    }

    struct Call
    {
        AuthAction action;
        QVariantMap arguments;
    };
    QList<Call> calls;
};

class AuthConfigurationTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void contractOneIsReadOnly();
    void allMutationEntrypointsFailClosed();
    void loginSnapshotIsPresentedReadOnly();
};

EngineSnapshot contractOneSnapshot()
{
    EngineSnapshot snapshot;
    snapshot.contractAvailable = true;
    snapshot.capabilities.mutationSupported = false;
    return snapshot;
}

void AuthConfigurationTest::contractOneIsReadOnly()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);

    configuration.applySnapshot(contractOneSnapshot());

    QVERIFY(configuration.contractAvailable() == false);
    QVERIFY(configuration.mutationSupported() == false);
    QVERIFY(configuration.canEnableLockScreen() == false);
    QVERIFY(configuration.canEnableLoginScreen() == false);
    QVERIFY(configuration.canDisable() == false);
    QCOMPARE(configuration.recoveryCommand(), QStringLiteral("sudo irlume login disable --apply"));
}

void AuthConfigurationTest::allMutationEntrypointsFailClosed()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);
    configuration.applySnapshot(contractOneSnapshot());

    const QList<std::function<void()>> operations{
        [&configuration]() { configuration.previewLockScreen(); },
        [&configuration]() { configuration.previewLoginScreen(); },
        [&configuration]() { configuration.previewDisable(); },
        [&configuration]() { configuration.enableLockScreen(); },
        [&configuration]() { configuration.enableLoginScreen(); },
        [&configuration]() { configuration.disable(); },
        [&configuration]() { configuration.disableNow(); },
        [&configuration]() { configuration.rollbackLastTransaction(); },
    };

    for (const auto &operation : operations)
    {
        operation();
        QCOMPARE(configuration.errorCode(), QStringLiteral("capability-unavailable"));
        QVERIFY(configuration.busy() == false);
    }
    QVERIFY(runner.calls.isEmpty());
}

void AuthConfigurationTest::loginSnapshotIsPresentedReadOnly()
{
    FakeSystemStateAdapter adapter;
    SystemState state;
    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::SecureIr));
    FakeAuthActionRunner runner;
    AuthConfiguration configuration(&state, &runner);
    EngineSnapshot snapshot = contractOneSnapshot();
    EngineLoginSnapshot login;
    login.loginManagerServices = {QStringLiteral("plasmalogin")};
    login.surfaces = {
        {QStringLiteral("kde"), QStringLiteral("lock-screen"), true, true, QStringLiteral("required")},
        {QStringLiteral("plasmalogin"), QStringLiteral("login-screen"), true, true, QStringLiteral("required")},
    };
    snapshot.login = login;

    configuration.applySnapshot(snapshot);

    QVERIFY(configuration.lockScreenEnabled());
    QVERIFY(configuration.loginScreenEnabled());
    QVERIFY(configuration.statusText().contains(QStringLiteral("read-only"), Qt::CaseInsensitive));
    QVERIFY(runner.calls.isEmpty());
}

QTEST_GUILESS_MAIN(AuthConfigurationTest)

#include "test_authconfiguration.moc"

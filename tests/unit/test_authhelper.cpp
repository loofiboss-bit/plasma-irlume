// SPDX-License-Identifier: GPL-3.0-or-later

#include "authhelper.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include <deque>

class AuthHelperTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void commandsAreFixedAndIdentifiersAreValidated();
    void planMustMatchTheActiveDisplayManager();
    void previewRejectsInactiveDisplayManagerTargets();
    void secureLockScreenTransactionIsVerified();
    void contractVersionTwoTransactionIsVerified();
    void disableTransactionReachesVerifiedCleanState();
    void failedApplyTriggersRollback();
    void failedVerificationTriggersRollback();
    void engineReportedRollbackIsNotRepeated();
    void loginScreenRequiresSecureTier();
    void unsupportedPlatformNeverInvokesEngine();
};

namespace
{
QJsonObject fixture(const QString &name)
{
    QFile file(QStringLiteral(IRLUME_SOURCE_DIR "/tests/fixtures/irlume/proposed-v1/") + name);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QJsonObject lockScreenPlan()
{
    QJsonObject document = fixture(QStringLiteral("login-plan.json"));
    QJsonObject data = document.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("requested_scopes"), QJsonArray{QStringLiteral("lock-screen")});
    const QJsonArray originalChanges = data.value(QStringLiteral("changes")).toArray();
    data.insert(QStringLiteral("changes"), QJsonArray{originalChanges.at(1)});
    document.insert(QStringLiteral("data"), data);
    return document;
}

QJsonObject lockScreenApply()
{
    QJsonObject document = fixture(QStringLiteral("login-apply.json"));
    QJsonObject data = document.value(QStringLiteral("data")).toObject();
    const QJsonArray originalOperations = data.value(QStringLiteral("operations")).toArray();
    data.insert(QStringLiteral("operations"), QJsonArray{originalOperations.at(1)});
    document.insert(QStringLiteral("data"), data);
    return document;
}

QJsonObject disablePlan()
{
    QJsonObject document = fixture(QStringLiteral("login-plan.json"));
    QJsonObject data = document.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("operation"), QStringLiteral("disable"));
    data.remove(QStringLiteral("requested_scopes"));
    document.insert(QStringLiteral("data"), data);
    return document;
}

QJsonObject disabledVerify()
{
    QJsonObject document = fixture(QStringLiteral("login-verify.json"));
    QJsonObject data = document.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("desired"), QStringLiteral("disabled"));
    data.insert(QStringLiteral("actual"), QStringLiteral("disabled"));
    document.insert(QStringLiteral("data"), data);
    return document;
}

AuthHelper::CommandResult response(QJsonObject document)
{
    return {document.value(QStringLiteral("ok")).toBool(), std::move(document), {}};
}

QJsonObject contractVersionTwo(QJsonObject document)
{
    document.insert(QStringLiteral("contract_version"), 2);
    return document;
}

struct Script
{
    std::deque<AuthHelper::CommandResult> replies;
    QList<QStringList> calls;

    AuthHelper::CommandResult run(const QStringList &arguments)
    {
        calls.push_back(arguments);
        if (replies.empty())
        {
            return {false, {}, QStringLiteral("unexpected-command")};
        }
        auto result = replies.front();
        replies.pop_front();
        return result;
    }
};

QByteArray fedora44()
{
    return QByteArrayLiteral("ID=fedora\nVERSION_ID=\"44\"\n");
}
} // namespace

void AuthHelperTest::commandsAreFixedAndIdentifiersAreValidated()
{
    QCOMPARE(AuthHelper::planArguments(QStringLiteral("lock-screen")),
             QStringList({QStringLiteral("login"), QStringLiteral("enable"), QStringLiteral("--scope"),
                          QStringLiteral("lock-screen"), QStringLiteral("--json")}));
    QCOMPARE(AuthHelper::planArguments(QStringLiteral("login-screen")).at(3), QStringLiteral("login-screen"));
    QCOMPARE(AuthHelper::planArguments(QStringLiteral("disable")),
             QStringList({QStringLiteral("login"), QStringLiteral("disable"), QStringLiteral("--json")}));
    QVERIFY(AuthHelper::planArguments(QStringLiteral("--apply;sh")).isEmpty());
    QVERIFY(AuthHelper::applyArguments(QStringLiteral("lock-screen"), QStringLiteral("../plan")).isEmpty());
    QVERIFY(AuthHelper::verifyArguments(QStringLiteral("tx;reboot")).isEmpty());
    QVERIFY(AuthHelper::rollbackArguments(QStringLiteral("/tmp/tx")).isEmpty());
    QVERIFY(AuthHelper::isSafeOpaqueId(QStringLiteral("transaction-example-001")));
}

void AuthHelperTest::previewRejectsInactiveDisplayManagerTargets()
{
    Script script;
    QJsonObject plan = lockScreenPlan();
    QJsonObject data = plan.value(QStringLiteral("data")).toObject();
    QJsonArray changes = data.value(QStringLiteral("changes")).toArray();
    QJsonObject first = changes.first().toObject();
    first.insert(QStringLiteral("target"), QStringLiteral("pam-service:sddm"));
    changes.replace(0, first);
    data.insert(QStringLiteral("changes"), changes);
    plan.insert(QStringLiteral("data"), data);
    script.replies = {response(fixture(QStringLiteral("version.json"))), response(plan)};

    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));
    const auto reply = helper.preview({{QStringLiteral("scope"), QStringLiteral("lock-screen")}});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(), QStringLiteral("unsafe-plan-target"));
}

void AuthHelperTest::planMustMatchTheActiveDisplayManager()
{
    Script script;
    QJsonObject plan = lockScreenPlan();
    QJsonObject data = plan.value(QStringLiteral("data")).toObject();
    QJsonObject displayManager = data.value(QStringLiteral("display_manager")).toObject();
    displayManager.insert(QStringLiteral("id"), QStringLiteral("sddm"));
    data.insert(QStringLiteral("display_manager"), displayManager);
    plan.insert(QStringLiteral("data"), data);
    script.replies = {response(fixture(QStringLiteral("version.json"))), response(plan)};

    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));
    const auto reply = helper.preview({{QStringLiteral("scope"), QStringLiteral("lock-screen")}});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(), QStringLiteral("unsafe-plan"));
}

void AuthHelperTest::secureLockScreenTransactionIsVerified()
{
    Script script;
    script.replies = {
        response(fixture(QStringLiteral("version.json"))),
        response(lockScreenPlan()),
        response(lockScreenApply()),
        response(fixture(QStringLiteral("login-verify.json"))),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::SuccessType);
    QCOMPARE(reply.data().value(QStringLiteral("verified")).toBool(), true);
    QCOMPARE(reply.data().value(QStringLiteral("passwordFallbackPreserved")).toBool(), true);
    QCOMPARE(reply.data().value(QStringLiteral("displayManager")).toString(), QStringLiteral("plasmalogin"));
    QCOMPARE(script.calls.size(), 4);
    QCOMPARE(script.calls.at(2),
             AuthHelper::applyArguments(QStringLiteral("lock-screen"), QStringLiteral("plan-example-001")));
    QCOMPARE(script.calls.at(3), AuthHelper::verifyArguments(QStringLiteral("transaction-example-001")));
}

void AuthHelperTest::contractVersionTwoTransactionIsVerified()
{
    Script script;
    script.replies = {
        response(contractVersionTwo(fixture(QStringLiteral("version.json")))),
        response(contractVersionTwo(lockScreenPlan())),
        response(contractVersionTwo(lockScreenApply())),
        response(contractVersionTwo(fixture(QStringLiteral("login-verify.json")))),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::SuccessType);
    QCOMPARE(reply.data().value(QStringLiteral("verified")).toBool(), true);
}

void AuthHelperTest::failedVerificationTriggersRollback()
{
    Script script;
    QJsonObject failedVerify = fixture(QStringLiteral("login-verify.json"));
    QJsonObject verifyData = failedVerify.value(QStringLiteral("data")).toObject();
    verifyData.insert(QStringLiteral("actual"), QStringLiteral("disabled"));
    failedVerify.insert(QStringLiteral("data"), verifyData);
    script.replies = {
        response(fixture(QStringLiteral("version.json"))),
        response(lockScreenPlan()),
        response(lockScreenApply()),
        response(failedVerify),
        response(fixture(QStringLiteral("login-rollback.json"))),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(),
             QStringLiteral("post-apply-verification-failed"));
    QCOMPARE(reply.data().value(QStringLiteral("rollbackRestored")).toBool(), true);
    QCOMPARE(script.calls.last(), AuthHelper::rollbackArguments(QStringLiteral("transaction-example-001")));
}

void AuthHelperTest::disableTransactionReachesVerifiedCleanState()
{
    Script script;
    script.replies = {
        response(fixture(QStringLiteral("version.json"))),
        response(disablePlan()),
        response(fixture(QStringLiteral("login-apply.json"))),
        response(disabledVerify()),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.disable({});

    QCOMPARE(reply.type(), KAuth::ActionReply::SuccessType);
    QCOMPARE(reply.data().value(QStringLiteral("desiredState")).toString(), QStringLiteral("disabled"));
    QCOMPARE(reply.data().value(QStringLiteral("verified")).toBool(), true);
    QCOMPARE(script.calls.at(1), AuthHelper::planArguments(QStringLiteral("disable")));
}

void AuthHelperTest::failedApplyTriggersRollback()
{
    Script script;
    QJsonObject failedApply = fixture(QStringLiteral("login-apply-failed-rolled-back.json"));
    QJsonObject data = failedApply.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("plan_id"), QStringLiteral("plan-example-001"));
    data.insert(QStringLiteral("transaction_id"), QStringLiteral("transaction-example-001"));
    QJsonObject rollback = data.value(QStringLiteral("rollback")).toObject();
    rollback.insert(QStringLiteral("state"), QStringLiteral("failed"));
    rollback.insert(QStringLiteral("restored"), false);
    data.insert(QStringLiteral("rollback"), rollback);
    failedApply.insert(QStringLiteral("data"), data);
    script.replies = {
        response(fixture(QStringLiteral("version.json"))),
        response(lockScreenPlan()),
        response(failedApply),
        response(fixture(QStringLiteral("login-rollback.json"))),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("rollbackRestored")).toBool(), true);
    QCOMPARE(script.calls.last(), AuthHelper::rollbackArguments(QStringLiteral("transaction-example-001")));
}

void AuthHelperTest::engineReportedRollbackIsNotRepeated()
{
    Script script;
    script.replies = {
        response(fixture(QStringLiteral("version.json"))),
        response(lockScreenPlan()),
        response(fixture(QStringLiteral("login-apply-failed-rolled-back.json"))),
    };
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("rollbackRestored")).toBool(), true);
    QCOMPARE(script.calls.size(), 3);
}

void AuthHelperTest::loginScreenRequiresSecureTier()
{
    Script script;
    QJsonObject plan = lockScreenPlan();
    QJsonObject data = plan.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("requested_scopes"), QJsonArray{QStringLiteral("login-screen")});
    data.insert(QStringLiteral("security_tier"), QStringLiteral("convenience"));
    const QJsonArray originalChanges = fixture(QStringLiteral("login-plan.json"))
                                           .value(QStringLiteral("data"))
                                           .toObject()
                                           .value(QStringLiteral("changes"))
                                           .toArray();
    data.insert(QStringLiteral("changes"), QJsonArray{originalChanges.at(0)});
    plan.insert(QStringLiteral("data"), data);
    script.replies = {response(fixture(QStringLiteral("version.json"))), response(plan)};
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); }, fedora44(),
                      QStringLiteral("plasmalogin"));

    const auto reply = helper.enableloginscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(), QStringLiteral("secure-tier-required"));
    QCOMPARE(script.calls.size(), 2);
}

void AuthHelperTest::unsupportedPlatformNeverInvokesEngine()
{
    Script script;
    AuthHelper helper([&script](const QStringList &arguments) { return script.run(arguments); },
                      QByteArrayLiteral("ID=fedora\nVERSION_ID=45\n"), QStringLiteral("plasmalogin"));

    const auto reply = helper.enablelockscreen({});

    QCOMPARE(reply.type(), KAuth::ActionReply::HelperErrorType);
    QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(), QStringLiteral("platform-unsupported"));
    QVERIFY(script.calls.isEmpty());
}

QTEST_GUILESS_MAIN(AuthHelperTest)

#include "test_authhelper.moc"

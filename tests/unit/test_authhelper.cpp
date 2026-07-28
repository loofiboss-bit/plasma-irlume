// SPDX-License-Identifier: GPL-3.0-or-later

#include "authhelper.h"

#include <QTest>

class AuthHelperTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void everyFixedActionFailsWithoutExecuting();
    void malformedArgumentsAreRejectedFirst();
};

void AuthHelperTest::everyFixedActionFailsWithoutExecuting()
{
    int executions = 0;
    AuthHelper helper([&executions]() { ++executions; });
    const QList<KAuth::ActionReply> replies = {
        helper.preview({{QStringLiteral("scope"), QStringLiteral("lock-screen")}}),
        helper.enablelockscreen({}),
        helper.enableloginscreen({}),
        helper.disable({}),
        helper.verify({{QStringLiteral("transactionId"), QStringLiteral("transaction-1")},
                       {QStringLiteral("desiredState"), QStringLiteral("enabled")}}),
        helper.rollback({{QStringLiteral("transactionId"), QStringLiteral("transaction-1")}}),
        helper.selectcamera({{QStringLiteral("pairId"), QStringLiteral("pair-1")}}),
        helper.setupemitter({}),
        helper.tunecamera({}),
    };
    QCOMPARE(executions, 0);
    for (const KAuth::ActionReply &reply : replies)
    {
        QCOMPARE(reply.data().value(QStringLiteral("errorCode")).toString(), QStringLiteral("capability-unavailable"));
        QCOMPARE(reply.data().value(QStringLiteral("retryable")).toBool(), false);
    }
}

void AuthHelperTest::malformedArgumentsAreRejectedFirst()
{
    AuthHelper helper;
    QCOMPARE(helper.preview({{QStringLiteral("scope"), QStringLiteral("arbitrary")}})
                 .data()
                 .value(QStringLiteral("errorCode"))
                 .toString(),
             QStringLiteral("invalid-operation-arguments"));
    QCOMPARE(helper.selectcamera({{QStringLiteral("pairId"), QStringLiteral("../../etc/shadow")}})
                 .data()
                 .value(QStringLiteral("errorCode"))
                 .toString(),
             QStringLiteral("invalid-operation-arguments"));
}

QTEST_MAIN(AuthHelperTest)

#include "test_authhelper.moc"

// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollmentsession.h"

#include <QSignalSpy>
#include <QTest>

class EnrollmentSessionTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void unsupportedOperationsNeverStart();
    void presentationStateRemainsEmpty();
};

void EnrollmentSessionTest::unsupportedOperationsNeverStart()
{
    EnrollmentSession session;
    QSignalSpy spy(&session, &EnrollmentSession::operationError);

    QVERIFY(!session.startOperation(EnrollmentSession::Operation::Enroll));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.constFirst().at(1).toString(), QStringLiteral("capability-unavailable"));
    QCOMPARE(spy.constFirst().at(2).toBool(), false);
    QVERIFY(!session.active());
}

void EnrollmentSessionTest::presentationStateRemainsEmpty()
{
    EnrollmentSession session;
    QVERIFY(!session.frameAvailable());
    QVERIFY(session.frame().isNull());
    QVERIFY(session.landmarks().isEmpty());
    QCOMPARE(session.quality(), 0);
}

QTEST_MAIN(EnrollmentSessionTest)

#include "test_enrollmentsession.moc"

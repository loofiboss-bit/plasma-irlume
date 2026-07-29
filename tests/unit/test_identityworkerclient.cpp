// SPDX-License-Identifier: BSD-2-Clause

#include "identityworkerclient.h"

#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QTest>

#ifndef KFACEAUTH_FAKE_IDENTITY_WORKER_PATH
#error "KFACEAUTH_FAKE_IDENTITY_WORKER_PATH must be defined"
#endif

namespace
{
QProcessEnvironment environmentFor(const QString &mode)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("KFACEAUTH_TEST_MODE"), mode);
    return environment;
}
} // namespace

class IdentityWorkerClientTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void completesOneBoundedRequest();
    void malformedResponseFailsClosed();
    void cancellationClearsTheActiveGeneration();
    void overlappingRequestIsRejected();
    void workerCrashIsUnavailable();
    void operationTimeoutTerminatesWorker();
};

void IdentityWorkerClientTest::completesOneBoundedRequest()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("valid")), this);
    QByteArray received;
    QString error;
    quint64 generation = 0;
    client.execute(1, QByteArrayLiteral("sensitive-request"),
                   [&](quint64 completed, QByteArrayView payload, const QString &failure)
                   {
                       generation = completed;
                       received = QByteArray(payload);
                       error = failure;
                   });

    QVERIFY(client.busy());
    QTRY_COMPARE(generation, quint64(1));
    QCOMPARE(received.size(), 12);
    QVERIFY(error.isEmpty());
    QVERIFY(!client.busy());
}

void IdentityWorkerClientTest::malformedResponseFailsClosed()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("malformed")), this);
    QString error;
    client.execute(2, QByteArrayLiteral("request"),
                   [&](quint64, QByteArrayView, const QString &failure) { error = failure; });

    QTRY_COMPARE(error, QStringLiteral("identity-protocol-error"));
    QVERIFY(!client.busy());
}

void IdentityWorkerClientTest::cancellationClearsTheActiveGeneration()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("hang")), this);
    QString error;
    quint64 generation = 0;
    client.execute(3, QByteArrayLiteral("sensitive-request"),
                   [&](quint64 completed, QByteArrayView, const QString &failure)
                   {
                       generation = completed;
                       error = failure;
                   });

    QVERIFY(client.busy());
    client.cancel();
    QTRY_COMPARE(generation, quint64(3));
    QCOMPARE(error, QStringLiteral("cancelled"));
    QVERIFY(!client.busy());
}

void IdentityWorkerClientTest::overlappingRequestIsRejected()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("delayed")), this);
    quint64 firstGeneration = 0;
    QString secondError;
    client.execute(4, QByteArrayLiteral("first"),
                   [&](quint64 completed, QByteArrayView, const QString &) { firstGeneration = completed; });
    client.execute(5, QByteArrayLiteral("second"),
                   [&](quint64, QByteArrayView, const QString &failure) { secondError = failure; });

    QCOMPARE(secondError, QStringLiteral("identity-worker-busy"));
    QTRY_COMPARE(firstGeneration, quint64(4));
    QVERIFY(!client.busy());
}

void IdentityWorkerClientTest::workerCrashIsUnavailable()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("crash")), this);
    QString error;
    client.execute(6, QByteArrayLiteral("request"),
                   [&](quint64, QByteArrayView, const QString &failure) { error = failure; });

    QTRY_COMPARE(error, QStringLiteral("identity-worker-failed"));
    QVERIFY(!client.busy());
}

void IdentityWorkerClientTest::operationTimeoutTerminatesWorker()
{
    IdentityWorkerClient client(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("hang")), 200, 40, 200, this);
    QString error;
    client.execute(7, QByteArrayLiteral("request"),
                   [&](quint64, QByteArrayView, const QString &failure) { error = failure; });

    QTRY_COMPARE_WITH_TIMEOUT(error, QStringLiteral("identity-operation-timeout"), 1000);
    QVERIFY(!client.busy());
}

QTEST_GUILESS_MAIN(IdentityWorkerClientTest)

#include "test_identityworkerclient.moc"

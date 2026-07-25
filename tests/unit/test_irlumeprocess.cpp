// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumeprocess.h"

#include <QFile>
#include <QJsonDocument>
#include <QTest>

class IrlumeProcessTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void buildsOnlyFixedCommands();
    void parsesEnrollmentEventSequence();
    void parsesCancellationAndCameraBusy();
    void parsesProfileDocuments();
    void rejectsSensitiveOrMalformedEvents();
};

namespace
{
QJsonObject objectFromLine(const QByteArray &line)
{
    const QJsonDocument document = QJsonDocument::fromJson(line);
    return document.object();
}

QVector<QJsonObject> fixtureLines(const QString &name)
{
    QFile file(QStringLiteral(IRLUME_SOURCE_DIR "/tests/fixtures/irlume/") + name);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    QVector<QJsonObject> objects;
    while (!file.atEnd())
    {
        const QByteArray line = file.readLine().trimmed();
        if (!line.isEmpty())
        {
            objects.push_back(objectFromLine(line));
        }
    }
    return objects;
}

QJsonObject fixtureDocument(const QString &name)
{
    QFile file(QStringLiteral(IRLUME_SOURCE_DIR "/tests/fixtures/irlume/") + name);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}
} // namespace

void IrlumeProcessTest::buildsOnlyFixedCommands()
{
    QCOMPARE(IrlumeProcess::argumentsForOperation(IrlumeProcess::Operation::Capabilities),
             QStringList({QStringLiteral("version"), QStringLiteral("--json")}));
    QCOMPARE(IrlumeProcess::argumentsForOperation(IrlumeProcess::Operation::AuthTest),
             QStringList({QStringLiteral("auth"), QStringLiteral("test"), QStringLiteral("--events=jsonl")}));

    const QString profileId = QStringLiteral("profile-example-001");
    const QStringList deleteArguments =
        IrlumeProcess::argumentsForOperation(IrlumeProcess::Operation::DeleteProfile, profileId);
    QCOMPARE(deleteArguments, QStringList({QStringLiteral("profiles"), QStringLiteral("delete"),
                                           QStringLiteral("--profile-id"), profileId, QStringLiteral("--json")}));
    QVERIFY(!deleteArguments.contains(QStringLiteral("--user")));
    QVERIFY(IrlumeProcess::isSafeOpaqueId(profileId));
    QVERIFY(!IrlumeProcess::isSafeOpaqueId(QStringLiteral("../../etc/shadow")));
    QVERIFY(!IrlumeProcess::isSafeOpaqueId(QStringLiteral("profile id")));
}

void IrlumeProcessTest::parsesEnrollmentEventSequence()
{
    const auto events = fixtureLines(QStringLiteral("proposed-v1/enroll.ndjson"));
    QCOMPARE(events.size(), 4);

    QString operationId;
    for (int sequence = 0; sequence < events.size(); ++sequence)
    {
        const auto result = IrlumeProcess::parseStreamEvent(events.at(sequence), IrlumeProcess::Operation::Enroll,
                                                            sequence, operationId);
        QVERIFY2(result.ok, qPrintable(result.errorCode));
        if (operationId.isEmpty())
        {
            operationId = result.event.operationId;
        }
        QCOMPARE(result.event.sequence, sequence);
        QCOMPARE(result.event.terminal, sequence == events.size() - 1);
    }
}

void IrlumeProcessTest::parsesCancellationAndCameraBusy()
{
    const auto cancelled = fixtureLines(QStringLiteral("proposed-v1/enroll-cancelled.ndjson"));
    const auto cancelResult = IrlumeProcess::parseStreamEvent(cancelled.constLast(), IrlumeProcess::Operation::Enroll,
                                                              2, QStringLiteral("operation-example-cancel"));
    QVERIFY(cancelResult.ok);
    QCOMPARE(cancelResult.event.type, QStringLiteral("cancelled"));
    QCOMPARE(cancelResult.event.errorCode, QStringLiteral("user-cancelled"));
    QVERIFY(cancelResult.event.retryable);

    const auto busy = fixtureLines(QStringLiteral("events/camera-busy.ndjson"));
    const auto busyResult = IrlumeProcess::parseStreamEvent(busy.constLast(), IrlumeProcess::Operation::AuthTest, 1,
                                                            QStringLiteral("operation-example-camera-busy"));
    QVERIFY(busyResult.ok);
    QCOMPARE(busyResult.event.errorCode, QStringLiteral("camera-busy"));
    QVERIFY(busyResult.event.retryable);
}

void IrlumeProcessTest::parsesProfileDocuments()
{
    const auto profiles = IrlumeProcess::parseDocument(
        fixtureDocument(QStringLiteral("proposed-v1/profiles-list.json")), IrlumeProcess::Operation::ListProfiles);
    QVERIFY2(profiles.ok, qPrintable(profiles.errorCode));
    QVERIFY(profiles.event.data.value(QStringLiteral("profiles")).isArray());

    const auto deleted = IrlumeProcess::parseDocument(fixtureDocument(QStringLiteral("events/profile-delete.json")),
                                                      IrlumeProcess::Operation::DeleteProfile);
    QVERIFY2(deleted.ok, qPrintable(deleted.errorCode));
    QVERIFY(deleted.event.data.value(QStringLiteral("deleted")).toBool());
}

void IrlumeProcessTest::rejectsSensitiveOrMalformedEvents()
{
    QJsonObject event = fixtureLines(QStringLiteral("proposed-v1/enroll.ndjson")).at(0);
    event.insert(QStringLiteral("frame"), QStringLiteral("not-allowed"));
    auto result = IrlumeProcess::parseStreamEvent(event, IrlumeProcess::Operation::Enroll, 0);
    QVERIFY(!result.ok);
    QCOMPARE(result.errorCode, QStringLiteral("invalid-event-contract"));

    event = fixtureLines(QStringLiteral("proposed-v1/enroll.ndjson")).at(1);
    result = IrlumeProcess::parseStreamEvent(event, IrlumeProcess::Operation::Enroll, 3,
                                             QStringLiteral("operation-example-enroll"));
    QVERIFY(!result.ok);
    QCOMPARE(result.errorCode, QStringLiteral("invalid-event-sequence"));

    event = fixtureLines(QStringLiteral("proposed-v1/auth-test.ndjson")).constLast();
    event[QStringLiteral("data")] = QJsonObject{
        {QStringLiteral("matched"), true},
        {QStringLiteral("credential"), QStringLiteral("secret")},
    };
    result = IrlumeProcess::parseStreamEvent(event, IrlumeProcess::Operation::AuthTest, 2,
                                             QStringLiteral("operation-example-auth-test"));
    QVERIFY(!result.ok);
}

QTEST_MAIN(IrlumeProcessTest)

#include "test_irlumeprocess.moc"

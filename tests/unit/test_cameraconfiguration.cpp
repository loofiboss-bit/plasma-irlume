// SPDX-License-Identifier: GPL-3.0-or-later

#include "cameraconfiguration.h"

#include <QSignalSpy>
#include <QTest>

class CameraConfigurationTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void exposesCameraStateReadOnly();
    void mutationsFailClosed();
};

void CameraConfigurationTest::exposesCameraStateReadOnly()
{
    CameraConfiguration configuration;
    EngineSnapshot snapshot;
    snapshot.contractAvailable = true;
    snapshot.capabilities.statusRead = true;
    snapshot.status = EngineStatusSnapshot{};
    configuration.applySnapshot(snapshot);

    QVERIFY(configuration.readOnlyAvailable());
    QVERIFY(!configuration.mutationSupported());
    QVERIFY(!configuration.ready());
    QVERIFY(!configuration.hasPairs());
}

void CameraConfigurationTest::mutationsFailClosed()
{
    CameraConfiguration configuration;
    configuration.selectPair();
    QCOMPARE(configuration.errorCode(), QStringLiteral("capability-unavailable"));
    configuration.setupEmitter();
    QCOMPARE(configuration.errorCode(), QStringLiteral("capability-unavailable"));
    configuration.tuneCamera();
    QCOMPARE(configuration.errorCode(), QStringLiteral("capability-unavailable"));
}

QTEST_MAIN(CameraConfigurationTest)

#include "test_cameraconfiguration.moc"

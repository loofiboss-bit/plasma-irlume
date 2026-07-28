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
    snapshot.handshake.state = ResultState::Available;
    snapshot.handshake.data = EngineHandshakeSnapshot{1, QStringLiteral("0.7.0")};
    snapshot.capabilities.features = EngineFeature::StatusRead;
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

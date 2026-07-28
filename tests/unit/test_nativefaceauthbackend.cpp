// SPDX-License-Identifier: GPL-3.0-or-later

#include "nativefaceauthbackend.h"

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>

class NativeFaceAuthBackendTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void unavailableEngineCompletesAsynchronously();
    void availableSkeletonReportsOnlySafeCapabilities();
    void newerGenerationCancelsOlderRefresh();
    void explicitCancellationIsGenerationAware();
};

void NativeFaceAuthBackendTest::unavailableEngineCompletesAsynchronously()
{
    NativeFaceAuthBackend backend;
    QSignalSpy progressSpy(&backend, &FaceAuthBackend::refreshProgress);
    QSignalSpy completedSpy(&backend, &FaceAuthBackend::refreshCompleted);
    QElapsedTimer elapsed;
    elapsed.start();

    backend.requestRefresh(1);

    QVERIFY(elapsed.elapsed() < 100);
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(completedSpy.count(), 0);
    QTRY_COMPARE(completedSpy.count(), 1);
    const EngineSnapshot snapshot = qvariant_cast<EngineSnapshot>(completedSpy.constFirst().at(1));
    QVERIFY(!snapshot.engineAvailable);
    QCOMPARE(snapshot.protocol.state, ResultState::Failed);
    QCOMPARE(snapshot.protocol.error->code, QStringLiteral("native-engine-unavailable"));
    QCOMPARE(snapshot.capabilities.authentication, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.enrollment, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.pamConfiguration, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.templatePersistence, OperationSupport::Unsupported);
}

void NativeFaceAuthBackendTest::availableSkeletonReportsOnlySafeCapabilities()
{
    NativeFaceAuthBackend backend(
        []()
        {
            return NativeFaceAuthBackend::Availability{
                true,
                1,
                QStringLiteral("0.1.0"),
            };
        });
    QSignalSpy completedSpy(&backend, &FaceAuthBackend::refreshCompleted);
    backend.requestRefresh(4);
    QTRY_COMPARE(completedSpy.count(), 1);

    const EngineSnapshot snapshot = qvariant_cast<EngineSnapshot>(completedSpy.constFirst().at(1));
    QVERIFY(snapshot.engineAvailable);
    QVERIFY(snapshot.protocolAvailable());
    QVERIFY(snapshot.capabilities.supports(EngineFeature::StatusRead));
    QVERIFY(snapshot.capabilities.supports(EngineFeature::CapabilityRead));
    QCOMPARE(snapshot.capabilities.authentication, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.enrollment, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.pamConfiguration, OperationSupport::Unsupported);
    QCOMPARE(snapshot.capabilities.templatePersistence, OperationSupport::Unsupported);
}

void NativeFaceAuthBackendTest::newerGenerationCancelsOlderRefresh()
{
    NativeFaceAuthBackend backend;
    QSignalSpy cancelledSpy(&backend, &FaceAuthBackend::refreshCancelled);
    QSignalSpy completedSpy(&backend, &FaceAuthBackend::refreshCompleted);

    backend.requestRefresh(10);
    backend.requestRefresh(11);

    QCOMPARE(cancelledSpy.count(), 1);
    QCOMPARE(cancelledSpy.constFirst().at(0).toULongLong(), quint64(10));
    QTRY_COMPARE(completedSpy.count(), 1);
    QCOMPARE(completedSpy.constFirst().at(0).toULongLong(), quint64(11));
}

void NativeFaceAuthBackendTest::explicitCancellationIsGenerationAware()
{
    NativeFaceAuthBackend backend;
    QSignalSpy cancelledSpy(&backend, &FaceAuthBackend::refreshCancelled);
    QSignalSpy completedSpy(&backend, &FaceAuthBackend::refreshCompleted);

    backend.requestRefresh(20);
    backend.cancelRefresh();

    QCOMPARE(cancelledSpy.count(), 1);
    QCOMPARE(cancelledSpy.constFirst().at(0).toULongLong(), quint64(20));
    QTest::qWait(1);
    QCOMPARE(completedSpy.count(), 0);
}

QTEST_GUILESS_MAIN(NativeFaceAuthBackendTest)

#include "test_nativefaceauthbackend.moc"

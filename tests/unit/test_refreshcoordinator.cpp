// SPDX-License-Identifier: GPL-3.0-or-later

#include "refreshcoordinator.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimer>

class ControlledBackend final : public FaceAuthBackend
{
    Q_OBJECT

  public:
    using FaceAuthBackend::FaceAuthBackend;

    void requestRefresh(quint64 generation) override
    {
        requests.push_back(generation);
    }

    void cancelRefresh() override
    {
        ++cancellations;
    }

    void progress(quint64 generation, const EngineSnapshot &snapshot)
    {
        Q_EMIT refreshProgress(generation, snapshot);
    }

    void complete(quint64 generation, const EngineSnapshot &snapshot)
    {
        Q_EMIT refreshCompleted(generation, snapshot);
    }

    QList<quint64> requests;
    int cancellations = 0;
};

class RefreshCoordinatorTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void constructionAndRefreshAreNonBlocking();
    void latestGenerationWins();
    void destructionCancelsAndDisconnects();
};

void RefreshCoordinatorTest::constructionAndRefreshAreNonBlocking()
{
    auto backend = std::make_unique<ControlledBackend>();
    ControlledBackend *controlled = backend.get();
    RefreshCoordinator coordinator(std::move(backend));
    QSignalSpy stateSpy(&coordinator, &RefreshCoordinator::stateChanged);

    coordinator.requestRefresh();

    QCOMPARE(controlled->requests, QList<quint64>{1});
    QVERIFY(coordinator.refreshing());
    QCOMPARE(stateSpy.size(), 1);
    QTimer::singleShot(0, &coordinator, [&coordinator]() { coordinator.cancelRefresh(); });
    QTRY_VERIFY(!coordinator.refreshing());
}

void RefreshCoordinatorTest::latestGenerationWins()
{
    auto backend = std::make_unique<ControlledBackend>();
    ControlledBackend *controlled = backend.get();
    RefreshCoordinator coordinator(std::move(backend));
    QSignalSpy snapshotSpy(&coordinator, &RefreshCoordinator::snapshotChanged);

    coordinator.requestRefresh();
    coordinator.requestRefresh();
    QCOMPARE(controlled->requests, (QList<quint64>{1, 2}));

    EngineSnapshot stale;
    stale.engineAvailable = false;
    controlled->complete(1, stale);
    QVERIFY(coordinator.refreshing());
    QCOMPARE(snapshotSpy.size(), 0);

    EngineSnapshot current;
    current.engineAvailable = true;
    current.protocol.state = ResultState::Available;
    current.protocol.data = EngineProtocolSnapshot{1, QStringLiteral("0.1.0")};
    controlled->progress(2, current);
    QCOMPARE(snapshotSpy.size(), 1);
    controlled->complete(2, current);
    QVERIFY(!coordinator.refreshing());
    QCOMPARE(snapshotSpy.size(), 2);
}

void RefreshCoordinatorTest::destructionCancelsAndDisconnects()
{
    auto backend = std::make_unique<ControlledBackend>();
    ControlledBackend *controlled = backend.get();
    bool backendDestroyed = false;
    connect(controlled, &QObject::destroyed, this, [&backendDestroyed]() { backendDestroyed = true; });
    auto coordinator = std::make_unique<RefreshCoordinator>(std::move(backend));
    coordinator->requestRefresh();

    coordinator.reset();

    QVERIFY(backendDestroyed);
}

QTEST_GUILESS_MAIN(RefreshCoordinatorTest)

#include "test_refreshcoordinator.moc"

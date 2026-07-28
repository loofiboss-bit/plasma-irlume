// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemprobe.h"
#include "systemstate.h"

#include <QSignalSpy>
#include <QTest>

namespace
{
SystemProbeInputs baseInputs()
{
    SystemProbeInputs inputs;
    inputs.osRelease = "NAME=Fedora Linux\nID=fedora\nVERSION_ID=\"44\"\n";
    inputs.plasmaVersion = QStringLiteral("6.6.2");
    inputs.displayManagerTarget = QStringLiteral("/usr/lib/systemd/system/plasmalogin.service");
    inputs.secureBootVariablePresent = true;
    inputs.secureBootVariable = QByteArray::fromHex("0700000001");
    return inputs;
}
} // namespace

class SystemStateTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void unavailableEngineFailsClosed();
    void skeletonReportsUnsupportedOperations();
    void applyingStateNotifiesConsumers();
    void hostProbeRunsAsynchronously();
};

void SystemStateTest::unavailableEngineFailsClosed()
{
    const SystemStateSnapshot snapshot = SystemProbe::evaluate(baseInputs());

    QCOMPARE(snapshot.engineStatus, SystemStateSnapshot::EngineStatus::Unavailable);
    QCOMPARE(snapshot.issueCode, QStringLiteral("native-engine-unavailable"));
    QCOMPARE(snapshot.authenticationStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.enrollmentStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.pamStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.templatePersistenceStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
}

void SystemStateTest::skeletonReportsUnsupportedOperations()
{
    SystemProbeInputs inputs = baseInputs();
    inputs.engine.engineAvailable = true;
    inputs.engine.protocol = EngineProtocolSnapshot{1, QStringLiteral("0.1.0")};
    inputs.engine.status = EngineStatusSnapshot{EngineStatusSnapshot::State::Skeleton};

    const SystemStateSnapshot snapshot = SystemProbe::evaluate(inputs);

    QCOMPARE(snapshot.engineStatus, SystemStateSnapshot::EngineStatus::SkeletonAvailable);
    QCOMPARE(snapshot.engineVersion, QStringLiteral("0.1.0"));
    QCOMPARE(snapshot.authenticationStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.enrollmentStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.pamStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QCOMPARE(snapshot.templatePersistenceStatus, SystemStateSnapshot::CapabilityStatus::Unsupported);
    QVERIFY(snapshot.issueCode.isEmpty());
}

void SystemStateTest::applyingStateNotifiesConsumers()
{
    SystemState state;
    QSignalSpy spy(&state, &SystemState::stateChanged);
    SystemStateSnapshot snapshot;
    snapshot.scenarioId = QStringLiteral("native-milestone-1");
    snapshot.engineStatus = SystemStateSnapshot::EngineStatus::Unavailable;
    state.apply(snapshot);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(state.scenarioId(), QStringLiteral("native-milestone-1"));
    QCOMPARE(state.engineStatus(), SystemState::EngineStatus::Unavailable);
    QCOMPARE(state.authenticationStatus(), SystemState::CapabilityStatus::Unsupported);
    QVERIFY(!state.authenticationStatusLabel().isEmpty());
}

void SystemStateTest::hostProbeRunsAsynchronously()
{
    SystemProbe probe;
    QSignalSpy spy(&probe, &SystemProbe::probeCompleted);
    probe.requestProbe(7, EngineSnapshot{});

    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.constFirst().at(0).toULongLong(), quint64(7));
    const auto snapshot = qvariant_cast<SystemStateSnapshot>(spy.constFirst().at(1));
    QVERIFY(snapshot.liveData);
}

QTEST_GUILESS_MAIN(SystemStateTest)

#include "test_systemstate.moc"

// SPDX-License-Identifier: GPL-3.0-or-later

#include "fakeadapter.h"
#include "systemprobe.h"
#include "systemstate.h"

#include <QSignalSpy>
#include <QTest>

class SystemStateTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void fakeAdapterExposesEveryRequiredScenario();
    void scenarioStatesAreTypedAndSafe();
    void applyingStateNotifiesConsumers();
    void invalidScenarioFailsClosed();
    void liveProbeDetectsSecureFedoraSystem();
    void liveProbeRestrictsRgbHardware();
    void liveProbeRejectsUnsupportedEngine();
    void liveProbeRejectsUnsupportedPlatform();
    void liveProbeDetectsDisplayManagerMigration();
    void supportReportIsRedacted();
    void hostProbeProducesSafeSnapshot();
};

void SystemStateTest::fakeAdapterExposesEveryRequiredScenario()
{
    const FakeSystemStateAdapter adapter;
    QCOMPARE(adapter.scenarioNames().size(), static_cast<int>(FakeSystemStateAdapter::ScenarioCount));

    const QStringList expectedIds = {
        QStringLiteral("secure-ir"),      QStringLiteral("rgb-only"),           QStringLiteral("no-camera"),
        QStringLiteral("missing-irlume"), QStringLiteral("unsupported-irlume"), QStringLiteral("broken-daemon"),
        QStringLiteral("pam-drift"),
    };

    for (int index = 0; index < expectedIds.size(); ++index)
    {
        const auto state = adapter.stateForScenario(index);
        QCOMPARE(state.scenarioId, expectedIds.at(index));
        QVERIFY(!state.headline.isEmpty());
        QVERIFY(!state.summary.isEmpty());
        QVERIFY(state.passwordFallbackPreserved);
    }
}

void SystemStateTest::scenarioStatesAreTypedAndSafe()
{
    const FakeSystemStateAdapter adapter;

    const auto secure = adapter.stateForScenario(FakeSystemStateAdapter::SecureIr);
    QCOMPARE(secure.securityTier, SystemStateSnapshot::SecurityTier::Secure);
    QCOMPARE(secure.cameraType, SystemStateSnapshot::CameraType::Infrared);
    QCOMPARE(secure.engineStatus, SystemStateSnapshot::EngineStatus::Ready);
    QCOMPARE(secure.daemonStatus, SystemStateSnapshot::DaemonStatus::Running);
    QCOMPARE(secure.pamStatus, SystemStateSnapshot::PamStatus::Clean);

    const auto rgb = adapter.stateForScenario(FakeSystemStateAdapter::RgbOnly);
    QCOMPARE(rgb.securityTier, SystemStateSnapshot::SecurityTier::Convenience);
    QCOMPARE(rgb.cameraType, SystemStateSnapshot::CameraType::Rgb);

    const auto unsupported = adapter.stateForScenario(FakeSystemStateAdapter::UnsupportedIrlume);
    QCOMPARE(unsupported.securityTier, SystemStateSnapshot::SecurityTier::Unsupported);
    QCOMPARE(unsupported.engineStatus, SystemStateSnapshot::EngineStatus::UnsupportedVersion);
    QCOMPARE(unsupported.engineVersion, QStringLiteral("0.7.0"));

    const auto drift = adapter.stateForScenario(FakeSystemStateAdapter::PamDrift);
    QCOMPARE(drift.pamStatus, SystemStateSnapshot::PamStatus::Drift);
    QCOMPARE(drift.issueCode, QStringLiteral("pam-drift"));
}

void SystemStateTest::applyingStateNotifiesConsumers()
{
    const FakeSystemStateAdapter adapter;
    SystemState state;
    QSignalSpy spy(&state, &SystemState::stateChanged);

    state.apply(adapter.stateForScenario(FakeSystemStateAdapter::BrokenDaemon));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(state.scenarioId(), QStringLiteral("broken-daemon"));
    QCOMPARE(state.daemonStatus(), SystemState::DaemonStatus::Broken);
    QCOMPARE(state.securityTier(), SystemState::SecurityTier::Unsupported);
    QCOMPARE(state.passwordFallbackPreserved(), true);
    QVERIFY(!state.daemonStatusLabel().isEmpty());
}

void SystemStateTest::invalidScenarioFailsClosed()
{
    const FakeSystemStateAdapter adapter;
    const auto state = adapter.stateForScenario(-1);

    QCOMPARE(state.scenarioId, QStringLiteral("invalid-scenario"));
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Unsupported);
    QCOMPARE(state.engineStatus, SystemStateSnapshot::EngineStatus::Unavailable);
    QVERIFY(!state.issueCode.isEmpty());
}

namespace
{
SystemProbeInputs secureProbeInputs()
{
    SystemProbeInputs inputs;
    inputs.osRelease = "NAME=Fedora Linux\nID=fedora\nVERSION_ID=\"44\"\n";
    inputs.plasmaVersion = QStringLiteral("6.6.2");
    inputs.displayManagerTarget = QStringLiteral("/usr/lib/systemd/system/plasmalogin.service");
    inputs.secureBootVariablePresent = true;
    inputs.secureBootVariable = QByteArray::fromHex("0700000001");
    inputs.tpmPresent = true;
    inputs.irlumePresent = true;
    inputs.irlumeVersionOutput = QStringLiteral("irlume 0.6.1\n");
    inputs.irlumeStatusOutput = QStringLiteral("irlume status for 'current-account'\n"
                                               "  daemon        : running ✅\n"
                                               "  enrollment    : 1 profile(s), 2 scan(s) ✅ · passive blink liveness\n"
                                               "  templates     : encrypted at rest ✅\n"
                                               "  cameras       : rgb=/dev/video0 ir=/dev/video2\n");
    inputs.irlumeDoctorOutput = QStringLiteral("[doctor] TPM 2.0: /dev/tpmrm0 ✓\n"
                                               "[doctor] Secure Boot: enabled ✓\n"
                                               "[doctor] models:\n"
                                               "  loaded by the daemon ✓\n");
    inputs.irlumeLoginStatusOutput = QStringLiteral("[login] wiring status (face auth in PAM):\n"
                                                    "  active login manager: plasmalogin\n"
                                                    "  /etc/pam.d/plasmalogin          ● wired (face-first)\n"
                                                    "  /etc/pam.d/kde                  ● wired\n");
    return inputs;
}
} // namespace

void SystemStateTest::liveProbeDetectsSecureFedoraSystem()
{
    const auto state = SystemProbe::evaluate(secureProbeInputs());

    QCOMPARE(state.fedoraVersion, QStringLiteral("44"));
    QCOMPARE(state.plasmaVersion, QStringLiteral("6.6.2"));
    QCOMPARE(state.activeDisplayManager, QStringLiteral("Plasma Login Manager"));
    QCOMPARE(state.engineVersion, QStringLiteral("0.6.1"));
    QCOMPARE(state.engineStatus, SystemStateSnapshot::EngineStatus::Ready);
    QCOMPARE(state.daemonStatus, SystemStateSnapshot::DaemonStatus::Running);
    QCOMPARE(state.cameraType, SystemStateSnapshot::CameraType::Infrared);
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Secure);
    QCOMPARE(state.profileStatus, SystemStateSnapshot::ProfileStatus::Enrolled);
    QCOMPARE(state.templateProtectionStatus, SystemStateSnapshot::CapabilityStatus::Available);
    QCOMPARE(state.secureBootStatus, SystemStateSnapshot::SecureBootStatus::Enabled);
}

void SystemStateTest::liveProbeRestrictsRgbHardware()
{
    auto inputs = secureProbeInputs();
    inputs.displayManagerTarget = QStringLiteral("/usr/lib/systemd/system/sddm.service");
    inputs.irlumeStatusOutput = QStringLiteral("  daemon        : running ✅\n"
                                               "  enrollment    : none ⚠\n"
                                               "  cameras       : rgb=/dev/video0 ir=none\n");
    inputs.irlumeLoginStatusOutput = QStringLiteral("[login] wiring status (face auth in PAM):\n"
                                                    "  active login manager: sddm\n"
                                                    "  /etc/pam.d/sddm                ● wired (face-first)\n"
                                                    "  /etc/pam.d/kde                 ● wired\n");

    const auto state = SystemProbe::evaluate(inputs);
    QCOMPARE(state.activeDisplayManager, QStringLiteral("SDDM"));
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Convenience);
    QCOMPARE(state.cameraType, SystemStateSnapshot::CameraType::Rgb);
    QCOMPARE(state.livenessStatus, SystemStateSnapshot::CapabilityStatus::Unavailable);
    QCOMPARE(state.profileStatus, SystemStateSnapshot::ProfileStatus::NotEnrolled);
}

void SystemStateTest::liveProbeRejectsUnsupportedEngine()
{
    auto inputs = secureProbeInputs();
    inputs.irlumeVersionOutput = QStringLiteral("irlume 0.7.0\n");

    const auto state = SystemProbe::evaluate(inputs);
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Unsupported);
    QCOMPARE(state.engineStatus, SystemStateSnapshot::EngineStatus::UnsupportedVersion);
    QCOMPARE(state.issueCode, QStringLiteral("engine-version-unsupported"));
}

void SystemStateTest::liveProbeRejectsUnsupportedPlatform()
{
    auto inputs = secureProbeInputs();
    inputs.osRelease = "ID=fedora\nVERSION_ID=\"45\"\n";

    const auto state = SystemProbe::evaluate(inputs);
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Unsupported);
    QCOMPARE(state.issueCode, QStringLiteral("platform-unsupported"));
}

void SystemStateTest::liveProbeDetectsDisplayManagerMigration()
{
    auto inputs = secureProbeInputs();
    inputs.irlumeLoginStatusOutput = QStringLiteral("[login] wiring status (face auth in PAM):\n"
                                                    "  active login manager: plasmalogin\n"
                                                    "  /etc/pam.d/plasmalogin          ● wired (face-first)\n"
                                                    "  /etc/pam.d/kde                  ● wired\n"
                                                    "  /etc/pam.d/sddm                 ● wired (stale)\n");

    const auto state = SystemProbe::evaluate(inputs);

    QCOMPARE(state.pamStatus, SystemStateSnapshot::PamStatus::Drift);
    QCOMPARE(state.securityTier, SystemStateSnapshot::SecurityTier::Unsupported);
    QCOMPARE(state.issueCode, QStringLiteral("display-manager-migration"));
}

void SystemStateTest::supportReportIsRedacted()
{
    auto inputs = secureProbeInputs();
    inputs.displayManagerTarget = QStringLiteral("/home/private/display-manager.service");
    inputs.irlumeStatusOutput += QStringLiteral("profile: private@example.test /dev/video9\n");

    const auto state = SystemProbe::evaluate(inputs);
    QVERIFY(!state.supportReport.contains(QStringLiteral("/home/")));
    QVERIFY(!state.supportReport.contains(QStringLiteral("/dev/")));
    QVERIFY(!state.supportReport.contains(QStringLiteral("@example.")));
    QVERIFY(!state.supportReport.contains(QStringLiteral("current-account")));
    QVERIFY(state.supportReport.contains(QStringLiteral("Fedora: 44")));
}

void SystemStateTest::hostProbeProducesSafeSnapshot()
{
    const auto state = SystemProbe().probe();

    QVERIFY(state.liveData);
    QCOMPARE(state.scenarioId, QStringLiteral("live-system"));
    QVERIFY(!state.supportReport.isEmpty());
    QVERIFY(!state.supportReport.contains(QStringLiteral("/home/")));
    QVERIFY(!state.supportReport.contains(QStringLiteral("/dev/")));
    QVERIFY(!state.supportReport.contains(QStringLiteral("/etc/")));
}

QTEST_MAIN(SystemStateTest)

#include "test_systemstate.moc"

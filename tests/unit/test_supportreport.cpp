// SPDX-License-Identifier: GPL-3.0-or-later

#include "authconfiguration.h"
#include "fakeadapter.h"
#include "profilemodel.h"
#include "supportreport.h"
#include "systemstate.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

namespace
{
class NullAuthActionRunner final : public AuthActionRunner
{
  public:
    using AuthActionRunner::AuthActionRunner;
    bool start(AuthAction, const QVariantMap &) override
    {
        return false;
    }
};
} // namespace

class SupportReportTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void mapsEveryPhaseFiveIssueToAnAction();
    void reportContainsOnlyRedactedTypedState();
    void exportsMarkdownAtomically();
};

void SupportReportTest::mapsEveryPhaseFiveIssueToAnAction()
{
    const QStringList codes = {
        QStringLiteral("camera-busy"),
        QStringLiteral("camera-unavailable"),
        QStringLiteral("ir-emitter-failed"),
        QStringLiteral("tpm-unseal-failed"),
        QStringLiteral("secure-boot-pcr-changed"),
        QStringLiteral("engine-version-unsupported"),
        QStringLiteral("pam-drift"),
        QStringLiteral("display-manager-migration"),
        QStringLiteral("kwallet-password-mismatch"),
    };

    for (const QString &code : codes)
    {
        QVERIFY2(!SupportReport::titleForCode(code).isEmpty(), qPrintable(code));
        const QString action = SupportReport::actionForCode(code);
        QVERIFY2(!action.isEmpty(), qPrintable(code));
        QVERIFY2(action != SupportReport::actionForCode(QStringLiteral("unknown-error")), qPrintable(code));
    }
}

void SupportReportTest::reportContainsOnlyRedactedTypedState()
{
    auto snapshot = FakeSystemStateAdapter().stateForScenario(FakeSystemStateAdapter::PamDrift);
    snapshot.dataSource = QStringLiteral("/home/private-user/session");
    snapshot.fedoraVersion = QStringLiteral("password=hunter2");
    snapshot.plasmaVersion = QStringLiteral("owner@example.test");
    snapshot.engineVersion = QStringLiteral("embedding=biometric-payload");

    SystemState state;
    state.apply(snapshot);
    ProfileModel profileModel;
    NullAuthActionRunner runner;
    AuthConfiguration authConfiguration(&state, &runner);
    SupportReport supportReport(&state, &profileModel, &authConfiguration);

    const QString report = supportReport.report();
    QVERIFY(!report.contains(QStringLiteral("private-user")));
    QVERIFY(!report.contains(QStringLiteral("hunter2")));
    QVERIFY(!report.contains(QStringLiteral("owner@example.test")));
    QVERIFY(!report.contains(QStringLiteral("biometric-payload")));
    QVERIFY(!report.contains(QStringLiteral("/home/")));
    QVERIFY(report.contains(QStringLiteral("[redacted]")));
    QCOMPARE(supportReport.issueCode(), QStringLiteral("pam-drift"));
}

void SupportReportTest::exportsMarkdownAtomically()
{
    SystemState state;
    state.apply(FakeSystemStateAdapter().stateForScenario(FakeSystemStateAdapter::SecureIr));
    ProfileModel profileModel;
    NullAuthActionRunner runner;
    AuthConfiguration authConfiguration(&state, &runner);
    SupportReport supportReport(&state, &profileModel, &authConfiguration);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QVERIFY(supportReport.exportToDirectory(directory.path()));
    const QStringList files = QDir(directory.path()).entryList({QStringLiteral("*.md")}, QDir::Files);
    QCOMPARE(files.size(), 1);
    QFile reportFile(QDir(directory.path()).filePath(files.constFirst()));
    QVERIFY(reportFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray contents = reportFile.readAll();
    QVERIFY(contents.startsWith("# plasma-irlume support report"));
    QVERIFY(!contents.contains("/home/"));
}

QTEST_MAIN(SupportReportTest)

#include "test_supportreport.moc"

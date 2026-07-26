// SPDX-License-Identifier: GPL-3.0-or-later

#include "profilemodel.h"

#include <QJsonArray>
#include <QSignalSpy>
#include <QTest>

class FakeIrlumeProcess final : public IrlumeProcess
{
    Q_OBJECT

  public:
    struct Start
    {
        Operation operation;
        QString profileId;
    };

    explicit FakeIrlumeProcess(QObject *parent = nullptr) : IrlumeProcess(QStringLiteral("/usr/bin/false"), parent) {}

    bool startOperation(Operation operation, const QString &profileId) override
    {
        starts.push_back({operation, profileId});
        return startSucceeds;
    }

    void cancel() override
    {
        cancelCalled = true;
    }

    void send(Operation operation, const QString &type, const QJsonObject &data = {}, const QString &errorCode = {},
              bool retryable = false)
    {
        Event event;
        event.operation = operation;
        event.command = commandName(operation);
        event.type = type;
        event.terminal =
            type == QLatin1String("completed") || type == QLatin1String("failed") || type == QLatin1String("cancelled");
        event.data = data;
        event.errorCode = errorCode;
        event.retryable = retryable;
        Q_EMIT eventReceived(event);
    }

    void sendError(Operation operation, const QString &code, bool retryable = false)
    {
        Q_EMIT operationError(operation, code, retryable);
    }

    QVector<Start> starts;
    bool startSucceeds = true;
    bool cancelCalled = false;
};

class ProfileModelTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void capabilityGateFailsClosed();
    void malformedProfileListFailsClosed();
    void enrollmentIsVerifiedBeforeItIsKept();
    void failedEnrollmentVerificationDeletesNewProfile();
    void failedStandaloneTestDoesNotModifyProfiles();
    void cameraBusyCanRetryAndCancellationIsForwarded();
    void appearanceScanRequiresSelectedProfileResult();
    void deletionIsLimitedToKnownProfile();
    void unsafeDeletionResultFailsClosed();
};

namespace
{
QJsonObject capabilities()
{
    return {
        {QStringLiteral("capabilities"),
         QJsonArray{QStringLiteral("status-json"), QStringLiteral("profiles-json"), QStringLiteral("events-jsonl"),
                    QStringLiteral("position-report"), QStringLiteral("preview-ir-jpeg")}},
        {QStringLiteral("limits"),
         QJsonObject{{QStringLiteral("max_profiles"), 3}, {QStringLiteral("max_scans_per_profile"), 30}}},
    };
}

QJsonObject profiles(bool populated)
{
    if (!populated)
    {
        return {{QStringLiteral("profiles"), QJsonArray{}}};
    }
    return {
        {QStringLiteral("profiles"),
         QJsonArray{
             QJsonObject{
                 {QStringLiteral("profile_id"), QStringLiteral("profile-example-001")},
                 {QStringLiteral("display_name"), QStringLiteral("Face Profile 1")},
                 {QStringLiteral("scans"),
                  QJsonArray{
                      QJsonObject{
                          {QStringLiteral("scan_id"), QStringLiteral("scan-example-001")},
                          {QStringLiteral("display_name"), QStringLiteral("Scan 1")},
                      },
                  }},
             },
         }},
    };
}

void makeReady(ProfileModel &model, FakeIrlumeProcess &process, bool populated)
{
    model.refresh();
    process.send(IrlumeProcess::Operation::Capabilities, QStringLiteral("completed"), capabilities());
    process.send(IrlumeProcess::Operation::ListProfiles, QStringLiteral("completed"), profiles(populated));
}

QJsonObject safeAuthResult(bool matched)
{
    return {
        {QStringLiteral("matched"), matched},
        {QStringLiteral("liveness"), QStringLiteral("live")},
        {QStringLiteral("credential_released"), false},
        {QStringLiteral("profile_modified"), false},
    };
}
} // namespace

void ProfileModelTest::capabilityGateFailsClosed()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);

    model.refresh();
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::Capabilities);
    process.send(IrlumeProcess::Operation::Capabilities, QStringLiteral("completed"),
                 {{QStringLiteral("capabilities"), QJsonArray{QStringLiteral("profiles-json")}}});

    QVERIFY(!model.contractAvailable());
    QVERIFY(!model.busy());
    QCOMPARE(model.errorCode(), QStringLiteral("structured-contract-unavailable"));
    QCOMPARE(process.starts.size(), 1);
}

void ProfileModelTest::malformedProfileListFailsClosed()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);

    model.refresh();
    process.send(IrlumeProcess::Operation::Capabilities, QStringLiteral("completed"), capabilities());
    process.send(IrlumeProcess::Operation::ListProfiles, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profiles"),
                      QJsonArray{
                          QJsonObject{
                              {QStringLiteral("profile_id"), QStringLiteral("profile-example-001")},
                              {QStringLiteral("display_name"), QStringLiteral("Face Profile\nDelete another user")},
                              {QStringLiteral("scans"), QJsonArray{}},
                          },
                      }},
                 });

    QVERIFY(!model.contractAvailable());
    QCOMPARE(model.errorCode(), QStringLiteral("invalid-profile-list"));
    QCOMPARE(model.rowCount(), 0);
}

void ProfileModelTest::enrollmentIsVerifiedBeforeItIsKept()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, false);

    model.enroll();
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::Enroll);
    process.send(IrlumeProcess::Operation::Enroll, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profile_id"), QStringLiteral("profile-example-new")},
                     {QStringLiteral("created"), true},
                     {QStringLiteral("added_scans"), 3},
                 });
    QCOMPARE(model.workflow(), ProfileModel::Workflow::VerifyingEnrollment);
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::AuthTest);

    process.send(IrlumeProcess::Operation::AuthTest, QStringLiteral("completed"), safeAuthResult(true));
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::Capabilities);
    QVERIFY(process.starts.size() >= 5);
    process.send(IrlumeProcess::Operation::Capabilities, QStringLiteral("completed"), capabilities());
    process.send(IrlumeProcess::Operation::ListProfiles, QStringLiteral("completed"), profiles(true));
    QVERIFY(model.statusText().contains(QStringLiteral("verified")));
}

void ProfileModelTest::failedEnrollmentVerificationDeletesNewProfile()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, false);

    model.enroll();
    process.send(IrlumeProcess::Operation::Enroll, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profile_id"), QStringLiteral("profile-example-new")},
                     {QStringLiteral("created"), true},
                 });
    process.send(IrlumeProcess::Operation::AuthTest, QStringLiteral("completed"), safeAuthResult(false));

    QCOMPARE(model.workflow(), ProfileModel::Workflow::CleaningUp);
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::DeleteProfile);
    QCOMPARE(process.starts.constLast().profileId, QStringLiteral("profile-example-new"));

    process.send(IrlumeProcess::Operation::DeleteProfile, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profile_id"), QStringLiteral("profile-example-new")},
                     {QStringLiteral("deleted"), true},
                     {QStringLiteral("mutated_other_profiles"), false},
                 });
    QVERIFY(!model.busy());
    QCOMPARE(model.errorCode(), QStringLiteral("recognition-not-matched"));
}

void ProfileModelTest::failedStandaloneTestDoesNotModifyProfiles()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, true);
    const int startsBeforeTest = process.starts.size();

    model.testRecognition(QStringLiteral("profile-example-001"));
    process.send(IrlumeProcess::Operation::AuthTest, QStringLiteral("completed"), safeAuthResult(false));

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.errorCode(), QStringLiteral("recognition-not-matched"));
    QCOMPARE(process.starts.size(), startsBeforeTest + 1);
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::AuthTest);
}

void ProfileModelTest::cameraBusyCanRetryAndCancellationIsForwarded()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, true);

    model.addAppearanceScan(QStringLiteral("profile-example-001"));
    model.cancel();
    QVERIFY(process.cancelCalled);

    process.send(IrlumeProcess::Operation::AddScan, QStringLiteral("failed"), {}, QStringLiteral("camera-busy"), true);
    QVERIFY(model.canRetry());
    const int startsBeforeRetry = process.starts.size();
    model.retry();
    QCOMPARE(process.starts.size(), startsBeforeRetry + 1);
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::AddScan);
    QCOMPARE(process.starts.constLast().profileId, QStringLiteral("profile-example-001"));
}

void ProfileModelTest::appearanceScanRequiresSelectedProfileResult()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, true);

    model.addAppearanceScan(QStringLiteral("profile-example-001"));
    process.send(IrlumeProcess::Operation::AddScan, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profile_id"), QStringLiteral("profile-example-001")},
                     {QStringLiteral("added_scans"), 1},
                     {QStringLiteral("mutated_other_profiles"), false},
                 });

    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::Capabilities);
    process.send(IrlumeProcess::Operation::Capabilities, QStringLiteral("completed"), capabilities());
    process.send(IrlumeProcess::Operation::ListProfiles, QStringLiteral("completed"), profiles(true));
    QCOMPARE(model.statusText(), QStringLiteral("Appearance scan added."));
}

void ProfileModelTest::deletionIsLimitedToKnownProfile()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, true);
    const int startsBeforeDelete = process.starts.size();

    model.deleteProfile(QStringLiteral("../../other-user"));
    QCOMPARE(process.starts.size(), startsBeforeDelete);

    model.deleteProfile(QStringLiteral("profile-example-001"));
    QCOMPARE(process.starts.size(), startsBeforeDelete + 1);
    QCOMPARE(process.starts.constLast().operation, IrlumeProcess::Operation::DeleteProfile);
    QCOMPARE(process.starts.constLast().profileId, QStringLiteral("profile-example-001"));
}

void ProfileModelTest::unsafeDeletionResultFailsClosed()
{
    FakeIrlumeProcess process;
    ProfileModel model(&process, nullptr);
    makeReady(model, process, true);

    model.deleteProfile(QStringLiteral("profile-example-001"));
    process.send(IrlumeProcess::Operation::DeleteProfile, QStringLiteral("completed"),
                 {
                     {QStringLiteral("profile_id"), QStringLiteral("profile-other")},
                     {QStringLiteral("deleted"), true},
                     {QStringLiteral("mutated_other_profiles"), true},
                 });

    QVERIFY(!model.contractAvailable());
    QVERIFY(!model.busy());
    QCOMPARE(model.errorCode(), QStringLiteral("unsafe-profile-mutation-result"));
    QCOMPARE(model.rowCount(), 1);
}

QTEST_MAIN(ProfileModelTest)

#include "test_profilemodel.moc"

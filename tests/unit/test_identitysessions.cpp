// SPDX-License-Identifier: BSD-2-Clause

#include "camerapreviewsession.h"
#include "enrollmentsession.h"
#include "identityworkerclient.h"
#include "kwalletkeyprovider.h"
#include "localverificationsession.h"

#include <QProcessEnvironment>
#include <QTest>

#ifndef KFACEAUTH_FAKE_IDENTITY_WORKER_PATH
#error "KFACEAUTH_FAKE_IDENTITY_WORKER_PATH must be defined"
#endif
#ifndef KFACEAUTH_FAKE_PREVIEW_WORKER_PATH
#error "KFACEAUTH_FAKE_PREVIEW_WORKER_PATH must be defined"
#endif

namespace
{
QProcessEnvironment environmentFor(const QString &mode)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("KFACEAUTH_TEST_MODE"), mode);
    return environment;
}

void startPreview(CameraPreviewSession *preview)
{
    preview->refreshDevices();
    QTRY_COMPARE(preview->state(), CameraPreviewSession::State::Ready);
    preview->startPreview();
    QTRY_COMPARE(preview->state(), CameraPreviewSession::State::Streaming);
    QTRY_VERIFY(preview->frameAvailable());
}

class FakeKeyProvider final : public KWalletKeyProvider
{
  public:
    State state = State::Available;
    QByteArray key = QByteArray(32, char(0x41));
    bool deferRead = false;
    int storeCalls = 0;
    int deleteCalls = 0;
    int cancelCalls = 0;

    State boundedState() const override
    {
        return state;
    }

    void requestKey(Completion completion) override
    {
        if (deferRead)
        {
            pending = std::move(completion);
            return;
        }
        complete(std::move(completion), state);
    }

    Result generateTransientKey() const override
    {
        return Result{State::Available, QByteArray(32, char(0x52))};
    }

    void storeKey(QByteArray candidate, Completion completion) override
    {
        ++storeCalls;
        candidate.fill(0);
        complete(std::move(completion), state == State::Unavailable ? State::Unavailable : State::Available);
    }

    void deleteKey(Completion completion) override
    {
        ++deleteCalls;
        complete(std::move(completion), State::Absent);
    }

    void cancel() override
    {
        ++cancelCalls;
        if (pending)
        {
            const auto completion = std::move(pending);
            pending = {};
            complete(completion, State::Cancelled);
        }
    }

  private:
    void complete(Completion completion, State resultState) const
    {
        if (!completion)
            return;
        completion(Result{resultState, resultState == State::Available ? key : QByteArray()});
    }

    Completion pending;
};
} // namespace

class IdentitySessionsTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void unavailableWalletStatesFailClosed_data();
    void unavailableWalletStatesFailClosed();
    void verificationRateLimitAndLifecycleClearMatch();
    void verificationRejectsStaleWorkerResponse();
    void enrollmentCancellationClearsTransientSamples();
    void pageHideCancelsActiveEnrollmentWorker();
};

void IdentitySessionsTest::unavailableWalletStatesFailClosed_data()
{
    QTest::addColumn<int>("keyState");
    QTest::addColumn<int>("expectedResult");
    QTest::newRow("locked") << int(KWalletKeyProvider::State::Locked)
                            << int(LocalVerificationSession::Result::VaultLocked);
    QTest::newRow("cancelled") << int(KWalletKeyProvider::State::Cancelled)
                               << int(LocalVerificationSession::Result::VaultLocked);
    QTest::newRow("unavailable") << int(KWalletKeyProvider::State::Unavailable)
                                 << int(LocalVerificationSession::Result::Unavailable);
    QTest::newRow("absent") << int(KWalletKeyProvider::State::Absent)
                            << int(LocalVerificationSession::Result::NoProfile);
}

void IdentitySessionsTest::unavailableWalletStatesFailClosed()
{
    QFETCH(int, keyState);
    QFETCH(int, expectedResult);
    CameraPreviewSession preview(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    IdentityWorkerClient worker(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("session")), this);
    FakeKeyProvider keys;
    keys.state = static_cast<KWalletKeyProvider::State>(keyState);
    LocalVerificationSession verification(&preview, &worker, &keys);
    startPreview(&preview);
    verification.setPageActive(true);

    verification.verifyCurrentFrame();

    QTRY_COMPARE(int(verification.result()), expectedResult);
    QVERIFY(!worker.busy());
    QCOMPARE(keys.storeCalls, 0);
    QCOMPARE(keys.deleteCalls, 0);
}

void IdentitySessionsTest::verificationRateLimitAndLifecycleClearMatch()
{
    CameraPreviewSession preview(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    IdentityWorkerClient worker(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("session")), this);
    FakeKeyProvider keys;
    LocalVerificationSession verification(&preview, &worker, &keys);
    startPreview(&preview);
    verification.setPageActive(true);

    verification.verifyCurrentFrame();
    QTRY_COMPARE(verification.result(), LocalVerificationSession::Result::Match);
    verification.verifyCurrentFrame();
    QCOMPARE(verification.state(), LocalVerificationSession::State::RateLimited);

    preview.stopPreview();
    QTRY_COMPARE(verification.result(), LocalVerificationSession::Result::None);
    QCOMPARE(verification.state(), LocalVerificationSession::State::Idle);
}

void IdentitySessionsTest::verificationRejectsStaleWorkerResponse()
{
    CameraPreviewSession preview(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    IdentityWorkerClient worker(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("stale")), this);
    FakeKeyProvider keys;
    LocalVerificationSession verification(&preview, &worker, &keys);
    startPreview(&preview);
    verification.setPageActive(true);

    verification.verifyCurrentFrame();

    QTRY_COMPARE(verification.result(), LocalVerificationSession::Result::InternalFailure);
    QCOMPARE(verification.errorCode(), QStringLiteral("identity-protocol-error"));
}

void IdentitySessionsTest::enrollmentCancellationClearsTransientSamples()
{
    CameraPreviewSession preview(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    IdentityWorkerClient worker(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("session")), this);
    FakeKeyProvider keys;
    keys.state = KWalletKeyProvider::State::Absent;
    EnrollmentSession enrollment(&preview, &worker, &keys);
    startPreview(&preview);
    enrollment.setPageActive(true);
    QTRY_COMPARE(enrollment.profileState(), EnrollmentSession::ProfileState::Absent);

    enrollment.startEnrollment();
    QTRY_COMPARE(enrollment.state(), EnrollmentSession::State::Enrolling);
    enrollment.captureSample();
    QTRY_COMPARE(enrollment.sampleCount(), 1);
    enrollment.cancel();

    QCOMPARE(enrollment.state(), EnrollmentSession::State::Cancelled);
    QCOMPARE(enrollment.sampleCount(), 0);
    QCOMPARE(keys.storeCalls, 0);
    QCOMPARE(keys.deleteCalls, 0);
}

void IdentitySessionsTest::pageHideCancelsActiveEnrollmentWorker()
{
    CameraPreviewSession preview(QStringLiteral(KFACEAUTH_FAKE_PREVIEW_WORKER_PATH), nullptr);
    IdentityWorkerClient worker(QStringLiteral(KFACEAUTH_FAKE_IDENTITY_WORKER_PATH),
                                environmentFor(QStringLiteral("session-hang-capture")), this);
    FakeKeyProvider keys;
    EnrollmentSession enrollment(&preview, &worker, &keys);
    startPreview(&preview);
    enrollment.setPageActive(true);
    QTRY_COMPARE(enrollment.profileState(), EnrollmentSession::ProfileState::Absent);
    enrollment.startEnrollment();
    QTRY_COMPARE(enrollment.state(), EnrollmentSession::State::Enrolling);
    enrollment.captureSample();
    QTRY_COMPARE(enrollment.state(), EnrollmentSession::State::Capturing);

    enrollment.setPageActive(false);

    QTRY_COMPARE(enrollment.state(), EnrollmentSession::State::Cancelled);
    QTRY_VERIFY(!worker.busy());
    QCOMPARE(enrollment.sampleCount(), 0);
    QCOMPARE(keys.storeCalls, 0);
}

QTEST_MAIN(IdentitySessionsTest)

#include "test_identitysessions.moc"

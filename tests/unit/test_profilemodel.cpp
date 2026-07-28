// SPDX-License-Identifier: GPL-3.0-or-later

#include "profilemodel.h"

#include <QSignalSpy>
#include <QTest>

class ProfileModelTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void exposesContractOneProfilesReadOnly();
    void mutationAttemptsFailClosed();
    void refreshRequestsOneBackendRefresh();
    void keepsOldRowsOnlyWhileUpdating();
};

EngineSnapshot profileSnapshot()
{
    EngineSnapshot snapshot;
    snapshot.executablePresent = true;
    snapshot.handshake.state = ResultState::Available;
    snapshot.handshake.data = EngineHandshakeSnapshot{1, QStringLiteral("0.7.0")};
    snapshot.capabilities.features = EngineFeature::ProfilesRead;
    snapshot.capabilities.maxProfiles = 3;
    EngineProfile profile;
    profile.displayName = QStringLiteral("Primary");
    profile.scanDisplayNames = {QStringLiteral("Default"), QStringLiteral("Glasses")};
    snapshot.profiles = EngineProfileSnapshot{{profile}, true, true};
    return snapshot;
}

void ProfileModelTest::exposesContractOneProfilesReadOnly()
{
    ProfileModel model;
    model.applySnapshot(profileSnapshot());

    QVERIFY(model.readOnlyAvailable());
    QVERIFY(!model.mutationSupported());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), ProfileModel::ProfileIdRole).toString(), QString());
    QCOMPARE(model.data(model.index(0), ProfileModel::DisplayNameRole).toString(), QStringLiteral("Primary"));
    QCOMPARE(model.data(model.index(0), ProfileModel::ScanCountRole).toInt(), 2);
}

void ProfileModelTest::mutationAttemptsFailClosed()
{
    ProfileModel model;
    model.applySnapshot(profileSnapshot());

    model.enroll();
    QCOMPARE(model.errorCode(), QStringLiteral("capability-unavailable"));
    QVERIFY(!model.canRetry());
    QVERIFY(!model.busy());

    model.deleteProfile(QString());
    QCOMPARE(model.errorCode(), QStringLiteral("capability-unavailable"));
    model.renameScan(QString(), QString(), QStringLiteral("Changed"));
    QCOMPARE(model.errorCode(), QStringLiteral("capability-unavailable"));
}

void ProfileModelTest::refreshRequestsOneBackendRefresh()
{
    ProfileModel model;
    QSignalSpy spy(&model, &ProfileModel::refreshRequested);
    model.refresh();
    QCOMPARE(spy.count(), 1);
}

void ProfileModelTest::keepsOldRowsOnlyWhileUpdating()
{
    ProfileModel model;
    model.applySnapshot(profileSnapshot());
    QCOMPARE(model.rowCount(), 1);

    EngineSnapshot updating = profileSnapshot();
    updating.profiles.state = ResultState::Loading;
    updating.profiles.data.reset();
    model.applySnapshot(updating);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.busy());

    EngineSnapshot failed = profileSnapshot();
    failed.profiles.state = ResultState::Failed;
    failed.profiles.data.reset();
    failed.profiles.error = EngineError{EngineOperation::Profiles, QStringLiteral("profiles-unavailable"), true};
    model.applySnapshot(failed);
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.readOnlyAvailable());
    QVERIFY(model.canRetry());
}

QTEST_MAIN(ProfileModelTest)

#include "test_profilemodel.moc"

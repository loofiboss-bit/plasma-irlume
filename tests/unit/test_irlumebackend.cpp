// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumebackend.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

class IrlumeBackendTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void buildsOnlyContractOneCommands();
    void acceptsFutureEngineVersions();
    void ignoresUnknownAllowedProperties();
    void gatesCommandsOnCapabilities();
    void rejectsInvalidHandshake();
    void preservesStructuredErrors();
    void rejectsInvalidEnvelopesAndProcessFailures();
    void parsesStatusSemantics();
    void parsesDoctorStatesWithoutReadingDetails();
    void parsesLoginStatusConservatively();
};

namespace
{
IrlumeBackend::ProcessResult response(const QJsonObject &object, int exitCode = 0)
{
    return {true, true, false, false, exitCode, QJsonDocument(object).toJson(QJsonDocument::Compact), {}};
}

QJsonObject envelope(const QString &command, const QJsonObject &data, QString version = QStringLiteral("0.7.0"))
{
    return {
        {QStringLiteral("contract_version"), 1}, {QStringLiteral("engine_version"), std::move(version)},
        {QStringLiteral("command"), command},    {QStringLiteral("ok"), true},
        {QStringLiteral("data"), data},
    };
}

QJsonObject versionData(const QJsonArray &capabilities)
{
    return {
        {QStringLiteral("capabilities"), capabilities},
        {QStringLiteral("contract_versions"), QJsonObject{{QStringLiteral("min"), 1}, {QStringLiteral("max"), 1}}},
        {QStringLiteral("limits"), QJsonObject{{QStringLiteral("max_profiles"), 3}}},
    };
}

EngineSnapshot refreshWith(IrlumeBackend::Command command, const QJsonObject &data)
{
    IrlumeBackend backend(
        [command, data](IrlumeBackend::Command requested) -> IrlumeBackend::ProcessResult
        {
            if (requested == IrlumeBackend::Command::Version)
            {
                return response(envelope(QStringLiteral("version"),
                                         versionData(QJsonArray{IrlumeBackend::capabilityName(command)})));
            }
            if (requested != command)
                return {};
            return response(envelope(IrlumeBackend::commandName(command), data));
        });
    return backend.refreshForTest();
}

QJsonObject statusData(const QString &daemon = QStringLiteral("running"),
                       const QString &templates = QStringLiteral("encrypted"), bool enrollmentKnown = true,
                       bool rgb = true, bool ir = true)
{
    QJsonObject enrollment{{QStringLiteral("known"), enrollmentKnown}};
    if (enrollmentKnown)
    {
        enrollment.insert(QStringLiteral("profiles"), 1);
        enrollment.insert(QStringLiteral("scans"), 2);
    }
    return {
        {QStringLiteral("daemon"), daemon},
        {QStringLiteral("auth_method"), QStringLiteral("auto")},
        {QStringLiteral("face_disabled"), false},
        {QStringLiteral("enrollment"), enrollment},
        {QStringLiteral("templates"), templates},
        {QStringLiteral("keyring"), enrollmentKnown ? QJsonObject{{QStringLiteral("known"), true},
                                                                  {QStringLiteral("armed"), true},
                                                                  {QStringLiteral("policy"), QJsonValue::Null}}
                                                    : QJsonObject{{QStringLiteral("known"), false}}},
        {QStringLiteral("recovery"),
         enrollmentKnown ? QJsonObject{{QStringLiteral("known"), true}, {QStringLiteral("passphrase_set"), true}}
                         : QJsonObject{{QStringLiteral("known"), false}}},
        {QStringLiteral("camera"), QJsonObject{{QStringLiteral("rgb"), rgb}, {QStringLiteral("ir"), ir}}},
        {QStringLiteral("fingerprint"), true},
    };
}
} // namespace

void IrlumeBackendTest::buildsOnlyContractOneCommands()
{
    QCOMPARE(IrlumeBackend::arguments(IrlumeBackend::Command::Version),
             QStringList({QStringLiteral("version"), QStringLiteral("--json")}));
    const QList<IrlumeBackend::Command> later = {
        IrlumeBackend::Command::Status,
        IrlumeBackend::Command::Doctor,
        IrlumeBackend::Command::ProfilesList,
        IrlumeBackend::Command::LoginStatus,
    };
    for (const auto command : later)
    {
        const QStringList arguments = IrlumeBackend::arguments(command);
        QVERIFY(arguments.contains(QStringLiteral("--json")));
        QCOMPARE(arguments.mid(arguments.size() - 2), QStringList({QStringLiteral("--contract"), QStringLiteral("1")}));
        QVERIFY(!arguments.contains(QStringLiteral("--user")));
    }
}

void IrlumeBackendTest::acceptsFutureEngineVersions()
{
    for (const QString &version :
         {QStringLiteral("0.7.0"), QStringLiteral("0.8.0"), QStringLiteral("1.0.0"), QStringLiteral("future-build")})
    {
        int calls = 0;
        IrlumeBackend backend(
            [&calls, version](IrlumeBackend::Command command) -> IrlumeBackend::ProcessResult
            {
                ++calls;
                if (command != IrlumeBackend::Command::Version)
                    return {};
                return response(envelope(QStringLiteral("version"), versionData(QJsonArray{}), version));
            });
        const EngineSnapshot snapshot = backend.refreshForTest();
        QVERIFY(snapshot.contractAvailable());
        QCOMPARE(snapshot.engineVersion(), version);
        QCOMPARE(calls, 1);
    }
}

void IrlumeBackendTest::ignoresUnknownAllowedProperties()
{
    QJsonObject data = versionData(QJsonArray{});
    data.insert(QStringLiteral("future_property"), QJsonObject{{QStringLiteral("opaque"), true}});
    QJsonObject document = envelope(QStringLiteral("version"), data);
    document.insert(QStringLiteral("future_envelope_property"), QStringLiteral("ignored"));
    IrlumeBackend backend([document](IrlumeBackend::Command) { return response(document); });

    const EngineSnapshot snapshot = backend.refreshForTest();

    QVERIFY(snapshot.contractAvailable());
    QCOMPARE(snapshot.capabilities.recognizedReadCount(), 0);
    QCOMPARE(snapshot.status.state, ResultState::NotAdvertised);
    QCOMPARE(snapshot.doctor.state, ResultState::NotAdvertised);
    QCOMPARE(snapshot.profiles.state, ResultState::NotAdvertised);
    QCOMPARE(snapshot.loginStatus.state, ResultState::NotAdvertised);
}

void IrlumeBackendTest::gatesCommandsOnCapabilities()
{
    QVector<IrlumeBackend::Command> calls;
    IrlumeBackend backend(
        [&calls](IrlumeBackend::Command command) -> IrlumeBackend::ProcessResult
        {
            calls.push_back(command);
            if (command == IrlumeBackend::Command::Version)
            {
                return response(envelope(
                    QStringLiteral("version"),
                    versionData(QJsonArray{QStringLiteral("status-json"), QStringLiteral("unknown-mutation-json")})));
            }
            if (command != IrlumeBackend::Command::Status)
                return {};
            return response(envelope(QStringLiteral("status"),
                                     {
                                         {QStringLiteral("daemon"), QStringLiteral("unreachable")},
                                         {QStringLiteral("auth_method"), QStringLiteral("auto")},
                                         {QStringLiteral("face_disabled"), false},
                                         {QStringLiteral("enrollment"), QJsonObject{{QStringLiteral("known"), false}}},
                                         {QStringLiteral("templates"), QStringLiteral("unknown")},
                                         {QStringLiteral("keyring"), QJsonObject{{QStringLiteral("known"), false}}},
                                         {QStringLiteral("recovery"), QJsonObject{{QStringLiteral("known"), false}}},
                                         {QStringLiteral("camera"),
                                          QJsonObject{{QStringLiteral("rgb"), true}, {QStringLiteral("ir"), false}}},
                                         {QStringLiteral("fingerprint"), true},
                                     }));
        });
    const EngineSnapshot snapshot = backend.refreshForTest();
    QCOMPARE(calls.size(), 2);
    QVERIFY(snapshot.status.has_value());
    QVERIFY(!snapshot.profiles.has_value());
    QCOMPARE(snapshot.status.state, ResultState::Available);
    QCOMPARE(snapshot.profiles.state, ResultState::NotAdvertised);
    QVERIFY(!snapshot.capabilities.supports(EngineFeature::ProfileMutation));
    QVERIFY(!snapshot.status->enrollmentKnown);
}

void IrlumeBackendTest::rejectsInvalidHandshake()
{
    const QList<QJsonObject> invalidData = {
        QJsonObject{{QStringLiteral("capabilities"), QJsonArray{}},
                    {QStringLiteral("limits"), QJsonObject{{QStringLiteral("max_profiles"), 3}}}},
        QJsonObject{
            {QStringLiteral("contract_versions"), QJsonObject{{QStringLiteral("min"), 1}, {QStringLiteral("max"), 1}}},
            {QStringLiteral("limits"), QJsonObject{{QStringLiteral("max_profiles"), 3}}}},
        versionData(QJsonArray{QStringLiteral("status-json"), QStringLiteral("status-json")}),
        QJsonObject{
            {QStringLiteral("capabilities"), QJsonArray{}},
            {QStringLiteral("contract_versions"), QJsonObject{{QStringLiteral("min"), 2}, {QStringLiteral("max"), 3}}},
            {QStringLiteral("limits"), QJsonObject{{QStringLiteral("max_profiles"), 3}}}},
    };
    for (const QJsonObject &data : invalidData)
    {
        IrlumeBackend backend([data](IrlumeBackend::Command)
                              { return response(envelope(QStringLiteral("version"), data)); });
        const EngineSnapshot snapshot = backend.refreshForTest();
        QVERIFY(!snapshot.contractAvailable());
        QVERIFY(snapshot.handshake.error.has_value());
    }
}

void IrlumeBackendTest::preservesStructuredErrors()
{
    IrlumeBackend backend(
        [](IrlumeBackend::Command command) -> IrlumeBackend::ProcessResult
        {
            if (command == IrlumeBackend::Command::Version)
            {
                return response(
                    envelope(QStringLiteral("version"), versionData(QJsonArray{QStringLiteral("status-json")})));
            }
            return response(
                {
                    {QStringLiteral("contract_version"), 1},
                    {QStringLiteral("engine_version"), QStringLiteral("0.7.0")},
                    {QStringLiteral("command"), QStringLiteral("status")},
                    {QStringLiteral("ok"), false},
                    {QStringLiteral("error"),
                     QJsonObject{{QStringLiteral("code"), QStringLiteral("daemon-unavailable")},
                                 {QStringLiteral("retryable"), true}}},
                },
                1);
        });
    const EngineSnapshot snapshot = backend.refreshForTest();
    QVERIFY(snapshot.status.error.has_value());
    QCOMPARE(snapshot.status.state, ResultState::Failed);
    QCOMPARE(snapshot.status.error->operation, EngineOperation::Status);
    QCOMPARE(snapshot.status.error->code, QStringLiteral("daemon-unavailable"));
    QVERIFY(snapshot.status.error->retryable);
}

void IrlumeBackendTest::rejectsInvalidEnvelopesAndProcessFailures()
{
    const QList<IrlumeBackend::ProcessResult> failures{
        {false, false, false, false, -1, {}, {}},
        {true, false, false, true, -1, {}, {}},
        {true, false, true, false, -1, {}, {}},
        {true, true, false, false, 0, {}, {}},
        {true, true, false, false, 0, QByteArrayLiteral("{not-json"), {}},
        response(envelope(QStringLiteral("status"), versionData(QJsonArray{}))),
        response(QJsonObject{{QStringLiteral("contract_version"), 2},
                             {QStringLiteral("engine_version"), QStringLiteral("0.7.0")},
                             {QStringLiteral("command"), QStringLiteral("version")},
                             {QStringLiteral("ok"), true},
                             {QStringLiteral("data"), versionData(QJsonArray{})}}),
    };

    for (const auto &failure : failures)
    {
        IrlumeBackend backend([failure](IrlumeBackend::Command) { return failure; });
        const EngineSnapshot snapshot = backend.refreshForTest();
        QVERIFY(!snapshot.contractAvailable());
        QVERIFY(snapshot.handshake.error.has_value());
    }
}

void IrlumeBackendTest::parsesStatusSemantics()
{
    const QList<QString> daemons{QStringLiteral("running"), QStringLiteral("unreachable"),
                                 QStringLiteral("access-denied")};
    for (const QString &daemon : daemons)
    {
        const EngineSnapshot snapshot = refreshWith(IrlumeBackend::Command::Status, statusData(daemon));
        QVERIFY(snapshot.status.has_value());
        QVERIFY(!snapshot.status.error.has_value());
    }

    for (const QString &templates :
         {QStringLiteral("encrypted"), QStringLiteral("plaintext"), QStringLiteral("unknown")})
    {
        const EngineSnapshot snapshot =
            refreshWith(IrlumeBackend::Command::Status, statusData(QStringLiteral("running"), templates));
        QVERIFY(snapshot.status.has_value());
    }

    const EngineSnapshot unknownEnrollment =
        refreshWith(IrlumeBackend::Command::Status,
                    statusData(QStringLiteral("unreachable"), QStringLiteral("unknown"), false, false, false));
    QVERIFY(unknownEnrollment.status.has_value());
    QVERIFY(!unknownEnrollment.status->enrollmentKnown);
    QVERIFY(!unknownEnrollment.status->profileCount.has_value());
    QVERIFY(!unknownEnrollment.status->scanCount.has_value());
    QVERIFY(!unknownEnrollment.status->rgbCamera);
    QVERIFY(!unknownEnrollment.status->irCamera);
    QVERIFY(unknownEnrollment.status->fingerprintPresent);

    QJsonObject contradictory = statusData();
    QJsonObject enrollment{{QStringLiteral("known"), false}, {QStringLiteral("profiles"), 0}};
    contradictory.insert(QStringLiteral("enrollment"), enrollment);
    const EngineSnapshot rejected = refreshWith(IrlumeBackend::Command::Status, contradictory);
    QVERIFY(!rejected.status.has_value());
    QCOMPARE(rejected.status.error->code, QStringLiteral("invalid-status-data"));
}

void IrlumeBackendTest::parsesDoctorStatesWithoutReadingDetails()
{
    QJsonArray checks;
    const QList<QString> states{QStringLiteral("pass"), QStringLiteral("warn"), QStringLiteral("fail"),
                                QStringLiteral("unknown"), QStringLiteral("info")};
    for (qsizetype index = 0; index < states.size(); ++index)
    {
        QJsonObject check{{QStringLiteral("id"), QStringLiteral("future-check-%1").arg(index)},
                          {QStringLiteral("state"), states.at(index)}};
        if (index == 0)
            check.insert(QStringLiteral("detail"), QStringLiteral("arbitrary localized prose"));
        checks.push_back(check);
    }
    const EngineSnapshot snapshot =
        refreshWith(IrlumeBackend::Command::Doctor,
                    QJsonObject{{QStringLiteral("checks"), checks}, {QStringLiteral("future-property"), true}});
    QVERIFY(snapshot.doctor.has_value());
    QCOMPARE(snapshot.doctor->size(), states.size());
    QVERIFY(!snapshot.doctor.error.has_value());
}

void IrlumeBackendTest::parsesLoginStatusConservatively()
{
    const QJsonObject data{
        {QStringLiteral("login_manager"),
         QJsonObject{{QStringLiteral("known"), true},
                     {QStringLiteral("name"), QStringLiteral("plasmalogin")},
                     {QStringLiteral("recognized"), true},
                     {QStringLiteral("services"), QJsonArray{QStringLiteral("plasmalogin")}}}},
        {QStringLiteral("surfaces"),
         QJsonArray{
             QJsonObject{{QStringLiteral("id"), QStringLiteral("plasmalogin")},
                         {QStringLiteral("role"), QStringLiteral("login-screen")},
                         {QStringLiteral("present"), true},
                         {QStringLiteral("wired"), true},
                         {QStringLiteral("mode"), QStringLiteral("required")}},
             QJsonObject{{QStringLiteral("id"), QStringLiteral("future-surface")},
                         {QStringLiteral("role"), QStringLiteral("future-role")},
                         {QStringLiteral("present"), false},
                         {QStringLiteral("wired"), false}},
         }},
        {QStringLiteral("selinux_module"), QStringLiteral("unknown")},
    };
    const EngineSnapshot snapshot = refreshWith(IrlumeBackend::Command::LoginStatus, data);
    QVERIFY(snapshot.loginStatus.has_value());
    QVERIFY(snapshot.loginStatus->loginManagerKnown);
    QVERIFY(snapshot.loginStatus->loginManagerRecognized);
    QCOMPARE(snapshot.loginStatus->surfaces.size(), 2);
    QCOMPARE(snapshot.loginStatus->selinuxModule, EngineLoginSnapshot::SelinuxModule::Unknown);

    QJsonObject contradictory = data;
    QJsonArray surfaces = contradictory.value(QStringLiteral("surfaces")).toArray();
    QJsonObject absent = surfaces.at(1).toObject();
    absent.insert(QStringLiteral("wired"), true);
    absent.insert(QStringLiteral("mode"), QStringLiteral("required"));
    surfaces[1] = absent;
    contradictory.insert(QStringLiteral("surfaces"), surfaces);
    const EngineSnapshot rejected = refreshWith(IrlumeBackend::Command::LoginStatus, contradictory);
    QVERIFY(!rejected.loginStatus.has_value());
}

QTEST_MAIN(IrlumeBackendTest)

#include "test_irlumebackend.moc"

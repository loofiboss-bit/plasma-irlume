// SPDX-License-Identifier: GPL-3.0-or-later

#include "cameraconfiguration.h"

#include <QJsonArray>
#include <QSignalSpy>
#include <QTest>

class FakeCameraProcess final : public IrlumeProcess
{
  public:
    using IrlumeProcess::IrlumeProcess;

    bool startOperation(Operation operation, const QString &, const QString &, const QString &) override
    {
        calls.push_back(operation);
        return startSucceeds;
    }

    QList<Operation> calls;
    bool startSucceeds = true;
};

class FakeCameraRunner final : public AuthActionRunner
{
  public:
    using AuthActionRunner::AuthActionRunner;

    bool start(AuthAction action, const QVariantMap &arguments) override
    {
        calls.push_back({action, arguments});
        return startSucceeds;
    }

    struct Call
    {
        AuthAction action;
        QVariantMap arguments;
    };

    QList<Call> calls;
    bool startSucceeds = true;
};

class CameraConfigurationTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void reviewedReadSequenceBecomesReady();
    void selectionUsesOnlyTheOpaqueListedId();
    void malformedCameraListFailsClosed();
};

namespace
{
IrlumeProcess::Event cameraEvent(IrlumeProcess::Operation operation, QJsonObject data)
{
    IrlumeProcess::Event result;
    result.operation = operation;
    result.type = QStringLiteral("completed");
    result.terminal = true;
    result.data = std::move(data);
    return result;
}

QJsonObject capabilities()
{
    return {
        {QStringLiteral("capabilities"), QJsonArray{QStringLiteral("camera-config-json")}},
    };
}

QJsonObject cameras(const QString &id = QStringLiteral("camera-pair-example-001"),
                    const QString &label = QStringLiteral("Built-in secure camera 1"))
{
    return {
        {QStringLiteral("pairs"), QJsonArray{QJsonObject{
                                      {QStringLiteral("pair_id"), id},
                                      {QStringLiteral("display_name"), label},
                                      {QStringLiteral("built_in"), true},
                                      {QStringLiteral("active"), true},
                                      {QStringLiteral("security_tier"), QStringLiteral("secure")},
                                  }}},
        {QStringLiteral("active_known"), true},
        {QStringLiteral("selection_requires_authorization"), true},
    };
}

QJsonObject emitter()
{
    return {
        {QStringLiteral("available"), true},
        {QStringLiteral("control_count"), 2},
        {QStringLiteral("mutated"), false},
    };
}

void completeRefresh(FakeCameraProcess &process)
{
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::Capabilities, capabilities()));
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::ListCameras, cameras()));
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::TestEmitter, emitter()));
}
} // namespace

void CameraConfigurationTest::reviewedReadSequenceBecomesReady()
{
    FakeCameraProcess process;
    FakeCameraRunner runner;
    CameraConfiguration configuration(&process, &runner);

    configuration.refresh();
    QCOMPARE(process.calls, QList<IrlumeProcess::Operation>{IrlumeProcess::Operation::Capabilities});
    completeRefresh(process);

    QCOMPARE(process.calls, QList<IrlumeProcess::Operation>({IrlumeProcess::Operation::Capabilities,
                                                             IrlumeProcess::Operation::ListCameras,
                                                             IrlumeProcess::Operation::TestEmitter}));
    QVERIFY(configuration.contractAvailable());
    QVERIFY(configuration.ready());
    QVERIFY(configuration.emitterTested());
    QVERIFY(configuration.emitterAvailable());
    QCOMPARE(configuration.emitterControlCount(), 2);
    QCOMPARE(configuration.activePairIndex(), 0);
}

void CameraConfigurationTest::selectionUsesOnlyTheOpaqueListedId()
{
    FakeCameraProcess process;
    FakeCameraRunner runner;
    CameraConfiguration configuration(&process, &runner);

    configuration.refresh();
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::Capabilities, capabilities()));
    QJsonObject available = cameras();
    QJsonArray pairs = available.value(QStringLiteral("pairs")).toArray();
    QJsonObject pair = pairs.first().toObject();
    pair.insert(QStringLiteral("active"), false);
    pairs.replace(0, pair);
    available.insert(QStringLiteral("pairs"), pairs);
    available.insert(QStringLiteral("active_known"), false);
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::ListCameras, available));
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::TestEmitter, emitter()));

    configuration.selectPair();

    QCOMPARE(runner.calls.size(), 1);
    QCOMPARE(runner.calls.constFirst().action, AuthAction::SelectCamera);
    QCOMPARE(runner.calls.constFirst().arguments.value(QStringLiteral("pairId")).toString(),
             QStringLiteral("camera-pair-example-001"));
}

void CameraConfigurationTest::malformedCameraListFailsClosed()
{
    FakeCameraProcess process;
    FakeCameraRunner runner;
    CameraConfiguration configuration(&process, &runner);

    configuration.refresh();
    Q_EMIT process.eventReceived(cameraEvent(IrlumeProcess::Operation::Capabilities, capabilities()));
    Q_EMIT process.eventReceived(
        cameraEvent(IrlumeProcess::Operation::ListCameras,
                    cameras(QStringLiteral("camera-pair-example-001"), QStringLiteral("/dev/video0"))));

    QVERIFY(!configuration.ready());
    QCOMPARE(configuration.errorCode(), QStringLiteral("invalid-camera-list"));
    QCOMPARE(process.calls.size(), 2);
}

QTEST_GUILESS_MAIN(CameraConfigurationTest)

#include "test_cameraconfiguration.moc"

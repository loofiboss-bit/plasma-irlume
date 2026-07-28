// SPDX-License-Identifier: GPL-3.0-or-later

#include "irlumekcm.h"

#include <KPluginMetaData>

#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

struct BackendState
{
    QList<quint64> requests;
    int cancellations = 0;
};

class PendingBackend final : public FaceAuthBackend
{
    Q_OBJECT

  public:
    explicit PendingBackend(BackendState *state) : m_state(state) {}

    void requestRefresh(quint64 generation) override
    {
        m_state->requests.push_back(generation);
    }

    void cancelRefresh() override
    {
        ++m_state->cancellations;
    }

  private:
    BackendState *m_state;
};

class IrlumeKcmTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void constructionKeepsEventLoopResponsive();
    void destructionCancelsActiveRefresh();
};

void IrlumeKcmTest::constructionKeepsEventLoopResponsive()
{
    BackendState state;
    QElapsedTimer elapsed;
    elapsed.start();
    IrlumeKcm kcm(nullptr, KPluginMetaData{}, std::make_unique<PendingBackend>(&state));

    QVERIFY(elapsed.elapsed() < 500);
    QCOMPARE(state.requests, QList<quint64>{1});
    QVERIFY(kcm.refreshing());

    bool eventDelivered = false;
    QTimer::singleShot(0, &kcm, [&eventDelivered]() { eventDelivered = true; });
    QTRY_VERIFY(eventDelivered);
}

void IrlumeKcmTest::destructionCancelsActiveRefresh()
{
    BackendState state;
    auto kcm = std::make_unique<IrlumeKcm>(nullptr, KPluginMetaData{}, std::make_unique<PendingBackend>(&state));

    kcm.reset();

    QCOMPARE(state.cancellations, 1);
}

QTEST_MAIN(IrlumeKcmTest)

#include "test_irlumekcm.moc"

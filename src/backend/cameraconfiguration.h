// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "authactionrunner.h"
#include "irlumeprocess.h"

#include <QObject>
#include <QString>
#include <QStringList>

class CameraConfiguration final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool contractAvailable READ contractAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool hasPairs READ hasPairs NOTIFY stateChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(QStringList pairLabels READ pairLabels NOTIFY stateChanged)
    Q_PROPERTY(int selectedPairIndex READ selectedPairIndex WRITE setSelectedPairIndex NOTIFY stateChanged)
    Q_PROPERTY(int activePairIndex READ activePairIndex NOTIFY stateChanged)
    Q_PROPERTY(bool emitterTested READ emitterTested NOTIFY stateChanged)
    Q_PROPERTY(bool emitterAvailable READ emitterAvailable NOTIFY stateChanged)
    Q_PROPERTY(int emitterControlCount READ emitterControlCount NOTIFY stateChanged)
    Q_PROPERTY(QString captureMode READ captureMode NOTIFY stateChanged)
    Q_PROPERTY(bool tuneConclusive READ tuneConclusive NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY stateChanged)

  public:
    explicit CameraConfiguration(IrlumeProcess *process, QObject *parent = nullptr);
    CameraConfiguration(IrlumeProcess *process, AuthActionRunner *runner, QObject *parent = nullptr);

    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool contractAvailable() const;
    [[nodiscard]] bool hasPairs() const;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] QStringList pairLabels() const;
    [[nodiscard]] int selectedPairIndex() const;
    void setSelectedPairIndex(int index);
    [[nodiscard]] int activePairIndex() const;
    [[nodiscard]] bool emitterTested() const;
    [[nodiscard]] bool emitterAvailable() const;
    [[nodiscard]] int emitterControlCount() const;
    [[nodiscard]] QString captureMode() const;
    [[nodiscard]] bool tuneConclusive() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString errorCode() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void selectPair();
    Q_INVOKABLE void setupEmitter();
    Q_INVOKABLE void tuneCamera();

  Q_SIGNALS:
    void stateChanged();
    void configurationChanged();

  private:
    enum class ReadPhase
    {
        Idle,
        Capabilities,
        Cameras,
        Emitter,
    };

    void handleEvent(const IrlumeProcess::Event &event);
    void handleProcessError(IrlumeProcess::Operation operation, const QString &errorCode, bool retryable);
    void handleActionCompleted(AuthAction action, bool success, const QVariantMap &data, const QString &errorCode);
    [[nodiscard]] bool parseCameras(const QJsonObject &data);
    void finishError(const QString &errorCode);
    void beginRead(ReadPhase phase, IrlumeProcess::Operation operation);

    IrlumeProcess *m_process = nullptr;
    AuthActionRunner *m_runner = nullptr;
    ReadPhase m_phase = ReadPhase::Idle;
    bool m_busy = false;
    bool m_contractAvailable = false;
    bool m_activeKnown = false;
    bool m_emitterTested = false;
    bool m_emitterAvailable = false;
    int m_emitterControlCount = 0;
    int m_selectedPairIndex = -1;
    int m_activePairIndex = -1;
    QStringList m_pairIds;
    QStringList m_pairLabels;
    QString m_captureMode;
    bool m_tuneConclusive = false;
    QString m_statusText;
    QString m_errorCode;
};

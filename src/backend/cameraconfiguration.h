// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QObject>
#include <QString>
#include <QStringList>

class CameraConfiguration final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool contractAvailable READ contractAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool readOnlyAvailable READ readOnlyAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool mutationSupported READ mutationSupported NOTIFY stateChanged)
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
    explicit CameraConfiguration(QObject *parent = nullptr);

    void applySnapshot(const EngineSnapshot &snapshot);
    [[nodiscard]] bool contractAvailable() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool readOnlyAvailable() const;
    [[nodiscard]] bool mutationSupported() const;
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
    void refreshRequested();

  private:
    void failCapability();

    bool m_readOnlyAvailable = false;
    bool m_contractAvailable = false;
    bool m_mutationSupported = false;
    ResultState m_resultState = ResultState::NotAdvertised;
    QString m_statusText;
    QString m_errorCode;
};

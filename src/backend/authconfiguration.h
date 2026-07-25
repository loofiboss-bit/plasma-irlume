// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "authactionrunner.h"

#include <QObject>
#include <QString>
#include <QStringList>

class SystemState;

class AuthConfiguration final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool contractAvailable READ contractAvailable NOTIFY stateChanged)
    Q_PROPERTY(bool canEnableLockScreen READ canEnableLockScreen NOTIFY stateChanged)
    Q_PROPERTY(bool canEnableLoginScreen READ canEnableLoginScreen NOTIFY stateChanged)
    Q_PROPERTY(bool canDisable READ canDisable NOTIFY stateChanged)
    Q_PROPERTY(bool canApplyLockScreen READ canApplyLockScreen NOTIFY stateChanged)
    Q_PROPERTY(bool canApplyLoginScreen READ canApplyLoginScreen NOTIFY stateChanged)
    Q_PROPERTY(bool canApplyDisable READ canApplyDisable NOTIFY stateChanged)
    Q_PROPERTY(bool lockScreenEnabled READ lockScreenEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool loginScreenEnabled READ loginScreenEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool recoveryAcknowledged READ recoveryAcknowledged WRITE setRecoveryAcknowledged NOTIFY stateChanged)
    Q_PROPERTY(bool previewAvailable READ previewAvailable NOTIFY stateChanged)
    Q_PROPERTY(QString previewTitle READ previewTitle NOTIFY stateChanged)
    Q_PROPERTY(QStringList previewChanges READ previewChanges NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY stateChanged)
    Q_PROPERTY(bool rollbackRestored READ rollbackRestored NOTIFY stateChanged)
    Q_PROPERTY(QString recoveryCommand READ recoveryCommand CONSTANT)

  public:
    explicit AuthConfiguration(SystemState *systemState, QObject *parent = nullptr);
    AuthConfiguration(SystemState *systemState, AuthActionRunner *runner, QObject *parent = nullptr);

    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool contractAvailable() const;
    [[nodiscard]] bool canEnableLockScreen() const;
    [[nodiscard]] bool canEnableLoginScreen() const;
    [[nodiscard]] bool canDisable() const;
    [[nodiscard]] bool canApplyLockScreen() const;
    [[nodiscard]] bool canApplyLoginScreen() const;
    [[nodiscard]] bool canApplyDisable() const;
    [[nodiscard]] bool lockScreenEnabled() const;
    [[nodiscard]] bool loginScreenEnabled() const;
    [[nodiscard]] bool recoveryAcknowledged() const;
    void setRecoveryAcknowledged(bool acknowledged);
    [[nodiscard]] bool previewAvailable() const;
    [[nodiscard]] QString previewTitle() const;
    [[nodiscard]] QStringList previewChanges() const;
    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString errorCode() const;
    [[nodiscard]] bool rollbackRestored() const;
    [[nodiscard]] QString recoveryCommand() const;

    Q_INVOKABLE void previewLockScreen();
    Q_INVOKABLE void previewLoginScreen();
    Q_INVOKABLE void previewDisable();
    Q_INVOKABLE void enableLockScreen();
    Q_INVOKABLE void enableLoginScreen();
    Q_INVOKABLE void disable();
    Q_INVOKABLE void disableNow();
    Q_INVOKABLE void rollbackLastTransaction();

  Q_SIGNALS:
    void stateChanged();
    void configurationChanged();

  private Q_SLOTS:
    void handleCompleted(AuthAction action, bool success, const QVariantMap &data, const QString &errorCode);

  private:
    [[nodiscard]] bool basePreflightReady() const;
    void startPreview(const QString &scope, const QString &title);
    void startMutation(AuthAction action);
    void finishLocalError(const QString &errorCode, const QString &message);
    [[nodiscard]] QString messageForError(const QString &errorCode) const;

    SystemState *m_systemState = nullptr;
    AuthActionRunner *m_runner = nullptr;
    bool m_busy = false;
    bool m_contractAvailable = false;
    bool m_lockScreenEnabled = false;
    bool m_loginScreenEnabled = false;
    bool m_recoveryAcknowledged = false;
    bool m_previewAvailable = false;
    bool m_rollbackRestored = false;
    QString m_previewTitle;
    QString m_previewScope;
    QStringList m_previewChanges;
    QString m_statusText;
    QString m_errorCode;
    QString m_lastTransactionId;
};

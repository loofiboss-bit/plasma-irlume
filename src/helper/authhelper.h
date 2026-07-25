// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <KAuth/ActionReply>

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <functional>

class AuthHelper final : public QObject
{
    Q_OBJECT

  public:
    enum class Operation
    {
        Preview,
        EnableLockScreen,
        EnableLoginScreen,
        Disable,
        Verify,
        Rollback,
    };
    Q_ENUM(Operation)

    struct CommandResult
    {
        bool ok = false;
        QJsonObject document;
        QString errorCode;
    };

    using CommandExecutor = std::function<CommandResult(const QStringList &)>;

    explicit AuthHelper(QObject *parent = nullptr);
    AuthHelper(CommandExecutor executor, QByteArray osRelease, QString activeDisplayManager, QObject *parent = nullptr);

    [[nodiscard]] static QStringList planArguments(const QString &scope);
    [[nodiscard]] static QStringList applyArguments(const QString &scope, const QString &planId);
    [[nodiscard]] static QStringList verifyArguments(const QString &transactionId);
    [[nodiscard]] static QStringList rollbackArguments(const QString &transactionId);
    [[nodiscard]] static bool isSafeOpaqueId(const QString &value);

  public Q_SLOTS:
    KAuth::ActionReply preview(const QVariantMap &arguments);
    KAuth::ActionReply enablelockscreen(const QVariantMap &arguments);
    KAuth::ActionReply enableloginscreen(const QVariantMap &arguments);
    KAuth::ActionReply disable(const QVariantMap &arguments);
    KAuth::ActionReply verify(const QVariantMap &arguments);
    KAuth::ActionReply rollback(const QVariantMap &arguments);

  private:
    struct Plan
    {
        QString scope;
        QString planId;
        QString displayManager;
        QString desiredState;
        QStringList changes;
    };

    struct ApplyResult
    {
        bool ok = false;
        QString transactionId;
        QString errorCode;
        bool alreadyRolledBack = false;
    };

    [[nodiscard]] KAuth::ActionReply runPreview(const QString &scope);
    [[nodiscard]] KAuth::ActionReply runTransaction(const QString &scope);
    [[nodiscard]] KAuth::ActionReply runVerify(const QString &transactionId, const QString &desiredState);
    [[nodiscard]] KAuth::ActionReply runRollback(const QString &transactionId);
    [[nodiscard]] CommandResult execute(const QStringList &arguments) const;
    [[nodiscard]] bool checkContract(QString *errorCode) const;
    [[nodiscard]] bool checkFedora(QString *errorCode) const;
    [[nodiscard]] bool parsePlan(const QJsonObject &document, const QString &scope, Plan *plan,
                                 QString *errorCode) const;
    [[nodiscard]] ApplyResult parseApply(const QJsonObject &document, const Plan &plan) const;
    [[nodiscard]] bool parseVerify(const QJsonObject &document, const QString &transactionId,
                                   const QString &desiredState, QString *errorCode) const;
    [[nodiscard]] bool parseRollback(const QJsonObject &document, const QString &transactionId,
                                     QString *errorCode) const;
    [[nodiscard]] static bool validEnvelope(const QJsonObject &document, const QString &command);
    [[nodiscard]] static bool containsSensitiveField(const QJsonObject &document);
    [[nodiscard]] static KAuth::ActionReply successReply(const QVariantMap &data = {});
    [[nodiscard]] static KAuth::ActionReply errorReply(const QString &errorCode, const QVariantMap &data = {});

    CommandExecutor m_executor;
    QByteArray m_osRelease;
    QString m_activeDisplayManager;
};

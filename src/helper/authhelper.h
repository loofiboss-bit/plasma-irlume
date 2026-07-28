// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <KAuth/ActionReply>

#include <QObject>
#include <QString>
#include <QVariantMap>

#include <functional>

class AuthHelper final : public QObject
{
    Q_OBJECT

  public:
    using UnexpectedExecutor = std::function<void()>;

    explicit AuthHelper(QObject *parent = nullptr);
    explicit AuthHelper(UnexpectedExecutor unexpectedExecutor, QObject *parent = nullptr);

    [[nodiscard]] static bool isSafeOpaqueId(const QString &value);

  public Q_SLOTS:
    KAuth::ActionReply preview(const QVariantMap &arguments);
    KAuth::ActionReply enablelockscreen(const QVariantMap &arguments);
    KAuth::ActionReply enableloginscreen(const QVariantMap &arguments);
    KAuth::ActionReply disable(const QVariantMap &arguments);
    KAuth::ActionReply verify(const QVariantMap &arguments);
    KAuth::ActionReply rollback(const QVariantMap &arguments);
    KAuth::ActionReply selectcamera(const QVariantMap &arguments);
    KAuth::ActionReply setupemitter(const QVariantMap &arguments);
    KAuth::ActionReply tunecamera(const QVariantMap &arguments);

  private:
    [[nodiscard]] static KAuth::ActionReply capabilityUnavailable();
    [[nodiscard]] static KAuth::ActionReply invalidArguments();

    UnexpectedExecutor m_unexpectedExecutor;
};

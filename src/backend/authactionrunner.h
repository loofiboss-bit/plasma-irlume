// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariantMap>

enum class AuthAction
{
    Preview,
    EnableLockScreen,
    EnableLoginScreen,
    Disable,
    Verify,
    Rollback,
    SelectCamera,
    SetupEmitter,
    TuneCamera,
};

class AuthActionRunner : public QObject
{
    Q_OBJECT

  public:
    explicit AuthActionRunner(QObject *parent = nullptr);
    ~AuthActionRunner() override;

    virtual bool start(AuthAction action, const QVariantMap &arguments = {}) = 0;

  Q_SIGNALS:
    void completed(AuthAction action, bool success, const QVariantMap &data, const QString &errorCode);
};

class KAuthActionRunner final : public AuthActionRunner
{
    Q_OBJECT

  public:
    explicit KAuthActionRunner(QObject *parent = nullptr);
    bool start(AuthAction action, const QVariantMap &arguments = {}) override;

  private:
    bool m_busy = false;
};

Q_DECLARE_METATYPE(AuthAction)

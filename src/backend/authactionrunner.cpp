// SPDX-License-Identifier: GPL-3.0-or-later

#include "authactionrunner.h"

#include <KAuth/Action>
#include <KAuth/ActionReply>
#include <KAuth/ExecuteJob>
#include <KJob>

namespace
{
constexpr auto HelperId = "io.github.loofibossbit.plasmairlume.helper";

QString actionSuffix(AuthAction action)
{
    switch (action)
    {
    case AuthAction::Preview:
        return QStringLiteral("preview");
    case AuthAction::EnableLockScreen:
        return QStringLiteral("enablelockscreen");
    case AuthAction::EnableLoginScreen:
        return QStringLiteral("enableloginscreen");
    case AuthAction::Disable:
        return QStringLiteral("disable");
    case AuthAction::Verify:
        return QStringLiteral("verify");
    case AuthAction::Rollback:
        return QStringLiteral("rollback");
    }
    return {};
}
} // namespace

AuthActionRunner::AuthActionRunner(QObject *parent) : QObject(parent) {}

AuthActionRunner::~AuthActionRunner() = default;

KAuthActionRunner::KAuthActionRunner(QObject *parent) : AuthActionRunner(parent) {}

bool KAuthActionRunner::start(AuthAction action, const QVariantMap &arguments)
{
    if (m_busy)
    {
        return false;
    }
    const QString suffix = actionSuffix(action);
    if (suffix.isEmpty())
    {
        return false;
    }

    KAuth::Action kauthAction(QString::fromLatin1(HelperId) + QLatin1Char('.') + suffix);
    kauthAction.setHelperId(QString::fromLatin1(HelperId));
    kauthAction.setArguments(arguments);
    KAuth::ExecuteJob *job = kauthAction.execute();
    if (!job)
    {
        return false;
    }

    m_busy = true;
    connect(job, &KJob::result, this,
            [this, action, job]()
            {
                m_busy = false;
                const QVariantMap data = job->data();
                QString errorCode = data.value(QStringLiteral("errorCode")).toString();
                if (job->error() != 0 && errorCode.isEmpty())
                {
                    errorCode = job->error() == KAuth::ActionReply::UserCancelledError
                                    ? QStringLiteral("authorization-cancelled")
                                    : QStringLiteral("authorization-failed");
                }
                Q_EMIT completed(action, job->error() == 0, data, errorCode);
            });
    job->start();
    return true;
}

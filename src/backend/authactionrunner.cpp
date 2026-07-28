// SPDX-License-Identifier: GPL-3.0-or-later

#include "authactionrunner.h"

AuthActionRunner::AuthActionRunner(QObject *parent) : QObject(parent) {}

AuthActionRunner::~AuthActionRunner() = default;

UnavailableAuthActionRunner::UnavailableAuthActionRunner(QObject *parent) : AuthActionRunner(parent) {}

bool UnavailableAuthActionRunner::start(AuthAction action, const QVariantMap &arguments)
{
    Q_UNUSED(arguments)
    Q_EMIT completed(action, false, {}, QStringLiteral("capability-unavailable"));
    return true;
}

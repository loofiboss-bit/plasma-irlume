// SPDX-License-Identifier: GPL-3.0-or-later

#include "authhelper.h"

#include <QRegularExpression>

namespace
{
KAuth::ActionReply errorReply(const QString &errorCode)
{
    KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
    reply.setData({{QStringLiteral("errorCode"), errorCode}, {QStringLiteral("retryable"), false}});
    reply.setErrorDescription(errorCode);
    return reply;
}
} // namespace

AuthHelper::AuthHelper(QObject *parent) : QObject(parent) {}

AuthHelper::AuthHelper(UnexpectedExecutor unexpectedExecutor, QObject *parent)
    : QObject(parent), m_unexpectedExecutor(std::move(unexpectedExecutor))
{
}

bool AuthHelper::isSafeOpaqueId(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral(R"(\A[A-Za-z0-9][A-Za-z0-9._:-]{0,127}\z)"));
    return pattern.match(value).hasMatch();
}

KAuth::ActionReply AuthHelper::capabilityUnavailable()
{
    return errorReply(QStringLiteral("capability-unavailable"));
}

KAuth::ActionReply AuthHelper::invalidArguments()
{
    return errorReply(QStringLiteral("invalid-operation-arguments"));
}

KAuth::ActionReply AuthHelper::preview(const QVariantMap &arguments)
{
    const QString scope = arguments.value(QStringLiteral("scope")).toString();
    if (arguments.size() != 1 || (scope != QLatin1String("lock-screen") && scope != QLatin1String("login-screen") &&
                                  scope != QLatin1String("disable")))
        return invalidArguments();
    return capabilityUnavailable();
}

KAuth::ActionReply AuthHelper::enablelockscreen(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? capabilityUnavailable() : invalidArguments();
}

KAuth::ActionReply AuthHelper::enableloginscreen(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? capabilityUnavailable() : invalidArguments();
}

KAuth::ActionReply AuthHelper::disable(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? capabilityUnavailable() : invalidArguments();
}

KAuth::ActionReply AuthHelper::verify(const QVariantMap &arguments)
{
    const QString transactionId = arguments.value(QStringLiteral("transactionId")).toString();
    const QString desiredState = arguments.value(QStringLiteral("desiredState")).toString();
    if (arguments.size() != 2 || !isSafeOpaqueId(transactionId) ||
        (desiredState != QLatin1String("enabled") && desiredState != QLatin1String("disabled")))
        return invalidArguments();
    return capabilityUnavailable();
}

KAuth::ActionReply AuthHelper::rollback(const QVariantMap &arguments)
{
    if (arguments.size() != 1 || !isSafeOpaqueId(arguments.value(QStringLiteral("transactionId")).toString()))
        return invalidArguments();
    return capabilityUnavailable();
}

KAuth::ActionReply AuthHelper::selectcamera(const QVariantMap &arguments)
{
    if (arguments.size() != 1 || !isSafeOpaqueId(arguments.value(QStringLiteral("pairId")).toString()))
        return invalidArguments();
    return capabilityUnavailable();
}

KAuth::ActionReply AuthHelper::setupemitter(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? capabilityUnavailable() : invalidArguments();
}

KAuth::ActionReply AuthHelper::tunecamera(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? capabilityUnavailable() : invalidArguments();
}

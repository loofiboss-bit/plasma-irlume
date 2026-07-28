// SPDX-License-Identifier: GPL-3.0-or-later

#include "authconfiguration.h"

#include "systemstate.h"

#include <QCoreApplication>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("AuthConfiguration", text);
}
} // namespace

AuthConfiguration::AuthConfiguration(SystemState *systemState, QObject *parent)
    : AuthConfiguration(systemState, new UnavailableAuthActionRunner, parent)
{
    m_runner->setParent(this);
}

AuthConfiguration::AuthConfiguration(SystemState *systemState, AuthActionRunner *runner, QObject *parent)
    : QObject(parent), m_systemState(systemState), m_runner(runner),
      m_statusText(translate("Preview a change before enabling face authentication."))
{
    Q_ASSERT(m_systemState);
    Q_ASSERT(m_runner);
    connect(m_runner, &AuthActionRunner::completed, this, &AuthConfiguration::handleCompleted);
    connect(m_systemState, &SystemState::stateChanged, this,
            [this]()
            {
                m_previewAvailable = false;
                m_previewScope.clear();
                m_previewChanges.clear();
                Q_EMIT stateChanged();
            });
}

bool AuthConfiguration::busy() const
{
    return m_busy || m_resultState == ResultState::Loading || m_resultState == ResultState::Pending;
}

bool AuthConfiguration::contractAvailable() const
{
    return m_contractAvailable;
}

bool AuthConfiguration::mutationSupported() const
{
    return m_mutationSupported;
}

void AuthConfiguration::applySnapshot(const EngineSnapshot &snapshot)
{
    m_contractAvailable = snapshot.contractAvailable();
    m_mutationSupported = snapshot.capabilities.supports(EngineFeature::AuthenticationMutation);
    m_resultState = snapshot.loginStatus.state;
    if (m_resultState == ResultState::Loading || m_resultState == ResultState::Pending)
    {
        m_statusText = translate("Updating read-only authentication wiring…");
        m_errorCode.clear();
        Q_EMIT stateChanged();
        return;
    }

    m_lockScreenEnabled = false;
    m_loginScreenEnabled = false;
    if (snapshot.loginStatus.data)
    {
        for (const EngineLoginSurface &surface : snapshot.loginStatus.data->surfaces)
        {
            if (surface.id == QLatin1String("kde") && surface.role == QLatin1String("lock-screen"))
                m_lockScreenEnabled = surface.present && surface.wired;
            if (snapshot.loginStatus.data->loginManagerServices.contains(surface.id) &&
                surface.role == QLatin1String("login-screen"))
                m_loginScreenEnabled = surface.present && surface.wired;
        }
    }
    m_previewAvailable = false;
    m_previewScope.clear();
    m_errorCode.clear();
    if (m_resultState == ResultState::Failed)
    {
        m_errorCode =
            snapshot.loginStatus.error ? snapshot.loginStatus.error->code : QStringLiteral("login-status-unavailable");
        m_statusText = translate("The read-only authentication wiring status is unavailable.");
    }
    else if (m_resultState == ResultState::NotAdvertised && m_contractAvailable)
        m_statusText = translate("The backend does not advertise read-only authentication wiring.");
    else
        m_statusText =
            translate("Authentication wiring is shown read-only. Contract 1 does not support configuration changes.");
    Q_EMIT stateChanged();
}

bool AuthConfiguration::basePreflightReady() const
{
    return m_mutationSupported && m_systemState->fedoraVersion() == QLatin1String("44") &&
           (m_systemState->activeDisplayManager() == QLatin1String("Plasma Login Manager") ||
            m_systemState->activeDisplayManager() == QLatin1String("SDDM")) &&
           m_systemState->engineStatus() == SystemState::EngineStatus::Ready &&
           m_systemState->daemonStatus() == SystemState::DaemonStatus::Running &&
           m_systemState->profileStatus() == SystemState::ProfileStatus::Enrolled &&
           m_systemState->pamStatus() != SystemState::PamStatus::Drift && m_systemState->passwordFallbackPreserved();
}

bool AuthConfiguration::canEnableLockScreen() const
{
    return !m_busy && basePreflightReady() &&
           (m_systemState->securityTier() == SystemState::SecurityTier::Secure ||
            m_systemState->securityTier() == SystemState::SecurityTier::Convenience);
}

bool AuthConfiguration::canEnableLoginScreen() const
{
    return !m_busy && basePreflightReady() && m_systemState->securityTier() == SystemState::SecurityTier::Secure;
}

bool AuthConfiguration::canDisable() const
{
    return m_mutationSupported && !m_busy && m_systemState->fedoraVersion() == QLatin1String("44") &&
           (m_systemState->activeDisplayManager() == QLatin1String("Plasma Login Manager") ||
            m_systemState->activeDisplayManager() == QLatin1String("SDDM")) &&
           m_systemState->engineStatus() == SystemState::EngineStatus::Ready;
}

bool AuthConfiguration::canApplyLockScreen() const
{
    return canEnableLockScreen() && m_recoveryAcknowledged && m_previewAvailable &&
           m_previewScope == QLatin1String("lock-screen");
}

bool AuthConfiguration::canApplyLoginScreen() const
{
    return canEnableLoginScreen() && m_recoveryAcknowledged && m_previewAvailable &&
           m_previewScope == QLatin1String("login-screen");
}

bool AuthConfiguration::canApplyDisable() const
{
    return canDisable() && m_previewAvailable && m_previewScope == QLatin1String("disable");
}

bool AuthConfiguration::lockScreenEnabled() const
{
    return m_lockScreenEnabled;
}

bool AuthConfiguration::loginScreenEnabled() const
{
    return m_loginScreenEnabled;
}

bool AuthConfiguration::recoveryAcknowledged() const
{
    return m_recoveryAcknowledged;
}

void AuthConfiguration::setRecoveryAcknowledged(bool acknowledged)
{
    if (m_recoveryAcknowledged == acknowledged)
    {
        return;
    }
    m_recoveryAcknowledged = acknowledged;
    Q_EMIT stateChanged();
}

bool AuthConfiguration::previewAvailable() const
{
    return m_previewAvailable;
}

QString AuthConfiguration::previewTitle() const
{
    return m_previewTitle;
}

QStringList AuthConfiguration::previewChanges() const
{
    return m_previewChanges;
}

QString AuthConfiguration::statusText() const
{
    return m_statusText;
}

QString AuthConfiguration::errorCode() const
{
    return m_errorCode;
}

bool AuthConfiguration::rollbackRestored() const
{
    return m_rollbackRestored;
}

QString AuthConfiguration::recoveryCommand() const
{
    return QStringLiteral("sudo irlume login disable --apply");
}

void AuthConfiguration::previewLockScreen()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!canEnableLockScreen())
    {
        finishLocalError(QStringLiteral("preflight-failed"), translate("Lock-screen activation is not ready."));
        return;
    }
    startPreview(QStringLiteral("lock-screen"), translate("Lock-screen face unlock"));
}

void AuthConfiguration::previewLoginScreen()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!canEnableLoginScreen())
    {
        finishLocalError(QStringLiteral("secure-tier-required"),
                         translate("Login-screen activation requires the Secure infrared tier."));
        return;
    }
    startPreview(QStringLiteral("login-screen"), translate("Login-screen face authentication"));
}

void AuthConfiguration::previewDisable()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!canDisable())
    {
        finishLocalError(QStringLiteral("preflight-failed"), translate("Face authentication cannot be disabled here."));
        return;
    }
    startPreview(QStringLiteral("disable"), translate("Disable face authentication"));
}

void AuthConfiguration::enableLockScreen()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!m_recoveryAcknowledged)
    {
        finishLocalError(QStringLiteral("recovery-not-acknowledged"),
                         translate("Read and acknowledge the TTY recovery command before enabling."));
        return;
    }
    if (!canApplyLockScreen())
    {
        finishLocalError(QStringLiteral("preview-required"),
                         translate("Preview the lock-screen plan before enabling."));
        return;
    }
    startMutation(AuthAction::EnableLockScreen);
}

void AuthConfiguration::enableLoginScreen()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!m_recoveryAcknowledged)
    {
        finishLocalError(QStringLiteral("recovery-not-acknowledged"),
                         translate("Read and acknowledge the TTY recovery command before enabling."));
        return;
    }
    if (!canApplyLoginScreen())
    {
        finishLocalError(QStringLiteral("preview-required"),
                         translate("Preview the login-screen plan before enabling."));
        return;
    }
    startMutation(AuthAction::EnableLoginScreen);
}

void AuthConfiguration::disable()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!canApplyDisable())
    {
        finishLocalError(QStringLiteral("preview-required"), translate("Preview the disable plan before applying it."));
        return;
    }
    startMutation(AuthAction::Disable);
}

void AuthConfiguration::disableNow()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (!canDisable())
    {
        finishLocalError(QStringLiteral("preflight-failed"),
                         translate("Face authentication cannot be disabled safely from this session."));
        return;
    }
    m_previewAvailable = false;
    m_previewScope.clear();
    startMutation(AuthAction::Disable);
}

void AuthConfiguration::rollbackLastTransaction()
{
    if (!m_mutationSupported)
    {
        finishLocalError(QStringLiteral("capability-unavailable"),
                         translate("Contract 1 does not support authentication configuration changes."));
        return;
    }
    if (m_busy || m_lastTransactionId.isEmpty())
    {
        return;
    }
    m_busy = true;
    m_errorCode.clear();
    m_rollbackRestored = false;
    m_statusText = translate("Restoring the previous authentication configuration…");
    Q_EMIT stateChanged();
    if (!m_runner->start(AuthAction::Rollback, {{QStringLiteral("transactionId"), m_lastTransactionId}}))
    {
        finishLocalError(QStringLiteral("operation-start-failed"), translate("Rollback could not be started."));
    }
}

void AuthConfiguration::startPreview(const QString &scope, const QString &title)
{
    m_busy = true;
    m_previewAvailable = false;
    m_previewScope.clear();
    m_previewTitle = title;
    m_previewChanges.clear();
    m_errorCode.clear();
    m_rollbackRestored = false;
    m_statusText = translate("Asking irlume for a non-mutating plan…");
    Q_EMIT stateChanged();
    if (!m_runner->start(AuthAction::Preview, {{QStringLiteral("scope"), scope}}))
    {
        finishLocalError(QStringLiteral("operation-start-failed"), translate("The preview could not be started."));
    }
}

void AuthConfiguration::startMutation(AuthAction action)
{
    m_busy = true;
    m_errorCode.clear();
    m_rollbackRestored = false;
    m_statusText = translate("Applying, verifying, and protecting password fallback…");
    Q_EMIT stateChanged();
    if (!m_runner->start(action))
    {
        finishLocalError(QStringLiteral("operation-start-failed"),
                         translate("The authentication change could not be started."));
    }
}

void AuthConfiguration::handleCompleted(AuthAction action, bool success, const QVariantMap &data,
                                        const QString &errorCode)
{
    m_busy = false;
    m_rollbackRestored = data.value(QStringLiteral("rollbackRestored")).toBool();
    if (!success)
    {
        m_errorCode = errorCode.isEmpty() ? QStringLiteral("authentication-operation-failed") : errorCode;
        m_contractAvailable = m_errorCode != QLatin1String("structured-contract-unavailable");
        m_statusText = messageForError(m_errorCode);
        if (m_rollbackRestored)
        {
            m_statusText += QLatin1Char(' ') + translate("The previous configuration was restored automatically.");
        }
        Q_EMIT stateChanged();
        return;
    }

    m_contractAvailable = true;
    m_errorCode.clear();
    if (action == AuthAction::Preview)
    {
        m_previewAvailable = true;
        m_previewScope = data.value(QStringLiteral("scope")).toString();
        m_previewChanges = data.value(QStringLiteral("changes")).toStringList();
        m_statusText = translate("The plan is read-only. Review it before applying the change.");
    }
    else if (action == AuthAction::Rollback)
    {
        m_lockScreenEnabled = false;
        m_loginScreenEnabled = false;
        m_rollbackRestored = true;
        m_lastTransactionId.clear();
        m_statusText = translate("The previous authentication configuration was restored.");
        Q_EMIT configurationChanged();
    }
    else
    {
        m_lastTransactionId = data.value(QStringLiteral("transactionId")).toString();
        if (action == AuthAction::EnableLockScreen)
        {
            m_lockScreenEnabled = true;
        }
        else if (action == AuthAction::EnableLoginScreen)
        {
            m_loginScreenEnabled = true;
        }
        else if (action == AuthAction::Disable)
        {
            m_lockScreenEnabled = false;
            m_loginScreenEnabled = false;
        }
        m_previewAvailable = false;
        m_previewScope.clear();
        m_statusText = translate("Authentication changed and verified. Password fallback remains available.");
        Q_EMIT configurationChanged();
    }
    Q_EMIT stateChanged();
}

void AuthConfiguration::finishLocalError(const QString &errorCode, const QString &message)
{
    m_busy = false;
    m_errorCode = errorCode;
    m_statusText = message;
    Q_EMIT stateChanged();
}

QString AuthConfiguration::messageForError(const QString &errorCode) const
{
    if (errorCode == QLatin1String("capability-unavailable"))
    {
        return translate("Contract 1 does not support authentication configuration changes.");
    }
    if (errorCode == QLatin1String("structured-contract-unavailable"))
    {
        return translate("The installed irlume release does not provide the reviewed login-transaction contract.");
    }
    if (errorCode == QLatin1String("authorization-cancelled"))
    {
        return translate("Administrator authentication was cancelled. Nothing was changed.");
    }
    if (errorCode == QLatin1String("secure-tier-required"))
    {
        return translate("Login-screen face authentication requires the Secure infrared tier.");
    }
    if (errorCode == QLatin1String("post-apply-verification-failed"))
    {
        return translate("Post-apply verification failed.");
    }
    if (errorCode == QLatin1String("rollback-failed"))
    {
        return translate("Automatic rollback could not be confirmed. Use the TTY recovery command now.");
    }
    return translate("The authentication operation failed without a verified configuration change.");
}

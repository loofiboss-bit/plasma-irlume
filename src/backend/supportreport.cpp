// SPDX-License-Identifier: GPL-3.0-or-later

#include "supportreport.h"

#include "authconfiguration.h"
#include "profilemodel.h"
#include "systemstate.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("SupportReport", text);
}

QString securityTierToken(SystemState::SecurityTier tier)
{
    switch (tier)
    {
    case SystemState::SecurityTier::Secure:
        return QStringLiteral("secure");
    case SystemState::SecurityTier::Convenience:
        return QStringLiteral("convenience");
    case SystemState::SecurityTier::Unsupported:
        return QStringLiteral("unsupported");
    case SystemState::SecurityTier::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}
} // namespace

SupportReport::SupportReport(SystemState *systemState, ProfileModel *profileModel, AuthConfiguration *authConfiguration,
                             QObject *parent)
    : QObject(parent), m_systemState(systemState), m_profileModel(profileModel), m_authConfiguration(authConfiguration)
{
    Q_ASSERT(m_systemState);
    Q_ASSERT(m_profileModel);
    Q_ASSERT(m_authConfiguration);

    connect(m_systemState, &SystemState::stateChanged, this, &SupportReport::rebuild);
    connect(m_profileModel, &ProfileModel::stateChanged, this, &SupportReport::rebuild);
    connect(m_authConfiguration, &AuthConfiguration::stateChanged, this, &SupportReport::rebuild);
    rebuild();
}

QString SupportReport::report() const
{
    return m_report;
}

QString SupportReport::issueCode() const
{
    return m_issueCode;
}

QString SupportReport::issueTitle() const
{
    return m_issueTitle;
}

QString SupportReport::recommendedAction() const
{
    return m_recommendedAction;
}

QString SupportReport::recoveryInstructions() const
{
    return translate("1. Press Ctrl+Alt+F3 to open a text console.\n"
                     "2. Sign in with your existing password.\n"
                     "3. Run: sudo irlume login disable --apply\n"
                     "4. Run: authselect check\n"
                     "5. Run: systemctl status irlumed --no-pager\n"
                     "6. Confirm password authentication before logging out or rebooting.");
}

QString SupportReport::lastExportPath() const
{
    return m_lastExportPath;
}

QString SupportReport::statusText() const
{
    return m_statusText;
}

bool SupportReport::hasIssue() const
{
    return !m_issueCode.isEmpty();
}

void SupportReport::copyReport()
{
    copyText(m_report, translate("The redacted support report was copied."));
}

void SupportReport::copyRecoveryInstructions()
{
    copyText(recoveryInstructions(), translate("The TTY recovery instructions were copied."));
}

bool SupportReport::exportReport()
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (directory.isEmpty())
    {
        directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    return exportToDirectory(directory);
}

bool SupportReport::exportToDirectory(const QString &directory)
{
    if (directory.isEmpty() || !QDir().mkpath(directory))
    {
        m_statusText = translate("The support report could not be exported.");
        Q_EMIT exportChanged();
        return false;
    }

    const QString stamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString path = QDir(directory).filePath(QStringLiteral("plasma-irlume-support-%1.md").arg(stamp));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text) || file.write(m_report.toUtf8()) < 0 || !file.commit())
    {
        m_statusText = translate("The support report could not be exported.");
        Q_EMIT exportChanged();
        return false;
    }

    m_lastExportPath = path;
    m_statusText = translate("The redacted support report was exported to Documents.");
    Q_EMIT exportChanged();
    return true;
}

QString SupportReport::redactedValue(const QString &value)
{
    QString sanitized = value.left(128);
    sanitized.replace(QRegularExpression(QStringLiteral(R"([\r\n\x00-\x1f])")), QStringLiteral(" "));
    sanitized.replace(QRegularExpression(QStringLiteral(R"((?:/home|/dev|/etc|/run/user)/[^\s]*)")),
                      QStringLiteral("[redacted]"));
    sanitized.replace(QRegularExpression(QStringLiteral(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})")),
                      QStringLiteral("[redacted]"));
    sanitized.replace(
        QRegularExpression(QStringLiteral(
            R"((?i)\b(?:password|passwd|token|secret|credential|authorization|embedding|template|frame|image)\b\s*[:=]\s*\S+)")),
        QStringLiteral("[redacted]"));
    sanitized.replace(QRegularExpression(QStringLiteral(R"(\b(?:sk|ghp|glpat)-[A-Za-z0-9_-]{8,}\b)")),
                      QStringLiteral("[redacted]"));
    return sanitized.trimmed().isEmpty() ? QStringLiteral("unknown") : sanitized.trimmed();
}

QString SupportReport::titleForCode(const QString &code)
{
    if (code == QLatin1String("camera-busy"))
        return translate("The camera is in use");
    if (code == QLatin1String("camera-unavailable"))
        return translate("The camera is missing");
    if (code == QLatin1String("ir-emitter-failed") || code == QLatin1String("emitter-unavailable"))
        return translate("The infrared emitter is unavailable");
    if (code == QLatin1String("tpm-unseal-failed"))
        return translate("TPM protection could not be unlocked");
    if (code == QLatin1String("secure-boot-pcr-changed"))
        return translate("The measured boot state changed");
    if (code == QLatin1String("unsupported-contract"))
        return translate("The installed backend does not support Machine API Contract 1");
    if (code == QLatin1String("capability-unavailable"))
        return translate("The requested backend capability is unavailable");
    if (code == QLatin1String("pam-drift"))
        return translate("Authentication configuration has drifted");
    if (code == QLatin1String("display-manager-migration"))
        return translate("A display-manager migration was detected");
    if (code == QLatin1String("kwallet-password-mismatch"))
        return translate("KWallet needs the account password");
    if (code == QLatin1String("rollback-failed"))
        return translate("Automatic recovery could not be confirmed");
    if (code == QLatin1String("daemon-unhealthy"))
        return translate("The irlume service is unavailable");
    if (code == QLatin1String("engine-missing"))
        return translate("irlume is not installed");
    return code.isEmpty() ? QString() : translate("Face Login needs attention");
}

QString SupportReport::actionForCode(const QString &code)
{
    if (code == QLatin1String("camera-busy"))
        return translate("Close applications using the camera, then retry the operation.");
    if (code == QLatin1String("camera-unavailable"))
        return translate("Reconnect or re-enable the camera, reboot into the previous kernel if this followed an "
                         "update, then refresh diagnostics.");
    if (code == QLatin1String("ir-emitter-failed") || code == QLatin1String("emitter-unavailable"))
        return translate(
            "Use password login, check the camera privacy controls and cabling, then run diagnostics again.");
    if (code == QLatin1String("tpm-unseal-failed"))
        return translate("Use password login. Do not reset the TPM; restore the previous boot state or re-arm "
                         "protection through irlume.");
    if (code == QLatin1String("secure-boot-pcr-changed"))
        return translate("Use password login and restore the expected Secure Boot or firmware state before re-arming "
                         "TPM protection.");
    if (code == QLatin1String("unsupported-contract"))
        return translate("Install an irlume release that advertises Machine API Contract 1.");
    if (code == QLatin1String("capability-unavailable"))
        return translate("Keep the unsupported operation disabled. Contract 1 is read-only.");
    if (code == QLatin1String("pam-drift"))
        return translate("Use the documented engine recovery procedure outside this read-only KCM, then refresh.");
    if (code == QLatin1String("display-manager-migration"))
        return translate("Use the engine recovery procedure to remove old wiring, then refresh this read-only view.");
    if (code == QLatin1String("kwallet-password-mismatch"))
        return translate(
            "Unlock KWallet with the account password and re-arm wallet integration after a password change.");
    if (code == QLatin1String("rollback-failed"))
        return translate("Keep the desktop session open and use the TTY recovery instructions now.");
    if (code == QLatin1String("daemon-unhealthy"))
        return translate("Use password login, check systemctl status irlumed, then refresh diagnostics.");
    if (code == QLatin1String("engine-missing"))
        return translate("Install the supported irlume package, then refresh diagnostics.");
    if (code.isEmpty())
        return translate("No known issue is currently reported.");
    return translate(
        "Use password login, avoid changing PAM files manually, and export the redacted report for support.");
}

void SupportReport::rebuild()
{
    m_issueCode = currentIssueCode();
    m_issueTitle = titleForCode(m_issueCode);
    m_recommendedAction = actionForCode(m_issueCode);

    m_report =
        QStringLiteral("# plasma-irlume support report\n\n"
                       "This report contains typed local status only. It excludes names, paths, images, templates, and "
                       "credentials.\n\n"
                       "- Data source: %1\n"
                       "- Fedora: %2\n"
                       "- Plasma: %3\n"
                       "- Display manager: %4\n"
                       "- irlume: %5\n"
                       "- Security tier: %6\n"
                       "- Camera: %7\n"
                       "- Daemon: %8\n"
                       "- PAM state: %9\n"
                       "- TPM: %10\n"
                       "- Template protection: %11\n"
                       "- Secure Boot: %12\n"
                       "- Diagnostic code: %13\n")
            .arg(redactedValue(m_systemState->dataSource()), redactedValue(m_systemState->fedoraVersion()),
                 redactedValue(m_systemState->plasmaVersion()), redactedValue(m_systemState->activeDisplayManager()),
                 redactedValue(m_systemState->engineVersion()), securityTierToken(m_systemState->securityTier()),
                 redactedValue(m_systemState->cameraStatusLabel()), redactedValue(m_systemState->daemonStatusLabel()),
                 redactedValue(m_systemState->pamStatusLabel()), redactedValue(m_systemState->tpmStatusLabel()),
                 redactedValue(m_systemState->templateProtectionStatusLabel()),
                 redactedValue(m_systemState->secureBootStatusLabel()),
                 redactedValue(m_issueCode.isEmpty() ? QStringLiteral("none") : m_issueCode));
    Q_EMIT reportChanged();
}

void SupportReport::copyText(const QString &text, const QString &successMessage)
{
    if (auto *clipboard = QGuiApplication::clipboard())
    {
        clipboard->setText(text, QClipboard::Clipboard);
        m_statusText = successMessage;
    }
    else
    {
        m_statusText = translate("The text could not be copied.");
    }
    Q_EMIT exportChanged();
}

QString SupportReport::currentIssueCode() const
{
    const QString authError = m_authConfiguration->errorCode();
    if (authError == QLatin1String("rollback-failed") || authError == QLatin1String("post-apply-verification-failed"))
        return authError;
    if (!m_systemState->issueCode().isEmpty() && m_systemState->issueCode() != QLatin1String("rgb-convenience-only"))
        return m_systemState->issueCode();
    if (!m_profileModel->errorCode().isEmpty())
        return m_profileModel->errorCode();
    if (!authError.isEmpty())
        return authError;
    return m_systemState->issueCode();
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "supportreport.h"

#include "camerapreviewsession.h"
#include "systemstate.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

#ifndef KFACEAUTH_SUPPORT_PREFIX
#define KFACEAUTH_SUPPORT_PREFIX "kfaceauth-support"
#endif

namespace
{
QString translate(const char *text)
{
    return QCoreApplication::translate("SupportReport", text);
}
} // namespace

SupportReport::SupportReport(SystemState *systemState, CameraPreviewSession *cameraPreviewSession, QObject *parent)
    : QObject(parent), m_systemState(systemState), m_cameraPreviewSession(cameraPreviewSession)
{
    Q_ASSERT(m_systemState);
    connect(m_systemState, &SystemState::stateChanged, this, &SupportReport::rebuild);
    if (m_cameraPreviewSession)
    {
        connect(m_cameraPreviewSession, &CameraPreviewSession::devicesChanged, this, &SupportReport::rebuild);
        connect(m_cameraPreviewSession, &CameraPreviewSession::stateChanged, this, &SupportReport::rebuild);
    }
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
    if (auto *clipboard = QGuiApplication::clipboard())
    {
        clipboard->setText(m_report, QClipboard::Clipboard);
        m_statusText = translate("The redacted support report was copied.");
    }
    else
    {
        m_statusText = translate("The text could not be copied.");
    }
    Q_EMIT exportChanged();
}

bool SupportReport::exportReport()
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (directory.isEmpty())
        directory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
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
    const QString path = QDir(directory).filePath(QStringLiteral(KFACEAUTH_SUPPORT_PREFIX "-%1.md").arg(stamp));
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
            R"((?i)\b(?:password|passwd|token|secret|credential|embedding|template|frame|image)\b\s*[:=]\s*\S+)")),
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
        return translate("The camera is unavailable");
    if (code == QLatin1String("native-engine-unavailable"))
        return translate("The native engine is unavailable");
    if (code == QLatin1String("native-protocol-unavailable"))
        return translate("The native protocol is unavailable");
    return code.isEmpty() ? QString() : translate("KFaceAuth needs attention");
}

QString SupportReport::actionForCode(const QString &code)
{
    if (code == QLatin1String("camera-busy"))
        return translate("Close applications using the camera, then retry the preview.");
    if (code == QLatin1String("camera-unavailable"))
        return translate("Reconnect or re-enable the camera, then refresh camera discovery.");
    if (code == QLatin1String("native-engine-unavailable") || code == QLatin1String("native-protocol-unavailable"))
        return translate("No action is required in Milestone 1. Biometric and PAM operations remain disabled.");
    if (code.isEmpty())
        return translate("No known issue is currently reported.");
    return translate("Keep unsupported operations disabled and export the redacted report for development support.");
}

void SupportReport::rebuild()
{
    m_issueCode = currentIssueCode();
    m_issueTitle = titleForCode(m_issueCode);
    m_recommendedAction = actionForCode(m_issueCode);

    m_report =
        QStringLiteral("# KFaceAuth support report\n\n"
                       "This report contains bounded local status only. It excludes device identifiers, images, "
                       "biometric data, paths, and credentials.\n\n"
                       "- Data source: %1\n"
                       "- Fedora: %2\n"
                       "- Plasma: %3\n"
                       "- Display manager: %4\n"
                       "- Native engine: %5\n"
                       "- Engine version: %6\n"
                       "- Vision: %7\n"
                       "- Enrollment: %8\n"
                       "- Authentication decisions: %9\n"
                       "- PAM configuration: %10\n"
                       "- Template persistence: %11\n"
                       "- Secure Boot: %12\n"
                       "- Diagnostic code: %13\n"
                       "- Native cameras: total=%14 rgb=%15 ir=%16 unknown=%17\n"
                       "- Native preview error: %18\n"
                       "- Native preview dropped frames: %19\n")
            .arg(redactedValue(m_systemState->dataSource()), redactedValue(m_systemState->fedoraVersion()),
                 redactedValue(m_systemState->plasmaVersion()), redactedValue(m_systemState->activeDisplayManager()),
                 redactedValue(m_systemState->engineStatusLabel()), redactedValue(m_systemState->engineVersion()),
                 redactedValue(m_systemState->visionStatusLabel()),
                 redactedValue(m_systemState->enrollmentStatusLabel()),
                 redactedValue(m_systemState->authenticationStatusLabel()),
                 redactedValue(m_systemState->pamStatusLabel()),
                 redactedValue(m_systemState->templatePersistenceStatusLabel()),
                 redactedValue(m_systemState->secureBootStatusLabel()),
                 redactedValue(m_issueCode.isEmpty() ? QStringLiteral("none") : m_issueCode))
            .arg(m_cameraPreviewSession ? m_cameraPreviewSession->deviceCount() : 0)
            .arg(m_cameraPreviewSession ? m_cameraPreviewSession->deviceCountForSpectrum(QStringLiteral("rgb")) : 0)
            .arg(m_cameraPreviewSession ? m_cameraPreviewSession->deviceCountForSpectrum(QStringLiteral("ir")) : 0)
            .arg(m_cameraPreviewSession ? m_cameraPreviewSession->deviceCountForSpectrum(QStringLiteral("unknown")) : 0)
            .arg(m_cameraPreviewSession && !m_cameraPreviewSession->errorCode().isEmpty()
                     ? m_cameraPreviewSession->errorCode()
                     : QStringLiteral("none"))
            .arg(m_cameraPreviewSession ? m_cameraPreviewSession->droppedFrames() : 0);
    Q_EMIT reportChanged();
}

QString SupportReport::currentIssueCode() const
{
    if (m_cameraPreviewSession && !m_cameraPreviewSession->errorCode().isEmpty())
        return m_cameraPreviewSession->errorCode();
    return m_systemState->issueCode();
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "authhelper.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <utility>

namespace
{
constexpr int ProcessTimeoutMs = 120000;
constexpr qsizetype MaximumOutputBytes = 256 * 1024;
constexpr auto IrlumeExecutable = "/usr/bin/irlume";

const QSet<QString> SensitiveFields = {
    QStringLiteral("argument"),    QStringLiteral("arguments"),   QStringLiteral("credential"),
    QStringLiteral("credentials"), QStringLiteral("device_path"), QStringLiteral("embedding"),
    QStringLiteral("embeddings"),  QStringLiteral("executable"),  QStringLiteral("frame"),
    QStringLiteral("frames"),      QStringLiteral("image"),       QStringLiteral("images"),
    QStringLiteral("pam_path"),    QStringLiteral("path"),        QStringLiteral("shell"),
    QStringLiteral("template"),    QStringLiteral("templates"),   QStringLiteral("user"),
    QStringLiteral("username"),
};

QByteArray readOsRelease()
{
    QFile file(QStringLiteral("/etc/os-release"));
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return file.read(64 * 1024);
}

QString osReleaseValue(const QByteArray &contents, const QString &key)
{
    const QString prefix = key + QLatin1Char('=');
    const auto lines = QString::fromUtf8(contents).split(QLatin1Char('\n'));
    for (const QString &line : lines)
    {
        if (!line.startsWith(prefix))
        {
            continue;
        }
        QString value = line.mid(prefix.size()).trimmed();
        if (value.size() >= 2 && value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
        {
            value = value.mid(1, value.size() - 2);
        }
        return value;
    }
    return {};
}

QString activeDisplayManager()
{
    const QString target =
        QFileInfo(QStringLiteral("/etc/systemd/system/display-manager.service")).symLinkTarget().toLower();
    if (target.contains(QStringLiteral("plasmalogin")) || target.contains(QStringLiteral("plasma-login-manager")))
    {
        return QStringLiteral("plasmalogin");
    }
    if (target.contains(QStringLiteral("sddm")))
    {
        return QStringLiteral("sddm");
    }
    return {};
}

bool jsonContainsSensitiveField(const QJsonValue &value)
{
    if (value.isArray())
    {
        const QJsonArray array = value.toArray();
        return std::any_of(array.cbegin(), array.cend(), jsonContainsSensitiveField);
    }
    if (!value.isObject())
    {
        return false;
    }
    const QJsonObject object = value.toObject();
    for (auto iterator = object.constBegin(); iterator != object.constEnd(); ++iterator)
    {
        if (SensitiveFields.contains(iterator.key().toLower()) || jsonContainsSensitiveField(iterator.value()))
        {
            return true;
        }
    }
    return false;
}

bool allChecksPass(const QJsonArray &checks, const QSet<QString> &required)
{
    QSet<QString> passed;
    for (const QJsonValue &value : checks)
    {
        if (!value.isObject())
        {
            return false;
        }
        const QJsonObject check = value.toObject();
        const QString id = check.value(QStringLiteral("id")).toString();
        if (check.value(QStringLiteral("state")).toString() == QLatin1String("pass"))
        {
            passed.insert(id);
        }
    }
    return std::all_of(required.cbegin(), required.cend(),
                       [&passed](const QString &check) { return passed.contains(check); });
}

QString desiredForScope(const QString &scope)
{
    return scope == QLatin1String("disable") ? QStringLiteral("disabled") : QStringLiteral("enabled");
}
} // namespace

AuthHelper::AuthHelper(QObject *parent)
    : AuthHelper(
          [](const QStringList &arguments)
          {
              CommandResult result;
              QProcess process;
              QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
              environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
              environment.insert(QStringLiteral("LANG"), QStringLiteral("C"));
              process.setProcessEnvironment(environment);
              process.setProgram(QString::fromLatin1(IrlumeExecutable));
              process.setArguments(arguments);
              process.setProcessChannelMode(QProcess::SeparateChannels);
              process.start(QIODevice::ReadOnly);
              if (!process.waitForStarted(3000))
              {
                  result.errorCode = QStringLiteral("engine-not-installed");
                  return result;
              }
              if (!process.waitForFinished(ProcessTimeoutMs))
              {
                  process.kill();
                  process.waitForFinished(1000);
                  result.errorCode = QStringLiteral("engine-timeout");
                  return result;
              }
              const QByteArray output = process.readAllStandardOutput();
              if (output.size() > MaximumOutputBytes)
              {
                  result.errorCode = QStringLiteral("engine-output-too-large");
                  return result;
              }
              QJsonParseError parseError;
              const QJsonDocument document = QJsonDocument::fromJson(output, &parseError);
              if (parseError.error != QJsonParseError::NoError || !document.isObject())
              {
                  result.errorCode = QStringLiteral("invalid-engine-response");
                  return result;
              }
              result.document = document.object();
              result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
              if (!result.ok)
              {
                  result.errorCode = result.document.value(QStringLiteral("error"))
                                         .toObject()
                                         .value(QStringLiteral("code"))
                                         .toString(QStringLiteral("engine-process-failed"));
              }
              return result;
          },
          readOsRelease(), activeDisplayManager(), parent)
{
}

AuthHelper::AuthHelper(CommandExecutor executor, QByteArray osRelease, QString activeDisplayManager, QObject *parent)
    : QObject(parent), m_executor(std::move(executor)), m_osRelease(std::move(osRelease)),
      m_activeDisplayManager(std::move(activeDisplayManager))
{
}

QStringList AuthHelper::planArguments(const QString &scope)
{
    if (scope == QLatin1String("disable"))
    {
        return {QStringLiteral("login"), QStringLiteral("disable"), QStringLiteral("--json")};
    }
    if (scope != QLatin1String("lock-screen") && scope != QLatin1String("login-screen"))
    {
        return {};
    }
    return {QStringLiteral("login"), QStringLiteral("enable"), QStringLiteral("--scope"), scope,
            QStringLiteral("--json")};
}

QStringList AuthHelper::applyArguments(const QString &scope, const QString &planId)
{
    QStringList arguments = planArguments(scope);
    if (arguments.isEmpty() || !isSafeOpaqueId(planId))
    {
        return {};
    }
    arguments.removeLast();
    arguments << QStringLiteral("--apply") << QStringLiteral("--plan-id") << planId << QStringLiteral("--json");
    return arguments;
}

QStringList AuthHelper::verifyArguments(const QString &transactionId)
{
    if (!isSafeOpaqueId(transactionId))
    {
        return {};
    }
    return {QStringLiteral("login"), QStringLiteral("verify"), QStringLiteral("--transaction-id"), transactionId,
            QStringLiteral("--json")};
}

QStringList AuthHelper::rollbackArguments(const QString &transactionId)
{
    if (!isSafeOpaqueId(transactionId))
    {
        return {};
    }
    return {QStringLiteral("login"), QStringLiteral("rollback"), QStringLiteral("--transaction-id"),
            transactionId,           QStringLiteral("--apply"),  QStringLiteral("--json")};
}

bool AuthHelper::isSafeOpaqueId(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral(R"(\A[A-Za-z0-9][A-Za-z0-9._:-]{0,127}\z)"));
    return pattern.match(value).hasMatch();
}

KAuth::ActionReply AuthHelper::preview(const QVariantMap &arguments)
{
    const QString scope = arguments.value(QStringLiteral("scope")).toString();
    if ((scope != QLatin1String("lock-screen") && scope != QLatin1String("login-screen") &&
         scope != QLatin1String("disable")) ||
        arguments.size() != 1)
    {
        return errorReply(QStringLiteral("invalid-operation-arguments"));
    }
    return runPreview(scope);
}

KAuth::ActionReply AuthHelper::enablelockscreen(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? runTransaction(QStringLiteral("lock-screen"))
                               : errorReply(QStringLiteral("invalid-operation-arguments"));
}

KAuth::ActionReply AuthHelper::enableloginscreen(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? runTransaction(QStringLiteral("login-screen"))
                               : errorReply(QStringLiteral("invalid-operation-arguments"));
}

KAuth::ActionReply AuthHelper::disable(const QVariantMap &arguments)
{
    return arguments.isEmpty() ? runTransaction(QStringLiteral("disable"))
                               : errorReply(QStringLiteral("invalid-operation-arguments"));
}

KAuth::ActionReply AuthHelper::verify(const QVariantMap &arguments)
{
    const QString transactionId = arguments.value(QStringLiteral("transactionId")).toString();
    const QString desiredState = arguments.value(QStringLiteral("desiredState")).toString();
    if (arguments.size() != 2 || !isSafeOpaqueId(transactionId) ||
        (desiredState != QLatin1String("enabled") && desiredState != QLatin1String("disabled")))
    {
        return errorReply(QStringLiteral("invalid-operation-arguments"));
    }
    return runVerify(transactionId, desiredState);
}

KAuth::ActionReply AuthHelper::rollback(const QVariantMap &arguments)
{
    const QString transactionId = arguments.value(QStringLiteral("transactionId")).toString();
    if (arguments.size() != 1 || !isSafeOpaqueId(transactionId))
    {
        return errorReply(QStringLiteral("invalid-operation-arguments"));
    }
    return runRollback(transactionId);
}

KAuth::ActionReply AuthHelper::runPreview(const QString &scope)
{
    QString errorCode;
    if (!checkFedora(&errorCode) || !checkContract(&errorCode))
    {
        return errorReply(errorCode);
    }
    const CommandResult result = execute(planArguments(scope));
    Plan plan;
    if (!result.ok || !parsePlan(result.document, scope, &plan, &errorCode))
    {
        return errorReply(errorCode.isEmpty() ? result.errorCode : errorCode);
    }
    return successReply({
        {QStringLiteral("scope"), plan.scope},
        {QStringLiteral("displayManager"), plan.displayManager},
        {QStringLiteral("desiredState"), plan.desiredState},
        {QStringLiteral("changes"), plan.changes},
        {QStringLiteral("passwordFallbackPreserved"), true},
    });
}

KAuth::ActionReply AuthHelper::runTransaction(const QString &scope)
{
    QString errorCode;
    if (!checkFedora(&errorCode) || !checkContract(&errorCode))
    {
        return errorReply(errorCode);
    }

    const CommandResult planResult = execute(planArguments(scope));
    Plan plan;
    if (!planResult.ok || !parsePlan(planResult.document, scope, &plan, &errorCode))
    {
        return errorReply(errorCode.isEmpty() ? planResult.errorCode : errorCode);
    }

    const CommandResult applyResult = execute(applyArguments(scope, plan.planId));
    const ApplyResult applied = parseApply(applyResult.document, plan);
    if (!applyResult.ok || !applied.ok)
    {
        bool restored = applied.alreadyRolledBack;
        if (!restored && isSafeOpaqueId(applied.transactionId))
        {
            QString rollbackError;
            const CommandResult rollbackResult = execute(rollbackArguments(applied.transactionId));
            restored =
                rollbackResult.ok && parseRollback(rollbackResult.document, applied.transactionId, &rollbackError);
        }
        return errorReply(applied.errorCode.isEmpty() ? applyResult.errorCode : applied.errorCode,
                          {{QStringLiteral("rollbackRestored"), restored}});
    }

    const CommandResult verifyResult = execute(verifyArguments(applied.transactionId));
    if (!verifyResult.ok || !parseVerify(verifyResult.document, applied.transactionId, plan.desiredState, &errorCode))
    {
        QString rollbackError;
        const CommandResult rollbackResult = execute(rollbackArguments(applied.transactionId));
        const bool restored =
            rollbackResult.ok && parseRollback(rollbackResult.document, applied.transactionId, &rollbackError);
        return errorReply(QStringLiteral("post-apply-verification-failed"),
                          {
                              {QStringLiteral("transactionId"), applied.transactionId},
                              {QStringLiteral("rollbackRestored"), restored},
                          });
    }

    return successReply({
        {QStringLiteral("scope"), scope},
        {QStringLiteral("displayManager"), plan.displayManager},
        {QStringLiteral("desiredState"), plan.desiredState},
        {QStringLiteral("transactionId"), applied.transactionId},
        {QStringLiteral("verified"), true},
        {QStringLiteral("passwordFallbackPreserved"), true},
    });
}

KAuth::ActionReply AuthHelper::runVerify(const QString &transactionId, const QString &desiredState)
{
    QString errorCode;
    const CommandResult result = execute(verifyArguments(transactionId));
    if (!result.ok || !parseVerify(result.document, transactionId, desiredState, &errorCode))
    {
        return errorReply(errorCode.isEmpty() ? result.errorCode : errorCode);
    }
    return successReply({
        {QStringLiteral("transactionId"), transactionId},
        {QStringLiteral("desiredState"), desiredState},
        {QStringLiteral("verified"), true},
        {QStringLiteral("passwordFallbackPreserved"), true},
    });
}

KAuth::ActionReply AuthHelper::runRollback(const QString &transactionId)
{
    QString errorCode;
    const CommandResult result = execute(rollbackArguments(transactionId));
    if (!result.ok || !parseRollback(result.document, transactionId, &errorCode))
    {
        return errorReply(errorCode.isEmpty() ? result.errorCode : errorCode);
    }
    return successReply({
        {QStringLiteral("transactionId"), transactionId},
        {QStringLiteral("rollbackRestored"), true},
    });
}

AuthHelper::CommandResult AuthHelper::execute(const QStringList &arguments) const
{
    if (arguments.isEmpty())
    {
        return {false, {}, QStringLiteral("invalid-engine-command")};
    }
    return m_executor(arguments);
}

bool AuthHelper::checkContract(QString *errorCode) const
{
    const CommandResult result = execute({QStringLiteral("version"), QStringLiteral("--json")});
    if (!result.ok || !validEnvelope(result.document, QStringLiteral("version")))
    {
        *errorCode = result.errorCode.isEmpty() ? QStringLiteral("structured-contract-unavailable") : result.errorCode;
        return false;
    }
    const QJsonArray capabilities =
        result.document.value(QStringLiteral("data")).toObject().value(QStringLiteral("capabilities")).toArray();
    const bool available = std::any_of(capabilities.cbegin(), capabilities.cend(), [](const QJsonValue &value)
                                       { return value.toString() == QLatin1String("login-transactions"); });
    if (!available)
    {
        *errorCode = QStringLiteral("structured-contract-unavailable");
    }
    return available;
}

bool AuthHelper::checkFedora(QString *errorCode) const
{
    if (osReleaseValue(m_osRelease, QStringLiteral("ID")).toLower() == QLatin1String("fedora") &&
        osReleaseValue(m_osRelease, QStringLiteral("VERSION_ID")) == QLatin1String("44"))
    {
        return true;
    }
    *errorCode = QStringLiteral("platform-unsupported");
    return false;
}

bool AuthHelper::parsePlan(const QJsonObject &document, const QString &scope, Plan *plan, QString *errorCode) const
{
    if (!validEnvelope(document, QStringLiteral("login.plan")) || !document.value(QStringLiteral("ok")).toBool())
    {
        *errorCode = QStringLiteral("invalid-plan");
        return false;
    }
    const QJsonObject data = document.value(QStringLiteral("data")).toObject();
    const QString operation = data.value(QStringLiteral("operation")).toString();
    const QString planId = data.value(QStringLiteral("plan_id")).toString();
    const QJsonObject displayManager = data.value(QStringLiteral("display_manager")).toObject();
    const QString displayManagerId = displayManager.value(QStringLiteral("id")).toString();
    const QString expectedOperation =
        scope == QLatin1String("disable") ? QStringLiteral("disable") : QStringLiteral("enable");
    if (operation != expectedOperation || !isSafeOpaqueId(planId) || data.value(QStringLiteral("apply")).toBool(true) ||
        data.value(QStringLiteral("mutated")).toBool(true) ||
        !displayManager.value(QStringLiteral("supported")).toBool(false) ||
        displayManagerId != m_activeDisplayManager ||
        !data.value(QStringLiteral("password_fallback")).toObject().value(QStringLiteral("preserved")).toBool(false))
    {
        *errorCode = QStringLiteral("unsafe-plan");
        return false;
    }

    if (scope == QLatin1String("login-screen") &&
        data.value(QStringLiteral("security_tier")).toString() != QLatin1String("secure"))
    {
        *errorCode = QStringLiteral("secure-tier-required");
        return false;
    }
    if (scope != QLatin1String("disable"))
    {
        const QJsonArray scopes = data.value(QStringLiteral("requested_scopes")).toArray();
        if (scopes.size() != 1 || scopes.first().toString() != scope)
        {
            *errorCode = QStringLiteral("scope-mismatch");
            return false;
        }
    }

    QSet<QString> requiredPreconditions = {
        QStringLiteral("engine.healthy"),
        QStringLiteral("login.password-fallback"),
    };
    if (scope != QLatin1String("disable"))
    {
        requiredPreconditions.insert(QStringLiteral("profile.enrolled"));
    }
    if (!allChecksPass(data.value(QStringLiteral("preconditions")).toArray(), requiredPreconditions))
    {
        *errorCode = QStringLiteral("preflight-failed");
        return false;
    }

    QStringList changes;
    const QJsonArray changeArray = data.value(QStringLiteral("changes")).toArray();
    if (changeArray.isEmpty() || changeArray.size() > 4)
    {
        *errorCode = QStringLiteral("unsafe-plan-target");
        return false;
    }
    QSet<QString> allowedTargets;
    if (scope == QLatin1String("lock-screen"))
    {
        allowedTargets.insert(QStringLiteral("pam-service:kde"));
    }
    else if (scope == QLatin1String("login-screen"))
    {
        allowedTargets.insert(QStringLiteral("pam-service:") + displayManagerId);
    }
    else
    {
        allowedTargets = {
            QStringLiteral("pam-service:kde"),
            QStringLiteral("pam-service:") + displayManagerId,
        };
    }
    QSet<QString> observedTargets;
    for (const QJsonValue &value : changeArray)
    {
        const QString target = value.toObject().value(QStringLiteral("target")).toString();
        if (!value.isObject() || !allowedTargets.contains(target) || observedTargets.contains(target))
        {
            *errorCode = QStringLiteral("unsafe-plan-target");
            return false;
        }
        observedTargets.insert(target);
        changes.push_back(target);
    }

    *plan = {scope, planId, displayManagerId, desiredForScope(scope), changes};
    return true;
}

AuthHelper::ApplyResult AuthHelper::parseApply(const QJsonObject &document, const Plan &plan) const
{
    ApplyResult result;
    const QJsonObject data = document.value(QStringLiteral("data")).toObject();
    result.transactionId = data.value(QStringLiteral("transaction_id")).toString();
    result.errorCode = document.value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString();
    result.alreadyRolledBack =
        data.value(QStringLiteral("rollback")).toObject().value(QStringLiteral("restored")).toBool(false);

    QSet<QString> appliedTargets;
    const QJsonArray operations = data.value(QStringLiteral("operations")).toArray();
    for (const QJsonValue &value : operations)
    {
        const QJsonObject operation = value.toObject();
        if (!value.isObject() || operation.value(QStringLiteral("result")).toString() != QLatin1String("applied"))
        {
            result.errorCode = QStringLiteral("unsafe-apply-result");
            return result;
        }
        appliedTargets.insert(operation.value(QStringLiteral("target")).toString());
    }
    const QSet<QString> plannedTargets(plan.changes.cbegin(), plan.changes.cend());

    if (!validEnvelope(document, QStringLiteral("login.apply")) || !document.value(QStringLiteral("ok")).toBool() ||
        data.value(QStringLiteral("plan_id")).toString() != plan.planId || !isSafeOpaqueId(result.transactionId) ||
        data.value(QStringLiteral("state")).toString() != QLatin1String("applied") ||
        !data.value(QStringLiteral("mutated")).toBool(false) || appliedTargets != plannedTargets ||
        operations.size() != plannedTargets.size())
    {
        if (result.errorCode.isEmpty())
        {
            result.errorCode = QStringLiteral("apply-failed");
        }
        return result;
    }
    result.ok = true;
    return result;
}

bool AuthHelper::parseVerify(const QJsonObject &document, const QString &transactionId, const QString &desiredState,
                             QString *errorCode) const
{
    const QJsonObject data = document.value(QStringLiteral("data")).toObject();
    const QSet<QString> requiredChecks = {
        QStringLiteral("daemon.reachable"),
        QStringLiteral("pam.targets-match-plan"),
        QStringLiteral("login.password-fallback"),
    };
    if (!validEnvelope(document, QStringLiteral("login.verify")) || !document.value(QStringLiteral("ok")).toBool() ||
        data.value(QStringLiteral("transaction_id")).toString() != transactionId ||
        data.value(QStringLiteral("state")).toString() != QLatin1String("verified") ||
        data.value(QStringLiteral("desired")).toString() != desiredState ||
        data.value(QStringLiteral("actual")).toString() != desiredState ||
        !allChecksPass(data.value(QStringLiteral("checks")).toArray(), requiredChecks))
    {
        *errorCode = QStringLiteral("post-apply-verification-failed");
        return false;
    }
    return true;
}

bool AuthHelper::parseRollback(const QJsonObject &document, const QString &transactionId, QString *errorCode) const
{
    const QJsonObject data = document.value(QStringLiteral("data")).toObject();
    const QSet<QString> allowedTargets = {
        QStringLiteral("pam-service:kde"),
        QStringLiteral("pam-service:") + m_activeDisplayManager,
    };
    const QJsonArray operations = data.value(QStringLiteral("operations")).toArray();
    const bool operationsRestored =
        !operations.isEmpty() &&
        std::all_of(operations.cbegin(), operations.cend(),
                    [&allowedTargets](const QJsonValue &value)
                    {
                        const QJsonObject operation = value.toObject();
                        return value.isObject() &&
                               allowedTargets.contains(operation.value(QStringLiteral("target")).toString()) &&
                               operation.value(QStringLiteral("result")).toString() == QLatin1String("restored");
                    });
    if (!validEnvelope(document, QStringLiteral("login.rollback")) || !document.value(QStringLiteral("ok")).toBool() ||
        data.value(QStringLiteral("transaction_id")).toString() != transactionId ||
        data.value(QStringLiteral("state")).toString() != QLatin1String("rolled-back") ||
        !data.value(QStringLiteral("restored")).toBool(false) || !operationsRestored)
    {
        *errorCode = QStringLiteral("rollback-failed");
        return false;
    }
    return true;
}

bool AuthHelper::validEnvelope(const QJsonObject &document, const QString &command)
{
    static const QRegularExpression versionPattern(QStringLiteral(R"(\A\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?\z)"));
    return !containsSensitiveField(document) && document.value(QStringLiteral("contract_version")).toInt(-1) == 1 &&
           versionPattern.match(document.value(QStringLiteral("engine_version")).toString()).hasMatch() &&
           document.value(QStringLiteral("command")).toString() == command &&
           document.value(QStringLiteral("ok")).isBool() && document.value(QStringLiteral("data")).isObject();
}

bool AuthHelper::containsSensitiveField(const QJsonObject &document)
{
    return jsonContainsSensitiveField(document);
}

KAuth::ActionReply AuthHelper::successReply(const QVariantMap &data)
{
    KAuth::ActionReply reply = KAuth::ActionReply::SuccessReply();
    reply.setData(data);
    return reply;
}

KAuth::ActionReply AuthHelper::errorReply(const QString &errorCode, const QVariantMap &data)
{
    KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
    QVariantMap replyData = data;
    replyData.insert(QStringLiteral("errorCode"),
                     errorCode.isEmpty() ? QStringLiteral("authentication-operation-failed") : errorCode);
    reply.setData(replyData);
    reply.setErrorDescription(replyData.value(QStringLiteral("errorCode")).toString());
    return reply;
}

// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>

class IrlumeBackend final : public FaceAuthBackend
{
  public:
    enum class Command
    {
        Version,
        Status,
        Doctor,
        ProfilesList,
        LoginStatus,
    };

    struct ProcessResult
    {
        bool started = false;
        bool finished = false;
        bool outputTooLarge = false;
        bool timedOut = false;
        int exitCode = -1;
        QByteArray standardOutput;
        QByteArray standardError;
    };

    using Executor = std::function<ProcessResult(Command)>;

    explicit IrlumeBackend(QString executable = QStringLiteral("/usr/bin/irlume"));
    explicit IrlumeBackend(Executor executor);

    [[nodiscard]] EngineSnapshot refresh() override;

    [[nodiscard]] static QString commandName(Command command);
    [[nodiscard]] static QString capabilityName(Command command);
    [[nodiscard]] static QStringList arguments(Command command);

  private:
    struct Envelope
    {
        bool ok = false;
        QString engineVersion;
        QJsonObject data;
        EngineError error;
    };

    [[nodiscard]] ProcessResult execute(Command command) const;
    [[nodiscard]] static std::optional<Envelope> parseEnvelope(const ProcessResult &result, Command command,
                                                               EngineError *parseError);
    [[nodiscard]] static bool parseVersion(const QJsonObject &data, EngineSnapshot *snapshot, EngineError *error);
    [[nodiscard]] static std::optional<EngineStatusSnapshot> parseStatus(const QJsonObject &data);
    [[nodiscard]] static std::optional<QVector<EngineDoctorCheck>> parseDoctor(const QJsonObject &data);
    [[nodiscard]] static std::optional<EngineProfileSnapshot> parseProfiles(const QJsonObject &data, int maxProfiles);
    [[nodiscard]] static std::optional<EngineLoginSnapshot> parseLogin(const QJsonObject &data);
    [[nodiscard]] static bool containsUnexpectedSensitiveField(const QJsonObject &object);
    [[nodiscard]] static EngineError processError(const ProcessResult &result);

    QString m_executable;
    Executor m_executor;
};

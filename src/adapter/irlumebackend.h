// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "faceauthbackend.h"

#include <QByteArray>
#include <QJsonObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <functional>
#include <optional>

class IrlumeBackend final : public FaceAuthBackend
{
    Q_OBJECT

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

    explicit IrlumeBackend(QObject *parent = nullptr);
    explicit IrlumeBackend(QString executable, QObject *parent = nullptr);
    explicit IrlumeBackend(Executor testExecutor, QObject *parent = nullptr);
    ~IrlumeBackend() override;

    void requestRefresh(quint64 generation) override;
    void cancelRefresh() override;

    // Synchronous by design for parser-only unit tests. Production never calls this.
    [[nodiscard]] EngineSnapshot refreshForTest();

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

    void beginRefresh(quint64 generation);
    void startCommand(Command command);
    void drainProcess();
    void finishProcess(const ProcessResult &result);
    void finishCommand(Command command, const ProcessResult &result);
    void startNextCommand();
    void completeRefresh();
    void failHandshake(const EngineError &error);
    void setOperationState(Command command, ResultState state);
    void setOperationError(Command command, const EngineError &error);
    void emitProgress();
    void cleanupProcess();
    void startPendingRefresh();

    [[nodiscard]] ProcessResult executeForTest(Command command) const;
    [[nodiscard]] static std::optional<Envelope> parseEnvelope(const ProcessResult &result, Command command,
                                                               EngineError *parseError);
    [[nodiscard]] static bool parseVersion(const QJsonObject &data, EngineSnapshot *snapshot, EngineError *error);
    [[nodiscard]] static std::optional<EngineStatusSnapshot> parseStatus(const QJsonObject &data);
    [[nodiscard]] static std::optional<QVector<EngineDoctorCheck>> parseDoctor(const QJsonObject &data);
    [[nodiscard]] static std::optional<EngineProfileSnapshot> parseProfiles(const QJsonObject &data, int maxProfiles);
    [[nodiscard]] static std::optional<EngineLoginSnapshot> parseLogin(const QJsonObject &data);
    [[nodiscard]] static EngineError processError(const ProcessResult &result, EngineOperation operation);
    [[nodiscard]] static EngineOperation operationFor(Command command);

    QString m_executable;
    Executor m_testExecutor;
    QProcess *m_process = nullptr;
    QTimer m_timeout;
    EngineSnapshot m_snapshot;
    QList<Command> m_pendingCommands;
    std::optional<quint64> m_pendingGeneration;
    quint64 m_generation = 0;
    Command m_currentCommand = Command::Version;
    QByteArray m_standardOutput;
    QByteArray m_standardError;
    bool m_outputTooLarge = false;
    bool m_timedOut = false;
    bool m_cancelling = false;
    bool m_processHandled = false;
};

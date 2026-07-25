// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <optional>

class IrlumeProcess : public QObject
{
    Q_OBJECT

  public:
    enum class Operation
    {
        Capabilities,
        ListProfiles,
        Enroll,
        AuthTest,
        AddScan,
        DeleteProfile,
    };
    Q_ENUM(Operation)

    struct Event
    {
        Operation operation = Operation::Capabilities;
        QString command;
        QString operationId;
        int sequence = -1;
        QString type;
        bool terminal = false;
        QJsonObject data;
        QString errorCode;
        bool retryable = false;
    };

    struct ParseResult
    {
        bool ok = false;
        Event event;
        QString errorCode;
    };

    explicit IrlumeProcess(QObject *parent = nullptr);
    explicit IrlumeProcess(QString executable, QObject *parent = nullptr);
    ~IrlumeProcess() override;

    virtual bool startOperation(Operation operation, const QString &profileId = {});
    virtual void cancel();

    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] static QString commandName(Operation operation);
    [[nodiscard]] static QStringList argumentsForOperation(Operation operation, const QString &profileId = {});
    [[nodiscard]] static bool isSafeOpaqueId(const QString &value);
    [[nodiscard]] static ParseResult parseStreamEvent(const QJsonObject &object, Operation operation,
                                                      int expectedSequence, const QString &expectedOperationId = {});
    [[nodiscard]] static ParseResult parseDocument(const QJsonObject &object, Operation operation);

  Q_SIGNALS:
    void eventReceived(const IrlumeProcess::Event &event);
    void operationError(IrlumeProcess::Operation operation, const QString &errorCode, bool retryable);

  private Q_SLOTS:
    void readStandardOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void operationTimedOut();
    void forceCancellation();

  private:
    [[nodiscard]] static bool isStreamingOperation(Operation operation);
    [[nodiscard]] static bool containsSensitiveField(const QJsonValue &value);
    void consumeStreamLines(bool finalChunk);
    void fail(const QString &errorCode, bool retryable = false);
    void reset();

    QString m_executable;
    QProcess m_process;
    QTimer m_timeout;
    QTimer m_cancelTimer;
    QByteArray m_standardOutput;
    QByteArray m_standardError;
    qsizetype m_totalOutputBytes = 0;
    Operation m_operation = Operation::Capabilities;
    QString m_operationId;
    int m_nextSequence = 0;
    bool m_running = false;
    bool m_terminalReceived = false;
    bool m_failureEmitted = false;
    QString m_pendingErrorCode;
    bool m_pendingErrorRetryable = false;
    std::optional<Event> m_pendingTerminalEvent;
    bool m_cancelRequested = false;
};

Q_DECLARE_METATYPE(IrlumeProcess::Event)
Q_DECLARE_METATYPE(IrlumeProcess::Operation)

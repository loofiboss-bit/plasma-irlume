// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

#include <functional>

class IdentityWorkerClient final : public QObject
{
    Q_OBJECT

  public:
    using Completion = std::function<void(quint64 generation, QByteArrayView payload, const QString &error)>;

    explicit IdentityWorkerClient(QObject *parent = nullptr);
    IdentityWorkerClient(QString workerPath, QProcessEnvironment environment, QObject *parent);
    ~IdentityWorkerClient() override;

    [[nodiscard]] bool busy() const;
    void execute(quint64 generation, QByteArray request, Completion completion);
    void cancel();

  Q_SIGNALS:
    void busyChanged();

  private:
    void readResponse();
    void processFinished(int exitCode, QProcess::ExitStatus status);
    void fail(const QString &code);
    void beginTermination(const QString &code);
    void finish(QByteArrayView payload, const QString &error);
    void clearSensitive();

    QString m_workerPath;
    QProcessEnvironment m_environment;
    QProcess *m_process = nullptr;
    Completion m_completion;
    QByteArray m_request;
    QByteArray m_response;
    QString m_pendingError;
    quint64 m_generation = 0;
    qsizetype m_stderrBytes = 0;
    bool m_responseComplete = false;
    bool m_cancelling = false;
    QTimer m_startupTimer;
    QTimer m_operationTimer;
    QTimer m_shutdownTimer;
};

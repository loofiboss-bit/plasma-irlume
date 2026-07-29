// SPDX-License-Identifier: GPL-3.0-or-later

#include "identityworkerclient.h"

#include "identityprotocol.h"

#include <QCoreApplication>
#include <QtEndian>

#include <utility>

#ifndef KFACEAUTH_IDENTITY_WORKER_PATH
#define KFACEAUTH_IDENTITY_WORKER_PATH "/usr/libexec/kfaceauth-identity-worker"
#endif

namespace
{
quint32 readU32(QByteArrayView bytes)
{
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(bytes.data()));
}

QProcessEnvironment productionEnvironment()
{
    QProcessEnvironment environment;
    const QProcessEnvironment system = QProcessEnvironment::systemEnvironment();
    for (const QString &name : {QStringLiteral("HOME"), QStringLiteral("XDG_DATA_HOME")})
    {
        if (system.contains(name))
            environment.insert(name, system.value(name));
    }
    return environment;
}
} // namespace

IdentityWorkerClient::IdentityWorkerClient(QObject *parent)
    : IdentityWorkerClient(QStringLiteral(KFACEAUTH_IDENTITY_WORKER_PATH), productionEnvironment(), parent)
{
}

IdentityWorkerClient::IdentityWorkerClient(QString workerPath, QProcessEnvironment environment, QObject *parent)
    : IdentityWorkerClient(std::move(workerPath), std::move(environment), 2000, 10000, 1000, parent)
{
}

IdentityWorkerClient::IdentityWorkerClient(QString workerPath, QProcessEnvironment environment, int startupTimeoutMs,
                                           int operationTimeoutMs, int shutdownTimeoutMs, QObject *parent)
    : QObject(parent), m_workerPath(std::move(workerPath)), m_environment(std::move(environment))
{
    m_startupTimer.setSingleShot(true);
    m_startupTimer.setInterval(startupTimeoutMs);
    m_operationTimer.setSingleShot(true);
    m_operationTimer.setInterval(operationTimeoutMs);
    m_shutdownTimer.setSingleShot(true);
    m_shutdownTimer.setInterval(shutdownTimeoutMs);
    connect(&m_startupTimer, &QTimer::timeout, this, [this]() { fail(QStringLiteral("identity-startup-timeout")); });
    connect(&m_operationTimer, &QTimer::timeout, this,
            [this]() { fail(QStringLiteral("identity-operation-timeout")); });
    connect(&m_shutdownTimer, &QTimer::timeout, this,
            [this]()
            {
                if (m_cancelling)
                    finish({}, m_pendingError);
                else
                    beginTermination(QStringLiteral("identity-shutdown-timeout"));
            });
}

IdentityWorkerClient::~IdentityWorkerClient()
{
    m_startupTimer.stop();
    m_operationTimer.stop();
    m_shutdownTimer.stop();
    m_completion = {};
    clearSensitive();
    if (m_process)
    {
        m_process->disconnect(this);
        m_process->kill();
        m_process->setParent(QCoreApplication::instance());
        connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), m_process, &QObject::deleteLater);
        m_process = nullptr;
    }
}

bool IdentityWorkerClient::busy() const
{
    return m_process != nullptr;
}

void IdentityWorkerClient::execute(quint64 generation, QByteArray request, Completion completion)
{
    if (generation == 0 || request.isEmpty() || m_process)
    {
        request.fill(0);
        if (completion)
            completion(generation, {}, QStringLiteral("identity-worker-busy"));
        return;
    }
    m_generation = generation;
    m_completion = std::move(completion);
    m_request = std::move(request);
    m_cancelling = false;
    m_responseComplete = false;
    m_stderrBytes = 0;
    m_process = new QProcess(this);
    m_process->setProgram(m_workerPath);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    m_process->setProcessEnvironment(m_environment);
    connect(m_process, &QProcess::started, this,
            [this]()
            {
                m_startupTimer.stop();
                if (m_cancelling)
                {
                    m_request.fill(0);
                    m_request.clear();
                    return;
                }
                const qsizetype expected = m_request.size();
                const qint64 accepted = m_process ? m_process->write(m_request) : -1;
                if (m_process)
                    m_process->closeWriteChannel();
                m_request.fill(0);
                m_request.clear();
                if (accepted != expected)
                {
                    fail(QStringLiteral("identity-protocol-error"));
                    return;
                }
                m_operationTimer.start();
            });
    connect(m_process, &QProcess::readyReadStandardOutput, this, &IdentityWorkerClient::readResponse);
    connect(m_process, &QProcess::readyReadStandardError, this,
            [this]()
            {
                if (!m_process)
                    return;
                QByteArray bytes = m_process->readAllStandardError();
                m_stderrBytes += bytes.size();
                bytes.fill(0);
                if (m_stderrBytes > 64 * 1024)
                    fail(QStringLiteral("identity-protocol-error"));
            });
    connect(m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error)
            {
                if (m_cancelling)
                    return;
                if (error == QProcess::FailedToStart)
                    fail(QStringLiteral("identity-startup-failed"));
                else if (error != QProcess::Crashed)
                    fail(QStringLiteral("identity-worker-failed"));
            });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            &IdentityWorkerClient::processFinished);
    m_process->start();
    m_startupTimer.start();
    Q_EMIT busyChanged();
}

void IdentityWorkerClient::cancel()
{
    if (!m_process)
    {
        clearSensitive();
        return;
    }
    beginTermination(QStringLiteral("cancelled"));
}

void IdentityWorkerClient::readResponse()
{
    if (!m_process)
        return;
    QByteArray bytes = m_process->readAllStandardOutput();
    if (m_responseComplete || m_response.size() + bytes.size() > IdentityProtocol::MaximumResponseBytes + 4)
    {
        bytes.fill(0);
        fail(QStringLiteral("identity-protocol-error"));
        return;
    }
    m_response.append(bytes);
    bytes.fill(0);
    if (m_response.size() < 4)
        return;
    const quint32 payloadSize = readU32(m_response);
    if (payloadSize < 12 || payloadSize > IdentityProtocol::MaximumResponseBytes)
    {
        fail(QStringLiteral("identity-protocol-error"));
        return;
    }
    if (m_response.size() < static_cast<qsizetype>(payloadSize) + 4)
        return;
    if (m_response.size() != static_cast<qsizetype>(payloadSize) + 4)
    {
        fail(QStringLiteral("identity-protocol-error"));
        return;
    }
    m_responseComplete = true;
    m_operationTimer.stop();
    m_shutdownTimer.start();
}

void IdentityWorkerClient::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_cancelling)
    {
        finish({}, m_pendingError);
        return;
    }
    if (!m_responseComplete || exitCode != 0 || status != QProcess::NormalExit)
    {
        fail(QStringLiteral("identity-worker-failed"));
        return;
    }
    const quint32 payloadSize = readU32(m_response);
    finish(QByteArrayView(m_response.constData() + 4, payloadSize), {});
}

void IdentityWorkerClient::fail(const QString &code)
{
    beginTermination(code);
}

void IdentityWorkerClient::beginTermination(const QString &code)
{
    if (!m_process)
        return;
    if (!m_cancelling)
    {
        m_cancelling = true;
        m_pendingError = code;
        m_startupTimer.stop();
        m_operationTimer.stop();
        m_request.fill(0);
        m_request.clear();
        m_process->kill();
        m_shutdownTimer.start();
    }
}

void IdentityWorkerClient::finish(QByteArrayView payload, const QString &error)
{
    m_startupTimer.stop();
    m_operationTimer.stop();
    m_shutdownTimer.stop();
    QProcess *process = m_process;
    m_process = nullptr;
    if (process)
    {
        process->disconnect(this);
        process->deleteLater();
    }
    const auto completion = std::move(m_completion);
    m_completion = {};
    const quint64 generation = std::exchange(m_generation, 0);
    if (completion)
        completion(generation, payload, error);
    clearSensitive();
    m_cancelling = false;
    m_pendingError.clear();
    Q_EMIT busyChanged();
}

void IdentityWorkerClient::clearSensitive()
{
    m_request.fill(0);
    m_request.clear();
    m_response.fill(0);
    m_response.clear();
    m_responseComplete = false;
    m_stderrBytes = 0;
}

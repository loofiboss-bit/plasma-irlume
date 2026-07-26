// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "irlumeprocess.h"

#include <QElapsedTimer>
#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QPointF>
#include <QProcess>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVector>

#include <optional>

class EnrollmentSession : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool frameAvailable READ frameAvailable NOTIFY frameChanged)
    Q_PROPERTY(QString spectrum READ spectrum NOTIFY stateChanged)
    Q_PROPERTY(QString guidance READ guidance NOTIFY stateChanged)
    Q_PROPERTY(int quality READ quality NOTIFY stateChanged)
    Q_PROPERTY(bool faceDetected READ faceDetected NOTIFY stateChanged)
    Q_PROPERTY(bool centered READ centered NOTIFY stateChanged)
    Q_PROPERTY(bool facingCamera READ facingCamera NOTIFY stateChanged)
    Q_PROPERTY(bool wellLit READ wellLit NOTIFY stateChanged)
    Q_PROPERTY(bool irReady READ irReady NOTIFY stateChanged)
    Q_PROPERTY(bool wellFramed READ wellFramed NOTIFY stateChanged)
    Q_PROPERTY(int countdown READ countdown NOTIFY stateChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY stateChanged)
    Q_PROPERTY(quint64 frameRevision READ frameRevision NOTIFY frameChanged)
    Q_PROPERTY(QVariantList landmarks READ landmarksVariant NOTIFY frameChanged)
    Q_PROPERTY(QRectF faceBox READ faceBox NOTIFY frameChanged)

  public:
    struct Position
    {
        bool faceDetected = false;
        bool centered = false;
        bool facingCamera = false;
        bool wellLit = false;
        bool irReady = false;
        bool wellFramed = false;
        int quality = 0;
        int countdown = 0;
        QString guidance;
    };

    struct ParseResult
    {
        bool ok = false;
        bool preview = false;
        IrlumeProcess::Event event;
        QString sessionId;
        QString spectrum;
        QImage frame;
        QVector<QPointF> landmarks;
        QRectF faceBox;
        Position position;
        QString errorCode;
    };

    explicit EnrollmentSession(QObject *parent = nullptr);
    explicit EnrollmentSession(QString executable, QObject *parent = nullptr);
    ~EnrollmentSession() override;

    virtual bool startOperation(IrlumeProcess::Operation operation, const QString &profileId = {});
    Q_INVOKABLE virtual void cancel();
    Q_INVOKABLE void clearFrame();

    [[nodiscard]] bool active() const;
    [[nodiscard]] bool frameAvailable() const;
    [[nodiscard]] QString spectrum() const;
    [[nodiscard]] QString guidance() const;
    [[nodiscard]] int quality() const;
    [[nodiscard]] bool faceDetected() const;
    [[nodiscard]] bool centered() const;
    [[nodiscard]] bool facingCamera() const;
    [[nodiscard]] bool wellLit() const;
    [[nodiscard]] bool irReady() const;
    [[nodiscard]] bool wellFramed() const;
    [[nodiscard]] int countdown() const;
    [[nodiscard]] int droppedFrames() const;
    [[nodiscard]] quint64 frameRevision() const;
    [[nodiscard]] QVariantList landmarksVariant() const;
    [[nodiscard]] QVector<QPointF> landmarks() const;
    [[nodiscard]] QRectF faceBox() const;
    [[nodiscard]] QImage frame() const;

    [[nodiscard]] static QStringList argumentsForOperation(IrlumeProcess::Operation operation,
                                                           const QString &profileId = {});
    [[nodiscard]] static ParseResult parseEvent(const QJsonObject &object, IrlumeProcess::Operation operation,
                                                int expectedSequence, const QString &expectedOperationId = {},
                                                const QString &expectedSessionId = {});

  Q_SIGNALS:
    void stateChanged();
    void frameChanged();
    void eventReceived(const IrlumeProcess::Event &event);
    void operationError(IrlumeProcess::Operation operation, const QString &errorCode, bool retryable);

  private Q_SLOTS:
    void readStandardOutput();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error);
    void operationTimedOut();
    void forceCancellation();

  private:
    [[nodiscard]] static bool isPreviewOperation(IrlumeProcess::Operation operation);
    void consumeLines(bool finalChunk);
    void acceptPreview(ParseResult parsed);
    void fail(const QString &errorCode, bool retryable = false);
    void reset();

    QString m_executable;
    QProcess m_process;
    QTimer m_timeout;
    QTimer m_cancelTimer;
    QElapsedTimer m_frameTimer;
    QByteArray m_output;
    QByteArray m_standardError;
    IrlumeProcess::Operation m_operation = IrlumeProcess::Operation::Enroll;
    QString m_operationId;
    QString m_sessionId;
    int m_nextSequence = 0;
    bool m_active = false;
    bool m_terminalReceived = false;
    bool m_failureEmitted = false;
    bool m_cancelRequested = false;
    QString m_pendingErrorCode;
    bool m_pendingErrorRetryable = false;
    std::optional<IrlumeProcess::Event> m_pendingTerminalEvent;

    QImage m_frame;
    QVector<QPointF> m_landmarks;
    QRectF m_faceBox;
    Position m_position;
    QString m_spectrum;
    int m_droppedFrames = 0;
    quint64 m_frameRevision = 0;
};

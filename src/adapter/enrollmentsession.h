// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVariantList>
#include <QVector>

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
    enum class Operation
    {
        Enroll,
        AuthenticationTest,
        AddScan,
    };
    Q_ENUM(Operation)

    explicit EnrollmentSession(QObject *parent = nullptr);

    virtual bool startOperation(Operation operation, const QString &profileId = {});
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

  Q_SIGNALS:
    void stateChanged();
    void frameChanged();
    void operationError(EnrollmentSession::Operation operation, const QString &errorCode, bool retryable);

  private:
    quint64 m_frameRevision = 0;
};

Q_DECLARE_METATYPE(EnrollmentSession::Operation)

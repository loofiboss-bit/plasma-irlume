// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollmentsession.h"

EnrollmentSession::EnrollmentSession(QObject *parent) : QObject(parent) {}

bool EnrollmentSession::startOperation(Operation operation, const QString &)
{
    Q_EMIT operationError(operation, QStringLiteral("capability-unavailable"), false);
    return false;
}

void EnrollmentSession::cancel() {}

void EnrollmentSession::clearFrame()
{
    ++m_frameRevision;
    Q_EMIT frameChanged();
}

bool EnrollmentSession::active() const
{
    return false;
}

bool EnrollmentSession::frameAvailable() const
{
    return false;
}

QString EnrollmentSession::spectrum() const
{
    return {};
}

QString EnrollmentSession::guidance() const
{
    return {};
}

int EnrollmentSession::quality() const
{
    return 0;
}

bool EnrollmentSession::faceDetected() const
{
    return false;
}

bool EnrollmentSession::centered() const
{
    return false;
}

bool EnrollmentSession::facingCamera() const
{
    return false;
}

bool EnrollmentSession::wellLit() const
{
    return false;
}

bool EnrollmentSession::irReady() const
{
    return false;
}

bool EnrollmentSession::wellFramed() const
{
    return false;
}

int EnrollmentSession::countdown() const
{
    return 0;
}

int EnrollmentSession::droppedFrames() const
{
    return 0;
}

quint64 EnrollmentSession::frameRevision() const
{
    return m_frameRevision;
}

QVariantList EnrollmentSession::landmarksVariant() const
{
    return {};
}

QVector<QPointF> EnrollmentSession::landmarks() const
{
    return {};
}

QRectF EnrollmentSession::faceBox() const
{
    return {};
}

QImage EnrollmentSession::frame() const
{
    return {};
}

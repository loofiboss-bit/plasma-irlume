// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollmentpreviewitem.h"

#include <QPainter>
#include <algorithm>

EnrollmentPreviewItem::EnrollmentPreviewItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setOpaquePainting(true);
}

EnrollmentSession *EnrollmentPreviewItem::session() const
{
    return m_session;
}

void EnrollmentPreviewItem::setSession(EnrollmentSession *session)
{
    if (m_session == session)
    {
        return;
    }
    if (m_session)
    {
        disconnect(m_session, nullptr, this, nullptr);
    }
    m_session = session;
    if (m_session)
    {
        connect(m_session, &EnrollmentSession::frameChanged, this, [this]() { update(); });
        connect(m_session, &EnrollmentSession::stateChanged, this, [this]() { update(); });
        connect(m_session, &QObject::destroyed, this,
                [this]()
                {
                    m_session = nullptr;
                    update();
                    Q_EMIT sessionChanged();
                });
    }
    update();
    Q_EMIT sessionChanged();
}

bool EnrollmentPreviewItem::mirrored() const
{
    return m_mirrored;
}

void EnrollmentPreviewItem::setMirrored(bool mirrored)
{
    if (m_mirrored == mirrored)
    {
        return;
    }
    m_mirrored = mirrored;
    update();
    Q_EMIT mirroredChanged();
}

QColor EnrollmentPreviewItem::landmarkColor() const
{
    return m_landmarkColor;
}

void EnrollmentPreviewItem::setLandmarkColor(const QColor &color)
{
    if (m_landmarkColor == color)
    {
        return;
    }
    m_landmarkColor = color;
    update();
    Q_EMIT landmarkColorChanged();
}

QColor EnrollmentPreviewItem::frameColor() const
{
    return m_frameColor;
}

void EnrollmentPreviewItem::setFrameColor(const QColor &color)
{
    if (m_frameColor == color)
    {
        return;
    }
    m_frameColor = color;
    update();
    Q_EMIT frameColorChanged();
}

void EnrollmentPreviewItem::paint(QPainter *painter)
{
    painter->fillRect(boundingRect(), QColor(QStringLiteral("#091016")));
    if (!m_session)
    {
        return;
    }
    const QImage frame = m_session->frame();
    if (frame.isNull())
    {
        return;
    }

    const QSizeF scaled = frame.size().scaled(boundingRect().size().toSize(), Qt::KeepAspectRatio);
    const QRectF target((width() - scaled.width()) / 2.0, (height() - scaled.height()) / 2.0, scaled.width(),
                        scaled.height());
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (m_mirrored)
    {
        painter->save();
        painter->translate(target.center().x() * 2.0, 0.0);
        painter->scale(-1.0, 1.0);
        painter->drawImage(target, frame);
        painter->restore();
    }
    else
    {
        painter->drawImage(target, frame);
    }

    QColor pointColor = m_landmarkColor;
    pointColor.setAlphaF(0.78);
    painter->setPen(Qt::NoPen);
    painter->setBrush(pointColor);
    const qreal radius = std::clamp(target.width() / 360.0, 0.8, 2.2);
    for (const QPointF &point : m_session->landmarks())
    {
        painter->drawEllipse(mappedPoint(point, target), radius, radius);
    }

    QPen boxPen(m_frameColor, std::clamp(target.width() / 220.0, 1.5, 3.0));
    boxPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(boxPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(mappedBox(m_session->faceBox(), target), 10.0, 10.0);
}

QPointF EnrollmentPreviewItem::mappedPoint(const QPointF &point, const QRectF &target) const
{
    const qreal x = m_mirrored ? 1.0 - point.x() : point.x();
    return QPointF(target.left() + x * target.width(), target.top() + point.y() * target.height());
}

QRectF EnrollmentPreviewItem::mappedBox(const QRectF &box, const QRectF &target) const
{
    const qreal x = m_mirrored ? 1.0 - box.x() - box.width() : box.x();
    return QRectF(target.left() + x * target.width(), target.top() + box.y() * target.height(),
                  box.width() * target.width(), box.height() * target.height());
}

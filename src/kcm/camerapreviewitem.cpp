// SPDX-License-Identifier: GPL-3.0-or-later

#include "camerapreviewitem.h"

#include <QPainter>

CameraPreviewItem::CameraPreviewItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    setOpaquePainting(true);
}

CameraPreviewSession *CameraPreviewItem::session() const
{
    return m_session;
}

void CameraPreviewItem::setSession(CameraPreviewSession *session)
{
    if (m_session == session)
        return;
    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);
    m_session = session;
    if (m_session)
    {
        connect(m_session, &CameraPreviewSession::frameChanged, this, [this]() { update(); });
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

bool CameraPreviewItem::mirrored() const
{
    return m_mirrored;
}

void CameraPreviewItem::setMirrored(bool mirrored)
{
    if (m_mirrored == mirrored)
        return;
    m_mirrored = mirrored;
    update();
    Q_EMIT mirroredChanged();
}

void CameraPreviewItem::paint(QPainter *painter)
{
    painter->fillRect(boundingRect(), QColor(QStringLiteral("#091016")));
    if (!m_session)
        return;
    const QImage frame = m_session->frame();
    if (frame.isNull())
        return;
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
        painter->drawImage(target, frame);
}

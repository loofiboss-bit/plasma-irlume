// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "camerapreviewsession.h"

#include <QQuickPaintedItem>

class CameraPreviewItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(CameraPreviewSession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)

  public:
    explicit CameraPreviewItem(QQuickItem *parent = nullptr);

    [[nodiscard]] CameraPreviewSession *session() const;
    void setSession(CameraPreviewSession *session);
    [[nodiscard]] bool mirrored() const;
    void setMirrored(bool mirrored);
    void paint(QPainter *painter) override;

  Q_SIGNALS:
    void sessionChanged();
    void mirroredChanged();

  private:
    CameraPreviewSession *m_session = nullptr;
    bool m_mirrored = true;
};

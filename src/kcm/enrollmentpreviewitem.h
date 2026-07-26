// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "enrollmentsession.h"

#include <QColor>
#include <QQuickPaintedItem>

class EnrollmentPreviewItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(EnrollmentSession *session READ session WRITE setSession NOTIFY sessionChanged)
    Q_PROPERTY(bool mirrored READ mirrored WRITE setMirrored NOTIFY mirroredChanged)
    Q_PROPERTY(QColor landmarkColor READ landmarkColor WRITE setLandmarkColor NOTIFY landmarkColorChanged)
    Q_PROPERTY(QColor frameColor READ frameColor WRITE setFrameColor NOTIFY frameColorChanged)

  public:
    explicit EnrollmentPreviewItem(QQuickItem *parent = nullptr);

    [[nodiscard]] EnrollmentSession *session() const;
    void setSession(EnrollmentSession *session);
    [[nodiscard]] bool mirrored() const;
    void setMirrored(bool mirrored);
    [[nodiscard]] QColor landmarkColor() const;
    void setLandmarkColor(const QColor &color);
    [[nodiscard]] QColor frameColor() const;
    void setFrameColor(const QColor &color);

    void paint(QPainter *painter) override;

  Q_SIGNALS:
    void sessionChanged();
    void mirroredChanged();
    void landmarkColorChanged();
    void frameColorChanged();

  private:
    [[nodiscard]] QPointF mappedPoint(const QPointF &point, const QRectF &target) const;
    [[nodiscard]] QRectF mappedBox(const QRectF &box, const QRectF &target) const;

    EnrollmentSession *m_session = nullptr;
    bool m_mirrored = true;
    QColor m_landmarkColor = QColor(QStringLiteral("#68d8ff"));
    QColor m_frameColor = QColor(QStringLiteral("#8fe388"));
};

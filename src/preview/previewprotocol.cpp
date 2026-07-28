// SPDX-License-Identifier: GPL-3.0-or-later

#include "previewprotocol.h"

#include <QCborParserError>
#include <QCborValue>
#include <QtEndian>

#include <utility>

namespace PreviewProtocol
{
QByteArray encode(const QCborMap &record)
{
    const QByteArray payload = QCborValue(record).toCbor();
    if (payload.isEmpty() || payload.size() > MaxRecordBytes)
        return {};

    QByteArray framed(sizeof(quint32), Qt::Uninitialized);
    qToBigEndian(static_cast<quint32>(payload.size()), framed.data());
    framed.append(payload);
    return framed;
}

bool Parser::append(QByteArrayView bytes, QVector<QCborMap> *records, QString *errorCode)
{
    if (!records || !errorCode)
        return false;
    m_buffer.append(bytes.data(), bytes.size());
    if (m_buffer.size() > MaxRecordBytes * 2 + static_cast<qsizetype>(sizeof(quint32)))
    {
        *errorCode = QStringLiteral("protocol-error");
        clear();
        return false;
    }

    while (m_buffer.size() >= static_cast<qsizetype>(sizeof(quint32)))
    {
        const quint32 payloadSize = qFromBigEndian<quint32>(m_buffer.constData());
        if (payloadSize == 0 || payloadSize > static_cast<quint32>(MaxRecordBytes))
        {
            *errorCode = QStringLiteral("protocol-error");
            clear();
            return false;
        }
        const qsizetype framedSize = static_cast<qsizetype>(sizeof(quint32)) + payloadSize;
        if (m_buffer.size() < framedSize)
            break;

        const QByteArray payload = m_buffer.mid(sizeof(quint32), payloadSize);
        m_buffer.remove(0, framedSize);
        QCborParserError parserError;
        const QCborValue value = QCborValue::fromCbor(payload, &parserError);
        if (parserError.error != QCborError::NoError || !value.isMap())
        {
            *errorCode = QStringLiteral("protocol-error");
            clear();
            return false;
        }
        records->push_back(value.toMap());
    }
    return true;
}

void Parser::clear()
{
    m_buffer.clear();
}

bool LatestFrameBuffer::hasFrame() const
{
    return !m_frame.isEmpty();
}

bool LatestFrameBuffer::replace(QByteArray frame)
{
    const bool dropped = hasFrame();
    m_frame = std::move(frame);
    return dropped;
}

QByteArray LatestFrameBuffer::take()
{
    QByteArray frame;
    frame.swap(m_frame);
    return frame;
}

void LatestFrameBuffer::clear()
{
    m_frame.clear();
}
} // namespace PreviewProtocol

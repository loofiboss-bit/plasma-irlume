// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QImage>
#include <QString>
#include <QtTypes>

namespace IdentityProtocol
{
constexpr quint16 Version = 1;
constexpr qsizetype KeyBytes = 32;
constexpr qsizetype EmbeddingBytes = 128 * 4;
constexpr quint8 MaximumSamples = 8;
constexpr quint32 DefaultTimeoutMs = 5000;
constexpr qsizetype MaximumResponseBytes = 64 + EmbeddingBytes;

enum class Operation : quint8
{
    Status = 1,
    ExtractEnrollmentSample = 2,
    CommitEnrollment = 3,
    ListProfileSummary = 4,
    VerifyOneFrame = 5,
    DeleteProfile = 6,
    RotateVaultKey = 7,
    ValidateVault = 8,
    ResetUnreadable = 9,
};

enum class ResponseKind : quint8
{
    Status = 0x81,
    Sample = 0x82,
    Ack = 0x83,
    Verification = 0x84,
    Error = 0xff,
};

struct Response
{
    ResponseKind kind = ResponseKind::Error;
    quint8 code = 0;
    quint64 generation = 0;
    QByteArray sensitivePayload;

    void clearSensitive()
    {
        sensitivePayload.fill(0);
        sensitivePayload.clear();
    }
};

[[nodiscard]] QByteArray statusRequest(quint64 generation, const QByteArray &key = {});
[[nodiscard]] QByteArray extractSampleRequest(quint64 generation, const QByteArray &priorEmbeddings, quint8 sampleCount,
                                              const QImage &frame, QString *error);
[[nodiscard]] QByteArray commitRequest(quint64 generation, const QByteArray &key, const QByteArray &embeddings,
                                       quint8 sampleCount);
[[nodiscard]] QByteArray summaryRequest(quint64 generation, const QByteArray &key);
[[nodiscard]] QByteArray verifyRequest(quint64 generation, const QByteArray &key, const QImage &frame, QString *error);
[[nodiscard]] QByteArray keyRequest(Operation operation, quint64 generation, const QByteArray &key);
[[nodiscard]] QByteArray resetRequest(quint64 generation);
[[nodiscard]] bool parseResponse(QByteArrayView payload, quint64 expectedGeneration, Response *response,
                                 QString *error);
} // namespace IdentityProtocol

// SPDX-License-Identifier: GPL-3.0-or-later

#include "enrollmentsession.h"

#include <QBuffer>
#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

class EnrollmentSessionTest final : public QObject
{
    Q_OBJECT

  private Q_SLOTS:
    void buildsOnlyFixedPreviewCommands();
    void parsesBoundedPreviewFrame();
    void rejectsUnsafePreviewPayloads();
    void validatesSessionAndSequence();
};

namespace
{
QJsonObject previewEvent()
{
    QImage image(32, 24, QImage::Format_Grayscale8);
    image.fill(96);
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 70);
    QJsonArray landmarks;
    for (int index = 0; index < 478; ++index)
    {
        const double x = 0.20 + 0.60 * (index % 22) / 21.0;
        const double y = 0.10 + 0.80 * (index / 22) / 21.0;
        landmarks.push_back(QJsonArray{x, y});
    }

    return {
        {QStringLiteral("contract_version"), 2},
        {QStringLiteral("engine_version"), QStringLiteral("0.7.0")},
        {QStringLiteral("command"), QStringLiteral("enroll")},
        {QStringLiteral("operation_id"), QStringLiteral("operation-preview-001")},
        {QStringLiteral("session_id"), QStringLiteral("session-preview-001")},
        {QStringLiteral("sequence"), 1},
        {QStringLiteral("event"), QStringLiteral("preview")},
        {QStringLiteral("terminal"), false},
        {QStringLiteral("data"),
         QJsonObject{
             {QStringLiteral("frame_jpeg_base64"), QString::fromLatin1(jpeg.toBase64())},
             {QStringLiteral("width"), 32},
             {QStringLiteral("height"), 24},
             {QStringLiteral("spectrum"), QStringLiteral("ir")},
             {QStringLiteral("landmarks"), landmarks},
             {QStringLiteral("face_box"), QJsonArray{0.20, 0.10, 0.60, 0.80}},
             {QStringLiteral("position"),
              QJsonObject{
                  {QStringLiteral("face_detected"), true},
                  {QStringLiteral("centered"), true},
                  {QStringLiteral("facing_camera"), true},
                  {QStringLiteral("well_lit"), true},
                  {QStringLiteral("ir_ready"), true},
                  {QStringLiteral("well_framed"), true},
                  {QStringLiteral("quality"), 94},
                  {QStringLiteral("countdown"), 2},
                  {QStringLiteral("guidance"), QStringLiteral("Hold still")},
              }},
         }},
    };
}
} // namespace

void EnrollmentSessionTest::buildsOnlyFixedPreviewCommands()
{
    QCOMPARE(
        EnrollmentSession::argumentsForOperation(IrlumeProcess::Operation::Enroll),
        QStringList({QStringLiteral("enroll"), QStringLiteral("--events=jsonl"), QStringLiteral("--preview=ir-jpeg"),
                     QStringLiteral("--preview-max-fps=8"), QStringLiteral("--preview-max-size=640x480")}));

    const QString profileId = QStringLiteral("profile-example-001");
    const QStringList arguments =
        EnrollmentSession::argumentsForOperation(IrlumeProcess::Operation::AddScan, profileId);
    QVERIFY(arguments.contains(profileId));
    QVERIFY(!arguments.contains(QStringLiteral("--user")));
    QVERIFY(!arguments.contains(QStringLiteral("/run/irlume.sock")));
}

void EnrollmentSessionTest::parsesBoundedPreviewFrame()
{
    const auto parsed = EnrollmentSession::parseEvent(previewEvent(), IrlumeProcess::Operation::Enroll, 1);
    QVERIFY2(parsed.ok, qPrintable(parsed.errorCode));
    QVERIFY(parsed.preview);
    QCOMPARE(parsed.sessionId, QStringLiteral("session-preview-001"));
    QCOMPARE(parsed.spectrum, QStringLiteral("ir"));
    QCOMPARE(parsed.frame.size(), QSize(32, 24));
    QCOMPARE(parsed.landmarks.size(), 478);
    QCOMPARE(parsed.position.quality, 94);
    QCOMPARE(parsed.position.countdown, 2);
    QVERIFY(parsed.position.wellFramed);
}

void EnrollmentSessionTest::rejectsUnsafePreviewPayloads()
{
    QJsonObject event = previewEvent();
    QJsonObject data = event.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("embedding"), QJsonArray{0.1, 0.2});
    event.insert(QStringLiteral("data"), data);
    auto parsed = EnrollmentSession::parseEvent(event, IrlumeProcess::Operation::Enroll, 1);
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.errorCode, QStringLiteral("invalid-event-contract"));

    event = previewEvent();
    data = event.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("width"), 641);
    event.insert(QStringLiteral("data"), data);
    parsed = EnrollmentSession::parseEvent(event, IrlumeProcess::Operation::Enroll, 1);
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.errorCode, QStringLiteral("invalid-preview-dimensions"));

    event = previewEvent();
    data = event.value(QStringLiteral("data")).toObject();
    data.insert(QStringLiteral("landmarks"), QJsonArray{QJsonArray{-0.1, 0.4}});
    event.insert(QStringLiteral("data"), data);
    parsed = EnrollmentSession::parseEvent(event, IrlumeProcess::Operation::Enroll, 1);
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.errorCode, QStringLiteral("invalid-preview-landmarks"));
}

void EnrollmentSessionTest::validatesSessionAndSequence()
{
    QJsonObject event = previewEvent();
    auto parsed = EnrollmentSession::parseEvent(event, IrlumeProcess::Operation::Enroll, 1, {},
                                                QStringLiteral("different-session"));
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.errorCode, QStringLiteral("invalid-preview-session"));

    parsed = EnrollmentSession::parseEvent(event, IrlumeProcess::Operation::Enroll, 2);
    QVERIFY(!parsed.ok);
    QCOMPARE(parsed.errorCode, QStringLiteral("invalid-event-sequence"));
}

QTEST_MAIN(EnrollmentSessionTest)

#include "test_enrollmentsession.moc"

// SPDX-License-Identifier: BSD-2-Clause

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QProcessEnvironment>
#include <QtEndian>

#include <chrono>
#include <cstdio>
#include <thread>

namespace
{
bool writeAll(QByteArray bytes)
{
    QFile output;
    if (!output.open(stdout, QIODevice::WriteOnly))
        return false;
    while (!bytes.isEmpty())
    {
        const qint64 written = output.write(bytes);
        if (written <= 0)
            return false;
        bytes.remove(0, written);
    }
    return output.flush();
}

QByteArray framedResponse(bool malformed)
{
    QByteArray payload(malformed ? 3 : 12, 0);
    QByteArray framed(4, 0);
    qToBigEndian(static_cast<quint32>(payload.size()), framed.data());
    framed.append(payload);
    return framed;
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QFile input;
    if (!input.open(stdin, QIODevice::ReadOnly))
        return 2;
    QByteArray request = input.readAll();
    request.fill(0);
    request.clear();

    const QString mode = QProcessEnvironment::systemEnvironment().value(QStringLiteral("KFACEAUTH_TEST_MODE"));
    if (mode == QLatin1String("crash"))
        return 7;
    if (mode == QLatin1String("hang"))
        std::this_thread::sleep_for(std::chrono::seconds(30));
    if (mode == QLatin1String("delayed"))
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

    return writeAll(framedResponse(mode == QLatin1String("malformed"))) ? 0 : 3;
}

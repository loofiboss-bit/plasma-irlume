// SPDX-License-Identifier: GPL-3.0-or-later

#include "previewworker.h"

#include <QCoreApplication>

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral("plasma-irlume-camera-preview-worker"));

    PreviewWorker worker;
    if (!worker.start())
        return 2;
    return application.exec();
}

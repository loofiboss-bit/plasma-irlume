// SPDX-License-Identifier: GPL-3.0-or-later

#include "previewworker.h"

#include <QCoreApplication>

#ifndef KFACEAUTH_PREVIEW_WORKER_NAME
#define KFACEAUTH_PREVIEW_WORKER_NAME "kfaceauth-camera-preview-worker"
#endif

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    application.setApplicationName(QStringLiteral(KFACEAUTH_PREVIEW_WORKER_NAME));

    PreviewWorker worker;
    if (!worker.start())
        return 2;
    return application.exec();
}

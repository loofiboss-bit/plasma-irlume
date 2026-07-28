// SPDX-License-Identifier: GPL-3.0-or-later

#include "backendfactory.h"

#include "irlumebackend.h"

std::unique_ptr<FaceAuthBackend> createProductionFaceAuthBackend()
{
    return std::make_unique<IrlumeBackend>(QStringLiteral("/usr/bin/irlume"));
}

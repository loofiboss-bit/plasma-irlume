// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

class FaceAuthBackend;

[[nodiscard]] std::unique_ptr<FaceAuthBackend> createProductionFaceAuthBackend();

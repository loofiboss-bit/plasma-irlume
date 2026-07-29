// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace KFaceAuthYuNet
{
inline bool validOutputShape(int32_t dimensions, int32_t type, int32_t expectedType, int32_t rows, int32_t columns)
{
    return dimensions == 2 && type == expectedType && rows >= 0 && columns == 15;
}
} // namespace KFaceAuthYuNet

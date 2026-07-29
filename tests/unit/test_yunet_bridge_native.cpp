// SPDX-License-Identifier: GPL-3.0-or-later

#include "yunet_bridge.h"
#include "yunet_output_validation.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{
constexpr int Width = 64;
constexpr int Height = 64;
constexpr size_t Stride = Width * 3;
constexpr size_t MaximumDetections = 5000;

std::vector<uint8_t> readModel()
{
    std::ifstream input(KFACEAUTH_YUNET_MODEL_PATH, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
} // namespace

int main()
{
    if (!KFaceAuthYuNet::validOutputShape(2, 5, 5, 1, 15) || KFaceAuthYuNet::validOutputShape(3, 5, 5, 1, 15) ||
        KFaceAuthYuNet::validOutputShape(2, 4, 5, 1, 15) || KFaceAuthYuNet::validOutputShape(2, 5, 5, -1, 15) ||
        KFaceAuthYuNet::validOutputShape(2, 5, 5, 1, 14))
        return 1;

    if (kfaceauth_yunet_disable_core_dumps() != KFACEAUTH_YUNET_OK)
        return 2;

    auto model = readModel();
    if (model.size() != 232589)
        return 3;
    void *detector = nullptr;
    if (kfaceauth_yunet_create(model.data(), model.size() - 1, Width, Height, 0.9F, 0.3F, 5000, &detector) !=
            KFACEAUTH_YUNET_INVALID_ARGUMENT ||
        detector)
        return 4;
    if (kfaceauth_yunet_create(model.data(), model.size(), Width, Height, 0.9F, 0.3F, 5000, &detector) !=
            KFACEAUTH_YUNET_OK ||
        !detector)
        return 5;

    std::vector<uint8_t> frame(Stride * Height, 0);
    const auto original = frame;
    std::vector<KFaceAuthYuNetDetection> detections(MaximumDetections);
    size_t count = 0;
    if (kfaceauth_yunet_detect(detector, frame.data(), frame.size(), Width, Height, Stride - 1, detections.data(),
                               detections.size(), &count) != KFACEAUTH_YUNET_INVALID_ARGUMENT)
    {
        kfaceauth_yunet_destroy(detector);
        return 6;
    }
    if (kfaceauth_yunet_detect(detector, frame.data(), frame.size(), Width, Height, Stride, detections.data(),
                               detections.size(), &count) != KFACEAUTH_YUNET_OK ||
        count > detections.size() || frame != original)
    {
        kfaceauth_yunet_destroy(detector);
        return 7;
    }

    std::fill(model.begin(), model.end(), uint8_t{0});
    std::fill(frame.begin(), frame.end(), uint8_t{0});
    std::fill(detections.begin(), detections.end(), KFaceAuthYuNetDetection{});
    kfaceauth_yunet_destroy(detector);
    return 0;
}

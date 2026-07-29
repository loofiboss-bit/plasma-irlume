// SPDX-License-Identifier: GPL-3.0-or-later

#include "yunet_bridge.h"
#include "yunet_output_validation.h"

#include <opencv2/core/version.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/objdetect/face.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <vector>

#if defined(__linux__)
#include <sys/prctl.h>
#include <sys/resource.h>
#endif

namespace
{
constexpr size_t ExpectedModelBytes = 232589;
constexpr int32_t MaximumTopK = 5000;
constexpr size_t MaximumDetections = 5000;

struct Detector
{
    cv::Ptr<cv::FaceDetectorYN> value;
};

static_assert(sizeof(KFaceAuthYuNetDetection) == sizeof(float) * 15);
static_assert(alignof(KFaceAuthYuNetDetection) == alignof(float));

bool validGeometry(int32_t width, int32_t height)
{
    return width > 0 && width <= 640 && height > 0 && height <= 480;
}

bool validThreshold(float value)
{
    return std::isfinite(value) && value > 0.0F && value < 1.0F;
}

void clearBytes(std::vector<uint8_t> *bytes)
{
    if (bytes)
        std::fill(bytes->begin(), bytes->end(), uint8_t{0});
}
} // namespace

extern "C" int kfaceauth_yunet_disable_core_dumps(void)
{
#if defined(__linux__)
    const rlimit limit{0, 0};
    if (setrlimit(RLIMIT_CORE, &limit) != 0)
        return KFACEAUTH_YUNET_HARDENING_FAILURE;
    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) != 0)
        return KFACEAUTH_YUNET_HARDENING_FAILURE;
#endif
    return KFACEAUTH_YUNET_OK;
}

extern "C" const char *kfaceauth_yunet_opencv_version(void)
{
    return CV_VERSION;
}

extern "C" int kfaceauth_yunet_create(const uint8_t *model_bytes, size_t model_size, int32_t width, int32_t height,
                                      float score_threshold, float nms_threshold, int32_t top_k, void **detector_out)
{
    if (!model_bytes || model_size != ExpectedModelBytes || !detector_out || *detector_out ||
        !validGeometry(width, height) || !validThreshold(score_threshold) || !validThreshold(nms_threshold) ||
        top_k <= 0 || top_k > MaximumTopK)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    std::vector<uint8_t> model;
    try
    {
        model.assign(model_bytes, model_bytes + model_size);
        const std::vector<uint8_t> emptyConfig;
        auto detector =
            cv::FaceDetectorYN::create("ONNX", model, emptyConfig, cv::Size(width, height), score_threshold,
                                       nms_threshold, top_k, cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
        clearBytes(&model);
        if (detector.empty())
            return KFACEAUTH_YUNET_RUNTIME_FAILURE;
        constexpr float ThresholdTolerance = 1.0e-6F;
        if (std::abs(detector->getScoreThreshold() - score_threshold) > ThresholdTolerance ||
            std::abs(detector->getNMSThreshold() - nms_threshold) > ThresholdTolerance || detector->getTopK() != top_k)
            return KFACEAUTH_YUNET_RUNTIME_FAILURE;
        auto result = std::make_unique<Detector>();
        result->value = std::move(detector);
        *detector_out = result.release();
        return KFACEAUTH_YUNET_OK;
    }
    catch (...)
    {
        clearBytes(&model);
        return KFACEAUTH_YUNET_RUNTIME_FAILURE;
    }
}

extern "C" int kfaceauth_yunet_detect(void *detector, const uint8_t *bgr_bytes, size_t bgr_size, int32_t width,
                                      int32_t height, size_t stride, KFaceAuthYuNetDetection *detections,
                                      size_t detection_capacity, size_t *detection_count)
{
    if (!detector || !bgr_bytes || !detections || !detection_count || !validGeometry(width, height) ||
        detection_capacity == 0 || detection_capacity > MaximumDetections)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    const auto widthSize = static_cast<size_t>(width);
    const auto heightSize = static_cast<size_t>(height);
    if (widthSize > std::numeric_limits<size_t>::max() / 3)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;
    const size_t minimumStride = widthSize * 3;
    if (stride != minimumStride || heightSize > std::numeric_limits<size_t>::max() / stride ||
        bgr_size != stride * heightSize)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    *detection_count = 0;
    cv::Mat image;
    cv::Mat faces;
    try
    {
        auto *typedDetector = static_cast<Detector *>(detector);
        typedDetector->value->setInputSize(cv::Size(width, height));
        image.create(height, width, CV_8UC3);
        for (int32_t row = 0; row < height; ++row)
        {
            const auto rowOffset = static_cast<size_t>(row) * stride;
            std::copy_n(bgr_bytes + rowOffset, stride, image.ptr<uint8_t>(row));
        }
        typedDetector->value->detect(image, faces);
        if (faces.empty())
        {
            image.setTo(0);
            return KFACEAUTH_YUNET_OK;
        }
        if (!KFaceAuthYuNet::validOutputShape(faces.dims, faces.type(), CV_32FC1, faces.rows, faces.cols))
        {
            image.setTo(0);
            faces.setTo(0);
            return KFACEAUTH_YUNET_MALFORMED_OUTPUT;
        }
        const auto count = static_cast<size_t>(faces.rows);
        if (count > detection_capacity)
        {
            image.setTo(0);
            faces.setTo(0);
            return KFACEAUTH_YUNET_OUTPUT_TOO_LARGE;
        }
        for (size_t row = 0; row < count; ++row)
        {
            const float *source = faces.ptr<float>(static_cast<int>(row));
            std::copy_n(source, 15, detections[row].values);
        }
        *detection_count = count;
        image.setTo(0);
        faces.setTo(0);
        return KFACEAUTH_YUNET_OK;
    }
    catch (...)
    {
        if (!image.empty())
            image.setTo(0);
        if (!faces.empty())
            faces.setTo(0);
        *detection_count = 0;
        return KFACEAUTH_YUNET_RUNTIME_FAILURE;
    }
}

extern "C" void kfaceauth_yunet_destroy(void *detector)
{
    try
    {
        delete static_cast<Detector *>(detector);
    }
    catch (...)
    {
        // Destructors must never unwind across the C ABI.
    }
}

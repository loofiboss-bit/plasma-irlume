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
#include <iterator>
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
constexpr size_t ExpectedSFaceModelBytes = 38696353;
constexpr int32_t MaximumTopK = 5000;
constexpr size_t MaximumDetections = 5000;

struct Detector
{
    cv::Ptr<cv::FaceDetectorYN> value;
};

struct Recognizer
{
    cv::Ptr<cv::FaceRecognizerSF> value;
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

bool validPackedBgr(size_t bgrSize, int32_t width, int32_t height, size_t stride)
{
    if (!validGeometry(width, height))
        return false;
    const auto widthSize = static_cast<size_t>(width);
    const auto heightSize = static_cast<size_t>(height);
    if (widthSize > std::numeric_limits<size_t>::max() / 3)
        return false;
    const size_t minimumStride = widthSize * 3;
    return stride == minimumStride && heightSize <= std::numeric_limits<size_t>::max() / stride &&
           bgrSize == stride * heightSize;
}

void clearBytes(std::vector<uint8_t> *bytes)
{
    if (bytes)
        std::fill(bytes->begin(), bytes->end(), uint8_t{0});
}

void clearMat(cv::Mat *matrix)
{
    if (matrix && !matrix->empty())
        matrix->setTo(0);
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
    if (!detector || !bgr_bytes || !detections || !detection_count ||
        !validPackedBgr(bgr_size, width, height, stride) || detection_capacity == 0 ||
        detection_capacity > MaximumDetections)
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

extern "C" int kfaceauth_sface_create(const uint8_t *model_bytes, size_t model_size, void **recognizer_out)
{
    if (!model_bytes || model_size != ExpectedSFaceModelBytes || !recognizer_out || *recognizer_out)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    std::vector<uint8_t> model;
    try
    {
        model.assign(model_bytes, model_bytes + model_size);
        const std::vector<uint8_t> emptyConfig;
        auto recognizer = cv::FaceRecognizerSF::create("ONNX", model, emptyConfig, cv::dnn::DNN_BACKEND_OPENCV,
                                                       cv::dnn::DNN_TARGET_CPU);
        clearBytes(&model);
        if (recognizer.empty())
            return KFACEAUTH_YUNET_RUNTIME_FAILURE;
        auto result = std::make_unique<Recognizer>();
        result->value = std::move(recognizer);
        *recognizer_out = result.release();
        return KFACEAUTH_YUNET_OK;
    }
    catch (...)
    {
        clearBytes(&model);
        return KFACEAUTH_YUNET_RUNTIME_FAILURE;
    }
}

extern "C" int kfaceauth_sface_extract(void *recognizer, const uint8_t *bgr_bytes, size_t bgr_size, int32_t width,
                                       int32_t height, size_t stride, const KFaceAuthYuNetDetection *detection,
                                       float *embedding, size_t embedding_capacity, size_t *embedding_count)
{
    if (!recognizer || !bgr_bytes || !detection || !embedding || !embedding_count ||
        !validPackedBgr(bgr_size, width, height, stride) || embedding_capacity != KFACEAUTH_SFACE_EMBEDDING_DIMENSION)
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;
    if (!std::all_of(std::begin(detection->values), std::end(detection->values),
                     [](float value) { return std::isfinite(value); }))
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    *embedding_count = 0;
    std::fill_n(embedding, embedding_capacity, 0.0F);
    cv::Mat image;
    cv::Mat faceRow;
    cv::Mat aligned;
    cv::Mat feature;
    try
    {
        image.create(height, width, CV_8UC3);
        for (int32_t row = 0; row < height; ++row)
        {
            const auto rowOffset = static_cast<size_t>(row) * stride;
            std::copy_n(bgr_bytes + rowOffset, stride, image.ptr<uint8_t>(row));
        }
        faceRow.create(1, 15, CV_32FC1);
        std::copy_n(detection->values, 15, faceRow.ptr<float>(0));

        auto *typedRecognizer = static_cast<Recognizer *>(recognizer);
        typedRecognizer->value->alignCrop(image, faceRow, aligned);
        if (aligned.dims != 2 || aligned.type() != CV_8UC3 || aligned.rows != KFACEAUTH_SFACE_ALIGNED_HEIGHT ||
            aligned.cols != KFACEAUTH_SFACE_ALIGNED_WIDTH)
        {
            clearMat(&image);
            clearMat(&faceRow);
            clearMat(&aligned);
            return KFACEAUTH_YUNET_MALFORMED_OUTPUT;
        }
        typedRecognizer->value->feature(aligned, feature);
        if (feature.dims != 2 || feature.type() != CV_32FC1 || feature.rows != 1 ||
            feature.cols != KFACEAUTH_SFACE_EMBEDDING_DIMENSION || !feature.isContinuous())
        {
            clearMat(&image);
            clearMat(&faceRow);
            clearMat(&aligned);
            clearMat(&feature);
            return KFACEAUTH_YUNET_MALFORMED_OUTPUT;
        }
        const float *source = feature.ptr<float>(0);
        if (!std::all_of(source, source + KFACEAUTH_SFACE_EMBEDDING_DIMENSION,
                         [](float value) { return std::isfinite(value); }))
        {
            clearMat(&image);
            clearMat(&faceRow);
            clearMat(&aligned);
            clearMat(&feature);
            return KFACEAUTH_YUNET_MALFORMED_OUTPUT;
        }
        std::copy_n(source, KFACEAUTH_SFACE_EMBEDDING_DIMENSION, embedding);
        *embedding_count = KFACEAUTH_SFACE_EMBEDDING_DIMENSION;
        clearMat(&image);
        clearMat(&faceRow);
        clearMat(&aligned);
        clearMat(&feature);
        return KFACEAUTH_YUNET_OK;
    }
    catch (...)
    {
        clearMat(&image);
        clearMat(&faceRow);
        clearMat(&aligned);
        clearMat(&feature);
        std::fill_n(embedding, embedding_capacity, 0.0F);
        *embedding_count = 0;
        return KFACEAUTH_YUNET_RUNTIME_FAILURE;
    }
}

extern "C" int kfaceauth_sface_cosine(void *recognizer, const float *left, size_t left_count, const float *right,
                                      size_t right_count, double *similarity)
{
    if (!recognizer || !left || !right || !similarity || left_count != KFACEAUTH_SFACE_EMBEDDING_DIMENSION ||
        right_count != KFACEAUTH_SFACE_EMBEDDING_DIMENSION ||
        !std::all_of(left, left + left_count, [](float value) { return std::isfinite(value); }) ||
        !std::all_of(right, right + right_count, [](float value) { return std::isfinite(value); }))
        return KFACEAUTH_YUNET_INVALID_ARGUMENT;

    *similarity = 0.0;
    try
    {
        const cv::Mat leftFeature(1, KFACEAUTH_SFACE_EMBEDDING_DIMENSION, CV_32FC1, const_cast<float *>(left));
        const cv::Mat rightFeature(1, KFACEAUTH_SFACE_EMBEDDING_DIMENSION, CV_32FC1, const_cast<float *>(right));
        const double score = static_cast<Recognizer *>(recognizer)
                                 ->value->match(leftFeature, rightFeature, cv::FaceRecognizerSF::FR_COSINE);
        if (!std::isfinite(score) || score < -1.000001 || score > 1.000001)
            return KFACEAUTH_YUNET_MALFORMED_OUTPUT;
        *similarity = score;
        return KFACEAUTH_YUNET_OK;
    }
    catch (...)
    {
        *similarity = 0.0;
        return KFACEAUTH_YUNET_RUNTIME_FAILURE;
    }
}

extern "C" void kfaceauth_sface_destroy(void *recognizer)
{
    try
    {
        delete static_cast<Recognizer *>(recognizer);
    }
    catch (...)
    {
        // Destructors must never unwind across the C ABI.
    }
}

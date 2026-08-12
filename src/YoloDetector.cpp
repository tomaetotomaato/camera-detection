#include "YoloDetector.h"

#include <stdexcept>
#include <string>

// Full definition of the engine that the header only forward-declares.
#include "yolos/tasks/detection.hpp"

namespace camdet {

YoloDetector::YoloDetector(
    const std::filesystem::path& model_path,
    const std::filesystem::path& labels_path,
    float conf_threshold,
    float iou_threshold
)
    : conf_threshold_(conf_threshold),
      iou_threshold_(iou_threshold) {
    // Check the paths ourselves first: a typo should produce a readable message,
    // not an ONNX Runtime stack trace.
    if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error("model file not found: " + model_path.string());
    }
    if (!std::filesystem::exists(labels_path)) {
        throw std::runtime_error("labels file not found: " + labels_path.string());
    }

    engine_ = std::make_unique<yolos::det::YOLODetector>(
        model_path.string(), labels_path.string(), /*useGPU=*/false);

    // YOLOs-CPP fails soft here: a missing or unreadable labels file leaves the
    // name list empty and every detection then draws as nothing, which looks
    // exactly like "the model found no objects". Fall back to the class names
    // embedded in the ONNX metadata, and refuse to run if both are empty.
    class_names_ = engine_->getClassNames();
    if (class_names_.empty()) {
        class_names_ = engine_->getExportedClassNamesFromMetadata();
    }
    if (class_names_.empty()) {
        throw std::runtime_error(
            "no class names available from labels file or model metadata: " + labels_path.string());
    }
}

// Must be out-of-line: unique_ptr<yolos::det::YOLODetector> cannot instantiate its
// deleter against the incomplete type visible in the header.
YoloDetector::~YoloDetector() = default;

std::vector<Detection> YoloDetector::detect(const cv::Mat& frame) {
    const std::vector<yolos::det::Detection> raw =
        engine_->detect(frame, conf_threshold_, iou_threshold_);

    // Translate the library's integer boxes into our own Detection type, so
    // nothing downstream of IDetector depends on YOLOs-CPP.
    std::vector<Detection> detections;
    detections.reserve(raw.size());
    for (const auto& det : raw) {
        detections.push_back(Detection{
            cv::Rect2f(
                static_cast<float>(det.box.x),
                static_cast<float>(det.box.y),
                static_cast<float>(det.box.width),
                static_cast<float>(det.box.height)),
            det.conf,
            det.classId});
    }
    return detections;
}

const std::vector<std::string>& YoloDetector::classNames() const {
    return class_names_;
}

} // namespace camdet

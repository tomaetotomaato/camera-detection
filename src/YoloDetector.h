#pragma once

#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <memory>
#include <string>
#include <vector>

#include "IDetector.h"
#include "Detection.h"

namespace yolos::det{
    class YOLODetector; 
} // namespace yolos::det

namespace camdet {
class YoloDetector : public IDetector {
public:
    // Constructor
    YoloDetector(
        const std::filesystem::path& model_path, 
        const std::filesystem::path& labels_path, 
        float conf_threshold = 0.4f, 
        float iou_threshold = 0.45f
    );

    std::vector<Detection> detect(const cv::Mat& frame) override; // Override of the pure virtual from IDetector.

    ~YoloDetector() override; // Virtual destructor

    // Class names for the loaded model, indexed by Detection::class_id.
    // Lives on the concrete detector, not IDetector: rendering is the app's job,
    // so main reads this once at startup and the frame loop stays on IDetector.
    const std::vector<std::string>& classNames() const;

private:
    float conf_threshold_;
    float iou_threshold_;
    std::unique_ptr<yolos::det::YOLODetector> engine_;
    std::vector<std::string> class_names_;
};

} // namespace camdet
#pragma once

#include <cstddef>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

#include "Detection.h"

// Rendering lives here rather than on the detector: IDetector produces data,
// and the application decides how to draw it.
namespace camdet::viz {

// Stable, well-spread colour for a class id.
cv::Scalar colorForClass(int class_id);

// Draws each box with a "name 87%" label. If class_names is empty or too short
// the label falls back to "class <id>" — a bad labels file must never make the
// boxes disappear silently.
void drawDetections(
    cv::Mat& frame,
    const std::vector<Detection>& detections,
    const std::vector<std::string>& class_names);

struct HudInfo {
    double inference_ms = 0.0;      // most recent frame
    double avg_inference_ms = 0.0;  // exponential moving average
    std::size_t detections = 0;
    std::size_t frame_index = 0;
    std::string source_label;
    bool paused = false;
};

// Translucent status bar across the top of the frame.
void drawHud(cv::Mat& frame, const HudInfo& info);

} // namespace camdet::viz

#include "Visualization.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <opencv2/imgproc.hpp>
#include <sstream>

namespace camdet::viz {

namespace {

// Scale strokes and text off the frame size, so a box is equally visible on a
// 768px video frame and on a 2560px stills.
float scaleFor(const cv::Mat& frame) {
    const int shorter_side = std::min(frame.rows, frame.cols);
    return std::max(1.0f, static_cast<float>(shorter_side) / 640.0f);
}

std::string formatLabel(
    const Detection& det,
    const std::vector<std::string>& class_names) {
    std::string name;
    if (det.class_id >= 0 && static_cast<std::size_t>(det.class_id) < class_names.size()) {
        name = class_names[det.class_id];
    } else {
        name = "class " + std::to_string(det.class_id);
    }
    return name + " " + std::to_string(static_cast<int>(det.confidence * 100.0f)) + "%";
}

} // namespace

cv::Scalar colorForClass(int class_id) {
    // Golden-ratio hue stepping keeps neighbouring class ids visually distinct.
    const int hue = static_cast<int>(std::fmod(std::abs(class_id) * 137.508, 180.0));
    cv::Mat hsv(1, 1, CV_8UC3, cv::Scalar(hue, 200, 255));
    cv::Mat bgr;
    cv::cvtColor(hsv, bgr, cv::COLOR_HSV2BGR);
    const cv::Vec3b& c = bgr.at<cv::Vec3b>(0, 0);
    return cv::Scalar(c[0], c[1], c[2]);
}

void drawDetections(
    cv::Mat& frame,
    const std::vector<Detection>& detections,
    const std::vector<std::string>& class_names) {
    const float scale = scaleFor(frame);
    const int thickness = std::max(2, static_cast<int>(std::lround(2.0f * scale)));
    const double font_scale = 0.5 * scale;
    const int font = cv::FONT_HERSHEY_SIMPLEX;

    for (const auto& det : detections) {
        const cv::Scalar color = colorForClass(det.class_id);
        const cv::Rect box(det.bbox); // cv::rectangle clips to the frame for us.
        cv::rectangle(frame, box, color, thickness, cv::LINE_AA);

        const std::string label = formatLabel(det, class_names);
        int baseline = 0;
        const cv::Size text_size =
            cv::getTextSize(label, font, font_scale, thickness / 2 + 1, &baseline);

        // Keep the label on screen when the box touches the top or right edge.
        const int label_w = text_size.width + 4;
        const int label_h = text_size.height + baseline + 4;
        const int label_top = std::max(box.y - label_h, 0);
        const int label_left = std::clamp(box.x, 0, std::max(0, frame.cols - label_w));
        const cv::Rect label_box =
            cv::Rect(label_left, label_top, label_w, label_h) & cv::Rect(0, 0, frame.cols, frame.rows);
        cv::rectangle(frame, label_box, color, cv::FILLED);
        cv::putText(
            frame,
            label,
            cv::Point(label_left + 2, label_top + text_size.height + 2),
            font,
            font_scale,
            cv::Scalar(0, 0, 0),
            thickness / 2 + 1,
            cv::LINE_AA);
    }
}

void drawHud(cv::Mat& frame, const HudInfo& info) {
    const float scale = scaleFor(frame);
    const int font = cv::FONT_HERSHEY_SIMPLEX;
    const int thickness = std::max(1, static_cast<int>(std::lround(scale)));
    const int margin = static_cast<int>(8 * scale);

    std::ostringstream text;
    text << std::fixed << std::setprecision(1)
         << "YOLOv11n | " << info.inference_ms << " ms"
         << " (avg " << info.avg_inference_ms << ")"
         << " | " << info.detections << (info.detections == 1 ? " object" : " objects")
         << " | frame " << info.frame_index
         << " | " << info.source_label;
    if (info.paused) {
        text << " | PAUSED";
    }
    const std::string line = text.str();

    // Shrink the font until the line fits the frame width. A narrow frame would
    // otherwise silently truncate the numbers the demo is meant to show off.
    double font_scale = 0.55 * scale;
    int baseline = 0;
    cv::Size text_size = cv::getTextSize(line, font, font_scale, thickness, &baseline);
    const int available = std::max(1, frame.cols - 2 * margin);
    if (text_size.width > available) {
        font_scale *= static_cast<double>(available) / text_size.width;
        text_size = cv::getTextSize(line, font, font_scale, thickness, &baseline);
    }

    const int bar_height =
        std::min(frame.rows, text_size.height + baseline + static_cast<int>(14 * scale));
    const cv::Rect bar(0, 0, frame.cols, bar_height);

    // Translucent backing so the text stays readable over any footage.
    cv::Mat overlay = frame(bar).clone();
    overlay.setTo(cv::Scalar(0, 0, 0));
    cv::addWeighted(overlay, 0.55, frame(bar), 0.45, 0.0, frame(bar));

    cv::putText(
        frame,
        line,
        cv::Point(margin, (bar_height + text_size.height) / 2),
        font,
        font_scale,
        cv::Scalar(255, 255, 255),
        thickness,
        cv::LINE_AA);
}

} // namespace camdet::viz

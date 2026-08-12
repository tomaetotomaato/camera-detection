#include "VideoCameraSource.h"

#include <stdexcept>

namespace fs = std::filesystem;

VideoCameraSource::VideoCameraSource(const fs::path& video_path, bool loop)
    : video_path_(video_path), loop_(loop) {
    // Two separate messages: a missing file and an undecodable one need
    // different fixes, and "could not open" alone tells you neither.
    if (!fs::exists(video_path_)) {
        throw std::runtime_error("video file not found: " + video_path_.string());
    }
    if (!capture_.open(video_path_.string())) {
        throw std::runtime_error(
            "could not open video (unsupported codec?): " + video_path_.string());
    }
}

std::optional<cv::Mat> VideoCameraSource::getNextFrame() {
    // A fresh Mat per call, deliberately: a member Mat would be reused by read(),
    // so every frame the caller is still holding would alias the next one.
    cv::Mat frame;

    if (capture_.read(frame) && !frame.empty()) {
        return frame;
    }

    if (loop_) {
        capture_.set(cv::CAP_PROP_POS_FRAMES, 0);
        // Single retry — if the rewound stream also fails, the source is done.
        if (capture_.read(frame) && !frame.empty()) {
            return frame;
        }
    }

    return std::nullopt;
}

double VideoCameraSource::sourceFps() const {
    const double fps = capture_.get(cv::CAP_PROP_FPS);
    return (fps > 0.0 && fps < 1000.0) ? fps : 0.0;
}

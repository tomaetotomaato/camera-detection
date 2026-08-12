#pragma once

#include <filesystem>
#include <optional>
#include <opencv2/videoio.hpp>

#include "ICameraSource.h"

// Frame source backed by a video file. Same role as TestCameraSource, but the
// frames arrive at video rate instead of one per image on disk.
class VideoCameraSource : public ICameraSource {
public:
    // Throws std::runtime_error if the file is missing or cannot be decoded.
    explicit VideoCameraSource(const std::filesystem::path& video_path, bool loop = false);

    std::optional<cv::Mat> getNextFrame() override;

    // Frames per second reported by the container, or 0 if unknown.
    double sourceFps() const;

private:
    cv::VideoCapture capture_;
    std::filesystem::path video_path_;
    bool loop_ = false;
};

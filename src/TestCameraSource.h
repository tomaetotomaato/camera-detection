#pragma once

#include <filesystem>
#include <vector>

#include "ICameraSource.h"

class TestCameraSource : public ICameraSource {
public:
    // Constructor
    explicit TestCameraSource(const std::filesystem::path& directory);
    
    std::optional<cv::Mat> getNextFrame() override; // Override of the pure virtual from ICameraSource.

private:
    std::vector<std::filesystem::path> image_paths_;
    std::size_t next_index_ = 0;
};


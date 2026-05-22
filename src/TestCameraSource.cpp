#include <filesystem>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

#include "TestCameraSource.h"

namespace fs = std::filesystem;

TestCameraSource::TestCameraSource(const fs::path& directory) {
    const std::vector<std::string> valid_ext = {
        ".jpg", 
        ".jpeg", 
        ".png", 
        ".bmp", 
        ".tiff"
    };

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (std::find(valid_ext.begin(), valid_ext.end(), ext) != valid_ext.end()) {
                image_paths_.push_back(entry.path());
            }
        }
    }

    // Sort the image paths to ensure consistent order.
    std::sort(image_paths_.begin(), image_paths_.end());
}

std::optional<cv::Mat> TestCameraSource::getNextFrame() {
    // No more frames available
    while (next_index_ < image_paths_.size()) {
        fs::path current_path = image_paths_[next_index_];
        cv::Mat img = cv::imread(current_path.string());
        ++next_index_;

        if (img.empty()) {
            std::cerr << "Could not read the image: " << current_path << std::endl;
            continue;
        }
        return img;
    }
    return std::nullopt; // No more frames available
}
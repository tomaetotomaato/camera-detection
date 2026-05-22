#pragma once

#include <optional>
#include <opencv2/opencv.hpp>

class ICameraSource {
    public:
    // Virtual destructor
    virtual ~ICameraSource() = default;

    // = 0 marks this as pure virtual, so derived classes must implement it.
    virtual std::optional<cv::Mat> getNextFrame() = 0;
};
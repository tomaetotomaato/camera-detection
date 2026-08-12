#pragma once

#include <opencv2/core/mat.hpp>
#include <vector>

#include "Detection.h"

class IDetector {
    public:
    // Virtual destructor
    virtual ~IDetector() = default;

    // = 0 marks this as pure virtual, so derived classes must implement it.
    virtual std::vector<Detection> detect(const cv::Mat& frame) = 0;
};
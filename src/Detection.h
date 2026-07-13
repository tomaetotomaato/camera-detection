#pragma once

#include <opencv2/core/types.hpp>

struct Detection {
    cv::Rect2f bbox;
    float confidence;
    int class_id;
};
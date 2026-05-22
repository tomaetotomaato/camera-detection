#include <iostream>
#include <filesystem>
#include <memory>
#include <opencv2/opencv.hpp>

#include "ICameraSource.h"
#include "TestCameraSource.h"

namespace fs = std::filesystem;

int main() {
    std::unique_ptr<ICameraSource> cameraSource = std::make_unique<TestCameraSource>("../test_data");

    while (true) {
        std::optional<cv::Mat> frame = cameraSource->getNextFrame();
        if (!frame) {
            std::cout << "No more frames available." << std::endl;
            break; // This breaks even if cameraSource is not unplugged. In a real implementation, you might want to handle this differently, such as waiting for new frames or exiting gracefully.
        }
        cv::imshow("Image", *frame);

        int key = cv::waitKey(500);
        if ((key & 0xFF) == 'q') {
            break;
        }
    }
    return 0;
}
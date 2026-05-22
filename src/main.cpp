#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    cv::Mat img = cv::imread("../test_data/SWARM-flight-in-progress-scaled.jpg");
    
    // imread silently fails if the image cannot be loaded, so we check if the image is empty
    if (img.empty()) {
        std::cout << "Could not read the image" << std::endl;
        return 1;
    }
    
    cv::imshow("Image", img);
    cv::waitKey(0); // Wait for a key press indefinitely. Without this, the window would close immediately after opening.
    
    return 0;
}
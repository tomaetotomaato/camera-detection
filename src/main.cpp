#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

int main() {

    std::vector<std::string> imgs = {
        "../test_data/SWARM-flight-in-progress-scaled.jpg",
        "../test_data/csm__DSC8815_copy_c950a426fd.jpg",
        "../test_data/tmobile-3-min.jpg"
    };

    for (const std::string& img_path : imgs) {
        cv::Mat img = cv::imread(img_path);

        // imread silently fails if the image cannot be loaded, so we check if the image is empty. 
        if (img.empty()) {
            std::cerr << "Could not read the image: " << img_path << std::endl;
            continue; // Skip to the next image instead of failing the entire program. 
        }

        cv::imshow("Image", img);
        cv::waitKey(0); // Wait for a key press indefinitely. Without this, the window would close immediately after opening.
    }
    
    return 0;
}
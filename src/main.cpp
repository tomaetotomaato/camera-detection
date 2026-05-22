#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

int main() {

    const std::string test_data_dir = "../test_data";

    const std::vector<std::string> valid_ext = {
        ".jpg", 
        ".jpeg", 
        ".png", 
        ".bmp", 
        ".tiff"
    };

    std::vector<std::string> imgs;

    for (const auto& entry : fs::directory_iterator(test_data_dir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (std::find(valid_ext.begin(), valid_ext.end(), ext) != valid_ext.end()) {
                imgs.push_back(entry.path().string());
            }
        }
    }

    std::sort(imgs.begin(), imgs.end());
    std::cout << "Found " << imgs.size() << " valid image(s) in the directory." << std::endl;

    for (const std::string& img_path : imgs) {
        cv::Mat img = cv::imread(img_path);
        // imread silently fails if the image cannot be loaded, so we check if the image is empty. 
        if (img.empty()) {
            std::cerr << "Could not read the image: " << img_path << std::endl;
            continue; // Skip to the next image instead of failing the entire program. 
        }

        cv::imshow("Image", img);

        int key = cv::waitKey(500);
        if ((key & 0xFF) == 'q') { // Masking with 0xFF to get the ASCII value of the key pressed.
            break;
        }
    }
    
    return 0;
}
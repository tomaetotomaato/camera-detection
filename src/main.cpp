#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "ICameraSource.h"
#include "IDetector.h"
#include "TestCameraSource.h"
#include "VideoCameraSource.h"
#include "Visualization.h"
#include "YoloDetector.h"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

constexpr const char* kWindowName = "camera_detection";

struct Options {
    fs::path input = "../demo_data/images";
    fs::path model = "../models/yolo11n.onnx";
    fs::path labels = "../models/coco.names";
    float conf = 0.35f;
    float iou = 0.45f;
    int delay_ms = -1; // -1 = pick automatically from the source type
    bool loop = false;
    bool force_video = false;
    bool no_display = false;
    fs::path save_dir;
    bool help = false;
};

void printUsage(const char* argv0) {
    std::cout
        << "Usage: " << argv0 << " [options]\n"
        << "  --input <path>     image directory or video file (default ../demo_data/images)\n"
        << "  --video            treat --input as a video regardless of extension\n"
        << "  --model <path>     ONNX model (default ../models/yolo11n.onnx)\n"
        << "  --labels <path>    class names file (default ../models/coco.names)\n"
        << "  --conf <float>     confidence threshold (default 0.35)\n"
        << "  --iou <float>      NMS IoU threshold (default 0.45)\n"
        << "  --delay <ms>       per-frame wait; 0 waits for a keypress\n"
        << "  --loop             restart the video when it ends\n"
        << "  --save-dir <path>  write every annotated frame to this directory\n"
        << "  --no-display       run without a GUI window (pairs with --save-dir)\n"
        << "  --help             show this message\n"
        << "\nKeys: q or Esc quit, space pause, s save the current frame\n";
}

// Returns false if the arguments could not be parsed.
bool parseArgs(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "[ERROR] " << name << " needs a value\n";
                return {};
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            opts.help = true;
        } else if (arg == "--input") {
            const std::string v = next("--input");
            if (v.empty()) return false;
            opts.input = v;
        } else if (arg == "--model") {
            const std::string v = next("--model");
            if (v.empty()) return false;
            opts.model = v;
        } else if (arg == "--labels") {
            const std::string v = next("--labels");
            if (v.empty()) return false;
            opts.labels = v;
        } else if (arg == "--conf") {
            const std::string v = next("--conf");
            if (v.empty()) return false;
            opts.conf = std::stof(v);
        } else if (arg == "--iou") {
            const std::string v = next("--iou");
            if (v.empty()) return false;
            opts.iou = std::stof(v);
        } else if (arg == "--delay") {
            const std::string v = next("--delay");
            if (v.empty()) return false;
            opts.delay_ms = std::stoi(v);
        } else if (arg == "--save-dir") {
            const std::string v = next("--save-dir");
            if (v.empty()) return false;
            opts.save_dir = v;
        } else if (arg == "--video") {
            opts.force_video = true;
        } else if (arg == "--loop") {
            opts.loop = true;
        } else if (arg == "--no-display") {
            opts.no_display = true;
        } else {
            std::cerr << "[ERROR] unknown argument: " << arg << "\n";
            return false;
        }
    }
    return true;
}

bool hasVideoExtension(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    static const std::vector<std::string> kVideoExt = {
        ".avi", ".mp4", ".mkv", ".mov", ".webm", ".m4v"};
    return std::find(kVideoExt.begin(), kVideoExt.end(), ext) != kVideoExt.end();
}

// Picks the frame source from the input path. The inference loop below never
// learns which one it got — that is the whole point of ICameraSource.
std::unique_ptr<ICameraSource> makeSource(const Options& opts, std::string& label_out) {
    if (opts.force_video || (fs::is_regular_file(opts.input) && hasVideoExtension(opts.input))) {
        label_out = "video: " + opts.input.filename().string();
        return std::make_unique<VideoCameraSource>(opts.input, opts.loop);
    }
    if (fs::is_directory(opts.input)) {
        label_out = "images: " + fs::weakly_canonical(opts.input).filename().string() + "/";
        return std::make_unique<TestCameraSource>(opts.input);
    }
    throw std::runtime_error(
        "input is neither a directory of images nor a video file: " + opts.input.string());
}

// Compact per-frame console summary, e.g. "person x5, car x2".
std::string summarize(
    const std::vector<Detection>& detections,
    const std::vector<std::string>& class_names) {
    std::map<std::string, int> counts;
    for (const auto& det : detections) {
        const bool named = det.class_id >= 0 &&
                           static_cast<std::size_t>(det.class_id) < class_names.size();
        counts[named ? class_names[det.class_id] : "class " + std::to_string(det.class_id)]++;
    }
    std::ostringstream out;
    bool first = true;
    for (const auto& [name, count] : counts) {
        if (!first) out << ", ";
        out << name << " x" << count;
        first = false;
    }
    return out.str();
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parseArgs(argc, argv, opts)) {
        printUsage(argv[0]);
        return 2;
    }
    if (opts.help) {
        printUsage(argv[0]);
        return 0;
    }

    // Construct the concrete detector so we can read its class names once, then
    // run the frame loop through the IDetector interface only.
    std::unique_ptr<camdet::YoloDetector> yolo;
    try {
        yolo = std::make_unique<camdet::YoloDetector>(
            opts.model, opts.labels, opts.conf, opts.iou);
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] detector init failed: " << e.what() << "\n"
                  << "        model:  " << fs::absolute(opts.model) << "\n"
                  << "        labels: " << fs::absolute(opts.labels) << "\n";
        return 1;
    }

    const std::vector<std::string> class_names = yolo->classNames();
    std::cout << "[INFO] " << class_names.size() << " class names loaded from "
              << opts.labels << "\n"
              << "[INFO] thresholds: conf=" << opts.conf << " iou=" << opts.iou << "\n";

    // The first ONNX Runtime call pays for graph optimisation and allocator
    // setup. Spend it here so the on-screen timings are honest from frame 0.
    {
        const cv::Mat warm = cv::Mat::zeros(640, 640, CV_8UC3);
        const auto start = Clock::now();
        yolo->detect(warm);
        const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        std::cout << "[INFO] warm-up inference: " << std::fixed << std::setprecision(1) << ms
                  << " ms\n";
    }

    IDetector& detector = *yolo;

    std::string source_label;
    std::unique_ptr<ICameraSource> source;
    try {
        source = makeSource(opts, source_label);
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
    std::cout << "[INFO] source: " << source_label << "\n";

    if (!opts.save_dir.empty()) {
        fs::create_directories(opts.save_dir);
        std::cout << "[INFO] saving annotated frames to " << fs::absolute(opts.save_dir) << "\n";
    }

    if (!opts.no_display) {
        // WINDOW_NORMAL, because the 2560px test images would otherwise open a
        // window larger than the screen.
        cv::namedWindow(kWindowName, cv::WINDOW_NORMAL);
        cv::resizeWindow(kWindowName, 1280, 720);
    }

    const bool is_video = source_label.rfind("video:", 0) == 0;
    const int delay_ms = opts.delay_ms >= 0 ? opts.delay_ms : (is_video ? 1 : 700);

    std::size_t frame_index = 0;
    double avg_ms = 0.0;
    bool paused = false;
    cv::Mat canvas;

    while (true) {
        if (!paused) {
            std::optional<cv::Mat> frame = source->getNextFrame();
            if (!frame) {
                std::cout << "[INFO] source exhausted after " << frame_index << " frames.\n";
                break;
            }

            const auto start = Clock::now();
            const std::vector<Detection> detections = detector.detect(*frame);
            const double ms =
                std::chrono::duration<double, std::milli>(Clock::now() - start).count();
            avg_ms = (avg_ms == 0.0) ? ms : (0.9 * avg_ms + 0.1 * ms);

            // Annotate a copy; the source frame stays pristine.
            canvas = frame->clone();
            camdet::viz::drawDetections(canvas, detections, class_names);
            camdet::viz::drawHud(
                canvas,
                {ms, avg_ms, detections.size(), frame_index, source_label, paused});

            std::cout << "[frame " << frame_index << "] " << detections.size() << " objects  "
                      << std::fixed << std::setprecision(1) << ms << " ms";
            if (!detections.empty()) {
                std::cout << "  (" << summarize(detections, class_names) << ")";
            }
            std::cout << std::endl;

            if (!opts.save_dir.empty()) {
                std::ostringstream name;
                name << "frame_" << std::setw(4) << std::setfill('0') << frame_index << ".jpg";
                cv::imwrite((opts.save_dir / name.str()).string(), canvas);
            }
            ++frame_index;
        }

        if (opts.no_display) {
            if (paused) break; // Nothing can unpause us without a keyboard.
            continue;
        }

        cv::Mat display = canvas;
        if (display.cols > 1600) {
            const double scale = 1600.0 / display.cols;
            cv::resize(canvas, display, cv::Size(), scale, scale, cv::INTER_AREA);
        }
        cv::imshow(kWindowName, display);

        const int key = cv::waitKey(paused ? 30 : std::max(1, delay_ms)) & 0xFF;
        if (key == 'q' || key == 27) {
            break;
        } else if (key == ' ') {
            paused = !paused;
            std::cout << "[INFO] " << (paused ? "paused" : "resumed") << "\n";
        } else if (key == 's' && !canvas.empty()) {
            std::ostringstream name;
            name << "snapshot_" << std::setw(4) << std::setfill('0') << frame_index << ".jpg";
            cv::imwrite(name.str(), canvas);
            std::cout << "[INFO] saved " << fs::absolute(name.str()) << "\n";
        }
    }

    if (!opts.no_display) {
        cv::destroyAllWindows();
    }
    return 0;
}

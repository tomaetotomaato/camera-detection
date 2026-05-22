# camera-detection

Real-time object detection for the Teledyne FLIR Blackfly S industrial camera, written in C++17 with OpenCV and ONNX Runtime. The project is structured so that the inference pipeline can be developed and tested against ordinary image files and video clips long before any camera hardware is in hand, and the live camera can then be plugged in by changing a single line of code.

## What this project does

At a high level the program takes one frame of imagery at a time, runs a YOLOv11 object detector across it, and overlays bounding boxes for every recognised object onto the displayed frame. Pressing `s` saves the current frame to disk and pressing `q` or `Esc` exits.

The interesting design choice sits behind the frame source. Rather than coupling the inference loop directly to the Spinnaker SDK (which talks to the FLIR camera), all frame acquisition goes through an abstract `ICameraSource` interface. A `TestCameraSource` implementation reads from a single image, a directory of images, or a video file, simulating the 4096×2160 monochrome output of the BFS-U3-88S6M-C. When real hardware is available, a `SpinnakerCameraSource` implementation takes its place, and the inference code never needs to know the difference. This separation is also what makes the build itself optional in layers: ONNX Runtime, YOLOs-CPP, and the Spinnaker SDK are each pulled in only if you point CMake at them, so a fresh clone will compile and run end-to-end on nothing more than OpenCV.

## Project structure

```
camera-detection/
├── include/
│   ├── camera_source.hpp           # ICameraSource — the abstract base
│   ├── test_camera_source.hpp      # File / directory / video implementation
│   └── spinnaker_camera_source.hpp # Live FLIR camera implementation (stub)
├── src/
│   └── main.cpp                    # Inference loop and display
├── test_data/                      # Drop test images and videos here
├── models/                         # yolo11n.onnx + coco.names go here
├── scripts/
│   └── download_test_images.sh     # Fetches 10 COCO validation images
└── CMakeLists.txt
```

## Building and running

The project is developed and tested under WSL2 on Ubuntu 24.04, but should build on any Linux system with a recent C++17 compiler. Builds are configured through CMake flags so that the three optional dependencies can each be enabled independently. The recommended path is to bring the project up in three phases, since each phase gives you a working program that exercises a meaningful slice of the pipeline.

### Phase 1: test mode with no detection model

The fastest way to confirm everything is wired up is to build with OpenCV alone and run the pipeline against test images. Install the system dependencies, fetch sample images, then configure and build with CMake.

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
bash scripts/download_test_images.sh
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The resulting `yolo_detector` binary accepts a directory of images (which it will loop through indefinitely), a single image file, or a video file.

```bash
./yolo_detector --input ../test_data
./yolo_detector --input ../test_data/000000039769.jpg
./yolo_detector --input /path/to/video.mp4
```

At this stage there is no detector yet, so no boxes will be drawn — but you should see frames flowing through the display window, confirming that the camera abstraction, file loading, and display pipeline all work.

### Phase 2: enable YOLO inference

To turn the test pipeline into an actual detector you need three additional pieces: a YOLOv11 model exported to ONNX, the ONNX Runtime shared library, and the YOLOs-CPP header-only wrapper that bridges OpenCV `Mat` objects into ONNX Runtime tensors.

Export the model using Ultralytics' Python tooling, drop the result into `models/`, and grab a copy of `coco.names` from the YOLOs-CPP repository so the detector knows the class labels.

```bash
pip install ultralytics
python3 -c "from ultralytics import YOLO; YOLO('yolo11n.pt').export(format='onnx', imgsz=640)"
mkdir -p models && cp yolo11n.onnx models/
```

Download a release of ONNX Runtime for Linux x64 from the [microsoft/onnxruntime](https://github.com/microsoft/onnxruntime/releases) releases page, extract it somewhere convenient, and clone the [YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP) repository alongside it. Then rebuild, passing CMake the two new paths, and uncomment the inference lines in `src/main.cpp` (search for `// auto detector`).

```bash
cd build
cmake .. -DORT_DIR=/path/to/onnxruntime-linux-x64-X.X.X \
         -DYOLOS_DIR=/path/to/YOLOs-CPP
make -j$(nproc)
```

You should now see labelled bounding boxes overlaid on each frame as it passes through the display window.

### Phase 3: live camera

The final phase swaps the test source for the real Blackfly S. Under WSL2 the USB camera is exposed to Linux using `usbipd-win`, which forwards the device from the Windows host. From an elevated Windows PowerShell, bind and attach the camera, then verify it appears on the Linux side with `lsusb`.

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

Install the Spinnaker SDK on the Linux side (downloadable from Teledyne's support page), rebuild the project with `-DSPINNAKER_DIR=/opt/spinnaker` to link in the camera library and enable the `USE_SPINNAKER` compile flag, and uncomment the Spinnaker code inside `spinnaker_camera_source.hpp`. The only change required to `main.cpp` is the line that constructs the source object.

```cpp
// Development:
auto camera = std::make_unique<TestCameraSource>("../test_data");
// Production:
auto camera = std::make_unique<SpinnakerCameraSource>(0);
```

Because the inference loop only ever sees an `ICameraSource*`, every other line of code stays exactly as it was during Phase 2.

## Controls

While the display window is focused, `q` or `Esc` will exit cleanly and `s` will save the current frame as a timestamped PNG in the working directory. Saved frames include any bounding boxes that were drawn, which makes them useful for debugging detection quality.

## Hardware target

The pipeline is sized for the [Teledyne FLIR Blackfly S BFS-U3-88S6M-C](https://www.teledynevisionsolutions.com/products/blackfly-s-usb3/), an 8.9-megapixel monochrome USB3 industrial camera at 4096×2160. The test source converts whatever it reads to 8-bit mono so the inference pipeline sees the same pixel format in development that it will see in production.

## Acknowledgements

The detection model is YOLOv11 from [Ultralytics](https://github.com/ultralytics/ultralytics), the C++ inference wrapper is [Geekgineer/YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP), and frame acquisition on real hardware uses [Teledyne FLIR's Spinnaker SDK](https://www.teledynevisionsolutions.com/support/spinnaker-sdk/).
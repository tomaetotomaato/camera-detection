# camera-detection

Real-time object detection for the Teledyne FLIR Blackfly S industrial camera, written in C++17 with OpenCV and ONNX Runtime. The project is structured so that the inference pipeline can be developed and tested against ordinary image files long before any camera hardware is in hand, and the live camera can then be plugged in by changing a single line of code.

## What this project does

Today the program walks a directory of test images, hands them one frame at a time to a display loop, and shows each in an OpenCV window. **Detection is not yet wired into the loop** — no bounding boxes are drawn. What exists is the scaffolding the detector will hang off of, plus a working end-to-end path from frame source to screen.

The interesting design choice sits behind the frame source. Rather than coupling the inference loop directly to the Spinnaker SDK (which talks to the FLIR camera), all frame acquisition goes through an abstract `ICameraSource` interface. A `TestCameraSource` implementation reads from a directory of images; when real hardware is available, a `SpinnakerCameraSource` implementation takes its place, and the inference code never needs to know the difference. The same split is being applied to detection: `IDetector` is the abstract seam, with `YoloDetector` as the YOLOs-CPP-backed implementation.

The original intent was for ONNX Runtime, YOLOs-CPP, and the Spinnaker SDK to be optional layers enabled independently at configure time. That isn't the case yet — the build links ONNX Runtime unconditionally, so you need it present even though no inference code runs. See [Known limitations](#known-limitations).

## Project structure

```
camera-detection/
├── src/
│   ├── main.cpp                 # Display loop
│   ├── ICameraSource.h          # Abstract frame source
│   ├── TestCameraSource.h/.cpp  # Directory-of-images implementation
│   ├── IDetector.h              # Abstract detector
│   ├── YoloDetector.h           # YOLOs-CPP implementation (declaration only)
│   └── Detection.h              # bbox / confidence / class_id
├── test_data/                   # Sample images, checked in
├── models/                      # yolo11n.onnx goes here (gitignored)
└── CMakeLists.txt
```

## Building and running

Developed and tested under WSL2 on Ubuntu 24.04, but should build on any Linux system with a recent C++17 compiler.

Three things must be in place before configuring. Install the system packages, extract an [ONNX Runtime](https://github.com/microsoft/onnxruntime/releases) release for Linux x64 to `~/onnxruntime`, and clone [YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP) to `~/YOLOs-CPP`. Both of those paths are currently hardcoded in `CMakeLists.txt`, so they have to live exactly there.

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
# extract ONNX Runtime to ~/onnxruntime, clone YOLOs-CPP to ~/YOLOs-CPP
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./camera_detection
```

Run the binary from inside `build/`. The test image directory is hardcoded in `src/main.cpp` as `../test_data`, resolved relative to the working directory, so launching from anywhere else finds no frames. The program also opens a GUI window, which means it needs a display — under WSL2, an X server on the Windows host.

## Controls

Frames advance automatically every 500 ms. Pressing `q` while the display window is focused exits; otherwise the program runs until the image directory is exhausted and then quits on its own.

## Known limitations

The `~/onnxruntime` and `~/YOLOs-CPP` paths should be `-DORT_DIR` and `-DYOLOS_DIR` cache options rather than hardcoded values, and ONNX Runtime should be an opt-in layer so a fresh clone builds on OpenCV alone. Neither is implemented; both are worth fixing before the dependency list grows further with Spinnaker.

## Roadmap

**In progress.** The `IDetector` and `YoloDetector` interfaces have landed on `feature/yolo-onnx-detector`, along with the `Detection` struct. `YoloDetector.cpp` is still unwritten, the class isn't in the CMake source list, and `main.cpp` doesn't call it — so the detector is declared but not yet functional. Wiring it up also needs `coco.names` copied into `models/` (a copy ships with YOLOs-CPP at `~/YOLOs-CPP/models/coco.names`) alongside a YOLOv11 model exported to ONNX:

```bash
pip install ultralytics
python3 -c "from ultralytics import YOLO; YOLO('yolo11n.pt').export(format='onnx', imgsz=640)"
mkdir -p models && cp yolo11n.onnx models/
```

**Planned.** A `--input` flag accepting a single image, a directory, or a video file, replacing the hardcoded path. Conversion of test frames to 8-bit mono so development matches the production pixel format. Saving the current frame to a timestamped PNG. The optional-dependency CMake flags described above.

**Live camera.** A `SpinnakerCameraSource` for real hardware. Under WSL2 the USB camera is exposed to Linux using `usbipd-win`, which forwards the device from the Windows host — from an elevated PowerShell:

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

The Linux side then needs the Spinnaker SDK (downloadable from Teledyne's support page) linked into the build. Because the loop only ever sees an `ICameraSource*`, swapping the test source for the live one should be the only application-level change required.

## Hardware target

The pipeline is sized for the [Teledyne FLIR Blackfly S BFS-U3-88S6M-C](https://www.teledynevisionsolutions.com/products/blackfly-s-usb3/), an 8.9-megapixel monochrome USB3 industrial camera at 4096×2160. The test source is intended to convert whatever it reads to 8-bit mono so the inference pipeline sees the same pixel format in development that it will see in production; at present it returns frames as `cv::imread` loads them, in 3-channel BGR.

## Acknowledgements

The detection model is YOLOv11 from [Ultralytics](https://github.com/ultralytics/ultralytics), the C++ inference wrapper is [Geekgineer/YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP), and frame acquisition on real hardware uses [Teledyne FLIR's Spinnaker SDK](https://www.teledynevisionsolutions.com/support/spinnaker-sdk/).

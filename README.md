# camera-detection

Real-time object detection for the Teledyne FLIR Blackfly S industrial camera, written in C++17 with OpenCV and ONNX Runtime. The inference pipeline is developed and tested against ordinary image files and video clips long before any camera hardware is in hand, so that when the camera arrives the live source can be plugged in by changing a single line of code.

The detector runs today. On CPU it turns in roughly 25–45 ms per frame with YOLOv11-nano at 640×640, against image directories and video files alike.

## Quickstart

Everything below is copy-pasteable on a fresh Ubuntu 24.04 machine. It ends with the detector running on the demo data.

```bash
# 1. System dependencies and the repo
sudo apt update
sudo apt install -y build-essential cmake git curl libopencv-dev python3-venv
git clone <this-repo> ~/camera-detection-demo

# 2. ONNX Runtime and YOLOs-CPP, in the two locations CMake looks for them
cd ~
curl -L -o onnxruntime.tgz \
  https://github.com/microsoft/onnxruntime/releases/download/v1.26.0/onnxruntime-linux-x64-1.26.0.tgz
tar xf onnxruntime.tgz && mv onnxruntime-linux-x64-1.26.0 onnxruntime
git clone https://github.com/Geekgineer/YOLOs-CPP.git ~/YOLOs-CPP

# 3. Export the model (coco.names is already in the repo; the .onnx is not).
#    A venv, because Ubuntu 24.04 refuses a system-wide pip install.
cd ~/camera-detection-demo
python3 -m venv ~/.venvs/ultralytics && source ~/.venvs/ultralytics/bin/activate
pip install ultralytics
python3 -c "from ultralytics import YOLO; YOLO('yolo11n.pt').export(format='onnx', imgsz=640)"
mv yolo11n.onnx models/
deactivate

# 4. Populate the demo data (see "Demo data" below — none of it is checked in)
mkdir -p demo_data/images demo_data/video demo_data/drone
for f in fruits.jpg home.jpg messi5.jpg; do
  curl -L -o "demo_data/images/$f" \
    "https://raw.githubusercontent.com/opencv/opencv/4.x/samples/data/$f"
done
curl -L -o demo_data/images/dog.jpg \
  https://raw.githubusercontent.com/pjreddie/darknet/master/data/dog.jpg
curl -L -o demo_data/video/vtest.avi \
  https://raw.githubusercontent.com/opencv/opencv/4.x/samples/data/vtest.avi
cp test_data/*.jpg demo_data/drone/

# 5. Build
cmake -S . -B build
cmake --build build -j$(nproc)

# 6. Run
cd build
./camera_detection --input ../demo_data/images
```

A successful run looks like this:

```
[INFO] Inference device: CPU
[INFO] Model loaded: ../models/yolo11n.onnx
[INFO] Input shape: 640x640
[INFO] 80 class names loaded from "../models/coco.names"
[INFO] thresholds: conf=0.35 iou=0.45
[INFO] warm-up inference: 40.7 ms
[INFO] source: images: images/
[frame 0] 4 objects  47.0 ms  (bicycle x1, car x1, dog x1, truck x1)
[frame 1] 6 objects  31.4 ms  (orange x6)
[frame 2] 4 objects  30.2 ms  (bird x4)
[frame 3] 5 objects  33.0 ms  (person x4, sports ball x1)
[INFO] source exhausted after 4 frames.
```

A window opens alongside that log showing each frame with its bounding boxes and a status bar. Image directories advance every 700 ms so you can actually see each result; video plays as fast as inference allows.

## Demo data

`demo_data/` is where the test imagery lives, and it is the default `--input`. Note that **`demo_data/` is deliberately not checked in** — it has a `.gitignore` containing `*`, so a fresh clone will not have it and step 4 of the quickstart is not optional. What that step builds is:

```
demo_data/
├── images/   dog.jpg  fruits.jpg  home.jpg  messi5.jpg   # stills, one frame each
├── video/    vtest.avi                                    # 795-frame street scene
└── drone/    three aerial drone photographs
```

The stills and the video come from the OpenCV and Darknet sample sets, chosen because they contain a good spread of COCO classes. The drone images are the three files in `test_data/`, which *is* tracked in git — step 4 just copies them across, so that part works offline.

## Running the demo

Run from inside `build/`; the default paths are relative to it.

```bash
# Still images, one per 700 ms, with a display window
./camera_detection --input ../demo_data/images

# Video, looping back to the start when it ends
./camera_detection --input ../demo_data/video/vtest.avi --loop

# Drone imagery, with a lower confidence threshold
./camera_detection --input ../demo_data/drone --conf 0.25

# Headless: no GUI at all, every annotated frame written to disk instead
./camera_detection --input ../demo_data/video/vtest.avi --no-display --save-dir ../demo_out
```

That last form is the one to reach for over a plain SSH session, or under a WSL2 setup without an X server. `--no-display` on its own still logs every frame to the console, but nothing is kept; pairing it with `--save-dir` is what lets you inspect the boxes afterwards.

The source type is inferred from the path: a directory becomes an image sequence, and a file with a video extension (`.avi`, `.mp4`, `.mkv`, `.mov`, `.webm`, `.m4v`) becomes a video. Pass `--video` to force the video reader on a file whose extension does not give it away.

### Options

| Flag | Default | Meaning |
| --- | --- | --- |
| `--input <path>` | `../demo_data/images` | Image directory or video file |
| `--video` | off | Treat `--input` as a video regardless of extension |
| `--model <path>` | `../models/yolo11n.onnx` | ONNX model |
| `--labels <path>` | `../models/coco.names` | Class names file |
| `--conf <float>` | `0.35` | Confidence threshold |
| `--iou <float>` | `0.45` | NMS IoU threshold |
| `--delay <ms>` | auto | Per-frame wait; `0` waits for a keypress |
| `--loop` | off | Restart the video when it ends (video only) |
| `--save-dir <path>` | — | Write every annotated frame here as `frame_NNNN.jpg` |
| `--no-display` | off | Run without a GUI window; pairs with `--save-dir` |
| `--help` | — | Print this list |

The automatic `--delay` is 700 ms for image directories and 1 ms for video. Use `--delay 0` to step through frame by frame with the keyboard.

### Controls

With the display window focused: `q` or `Esc` exits, `space` pauses and resumes, and `s` writes the current annotated frame to `snapshot_NNNN.jpg` in the working directory. Snapshots include the boxes and the status bar, which makes them convenient for reporting a detection problem.

## Prerequisites

OpenCV comes from the distribution (`libopencv-dev`, tested against 4.6.0). The other two dependencies are **not optional** and are **not configurable by CMake flag** — [CMakeLists.txt](CMakeLists.txt) looks for them at two fixed paths under your home directory:

- `~/onnxruntime/lib/libonnxruntime.so` and `~/onnxruntime/include`, from the [ONNX Runtime releases page](https://github.com/microsoft/onnxruntime/releases) (tested against 1.26.0)
- `~/YOLOs-CPP/include`, from [Geekgineer/YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP), a header-only wrapper that bridges OpenCV `Mat` objects into ONNX Runtime tensors

If either is missing, configuration stops immediately with `ONNX Runtime not found at ...` or `YOLOs-CPP headers not found at ...` rather than failing later at link time. To keep them somewhere else, edit `ONNXRUNTIME_ROOT_DIR` and `YOLOS_CPP_INCLUDE_DIR` at the top of `CMakeLists.txt`.

Builds default to `Release`. This matters more than usual: an unoptimised build makes inference several times slower, so a debug build that feels sluggish is behaving as expected.

The project is developed under WSL2 on Ubuntu 24.04 with GCC 13.3 and CMake 3.28, and should build on any Linux system with a recent C++17 compiler.

## How it fits together

```
camera-detection/
├── src/
│   ├── ICameraSource.h        # Abstract frame source: getNextFrame() -> optional<Mat>
│   ├── TestCameraSource.*     # Reads a directory of images, sorted, one frame each
│   ├── VideoCameraSource.*    # Reads a video file via cv::VideoCapture, optional looping
│   ├── IDetector.h            # Abstract detector: detect(Mat) -> vector<Detection>
│   ├── YoloDetector.*         # YOLOs-CPP / ONNX Runtime implementation of IDetector
│   ├── Detection.h            # Box, confidence, class id — the type both sides agree on
│   ├── Visualization.*        # Box and HUD drawing, as free functions
│   └── main.cpp               # Argument parsing, the frame loop, display and saving
├── models/                    # coco.names (tracked) + yolo11n.onnx (you export it)
├── test_data/                 # Three drone images, tracked in git
├── demo_data/                 # Demo imagery, not tracked — see "Demo data"
└── CMakeLists.txt
```

Two abstractions carry the design, and they exist for the same reason: the frame loop in `main.cpp` should not have to change when the things on either end of it are replaced.

`ICameraSource` is the seam at the input end. It has exactly one method, `getNextFrame()`, returning `std::optional<cv::Mat>` — a frame, or nothing when the source is exhausted. `TestCameraSource` walks a sorted directory of images; `VideoCameraSource` pulls frames from `cv::VideoCapture`. When the Blackfly S is available a `SpinnakerCameraSource` will implement the same method, and the loop above it will not notice.

`IDetector` is the seam at the other end, and it earns its keep by keeping YOLOs-CPP types out of the rest of the program. `YoloDetector` owns the ONNX session and translates the library's own detection structs into the project's `Detection` — a `cv::Rect2f`, a confidence, and a class id. Nothing downstream of that translation knows which inference library produced the numbers.

Drawing lives in `Visualization` rather than on the detector, on the principle that `IDetector` produces data and the application decides how to render it. The HUD across the top of each frame shows the per-frame and rolling-average inference time, the detection count, the frame index, and the source label.

Two details in `main.cpp` are deliberate and worth knowing about. The first inference call is run against a blank 640×640 frame before the loop starts, because that call pays for ONNX Runtime's graph optimisation and allocator setup — spending it up front keeps the on-screen timings honest from frame 0. And frames are annotated on a clone, so the frame handed over by the source is never modified.

## Interpreting the output

The bundled model is stock YOLOv11-nano trained on COCO, so it recognises the 80 COCO classes listed in `models/coco.names` and nothing else. That is worth keeping in mind when looking at the drone imagery:

```
$ ./camera_detection --input ../demo_data/drone --no-display
[frame 0] 2 objects  38.0 ms  (aeroplane x2)
[frame 1] 1 objects  25.4 ms  (aeroplane x1)
[frame 2] 1 objects  25.2 ms  (aeroplane x1)
```

COCO has no "drone" class, so drones are reported as the nearest thing the model knows — usually `aeroplane`, sometimes `bird` or `kite`. The pipeline is working correctly here; the vocabulary is the limitation. Detecting drones as drones needs a model fine-tuned on drone imagery, which is a training problem rather than a pipeline one. Because the model path is just a flag, swapping one in is `--model /path/to/custom.onnx --labels /path/to/custom.names`.

## Roadmap: live camera

> Not yet implemented. `SpinnakerCameraSource` does not exist in the tree — this section records the intended path, and the `ICameraSource` abstraction is what keeps it cheap.

Under WSL2 the USB camera is exposed to Linux using [`usbipd-win`](https://github.com/dorssel/usbipd-win), which forwards the device from the Windows host. From an elevated Windows PowerShell, bind and attach the camera, then confirm it appears on the Linux side with `lsusb`:

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

On the Linux side, install the Spinnaker SDK from [Teledyne's support page](https://www.teledynevisionsolutions.com/support/spinnaker-sdk/), then add a `SpinnakerCameraSource` implementing `ICameraSource` and link the SDK in `CMakeLists.txt`. The change to `main.cpp` is one line in `makeSource()`:

```cpp
// Development:
return std::make_unique<TestCameraSource>(opts.input);
// Production:
return std::make_unique<SpinnakerCameraSource>(0);
```

Every other line — the detector, the timing, the HUD, the save logic — stays exactly as it is, because the loop only ever sees an `ICameraSource&`.

## Hardware target

The pipeline is sized for the [Teledyne FLIR Blackfly S BFS-U3-88S6M-C](https://www.teledynevisionsolutions.com/products/blackfly-s-usb3/), an 8.9-megapixel monochrome USB3 industrial camera at 4096×2160. The display path already accounts for imagery of that scale: the window opens resizable at 1280×720 and anything wider than 1600 px is downscaled for display only, so inference always sees full-resolution pixels.

Note that the test sources currently hand over 3-channel BGR frames, since that is what `cv::imread` and `cv::VideoCapture` produce by default. Matching the camera's true 8-bit monochrome output is a change to make when the Spinnaker source lands, so that development and production see the same pixel format.

## Acknowledgements

The detection model is YOLOv11 from [Ultralytics](https://github.com/ultralytics/ultralytics), the C++ inference wrapper is [Geekgineer/YOLOs-CPP](https://github.com/Geekgineer/YOLOs-CPP), and frame acquisition on real hardware will use [Teledyne FLIR's Spinnaker SDK](https://www.teledynevisionsolutions.com/support/spinnaker-sdk/).

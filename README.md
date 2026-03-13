# YOLOv8 ONNX C++ 实时目标检测

基于 ONNX Runtime 和 OpenCV 的 YOLOv8 目标检测 C++ 实现，支持实时视频流处理。

## 项目简介

本项目实现了一个高效的 YOLOv8 目标检测系统，使用 ONNX Runtime 进行模型推理，OpenCV 处理视频流。项目采用多线程流水线架构，实现了捕获、推理和显示的并行处理，确保实时性能。

## 主要特性

- 🚀 **实时性能**：多线程流水线设计，优化帧处理流程
- 🎯 **YOLOv8 支持**：完整支持 YOLOv8n 模型推理
- 🔄 **高效队列**：线程安全队列，支持最新帧保留机制
- 🐳 **Docker 支持**：提供完整的 Docker 部署方案
- 📊 **性能监控**：实时 FPS 显示和性能统计
- 🎨 **可视化**：支持检测框绘制和标签显示

## 系统要求

### 本地构建
- C++17 或更高版本
- CMake 3.18+
- OpenCV 4.x
- ONNX Runtime 1.12.1+
- Linux/Windows 系统

### Docker 部署
- Docker 20.10+
- Docker Compose 1.29+

## 项目结构

```
.
├── CMakeLists.txt          # CMake 构建配置
├── Dockerfile              # Docker 镜像构建文件
├── docker-compose.yaml     # Docker Compose 配置
├── Makefile                # Make 构建配置
├── include/                # 头文件目录
│   ├── safe.h             # 安全队列实现
│   ├── thread_safe_queue.h # 线程安全队列
│   ├── vedio_pipline.h    # 视频处理流水线
│   └── yolov8_detector.h  # YOLOv8 检测器
└── src/
    └── main.cpp           # 主程序入口
```

## 快速开始

### 方式一：Docker 部署（推荐）

1. 克隆项目并准备模型文件：
```bash
git clone https://github.com/desyang-hub/yolo-onnx-cpp.git
cd yolo-onnx-cpp-main
# 将 yolov8n.onnx 模型放入 models/ 目录
# 将测试视频放入 assets/ 目录
```

2. 使用 Docker Compose 启动：
```bash
docker-compose up --build
```

3. 查看检测结果：
```bash
# 检测结果将保存到 assets/output_result.jpg
```

### 方式二：本地构建

1. 安装依赖：
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libopencv-dev

# 下载 ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.12.1/onnxruntime-linux-x64-1.12.1.tgz
tar -xzf onnxruntime-linux-x64-1.12.1.tgz
```

2. 构建项目：
```bash
mkdir build && cd build
cmake .. -DONNXRUNTIME_ROOT=/path/to/onnxruntime
make -j4
```

3. 运行程序：
```bash
./yolo_onnx_cpu
```

## 配置说明

### 模型配置
- 默认模型路径：`./models/yolov8n.onnx`
- 输入尺寸：640x640
- 置信度阈值：0.5
- NMS 阈值：0.45

### 视频源配置
在 `vedio_pipline.h` 中修改视频源：
```cpp
// 使用视频文件
cv::VideoCapture cap("assets/video.avi");

// 或使用摄像头
cv::VideoCapture cap(camera_id);  // camera_id 通常为 0
```

## 性能优化

项目采用了多种优化策略：
- 多线程并行处理
- 最新帧保留机制（避免帧堆积）
- ONNX Runtime 图优化
- 高效的内存管理

## 注意事项

1. **Docker 环境**：由于 Docker 容器内无显示环境，检测结果会保存为图片文件而非实时显示
2. **模型文件**：请确保将 YOLOv8n.onnx 模型放置在正确的目录
3. **视频路径**：根据实际情况修改视频文件路径
4. **性能调优**：可根据硬件配置调整线程数和推理参数

## 许可证

本项目遵循[MIT](LICENSE)开源许可证。

## 更新日志

- 2026-03-13: 初始版本发布

## GitHub Actions CI

项目配置了GitHub Actions持续集成，在每次push和pull request到main分支时自动构建项目。CI工作流会：

1. 检出代码
2. 配置CMake
3. 构建项目
4. 运行测试（如果配置了）

CI配置文件位于 `.github/workflows/cmake-single-platform.yml`
/**
 * @FilePath     : /yolo-onnx-cpp/src/main.cpp
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-03-13 11:43:03
 * @LastEditors  : desyang
 * @LastEditTime : 2026-03-13 11:43:08
**/
// main.cpp
#include "thread_safe_queue.h"
#include "yolov8_detector.h"
#include "vedio_pipline.h"

#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <memory>

// 定义 detector（全局，供 inference_thread 使用）
YOLOv8Detector detector("./models/yolov8n.onnx");

int main() {
    FrameQueue frame_queue;
    ResultQueue result_queue;
    std::atomic<bool> running{true};

    // 启动线程
    std::thread cap_th(capture_thread, 0, std::ref(frame_queue), std::ref(running));
    std::thread infer_th(inference_thread, std::ref(frame_queue), std::ref(result_queue), std::ref(running));
    std::thread disp_th(display_thread, std::ref(result_queue), std::ref(running));

    // 等待 ESC 或异常
    disp_th.join();
    running = false;

    cap_th.join();
    infer_th.join();

    std::cout << "Pipeline stopped." << std::endl;
    return 0;
}
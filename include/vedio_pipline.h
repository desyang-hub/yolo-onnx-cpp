#pragma once


// video_pipeline.cpp
#include "thread_safe_queue.h"
#include "yolov8_detector.h"


#include <opencv2/opencv.hpp>
#include <thread>
#include <iostream>
#include <atomic>


using FrameQueue = ThreadSafeQueue<cv::Mat>;
using ResultQueue = ThreadSafeQueue<std::pair<cv::Mat, std::vector<YOLOv8Detector::Detection>>>;

extern YOLOv8Detector detector; // 声明（实际定义在 main.cpp）

void capture_thread(int camera_id, FrameQueue& frame_queue, std::atomic<bool>& running) {
    cv::VideoCapture cap("assets/video.avi");
    // cv::VideoCapture cap(camera_id);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open camera!" << std::endl;
        running = false;
        return;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame;
    while (running) {
        cap >> frame;
        if (frame.empty()) break;
        frame_queue.push_keep_latest(frame.clone()); // 关键：只保留最新帧！
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 避免过快
    }
    cap.release();
}

void inference_thread(FrameQueue& frame_queue, ResultQueue& result_queue, std::atomic<bool>& running) {
    cv::Mat latest_frame;
    while (running) {
        // 丢弃中间帧，只取最新
        while (!frame_queue.empty()) {
            frame_queue.try_pop(latest_frame);
        }
        if (!latest_frame.empty()) {
            auto detections = detector.run_inference(latest_frame);
            result_queue.push_keep_latest({latest_frame.clone(), detections});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void display_thread(ResultQueue& result_queue, std::atomic<bool>& running) {
    cv::Mat display_frame;
    std::vector<YOLOv8Detector::Detection> detections;

    std::pair<cv::Mat, std::vector<YOLOv8Detector::Detection>> p;

    int nums_of_frame = 0;
    int time_spends = 0;
    int fps;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = start_time;

    while (running) {
        if (result_queue.try_pop(p)) {
            display_frame = std::move(p.first);
            detections = std::move(p.second);
            // 绘制检测框
            for (const auto& det : detections) {
                cv::rectangle(display_frame, det.box, cv::Scalar(0, 255, 0), 2);
                std::string label = std::to_string(det.class_id) + ": " + std::to_string(det.conf);
                cv::putText(display_frame, label, cv::Point(det.box.x, det.box.y - 10),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
            }

            ++nums_of_frame;
            current_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
            if (elapsed.count() > 1000) {
                fps = int(1.0 * nums_of_frame * 1000 / elapsed.count());
                nums_of_frame = 0;
                start_time = current_time;
            }

            // 超过1s绘制FPS，并重制秒表
            cv::putText(display_frame, 
                "FPS: " + std::to_string(fps), 
                cv::Point(10, 10),
                cv::FONT_HERSHEY_SIMPLEX, 
                0.5, 
                cv::Scalar(0, 0, 255), 
                2
            );
            
            cv::imwrite("/app/assets/output_result.jpg", display_frame);
            // cv::imshow("YOLOv8 CPU Real-Time", display_frame);
            // if (cv::waitKey(1) == 27) running = false; // ESC 退出
        }

        
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    cv::destroyAllWindows();
}
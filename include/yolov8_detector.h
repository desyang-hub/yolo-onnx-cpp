#pragma once

// yolov8_detector.cpp
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>


#ifdef _WIN32
    #include <windows.h>
#else
    #include <iconv.h> // Linux/macOS 通常需要这个，或者自己实现 UTF-8 解析
    #include <cstring>
    #include <stdexcept>
#endif

std::wstring to_wide_string(const char* input) {
    if (!input) return L"";

#ifdef _WIN32
    // --- Windows 路径 ---
    // 假设输入是 UTF-8 (现代跨平台软件通常统一用 UTF-8)
    int len = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
    if (len == 0) return L"";
    
    std::wstring output(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, input, -1, &output[0], len);
    return output;

#else
    // --- Linux / macOS 路径 ---
    // 在 Linux/macOS 上，wchar_t 通常是 UTF-32，而 char* 通常是 UTF-8
    // 我们可以使用 iconv 或者直接手写 UTF-8 转 UTF-32 (如果确定是 UTF-8)
    
    // 方法 A: 使用 iconv (通用性强)
    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    if (cd == (iconv_t)-1) {
        // 如果失败，尝试其他名称，不同系统对 wchar_t 的 iconv 名称不同
        // 有些系统是 "UTF-32LE" 或 "UTF-32BE"
        // 这里简化处理，实际生产环境需更严谨
        return L""; 
    }

    size_t in_bytes_left = strlen(input);
    size_t out_bytes_left = in_bytes_left * 4; // wchar_t 最大 4 字节
    std::vector<wchar_t> buffer(out_bytes_left / sizeof(wchar_t));
    
    char* in_buf = const_cast<char*>(input);
    char* out_buf = reinterpret_cast<char*>(buffer.data());

    size_t result = iconv(cd, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
    iconv_close(cd);

    if (result == (size_t)-1) return L"";

    // 计算实际长度
    size_t wide_len = (out_bytes_left > 0) ? (buffer.size() - out_bytes_left / sizeof(wchar_t)) : buffer.size();
    // 简单处理：寻找结束符或根据字节计算，此处省略复杂边界检查
    // 更简单的做法：在 Linux 上，如果确定是 UTF-8，可以直接用现成的转换函数
    
    // 方法 B (推荐): 在 Linux/macOS 上，如果只是简单的 UTF-8 -> wstring
    // 很多现代编译器支持 mbstowcs，前提是 locale 设置正确
    setlocale(LC_ALL, ""); 
    size_t len = mbstowcs(nullptr, input, 0);
    if (len == (size_t)-1) return L"";
    
    std::wstring output(len, 0);
    mbstowcs(&output[0], input, len);
    return output;
#endif
}

class YOLOv8Detector {
private:
    Ort::Env env;
    Ort::Session* session;
    std::vector<float> input_tensor_values;
    std::vector<int64_t> input_shape = {1, 3, 640, 640}; // NCHW
    Ort::MemoryInfo memory_info;
    cv::Size input_size = cv::Size(640, 640);

public:
    YOLOv8Detector(const std::string& model_path) 
        : env(ORT_LOGGING_LEVEL_WARNING, "YOLOv8"),
          memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
        
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(2);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        // auto w_model_path = to_wide_string(model_path.c_str()).data();
        session = new Ort::Session(env, model_path.data(), session_options);

        input_tensor_values.resize(3 * input_size.height * input_size.width);
    }

    ~YOLOv8Detector() {
        delete session;
    }

    // 预处理：BGR -> RGB -> CHW -> 归一化
    void preprocess(const cv::Mat& frame, std::vector<float>& input_tensor_values) {
        cv::Mat resized, rgb, normalized;
        cv::resize(frame, resized, input_size);
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
        rgb.convertTo(normalized, CV_32F, 1.0f / 255.0f);

        // HWC -> CHW
        std::vector<cv::Mat> channels(3);
        cv::split(normalized, channels);
        size_t hw = input_size.height * input_size.width;
        for (int c = 0; c < 3; ++c) {
            memcpy(input_tensor_values.data() + c * hw, channels[c].ptr<float>(), hw * sizeof(float));
        }
    }

    struct Detection {
        cv::Rect box;
        float conf;
        int class_id;
    };

    std::vector<Detection> run_inference(const cv::Mat& frame) {
        preprocess(frame, input_tensor_values);

        auto input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(), input_tensor_values.size(),
            input_shape.data(), input_shape.size()
        );

        const char* input_names[] = {"images"};
        const char* output_names[] = {"output0"};

        auto start = std::chrono::steady_clock::now();
        auto output_tensors = session->Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 1
        );
        auto end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        // std::cout << "Inference: " << ms << " ms" << std::endl;

        // 解析输出: (1, 84, 8400) for YOLOv8
        auto output = output_tensors[0].GetTensorData<float>();
        std::vector<int64_t> output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
        int num_boxes = output_shape[2]; // 8400
        std::vector<Detection> detections;

        const float conf_threshold = 0.5f;
        const float nms_threshold = 0.45f;

        std::vector<cv::Rect> boxes;
        std::vector<float> scores;
        std::vector<int> class_ids;

        for (int i = 0; i < num_boxes; ++i) {
            const float* box_data = output + i * 84; // 每个 box 有 84 个值（4 box + 80 cls）
            float obj_conf = box_data[4];
            if (obj_conf < conf_threshold) continue;

            int class_id = 0;
            float max_class_conf = 0;
            for (int c = 0; c < 80; ++c) {
                float cls_conf = box_data[5 + c];
                if (cls_conf > max_class_conf) {
                    max_class_conf = cls_conf;
                    class_id = c;
                }
            }

            float conf = obj_conf * max_class_conf;
            if (conf < conf_threshold) continue;

            float x = box_data[0];
            float y = box_data[1];
            float w = box_data[2];
            float h = box_data[3];

            int left = (int)(x - w / 2);
            int top = (int)(y - h / 2);
            int width = (int)w;
            int height = (int)h;

            boxes.push_back(cv::Rect(left, top, width, height));
            scores.push_back(conf);
            class_ids.push_back(class_id);
        }

        // NMS
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, scores, conf_threshold, nms_threshold, indices);

        for (int idx : indices) {
            detections.push_back({boxes[idx], scores[idx], class_ids[idx]});
        }

        return detections;
    }
};
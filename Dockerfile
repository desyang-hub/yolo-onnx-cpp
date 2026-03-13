# 使用 Ubuntu 22.04 作为基础镜像
FROM ubuntu:22.04

# 避免交互式提示
ENV DEBIAN_FRONTEND=noninteractive

# 安装系统依赖
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    libopencv-dev \
    ca-certificates \
    wget \
    tar \
    && rm -rf /var/lib/apt/lists/*

# ==============================
# 下载并安装 ONNX Runtime CPU 版本（C++ API）
# ==============================
ENV ONNXRUNTIME_VERSION=1.12.1
ENV ONNXRUNTIME_DIR=/opt/onnxruntime

# 方式一：自己下载好后放到lib/onnxruntime-linux-x64-1.12.1下
# COPY lib/onnxruntime-linux-x64-1.12.1 /opt/onnxruntime

# 方式二：直接网络下载
RUN mkdir -p ${ONNXRUNTIME_DIR} && \
    cd /tmp && \
    wget https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz && \
    tar -xzf onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz --strip-components=1 -C ${ONNXRUNTIME_DIR} && \
    rm -f onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz

# 将 ONNX Runtime 的 lib 加入 LD_LIBRARY_PATH
ENV LD_LIBRARY_PATH=${ONNXRUNTIME_DIR}/lib:${LD_LIBRARY_PATH}

# 设置工作目录
WORKDIR /app

# 复制源码和 CMakeLists.txt
COPY CMakeLists.txt .
COPY include/ ./include/
COPY src/ ./src/
# COPY models/ ./models/   # 如果你有默认模型（可选）

# 构建项目
RUN mkdir build && cd build && \
    cmake .. \
        -DONNXRUNTIME_ROOT=${ONNXRUNTIME_DIR} \
        -DCMAKE_BUILD_TYPE=Release && \
    make -j4

# 默认运行推理程序
CMD ["./build/yolo_onnx_cpu"]
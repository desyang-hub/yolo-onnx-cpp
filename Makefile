all:
	mkdir -p build && \
	cd build && \
	cmake .. -DONNXRUNTIME_ROOT=lib/onnxruntime-linux-x64-1.12.1 && \
	make

# 运行（确保 models/yolov8n.onnx 存在）
run:
	./build/yolo_onnx_cpu
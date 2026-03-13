原来的逻辑是运行在有界面的系统中，直接用的cv::show
// cv::imshow("YOLOv8 CPU Real-Time", display_frame);
// if (cv::waitKey(1) == 27) running = false; // ESC 退出

docker内部无显示环境，这里就将结果导出到这里
cv::imwrite("/app/assets/output_result.jpg", display_frame);
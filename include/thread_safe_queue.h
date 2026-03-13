#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    // 只保留最新一帧：清空旧帧，只留最后一个
    void push_keep_latest(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        while (!data_queue.empty()) data_queue.pop(); // 丢弃所有旧帧
        data_queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{ return !data_queue.empty(); });
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.size();
    }
};
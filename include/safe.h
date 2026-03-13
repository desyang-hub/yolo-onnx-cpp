/**
 * @FilePath     : /yolo-onnx-cpp/include/safe.h
 * @Description  :  
 * @Author       : desyang
 * @Date         : 2026-03-13 10:06:16
 * @LastEditors  : desyang
 * @LastEditTime : 2026-03-13 11:41:48
**/
#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <condition_variable>

namespace safe
{

template<class T>
class Queue
{
private:
    std::queue<T> m_que;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_stop{false};
    

    using Reference = T&;
    using const_Reference = const T&;
    using rvalue_Reference = T&&;
    
public:
    Queue();
    virtual ~Queue();

    Reference front() const;

    Reference back() const;

    Reference emplace(const_Reference element);

    Reference emplace(rvalue_Reference element);

    bool empty() const;

    bool pop(Reference val) const;

    void push(const_Reference element);

    void push(rvalue_Reference element);

    size_t size() const;
};

template<class T>
Queue<T>::Queue() {
}

template<class T>
Queue<T>::~Queue() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    // 通知所有wait pop停止
    m_condition.notify_all();
}

template<class T>
Queue<T>::Reference Queue<T>::front() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.front();
}

template<class T>
Queue<T>::Reference Queue<T>::back() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.back();
}

template<class T>
Queue<T>::Reference Queue<T>::emplace(const_Reference element) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.emplace(element);
}

template<class T>
Queue<T>::Reference Queue<T>::emplace(rvalue_Reference element) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.emplace(std::move(element));
}

template<class T>
bool Queue<T>::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.empty();
}

template<class T>
bool Queue<T>::pop(Reference val) const {
    std::unique_lock<std::mutex> lock(m_mutex);

    // 元素未抵达的时候，pop需要进行阻塞
    m_condition.wait(lock, [this]() {
        return !m_que.empty() || m_stop;
    });

    if (m_stop) {
        return false;
    }
    else {
        val = std::move(m_que.front());
        m_que.pop();
        return true;
    }
}

template<class T>
void Queue<T>::push(const_Reference element) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_que.push(element);
    }
    // m_condition 无需上锁
    m_condition.notify_one();
}

template<class T>
void Queue<T>::push(rvalue_Reference element) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_que.push(std::move(element));
    }
    // m_condition 无需上锁
    m_condition.notify_one();
}

template<class T>
size_t Queue<T>::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_que.size();
}

    
} // namespace safe
#include "include/queue.h"

Queue::Queue(size_t max_size) : max_size_(max_size), eof_(false) {
    printf("Init queue %ld", max_size_);
}

Queue::~Queue() {
    std::lock_guard<std::mutex> lock(mtx_);
    while (!q_.empty()) {
        AVFrame* frame = q_.front();
        av_frame_free(&frame);
        q_.pop();
    }
}

void Queue::Push(AVFrame* frame) {
    std::unique_lock<std::mutex> lock(mtx_);

    cv_push_.wait(lock, [this]() { return q_.size() < max_size_; });

    q_.push(frame);

    cv_pop_.notify_one();
}

AVFrame* Queue::Pop() {
    std::unique_lock<std::mutex> lock(mtx_);

    cv_pop_.wait(lock, [this]() { return !q_.empty() || eof_; });

    if (q_.empty()) return nullptr;

    AVFrame* frame = q_.front();
    q_.pop();
    
    cv_push_.notify_one();

    return frame;
}

void Queue::SignalEOF() {
    std::lock_guard<std::mutex> lock(mtx_);
    eof_ = true;
    cv_pop_.notify_all();
}

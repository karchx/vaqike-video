#ifndef QUEUE_H
#define QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <iostream>
extern "C" {
    #include <libswscale/swscale.h>
}

class Queue {
public:
    // 30fps
    explicit Queue(size_t max_size = 30);
   ~Queue();
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void Push(AVFrame* frame);
    AVFrame* Pop();
    void SignalEOF();

private:
    std::queue<AVFrame*> q_;
    std::mutex mtx_;
    std::condition_variable cv_pop_;
    std::condition_variable cv_push_;
    size_t max_size_;
    bool eof_;
};

#endif // QUEUE_H

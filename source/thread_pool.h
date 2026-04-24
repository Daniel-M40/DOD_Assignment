#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void Submit(std::function<void()> task);
    void WaitAll();

private:
    std::vector<std::thread> mThreads;
    std::queue<std::function<void()>> mTasks;
    std::mutex mQueueMutex;
    std::condition_variable mCv;
    std::condition_variable mCvFinished;
    std::atomic<uint32_t> mPending{ 0 };
    bool mStop = false;
};
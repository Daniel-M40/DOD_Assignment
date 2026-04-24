#include "thread_pool.h"

ThreadPool::ThreadPool(size_t num_threads)
{
    for (size_t i = 0; i < num_threads; ++i)
    {
        mThreads.emplace_back([this]
        {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mQueueMutex);
                    mCv.wait(lock, [this]
                    {
                        return !mTasks.empty() || mStop;
                    });

                    if (mStop && mTasks.empty())
                    {
	                    return;
                    }

                    task = std::move(mTasks.front());
                    mTasks.pop();
                }

                task();
                --mPending;
                mCvFinished.notify_one();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mStop = true;
    }
    mCv.notify_all();

    for (auto& thread : mThreads)
    {
        thread.join();
    }
}

void ThreadPool::Submit(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(mQueueMutex);
        mPending++;
        mTasks.emplace(std::move(task));
    }
    mCv.notify_one();
}

void ThreadPool::WaitAll()
{
    while (mPending > 0)
    {
        std::this_thread::yield();
    }
}
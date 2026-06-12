#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief 线程池类，负责管理线程生命周期和任务分发
 */
class ThreadPool{
public:
    ThreadPool(int threadCount = 10);
    ~ThreadPool();
    void SubmitTask(std::function<void()> task);
    double GetThreadPoolUsage();

private:
    void WorkerThread();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::atomic<int> activeThreads;
    int threadCount;
};

#endif // THREAD_POOL_H
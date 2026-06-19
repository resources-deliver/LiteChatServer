#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <iostream>

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
    std::vector<std::thread> workers;  // 线程池
    std::queue<std::function<void()>> taskQueue;  // 任务队列
    std::mutex queueMutex;  // 互斥锁
    std::condition_variable condition;  // 条件变量
    std::atomic<bool> stop;  // 原子性线程池状态
    std::atomic<int> activeThreads;  // 原子性活跃线程数
    int threadCount;  // 线程个数
};

#endif // THREAD_POOL_H
#include "thread_pool.h"

/**
 * @brief ThreadPool构造函数，创建指定数量的工作线程
 * @param threadCount 工作线程数量，默认10
 */
ThreadPool::ThreadPool(int threadCount)
    : stop(false)
    , activeThreads(0)
    , threadCount(threadCount) {
    for(int i = 0; i < threadCount; ++i){
        workers.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

/**
 * @brief ThreadPool析构函数，停止所有工作线程并等待完成
 */
ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    for(std::thread& worker : workers){
        if(worker.joinable()){
            worker.join();
        }
    }
}

/**
 * @brief 提交任务到线程池
 * @param task 要执行的任务函数
 */
void ThreadPool::SubmitTask(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(std::move(task));
    }
    condition.notify_one();
}

/**
 * @brief 获取线程池使用率
 * @return 返回线程池使用率（活跃线程数/总线程数）
 */
double ThreadPool::GetThreadPoolUsage(){
    if(threadCount == 0){
        return 0.0;
    }
    return static_cast<double>(activeThreads.load()) / threadCount;
}

/**
 * @brief 工作线程函数，从任务队列获取并执行任务
 */
void ThreadPool::WorkerThread(){
    while(true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this](){
                return stop.load() || !taskQueue.empty();
            });
            
            if(stop.load() && taskQueue.empty()){
                return;
            }
            
            task = std::move(taskQueue.front());
            taskQueue.pop();
            activeThreads++;
        }
        
        task();
        activeThreads--;
    }
}

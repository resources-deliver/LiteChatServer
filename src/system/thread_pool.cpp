#include "thread_pool.h"

/**
 * @brief ThreadPool构造函数，初始化类内私有属性+创建指定数量的线程进入线程池
 * @param threadCount 线程个数，默认10
 */
ThreadPool::ThreadPool(int threadCount)
    : stop(false)
    , activeThreads(0)
    , threadCount(threadCount)
{
    for(int i = 0; i < threadCount; ++i){  // 创建指定数量的线程
        workers.emplace_back(&ThreadPool::WorkerThread, this);  // 进入线程池
    }
}

/**
 * @brief ThreadPool析构函数，停止所有工作线程并等待完成
 */
ThreadPool::~ThreadPool(){
    stop = true;  // 设置线程池状态为停止
    condition.notify_all();  // 使用条件变量通知所有线程
    for(std::thread& worker : workers){  // 遍历线程池中的线程
        if(worker.joinable()){  // 如果线程在运行中
            worker.join();  // 等待线程执行完成
        }
    }
}

/**
 * @brief 提交任务到线程池
 * @param task 要执行的任务函数
 */
void ThreadPool::SubmitTask(std::function<void()> task){
    {  // 作用域
        std::lock_guard<std::mutex> lock(queueMutex);  // 加锁使得共享资源同一时间只有一个线程访问
        taskQueue.push(std::move(task));  // 将任务添加到任务队列
    }
    condition.notify_one();  // 使用条件变量通知1个线程：等待连接池有可用连接的线程
}

/**
 * @brief 获取线程池使用率
 * @return 返回线程池使用率（活跃线程数/总线程数）
 */
double ThreadPool::GetThreadPoolUsage(){
    if(threadCount == 0){  // 如果线程池为空
        return 0.0;
    }
    auto used = static_cast<double>(activeThreads.load()) / threadCount;  // 计算活跃线程数占总线程数的比例
    return used;
}

/**
 * @brief 线程进入的工作函数，从任务队列获取并执行任务
 */
void ThreadPool::WorkerThread(){
    while(true){  // 若线程池未停止
        std::function<void()> task;  // 任务函数
        {  // 作用域
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this](){ return stop.load() || !taskQueue.empty(); });  // 使用条件变量等待连接池有可用连接
            if(stop.load() && taskQueue.empty()){  // 如果线程池已停止且任务队列为空
                std::cout << "[ThreadPool::WorkerThread]线程池已停止，任务队列为空" << std::endl;
                return;
            }
            task = std::move(taskQueue.front());  // 从任务队列获取任务
            taskQueue.pop();  // 从任务队列移除任务
            activeThreads++;  // 增加活跃线程数
        }
        task();  // 执行任务
        activeThreads--;  // 减少活跃线程数
    }
}

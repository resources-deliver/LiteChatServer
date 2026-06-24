#ifndef SERVER_MONITOR_H
#define SERVER_MONITOR_H

#include <thread>
#include <atomic>
#include <string>

class DBManager;
class SessionManager;
class ThreadPool;

/**
 * @brief 服务器监控类，负责定期采集服务器状态指标并记录到日志
 */
class ServerMonitor{
public:
    ServerMonitor(DBManager* dbManager, SessionManager* sessionManager, ThreadPool* threadPool, std::atomic<int>* currentConnections);
    ~ServerMonitor();
    void StartMonitor();
    void StopMonitor();
    void CollectStatus();
    void RecordMonitorData(const std::string& data);

private:
    void MonitorThreadFunc();

private:
    DBManager* dbManager;  // 数据库管理器
    SessionManager* sessionManager;  // 会话管理器
    ThreadPool* threadPool;  // 线程池
    std::atomic<int>* currentConnections;  // 当前连接数
    int monitorInterval;  // 监控间隔（秒）
    std::thread* monitorThread;  // 监控线程
    std::atomic<bool> isRunning;  // 监控线程是否运行中
};

#endif // SERVER_MONITOR_H
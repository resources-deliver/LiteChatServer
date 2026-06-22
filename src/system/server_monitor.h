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
    DBManager* dbManager;
    SessionManager* sessionManager;
    ThreadPool* threadPool;
    std::atomic<int>* currentConnections;
    int monitorInterval;
    std::thread* monitorThread;
    std::atomic<bool> isRunning;
};

#endif // SERVER_MONITOR_H
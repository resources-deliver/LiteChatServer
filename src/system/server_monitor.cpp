#include "server_monitor.h"
#include "session_manager.h"
#include "db_manager.h"
#include "thread_pool.h"
#include "server_logger.h"
#include <sstream>
#include <chrono>
#include <jsoncpp/json/json.h>

/**
 * @brief ServerMonitor构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 * @param sessionManager 会话管理器指针
 * @param threadPool 线程池指针
 * @param currentConnections 当前连接数原子变量指针
 */
ServerMonitor::ServerMonitor(
    DBManager* dbManager,
    SessionManager* sessionManager,
    ThreadPool* threadPool,
    std::atomic<int>* currentConnections
)
    : dbManager(dbManager)
    , sessionManager(sessionManager)
    , threadPool(threadPool)
    , currentConnections(currentConnections)
    , monitorInterval(60)
    , monitorThread(nullptr)
    , isRunning(false)
{
}

/**
 * @brief ServerMonitor析构函数，确保监控线程停止并释放资源
 */
ServerMonitor::~ServerMonitor(){
    StopMonitor();
}

/**
 * @brief 启动监控线程，开始定期采集服务器状态
 */
void ServerMonitor::StartMonitor(){
    if(isRunning.load()){
        return;
    }
    isRunning.store(true);
    monitorThread = new std::thread(&ServerMonitor::MonitorThreadFunc, this);
}

/**
 * @brief 停止监控线程并等待其退出
 */
void ServerMonitor::StopMonitor(){
    if(!isRunning.load()){
        return;
    }
    isRunning.store(false);
    if(monitorThread){
        if(monitorThread->joinable()){
            monitorThread->join();
        }
        delete monitorThread;
        monitorThread = nullptr;
    }
}

/**
 * @brief 监控线程主函数，每隔 monitorInterval 秒采集一次服务器状态
 */
void ServerMonitor::MonitorThreadFunc(){
    while(isRunning.load()){
        std::this_thread::sleep_for(std::chrono::seconds(monitorInterval));
        if(!isRunning.load()){
            break;
        }
        CollectStatus();
    }
}

/**
 * @brief 采集当前服务器状态指标，构建JSON数据并记录到日志
 */
void ServerMonitor::CollectStatus(){
    Json::Value data;
    int onlineUsers = 0;
    if(sessionManager){
        onlineUsers = sessionManager->GetOnlineUserCount();
    }
    data["online_users"] = onlineUsers;
    int connections = 0;
    if(currentConnections){
        connections = currentConnections->load();
    }
    data["connections"] = connections;
    double poolUsage = 0.0;
    if(threadPool){
        poolUsage = threadPool->GetThreadPoolUsage();
    }
    data["thread_pool_usage"] = poolUsage;
    std::string dbStatus = "unknown";
    if(dbManager){
        if(dbManager->ValidateConnection()){
            dbStatus = "normal";
        }
        else{
            dbStatus = "error";
        }
    }
    data["db_status"] = dbStatus;
    Json::FastWriter writer;
    std::string jsonData = writer.write(data);
    RecordMonitorData(jsonData);
}

/**
 * @brief 将监控数据记录到日志中
 * @param data 监控数据JSON字符串
 */
void ServerMonitor::RecordMonitorData(const std::string& data){
    ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "Monitor", data);
}
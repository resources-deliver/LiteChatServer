#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>
#include "client_session.h"
#include "thread_pool.h"
#include "db_manager.h"
#include "session_manager.h"
#include "user_manager.h"

/**
 * @brief 服务器类，负责管理客户端连接和消息收发
 */
class Server{
public:
    Server();
    ~Server();
    void SetDBManager(DBManager* dbMgr);
    bool Start();
    void Stop();
    void AcceptConnections();
    void HandleClient(int clientSocket);
    std::string ReceiveData(int clientSocket);
    bool SendData(int clientSocket, const std::string& data);
    uint32_t ParseMessageHeader(const char* header);
    std::string PackageMessage(const std::string& body);
    void AddSession(int clientSocket, ClientSession* session);
    void RemoveSession(int clientSocket);
    ClientSession* GetSession(int clientSocket);
    ClientSession* GetSessionByUsername(const std::string& username);
    int GetCurrentConnections();

private:
    void StartHeartbeatCheck();
    void HeartbeatCheckThread();
    void DispatchRequest(int clientSocket, const std::string& data);

private:
    int serverSocket;  // 服务器套接字描述符
    std::string serverIP;  // 服务器IP地址
    int serverPort;  // 服务器端口号
    int maxConnections;  // 最大连接数
    std::atomic<int> currentConnections;  // 当前连接数
    ThreadPool* threadPool;  // 线程池指针
    DBManager* dbManager;  // 数据库管理指针
    SessionManager* sessionManager;  // 会话管理指针
    UserManager* userManager;  // 用户管理指针
    std::thread* heartbeatThread;  // 心跳检查线程指针
    std::unordered_map<int, ClientSession*> sessionMap;  // 会话映射表，键为客户端套接字描述符，值为会话指针
    std::mutex sessionMutex;  // 会话映射表互斥锁
    bool running;  // 服务器运行状态
};

#endif // SERVER_H
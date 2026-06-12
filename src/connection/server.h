#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include "client_session.h"
#include "thread_pool.h"
#include "db_manager.h"

/**
 * @brief 服务器类，负责管理客户端连接和消息收发
 */
class Server{
public:
    Server();
    ~Server();
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
    int serverSocket;
    std::string serverIP;
    int serverPort;
    int maxConnections;
    std::atomic<int> currentConnections;
    ThreadPool* threadPool;
    DBManager* dbManager;
    std::unordered_map<int, ClientSession*> sessionMap;
    std::mutex sessionMutex;
    bool running;
};

#endif // SERVER_H
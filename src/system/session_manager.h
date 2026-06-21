#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "client_session.h"

/**
 * @brief 会话管理器类，负责管理所有客户端会话
 */
class SessionManager{
public:
    SessionManager();
    ~SessionManager();
    void AddSession(int socket, ClientSession* cliSession);
    void RemoveSession(int socket);
    ClientSession* GetSession(int socket);
    ClientSession* GetSessionByUsername(const std::string& username);
    void BindUsername(int socket, const std::string& username);
    std::unordered_map<int, ClientSession*> GetAllSessions();
    std::mutex& GetMutex();

private:
    std::unordered_map<int, ClientSession*> cliSessionMap;  // 会话映射表，键为客户端socket，值为会话指针
    std::unordered_map<std::string, int> usernameToSocket;  // 用户名到客户端socket的映射表
    std::mutex sessionMutex;  // 会话互斥锁
};

#endif // SESSION_MANAGER_H
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

    void AddSession(int socket, ClientSession* session);
    void RemoveSession(int socket);
    ClientSession* GetSession(int socket);
    ClientSession* GetSessionByUsername(const std::string& username);
    std::unordered_map<int, ClientSession*> GetAllSessions();
    std::mutex& GetMutex();

private:
    std::unordered_map<int, ClientSession*> sessionMap;
    std::unordered_map<std::string, int> usernameToSocket;
    std::mutex sessionMutex;
};

#endif // SESSION_MANAGER_H
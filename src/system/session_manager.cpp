#include "session_manager.h"
#include <iostream>

/**
 * @brief SessionManager构造函数
 */
SessionManager::SessionManager(){
}

/**
 * @brief SessionManager析构函数，清理所有会话
 */
SessionManager::~SessionManager(){
    std::lock_guard<std::mutex> lock(sessionMutex);
    for(auto& pair : sessionMap){
        delete pair.second;
    }
    sessionMap.clear();
    usernameToSocket.clear();
}

/**
 * @brief 添加会话
 * @param socket 客户端socket
 * @param session 会话对象指针
 */
void SessionManager::AddSession(int socket, ClientSession* session){
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessionMap[socket] = session;
    if(!session->GetUsername().empty()){
        usernameToSocket[session->GetUsername()] = socket;
    }
}

/**
 * @brief 移除会话
 * @param socket 客户端socket
 */
void SessionManager::RemoveSession(int socket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessionMap.find(socket);
    if(it != sessionMap.end()){
        std::string username = it->second->GetUsername();
        if(!username.empty()){
            usernameToSocket.erase(username);
        }
        delete it->second;
        sessionMap.erase(it);
    }
}

/**
 * @brief 根据socket获取会话
 * @param socket 客户端socket
 * @return 返回会话对象指针，不存在返回nullptr
 */
ClientSession* SessionManager::GetSession(int socket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessionMap.find(socket);
    if(it != sessionMap.end()){
        return it->second;
    }
    return nullptr;
}

/**
 * @brief 根据用户名获取会话
 * @param username 用户名
 * @return 返回会话对象指针，不存在返回nullptr
 */
ClientSession* SessionManager::GetSessionByUsername(const std::string& username){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = usernameToSocket.find(username);
    if(it != usernameToSocket.end()){
        auto sessionIt = sessionMap.find(it->second);
        if(sessionIt != sessionMap.end()){
            return sessionIt->second;
        }
    }
    return nullptr;
}

/**
 * @brief 获取所有会话
 * @return 返回所有会话的map
 */
std::unordered_map<int, ClientSession*> SessionManager::GetAllSessions(){
    std::lock_guard<std::mutex> lock(sessionMutex);
    return sessionMap;
}

/**
 * @brief 获取会话互斥锁
 * @return 返回互斥锁引用
 */
std::mutex& SessionManager::GetMutex(){
    return sessionMutex;
}
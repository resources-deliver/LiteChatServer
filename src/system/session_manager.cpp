#include "session_manager.h"
#include <iostream>

/**
 * @brief SessionManager构造函数
 */
SessionManager::SessionManager(){}

/**
 * @brief SessionManager析构函数，清理所有会话
 */
SessionManager::~SessionManager(){
    std::lock_guard<std::mutex> lock(sessionMutex);  // 加锁使得共享资源同一时间只有一个线程访问
    for(auto& pair : cliSessionMap){  // 遍历会话映射表
        delete pair.second;  // 删除会话对象
    }
    cliSessionMap.clear();  // 清空会话映射表
    usernameToSocket.clear();  // 清空用户名到客户端socket的映射表
}

/**
 * @brief 添加会话
 * @param socket 客户端socket
 * @param session 会话对象指针
 */
void SessionManager::AddSession(int socket, ClientSession* cliSession){
    std::lock_guard<std::mutex> lock(sessionMutex);
    cliSessionMap[socket] = cliSession;  // 将会话对象添加到会话映射表中
    if(!cliSession->GetUsername().empty()){  // 如果会话对象有用户名
        usernameToSocket[cliSession->GetUsername()] = socket;  // 将用户名添加到用户名到客户端socket的映射表中
    }
}

/**
 * @brief 移除会话
 * @param socket 客户端socket
 */
void SessionManager::RemoveSession(int socket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = cliSessionMap.find(socket);  // 查找会话映射表中是否存在该socket的会话
    if(it != cliSessionMap.end()){  // 如果存在
        std::string username = it->second->GetUsername();  // 获取会话对象的用户名
        if(!username.empty()){  // 如果用户名不为空
            auto nameIt = usernameToSocket.find(username);  // 查找用户名到客户端socket的映射表中是否存在该用户名
            if(nameIt != usernameToSocket.end() && nameIt->second == socket){  // 如果存在且用户名对应的socket与当前socket相同
                usernameToSocket.erase(nameIt);  // 从用户名到客户端socket的映射表中移除该用户名
            }
        }
        delete it->second;  // 删除会话对象
        cliSessionMap.erase(it);  // 从会话映射表中移除该socket的会话
    }
}

/**
 * @brief 根据socket获取会话
 * @param socket 客户端socket
 * @return 返回会话对象指针，不存在返回nullptr
 */
ClientSession* SessionManager::GetSession(int socket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = cliSessionMap.find(socket);  // 查找会话映射表中是否存在该socket的会话
    if(it != cliSessionMap.end()){  // 如果存在
        return it->second;  // 返回会话对象指针
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
    auto it = usernameToSocket.find(username);  // 查找用户名到客户端socket的映射表中是否存在该用户名
    if(it != usernameToSocket.end()){  // 如果存在
        auto sessionIt = cliSessionMap.find(it->second);  // 查找会话映射表中是否存在该socket的会话
        if(sessionIt != cliSessionMap.end()){  // 如果存在
            return sessionIt->second;  // 返回会话对象指针
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
    return cliSessionMap;
}

/**
 * @brief 获取在线用户数量（已绑定用户名的会话数）
 * @return 在线用户数量
 */
int SessionManager::GetOnlineUserCount(){
    std::lock_guard<std::mutex> lock(sessionMutex);
    return static_cast<int>(usernameToSocket.size());
}

/**
 * @brief 获取会话互斥锁
 * @return 返回互斥锁引用
 */
std::mutex& SessionManager::GetMutex(){
    return sessionMutex;
}

/**
 * @brief 绑定用户名到会话（登录成功后调用）
 * @param socket 客户端socket
 * @param username 用户名
 */
void SessionManager::BindUsername(int socket, const std::string& username){
    std::lock_guard<std::mutex> lock(sessionMutex);
    usernameToSocket[username] = socket;
}
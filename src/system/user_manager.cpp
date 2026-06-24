#include "user_manager.h"
#include "server_logger.h"
#include <iostream>
#include <regex>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <jsoncpp/json/json.h>
#include <unistd.h>

/**
 * @brief UserManager构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 * @param sessionManager 会话管理器指针
 */
UserManager::UserManager(DBManager* dbManager, SessionManager* sessionManager)
    : dbManager(dbManager)
    , sessionManager(sessionManager)
    , userDAO(new UserDAO(dbManager))
{
}

/**
 * @brief UserManager析构函数，清理资源
 */
UserManager::~UserManager(){
    if(userDAO){
        delete userDAO;
    }
}

/**
 * @brief 处理客户端发来的注册请求
 * @param request 客户端发来的注册请求信息结构体
 * @return 返回服务器返回给客户端的响应信息结构体
 */
Response UserManager::HandleRegister(const Request& request){
    Response response;
    if(!userDAO){
        response.code = 5001;
        response.msg = "服务器内部错误";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理注册请求时服务器内部错误");
        return response;
    }
    if(!ValidateUsername(request.username)){
        response.code = 2001;
        response.msg = "用户名格式不合法";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理注册请求时用户名格式不合法");
        return response;
    }
    if(!ValidatePassword(request.password)){
        response.code = 2001;
        response.msg = "密码格式不合法";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理注册请求时密码格式不合法");
        return response;
    }
    if(userDAO->CheckUsernameExists(request.username)){
        response.code = 2002;
        response.msg = "用户名已存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理注册请求时用户名已存在");
        return response;
    }
    std::string passwordHash = MD5Encrypt(request.password);
    if(!userDAO->InsertUser(request.username, passwordHash)){
        response.code = 5001;
        response.msg = "数据库操作失败";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理注册请求时数据库操作失败");
        return response;
    }
    response.code = 0;
    response.msg = "注册成功";
    ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserManager", "处理注册请求时用户注册成功：" + request.username);
    return response;
}

/**
 * @brief 处理客户端发来的登录请求
 * @param request 客户端发来的登录请求信息结构体
 * @return 返回服务器返回给客户端的响应信息结构体
 */
Response UserManager::HandleLogin(const Request& request){
    Response response;
    if(!ValidateUsername(request.username)){
        response.code = 2001;
        response.msg = "用户名格式不合法";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理登录请求时用户名格式不合法");
        return response;
    }
    int userId = 0;
    std::string passwordHash;
    if(!userDAO->GetUserByUsername(request.username, userId, passwordHash)){
        response.code = 2003;
        response.msg = "用户不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理登录请求时用户不存在");
        return response;
    }
    std::string inputHash = MD5Encrypt(request.password);
    if(inputHash != passwordHash){
        response.code = 2004;
        response.msg = "密码错误";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理登录请求时密码错误");
        return response;
    }

    bool isOnline = false;
    userDAO->GetUserOnlineStatus(userId, isOnline);
    if(isOnline){
        userDAO->UpdateOnlineStatus(userId, false);
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserManager", "用户" + request.username + "已在其他地方登录，已强制下线");
        ClientSession* oldSession = sessionManager->GetSessionByUsername(request.username);
        if(oldSession){
            Json::Value kickJson;
            kickJson["type"] = "KICKED";
            kickJson["code"] = 2005;
            kickJson["msg"] = "账号已在其他地方登录";
            Json::FastWriter writer;
            std::string kickStr = writer.write(kickJson);
            int oldSocket = oldSession->GetSocket();
            if(sendCallback){
                sendCallback(oldSocket, kickStr);
            }
            oldSession->SetUsername("");
            shutdown(oldSocket, SHUT_RDWR);
        }
    }
    userDAO->UpdateOnlineStatus(userId, true);
    userDAO->UpdateLastLoginTime(userId);
    ClientSession* session = sessionManager->GetSession(request.clientSocket);
    if(session){
        session->SetUsername(request.username);
        sessionManager->BindUsername(request.clientSocket, request.username);
        session->UpdateHeartbeat();
    }
    response.code = 0;
    response.msg = "登录成功";
    ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserManager", "处理登录请求时用户登录成功：" + request.username);
    NotifyFriendsStatusChange(request.username, UserStatus::Online);
    return response;
}

/**
 * @brief 处理客户端发来的用户信息修改请求
 * @param request 客户端发来的用户信息修改请求信息结构体
 * @return 返回服务器返回给客户端的响应信息结构体
 */
Response UserManager::HandleUpdateUser(const Request& request){
    Response response;
    int userId = 0;
    std::string passwordHash;
    if(!userDAO->GetUserByUsername(request.username, userId, passwordHash)){
        response.code = 2003;
        response.msg = "用户不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时用户不存在");
        return response;
    }
    std::string verifyHash = MD5Encrypt(request.verifyPassword);
    if(verifyHash != passwordHash){
        response.code = 2006;
        response.msg = "验证密码错误";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时验证密码错误");
        return response;
    }
    bool updated = false;
    if(!request.newUsername.empty()){
        if(!ValidateUsername(request.newUsername)){
            response.code = 2001;
            response.msg = "新用户名格式不合法";
            ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时新用户名格式不合法");
            return response;
        }
        if(userDAO->CheckUsernameExists(request.newUsername)){
            response.code = 2002;
            response.msg = "新用户名已存在";
            ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时新用户名已存在");
            return response;
        }
        if(!userDAO->UpdateUsername(userId, request.newUsername)){
            response.code = 5001;
            response.msg = "数据库操作失败";
            ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时数据库操作失败");
            return response;
        }
        updated = true;
    }
    if(!request.newPassword.empty()){
        if(!ValidatePassword(request.newPassword)){
            response.code = 2001;
            response.msg = "新密码格式不合法";
            ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时新密码格式不合法");
            return response;
        }
        std::string newPasswordHash = MD5Encrypt(request.newPassword);
        if(!userDAO->UpdatePassword(userId, newPasswordHash)){
            response.code = 5001;
            response.msg = "数据库操作失败";
            ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时数据库操作失败");
            return response;
        }
        updated = true;
    }
    if(!updated){
        response.code = 1001;
        response.msg = "未修改任何信息";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户信息修改请求时未修改任何信息");
        return response;
    }
    response.code = 0;
    response.msg = "修改成功";
    ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserManager", "处理用户信息修改成功：" + request.username);
    return response;
}

/**
 * @brief 处理客户端发来的用户状态查询请求
 * @param request 客户端发来的用户状态查询请求信息结构体
 * @return 返回服务器返回给客户端的响应信息结构体
 */
Response UserManager::HandleQueryStatus(const Request& request){
    Response response;
    int userId = 0;
    std::string passwordHash;
    if(!userDAO->GetUserByUsername(request.username, userId, passwordHash)){
        response.code = 2003;
        response.msg = "用户不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户状态查询请求时用户不存在");
        return response;
    }
    bool isOnline = false;
    if(!userDAO->GetUserOnlineStatus(userId, isOnline)){
        response.code = 5002;
        response.msg = "数据库查询失败";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理用户状态查询请求时数据库查询失败");
        return response;
    }
    response.code = 0;
    response.msg = isOnline ? "在线" : "离线";
    Json::Value dataObj;
    dataObj["status"] = isOnline ? "online" : "offline";
    Json::FastWriter writer;
    response.data = writer.write(dataObj);
    ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserManager", "处理用户状态查询成功：" + request.username);
    return response;
}

/**
 * @brief 校验用户名格式
 * @param username 用户名
 * @return 格式合法返回true，不合法返回false
 */
bool UserManager::ValidateUsername(const std::string& username){
    std::regex pattern("^[a-zA-Z0-9_]{3,20}$");
    return std::regex_match(username, pattern);
}

/**
 * @brief 校验密码格式
 * @param password 密码
 * @return 格式合法返回true，不合法返回false
 */
bool UserManager::ValidatePassword(const std::string& password){
    return password.length() >= 6;
}

/**
 * @brief 通知好友状态变更
 * @param username 用户名
 * @param status 用户状态
 */
void UserManager::NotifyFriendsStatusChange(const std::string& username, UserStatus status){
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(username, userId)){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理好友状态变更通知时用户不存在");
        return;
    }
    std::vector<std::pair<std::string, bool>> friends;
    if(!userDAO->GetFriendsOfUser(userId, friends)){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserManager", "处理好友状态变更通知时数据库查询好友列表失败");
        return;
    }
    std::string statusStr = (status == UserStatus::Online) ? "online" : "offline";
    Json::Value notifyJson;
    notifyJson["type"] = "STATUS_NOTIFY";
    notifyJson["data"]["username"] = username;
    notifyJson["data"]["status"] = statusStr;
    Json::FastWriter writer;
    std::string notifyStr = writer.write(notifyJson);
    for(auto& friendPair : friends){
        std::string friendUsername = friendPair.first;
        bool isOnline = friendPair.second;
        if(isOnline){
            ClientSession* friendSession = sessionManager->GetSessionByUsername(friendUsername);
            if(friendSession && sendCallback){
                sendCallback(friendSession->GetSocket(), notifyStr);
            }
        }
    }
    ServerLogger::GetInstance().WriteLog(
        LogLevel::INFO, "UserManager", "处理好友状态变更通知时成功通知好友状态变更：" + username + " 状态：" + statusStr
    );
}

/**
 * @brief 更新用户在线状态
 * @param username 用户名
 * @param isOnline 在线状态（true=在线，false=离线）
 * @return 更新成功返回true，失败返回false
 */
bool UserManager::UpdateUserOnlineStatus(const std::string& username, bool isOnline){
    return userDAO->UpdateOnlineStatusByUsername(username, isOnline);
}

/**
 * @brief 设置发送数据的回调函数
 * @param callback 发送回调函数
 */
void UserManager::SetSendCallback(std::function<bool(int, const std::string&)> callback){
    sendCallback = callback;
}

/**
 * @brief MD5加密密码
 * @param password 明文密码
 * @return 返回MD5哈希值（32位十六进制字符串）
 */
std::string UserManager::MD5Encrypt(const std::string& password){
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if(ctx){
        EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
        EVP_DigestUpdate(ctx, password.c_str(), password.length());
        EVP_DigestFinal_ex(ctx, digest, &digestLength);
        EVP_MD_CTX_free(ctx);
    }
    std::stringstream ss;
    for(unsigned int i = 0; i < digestLength; ++i){
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)digest[i];
    }
    return ss.str();
}
#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include <functional>
#include "db_manager.h"
#include "session_manager.h"
#include "user_dao.h"

/**
 * @brief 响应信息结构体，用于存储服务器返回给客户端的响应信息
 */
struct Response{
    int code;  // 响应状态码
    std::string msg;  // 响应消息
    std::string data;  // 响应数据
};

/**
 * @brief 请求信息结构体，用于存储客户端发来的请求信息
 */
struct Request{
    std::string type;  // 请求类型
    std::string username;  // 用户名
    std::string password;  // 密码
    std::string newUsername;  // 新用户名
    std::string newPassword;  // 新密码
    std::string verifyPassword;  // 验证密码
    std::string targetUsername;  // 目标用户名
    std::string content;  // 消息内容
    int count;  // 查询条数
    int clientSocket;  // 客户端套接字描述符
};

/**
 * @brief 用户在线状态枚举，用于表示用户在线状态
 */
enum class UserStatus{
    Online,
    Offline
};

/**
 * @brief 用户管理类，负责处理用户注册、登录、信息修改等业务逻辑
 */
class UserManager{
public:
    UserManager(DBManager* dbManager, SessionManager* sessionManager);
    ~UserManager();
    Response HandleRegister(const Request& request);
    Response HandleLogin(const Request& request);
    Response HandleUpdateUser(const Request& request);
    Response HandleQueryStatus(const Request& request);
    bool ValidateUsername(const std::string& username);
    bool ValidatePassword(const std::string& password);
    void NotifyFriendsStatusChange(const std::string& username, UserStatus status);
    bool UpdateUserOnlineStatus(const std::string& username, bool isOnline);
    void SetSendCallback(std::function<bool(int, const std::string&)> callback);

private:
    std::string MD5Encrypt(const std::string& password);

private:
    DBManager* dbManager;  // 数据库管理器指针
    SessionManager* sessionManager;  // 会话管理器指针
    UserDAO* userDAO;  // 用户数据访问指针
    std::function<bool(int, const std::string&)> sendCallback;  // 发送回调函数指针
};

#endif // USER_MANAGER_H
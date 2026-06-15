#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include <string>
#include "db_manager.h"
#include "session_manager.h"
#include "user_dao.h"

struct Response{
    int code;
    std::string msg;
    std::string data;
};

struct Request{
    std::string type;
    std::string username;
    std::string password;
    std::string newUsername;
    std::string newPassword;
    std::string verifyPassword;
    int clientSocket;
};

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

private:
    DBManager* dbManager;
    SessionManager* sessionManager;
    UserDAO* userDAO;
    std::string MD5Encrypt(const std::string& password);
};

#endif // USER_MANAGER_H
#ifndef USER_DAO_H
#define USER_DAO_H

#include "db_manager.h"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief 用户数据访问类，负责用户相关的数据库操作
 */
class UserDAO{
public:
    UserDAO(DBManager* dbManager);
    ~UserDAO();
    bool InsertUser(const std::string& username, const std::string& passwordHash);
    bool GetUserByUsername(const std::string& username, int& userId, std::string& passwordHash);
    bool UpdateUsername(int userId, const std::string& newUsername);
    bool UpdatePassword(int userId, const std::string& newPasswordHash);
    bool UpdateOnlineStatus(int userId, bool isOnline);
    bool UpdateLastLoginTime(int userId);
    bool CheckUsernameExists(const std::string& username);
    bool GetUserOnlineStatus(int userId, bool& isOnline);
    bool UpdateOnlineStatusByUsername(const std::string& username, bool isOnline);
    bool GetUserIdByUsername(const std::string& username, int& userId);
    bool GetFriendsOfUser(int userId, std::vector<std::pair<std::string, bool>>& friends);

private:
    DBManager* dbManager;  // 数据库管理器指针
};

#endif // USER_DAO_H
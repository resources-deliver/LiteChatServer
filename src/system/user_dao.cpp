#include "user_dao.h"
#include <iostream>
#include <sstream>

/**
 * @brief UserDAO构造函数，初始化数据库管理器引用
 * @param dbManager 数据库管理器指针
 */
UserDAO::UserDAO(DBManager* dbManager)
    : dbManager(dbManager){
}

/**
 * @brief UserDAO析构函数
 */
UserDAO::~UserDAO(){
}

/**
 * @brief 插入新用户（注册）
 * @param username 用户名
 * @param passwordHash 密码MD5哈希值
 * @return 插入成功返回true，失败返回false
 */
bool UserDAO::InsertUser(const std::string& username, const std::string& passwordHash){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "INSERT INTO users (username, password_hash) VALUES ('"
        << username << "', '" << passwordHash << "')";
    
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 根据用户名查询用户信息和密码
 * @param username 用户名
 * @param userId 输出参数：用户ID
 * @param passwordHash 输出参数：密码哈希值
 * @return 查询成功返回true，用户不存在返回false
 */
bool UserDAO::GetUserByUsername(const std::string& username, int& userId, std::string& passwordHash){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "SELECT user_id, password_hash FROM users WHERE username = '"
        << username << "'";
    
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        dbManager->ReleaseConnection(conn);
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    
    userId = std::stoi(row[0]);
    passwordHash = row[1];
    
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 更新用户名
 * @param userId 用户ID
 * @param newUsername 新用户名
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateUsername(int userId, const std::string& newUsername){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "UPDATE users SET username = '" << newUsername
        << "' WHERE user_id = " << userId;
    
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户密码
 * @param userId 用户ID
 * @param newPasswordHash 新密码MD5哈希值
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdatePassword(int userId, const std::string& newPasswordHash){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "UPDATE users SET password_hash = '" << newPasswordHash
        << "' WHERE user_id = " << userId;
    
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户在线状态
 * @param userId 用户ID
 * @param isOnline 在线状态（true=在线，false=离线）
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateOnlineStatus(int userId, bool isOnline){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "UPDATE users SET is_online = " << (isOnline ? 1 : 0)
        << " WHERE user_id = " << userId;
    
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户最后登录时间
 * @param userId 用户ID
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateLastLoginTime(int userId){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "UPDATE users SET last_login_time = NOW() WHERE user_id = " << userId;
    
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 检查用户名是否已存在
 * @param username 用户名
 * @return 已存在返回true，不存在返回false
 */
bool UserDAO::CheckUsernameExists(const std::string& username){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM users WHERE username = '"
        << username << "'";
    
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        dbManager->ReleaseConnection(conn);
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    bool exists = false;
    if(row && std::stoi(row[0]) > 0){
        exists = true;
    }
    
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return exists;
}

/**
 * @brief 查询用户在线状态
 * @param userId 用户ID
 * @param isOnline 输出参数：在线状态
 * @return 查询成功返回true，失败返回false
 */
bool UserDAO::GetUserOnlineStatus(int userId, bool& isOnline){
    MYSQL* conn = dbManager->GetConnection();
    
    std::ostringstream sql;
    sql << "SELECT is_online FROM users WHERE user_id = " << userId;
    
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        dbManager->ReleaseConnection(conn);
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    
    isOnline = (std::stoi(row[0]) == 1);
    
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}
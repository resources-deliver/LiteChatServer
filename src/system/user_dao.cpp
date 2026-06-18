#include "user_dao.h"
#include <iostream>
#include <sstream>

/**
 * @brief UserDAO构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 */
UserDAO::UserDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

/**
 * @brief UserDAO析构函数
 */
UserDAO::~UserDAO()
{
}

/**
 * @brief 插入新用户（注册）到数据库
 * @param username 用户名
 * @param passwordHash 密码MD5哈希值
 * @return 插入成功返回true，失败返回false
 */
bool UserDAO::InsertUser(const std::string& username, const std::string& passwordHash){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::InsertUser]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "INSERT INTO users (username, password_hash) VALUES ('" << username << "', '" << passwordHash << "')";
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::InsertUser]插入用户失败,用户名: " << username << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::InsertUser]插入用户成功,用户名: " << username << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 根据用户名从数据库查询用户信息和密码
 * @param username 用户名
 * @param userId 输出参数：用户ID
 * @param passwordHash 输出参数：密码哈希值
 * @return 查询成功返回true，用户不存在返回false
 */
bool UserDAO::GetUserByUsername(const std::string& username, int& userId, std::string& passwordHash){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "SELECT user_id, password_hash FROM users WHERE username = '" << username << "'";
    // 执行查询语句
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::GetUserByUsername]查询用户失败,用户名: " << username << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    // 获取与处理查询结果
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        std::cerr << "[UserDAO::GetUserByUsername]用户不存在,用户名: " << username << std::endl;
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    userId = std::stoi(row[0]);
    passwordHash = row[1];
    std::cout << "[UserDAO::GetUserByUsername]查询用户成功,用户ID: " << userId << ", 密码哈希值: " << passwordHash << std::endl;
    // 释放资源
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 更新用户名到数据库
 * @param userId 用户ID
 * @param newUsername 新用户名
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateUsername(int userId, const std::string& newUsername){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "UPDATE users SET username = '" << newUsername << "' WHERE user_id = " << userId;
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::UpdateUsername]更新用户名失败,用户ID: " << userId << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::UpdateUsername]更新用户名成功,用户ID: " << userId << ", 新用户名: " << newUsername << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户密码到数据库
 * @param userId 用户ID
 * @param newPasswordHash 新密码MD5哈希值
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdatePassword(int userId, const std::string& newPasswordHash){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdatePassword]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "UPDATE users SET password_hash = '" << newPasswordHash << "' WHERE user_id = " << userId;
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::UpdatePassword]更新密码失败,用户ID: " << userId << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::UpdatePassword]更新密码成功,用户ID: " << userId << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户在线状态到数据库
 * @param userId 用户ID
 * @param isOnline 在线状态（true=在线，false=离线）
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateOnlineStatus(int userId, bool isOnline){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateOnlineStatus]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "UPDATE users SET is_online = " << (isOnline ? 1 : 0) << " WHERE user_id = " << userId;
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::UpdateOnlineStatus]更新在线状态失败,用户ID: " << userId << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::UpdateOnlineStatus]更新在线状态成功,用户ID: " << userId << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户最后登录时间到数据库
 * @param userId 用户ID
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateLastLoginTime(int userId){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateLastLoginTime]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "UPDATE users SET last_login_time = NOW() WHERE user_id = " << userId;
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::UpdateLastLoginTime]更新最后登录时间失败,用户ID: " << userId << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::UpdateLastLoginTime]更新最后登录时间成功,用户ID: " << userId << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库检查用户名是否已存在
 * @param username 用户名
 * @return 已存在返回true，不存在返回false
 */
bool UserDAO::CheckUsernameExists(const std::string& username){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::CheckUsernameExists]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM users WHERE username = '" << username << "'";
    // 执行查询语句
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::CheckUsernameExists]查询用户名是否存在失败,用户名: " << username << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    // 获取与处理查询结果
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        std::cerr << "[UserDAO::CheckUsernameExists]查询用户名是否存在失败,用户名: " << username << std::endl;
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool exists = false;
    if(row && std::stoi(row[0]) > 0){
        exists = true;
    }
    // 释放资源
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return exists;
}

/**
 * @brief 从数据库查询用户在线状态
 * @param userId 用户ID
 * @param isOnline 输出参数：在线状态
 * @return 查询成功返回true，失败返回false
 */
bool UserDAO::GetUserOnlineStatus(int userId, bool& isOnline){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserOnlineStatus]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "SELECT is_online FROM users WHERE user_id = " << userId;
    // 执行查询语句
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::GetUserOnlineStatus]查询用户在线状态失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    // 获取与处理查询结果
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        std::cerr << "[UserDAO::GetUserOnlineStatus]查询用户在线状态失败,用户ID: " << userId << std::endl;
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    isOnline = (std::stoi(row[0]) == 1);
    // 释放资源
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 根据用户名更新在线状态到数据库
 * @param username 用户名
 * @param isOnline 在线状态（true=在线，false=离线）
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateOnlineStatusByUsername(const std::string& username, bool isOnline){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "UPDATE users SET is_online = " << (isOnline ? 1 : 0)
        << " WHERE username = '" << username << "'";
    // 执行更新语句
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]更新用户在线状态失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[UserDAO::UpdateOnlineStatusByUsername]更新用户在线状态成功,用户名: " << username << std::endl;
    // 释放资源
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 根据用户名从数据库查询用户ID
 * @param username 用户名
 * @param userId 输出参数：用户ID
 * @return 查询成功返回true，用户不存在返回false
 */
bool UserDAO::GetUserIdByUsername(const std::string& username, int& userId){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserIdByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "SELECT user_id FROM users WHERE username = '" << username << "'";
    // 执行查询语句
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[UserDAO::GetUserIdByUsername]查询用户ID失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    // 获取与处理查询结果
    MYSQL_ROW row = mysql_fetch_row(result);
    if(!row){
        mysql_free_result(result);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    userId = std::stoi(row[0]);
    std::cout << "[UserDAO::GetUserIdByUsername]查询用户ID成功,用户名: " << username << ",用户ID: " << userId << std::endl;
    // 释放资源
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 从数据库查询用户的所有好友（用户名和在线状态）
 * @param userId 用户ID
 * @param friends 输出参数：好友列表，每项为<用户名, 是否在线>
 * @return 查询成功返回true，失败返回false
 */
bool UserDAO::GetFriendsOfUser(int userId, std::vector<std::pair<std::string, bool>>& friends){
    // 从MYSQL连接池获取可用连接
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetFriendsOfUser]获取数据库连接失败" << std::endl;
        return false;
    }
    // 用于构建SQL语句的字符串流
    std::ostringstream sql;
    sql << "SELECT u.username, u.is_online FROM users u "
        << "INNER JOIN friends f ON (f.user_id1 = u.user_id OR f.user_id2 = u.user_id) "
        << "WHERE (f.user_id1 = " << userId << " OR f.user_id2 = " << userId << ") "
        << "AND u.user_id != " << userId;
    // 执行查询语句
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        dbManager->ReleaseConnection(conn);
        std::cerr << "[UserDAO::GetFriendsOfUser]查询用户好友失败" << std::endl;
        return false;
    }
    // 获取与处理查询结果
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        std::string friendUsername = row[0] ? row[0] : "";
        bool isOnline = (row[1] && std::stoi(row[1]) == 1);
        friends.push_back({friendUsername, isOnline});
    }
    std::cout << "[UserDAO::GetFriendsOfUser]查询用户好友成功,用户ID: " << userId << ",好友数量: " << friends.size() << std::endl;
    // 释放资源
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}
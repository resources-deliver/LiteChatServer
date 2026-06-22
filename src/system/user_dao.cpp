#include "user_dao.h"
#include <iostream>
#include <cstring>

UserDAO::UserDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

UserDAO::~UserDAO(){}

/**
 * @brief 插入新用户（注册）到数据库
 * @param username 用户名
 * @param passwordHash 密码MD5哈希值
 * @return 插入成功返回true，失败返回false
 */
bool UserDAO::InsertUser(const std::string& username, const std::string& passwordHash){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::InsertUser]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "INSERT INTO users (username, password_hash) VALUES (?, ?)";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::InsertUser]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    unsigned long usernameLen = static_cast<unsigned long>(username.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = usernameLen;
    bind[0].length = &usernameLen;
    unsigned long passwordLen = static_cast<unsigned long>(passwordHash.length());
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(passwordHash.c_str());
    bind[1].buffer_length = passwordLen;
    bind[1].length = &passwordLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::InsertUser]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::InsertUser]插入用户到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::InsertUser]插入用户到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT user_id, password_hash FROM users WHERE username = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::GetUserByUsername]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    unsigned long usernameLen = static_cast<unsigned long>(username.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = usernameLen;
    bind[0].length = &usernameLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::GetUserByUsername]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[UserDAO::GetUserByUsername]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int resultUserId = 0;
    char resultPassword[256];
    unsigned long passwordLen = 0;
    MYSQL_BIND result[2];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &resultUserId;
    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = resultPassword;
    result[1].buffer_length = sizeof(resultPassword);
    result[1].length = &passwordLen;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[UserDAO::GetUserByUsername]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[UserDAO::GetUserByUsername]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool found = false;
    if(mysql_stmt_fetch(stmt) == 0){
        userId = resultUserId;
        passwordHash = std::string(resultPassword, passwordLen);
        found = true;
        std::cout << "[UserDAO::GetUserByUsername]从数据库查询用户信息成功" << std::endl;
    } else {
        std::cerr << "[UserDAO::GetUserByUsername]从数据库查询用户信息失败，用户不存在" << std::endl;
    }
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return found;
}

/**
 * @brief 更新用户名到数据库
 * @param userId 用户ID
 * @param newUsername 新用户名
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateUsername(int userId, const std::string& newUsername){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE users SET username = ? WHERE user_id = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::UpdateUsername]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    unsigned long usernameLen = static_cast<unsigned long>(newUsername.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(newUsername.c_str());
    bind[0].buffer_length = usernameLen;
    bind[0].length = &usernameLen;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::UpdateUsername]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::UpdateUsername]更新用户名到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::UpdateUsername]更新用户名到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdatePassword]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE users SET password_hash = ? WHERE user_id = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::UpdatePassword]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    unsigned long passwordLen = static_cast<unsigned long>(newPasswordHash.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(newPasswordHash.c_str());
    bind[0].buffer_length = passwordLen;
    bind[0].length = &passwordLen;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::UpdatePassword]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::UpdatePassword]更新用户密码到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::UpdatePassword]更新用户密码到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateOnlineStatus]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE users SET is_online = ? WHERE user_id = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::UpdateOnlineStatus]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    int onlineVal = isOnline ? 1 : 0;
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &onlineVal;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::UpdateOnlineStatus]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::UpdateOnlineStatus]更新用户在线状态到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::UpdateOnlineStatus]更新用户在线状态到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户最后登录时间到数据库
 * @param userId 用户ID
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateLastLoginTime(int userId){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateLastLoginTime]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE users SET last_login_time = NOW() WHERE user_id = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::UpdateLastLoginTime]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::UpdateLastLoginTime]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::UpdateLastLoginTime]更新用户最后登录时间到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::UpdateLastLoginTime]更新用户最后登录时间到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库检查用户名是否已存在
 * @param username 用户名
 * @return 已存在返回true，不存在返回false
 */
bool UserDAO::CheckUsernameExists(const std::string& username){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::CheckUsernameExists]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT COUNT(*) FROM users WHERE username = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::CheckUsernameExists]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    unsigned long usernameLen = static_cast<unsigned long>(username.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = usernameLen;
    bind[0].length = &usernameLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::CheckUsernameExists]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[UserDAO::CheckUsernameExists]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int count = 0;
    MYSQL_BIND result[1];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &count;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[UserDAO::CheckUsernameExists]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[UserDAO::CheckUsernameExists]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool exists = false;
    if(mysql_stmt_fetch(stmt) == 0 && count > 0){
        exists = true;
        std::cout << "[UserDAO::CheckUsernameExists]用户名已存在" << std::endl;
    } else {
        std::cout << "[UserDAO::CheckUsernameExists]用户名不存在" << std::endl;
    }
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserOnlineStatus]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT is_online FROM users WHERE user_id = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::GetUserOnlineStatus]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::GetUserOnlineStatus]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[UserDAO::GetUserOnlineStatus]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int onlineVal = 0;
    MYSQL_BIND result[1];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &onlineVal;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[UserDAO::GetUserOnlineStatus]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[UserDAO::GetUserOnlineStatus]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool found = false;
    if(mysql_stmt_fetch(stmt) == 0){
        isOnline = (onlineVal == 1);
        found = true;
        std::cout << "[UserDAO::GetUserOnlineStatus]查询用户在线状态成功" << std::endl;
    } else {
        std::cerr << "[UserDAO::GetUserOnlineStatus]用户不存在" << std::endl;
    }
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return found;
}

/**
 * @brief 根据用户名更新在线状态到数据库
 * @param username 用户名
 * @param isOnline 在线状态（true=在线，false=离线）
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateOnlineStatusByUsername(const std::string& username, bool isOnline){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE users SET is_online = ? WHERE username = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    int onlineVal = isOnline ? 1 : 0;
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &onlineVal;
    unsigned long usernameLen = static_cast<unsigned long>(username.length());
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = const_cast<char*>(username.c_str());
    bind[1].buffer_length = usernameLen;
    bind[1].length = &usernameLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[UserDAO::UpdateOnlineStatusByUsername]更新在线状态失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[UserDAO::UpdateOnlineStatusByUsername]更新在线状态成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetUserIdByUsername]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT user_id FROM users WHERE username = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::GetUserIdByUsername]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    unsigned long usernameLen = static_cast<unsigned long>(username.length());
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = const_cast<char*>(username.c_str());
    bind[0].buffer_length = usernameLen;
    bind[0].length = &usernameLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::GetUserIdByUsername]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[UserDAO::GetUserIdByUsername]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int resultUserId = 0;
    MYSQL_BIND result[1];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &resultUserId;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[UserDAO::GetUserIdByUsername]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[UserDAO::GetUserIdByUsername]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool found = false;
    if(mysql_stmt_fetch(stmt) == 0){
        userId = resultUserId;
        found = true;
        std::cout << "[UserDAO::GetUserIdByUsername]查询用户ID成功" << std::endl;
    } else {
        std::cerr << "[UserDAO::GetUserIdByUsername]用户不存在" << std::endl;
    }
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return found;
}

/**
 * @brief 从数据库查询用户的所有好友（用户名和在线状态）
 * @param userId 用户ID
 * @param friends 输出参数：好友列表，每项为<用户名, 是否在线>
 * @return 查询成功返回true，失败返回false
 */
bool UserDAO::GetFriendsOfUser(int userId, std::vector<std::pair<std::string, bool>>& friends){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[UserDAO::GetFriendsOfUser]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT u.username, u.is_online FROM users u "
        "INNER JOIN friends f ON (f.user_id1 = u.user_id OR f.user_id2 = u.user_id) "
        "WHERE (f.user_id1 = ? OR f.user_id2 = ?) AND u.user_id != ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[UserDAO::GetFriendsOfUser]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId;
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[UserDAO::GetFriendsOfUser]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[UserDAO::GetFriendsOfUser]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    char friendUsername[256];
    int onlineVal = 0;
    unsigned long usernameLen = 0;
    MYSQL_BIND result[2];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_STRING;
    result[0].buffer = friendUsername;
    result[0].buffer_length = sizeof(friendUsername);
    result[0].length = &usernameLen;
    result[1].buffer_type = MYSQL_TYPE_LONG;
    result[1].buffer = &onlineVal;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[UserDAO::GetFriendsOfUser]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[UserDAO::GetFriendsOfUser]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    while(mysql_stmt_fetch(stmt) == 0){
        std::string username(friendUsername, usernameLen);
        bool isOnline = (onlineVal == 1);
        friends.push_back({username, isOnline});
    }
    std::cout << "[UserDAO::GetFriendsOfUser]查询好友列表成功" << std::endl;
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return true;
}
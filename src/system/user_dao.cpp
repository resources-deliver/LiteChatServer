#include "user_dao.h"
#include "server_logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

/**
 * @brief 构造函数
 * @param dbManager 数据库管理器对象
 */
UserDAO::UserDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

/**
 * @brief 析构函数
 */
UserDAO::~UserDAO(){}

/**
 * @brief 插入新用户（注册）到数据库
 * @param username 用户名
 * @param passwordHash 密码MD5哈希值
 * @return 插入成功返回true，失败返回false
 */
bool UserDAO::InsertUser(const std::string& username, const std::string& passwordHash){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "INSERT INTO users (username, password_hash) VALUES (?, ?)"
        );  // 创建预处理语句
        pstmt->setString(1, username);  // 设置第一个参数位为用户名
        pstmt->setString(2, passwordHash);  // 设置第二个参数位为密码哈希值
        pstmt->executeUpdate();  // 执行预处理语句（插入）
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "插入用户到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "插入用户到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool found = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT user_id, password_hash FROM users WHERE username = ?"
        );
        pstmt->setString(1, username);
        sql::ResultSet* res = pstmt->executeQuery();  // 执行预处理语句（查询）
        if(res->next()){  // 如果查询结果有下一行
            userId = res->getInt("user_id");  // 获取用户ID
            passwordHash = res->getString("password_hash");  // 获取密码哈希值
            found = true;
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "从数据库查询用户信息成功");
        }
        else{
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "从数据库查询用户信息失败，用户不存在");
        }
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "查询用户信息失败: " + std::string(e.what())
        );
        found = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE users SET username = ? WHERE user_id = ?"
        );
        pstmt->setString(1, newUsername);
        pstmt->setInt(2, userId);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "更新用户名到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "更新用户名到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE users SET password_hash = ? WHERE user_id = ?"
        );
        pstmt->setString(1, newPasswordHash);
        pstmt->setInt(2, userId);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "更新用户密码到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "更新用户密码到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE users SET is_online = ? WHERE user_id = ?"
        );
        pstmt->setBoolean(1, isOnline);  // 设置第一个参数位为在线状态
        pstmt->setInt(2, userId);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "更新用户在线状态到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "更新用户在线状态到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 更新用户最后登录时间到数据库
 * @param userId 用户ID
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::UpdateLastLoginTime(int userId){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE users SET last_login_time = NOW() WHERE user_id = ?"
        );
        pstmt->setInt(1, userId);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "更新用户最后登录时间到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "更新用户最后登录时间到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库检查用户名是否已存在
 * @param username 用户名
 * @return 已存在返回true，不存在返回false
 */
bool UserDAO::CheckUsernameExists(const std::string& username){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool exists = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM users WHERE username = ?"
        );
        pstmt->setString(1, username);
        sql::ResultSet* res = pstmt->executeQuery();
        if(res->next() && res->getInt(1) > 0){  // 检查查询结果是否为空且计数大于0
            exists = true;
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "用户名已存在");
        }
        else{
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "用户名不存在");
        }
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "检查用户名是否存在失败: " + std::string(e.what())
        );
        exists = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool found = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT is_online FROM users WHERE user_id = ?"
        );
        pstmt->setInt(1, userId);
        sql::ResultSet* res = pstmt->executeQuery();
        if(res->next()){
            isOnline = res->getBoolean("is_online");  // 获取在线状态
            found = true;
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "查询用户在线状态成功");
        }
        else{
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "用户不存在");
        }
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "查询用户在线状态失败: " + std::string(e.what())
        );
        found = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE users SET is_online = ? WHERE username = ?"
        );
        pstmt->setBoolean(1, isOnline);
        pstmt->setString(2, username);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "更新在线状态成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "更新在线状态失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool found = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT user_id FROM users WHERE username = ?"
        );
        pstmt->setString(1, username);
        sql::ResultSet* res = pstmt->executeQuery();
        if(res->next()){
            userId = res->getInt("user_id");
            found = true;
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "查询用户ID成功");
        }
        else{
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "用户不存在");
        }
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "查询用户ID失败: " + std::string(e.what())
        );
        found = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "UserDAO", "获取数据库连接失败");
        return false;
    }
    bool result = true;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT u.username, u.is_online FROM users u "
            "INNER JOIN friends f ON (f.user_id1 = u.user_id OR f.user_id2 = u.user_id) "
            "WHERE (f.user_id1 = ? OR f.user_id2 = ?) AND u.user_id != ?"
        );
        pstmt->setInt(1, userId);
        pstmt->setInt(2, userId);
        pstmt->setInt(3, userId);
        sql::ResultSet* res = pstmt->executeQuery();
        while(res->next()){
            std::string username = res->getString("username");
            bool isOnline = res->getBoolean("is_online");
            friends.push_back({username, isOnline});  // 添加好友到列表
        }
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "UserDAO", "查询好友列表成功");
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "UserDAO", "查询好友列表失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}
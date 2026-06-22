#include "friend_dao.h"
#include "server_logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

/**
 * @brief FriendDAO构造函数，初始化数据库管理器指针
 * @param dbManager 数据库管理器指针
 */
FriendDAO::FriendDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

/**
 * @brief FriendDAO析构函数
 */
FriendDAO::~FriendDAO(){}

/**
 * @brief 在数据库中添加好友关系
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 添加成功返回true，失败返回false
 */
bool FriendDAO::InsertFriend(int userId1, int userId2){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "FriendDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "INSERT INTO friends (user_id1, user_id2) VALUES (?, ?)"
        );  // 创建预处理语句
        pstmt->setInt(1, userId1);  // 设置第一个参数位为用户ID1
        pstmt->setInt(2, userId2);  // 设置第二个参数位为用户ID2
        pstmt->executeUpdate();  // 执行预处理语句（插入）
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "添加好友关系成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "FriendDAO", "添加好友关系失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库删除好友关系
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 删除成功返回true，失败返回false
 */
bool FriendDAO::DeleteFriend(int userId1, int userId2){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "FriendDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "DELETE FROM friends WHERE user_id1 = ? AND user_id2 = ?"
        );
        pstmt->setInt(1, userId1);
        pstmt->setInt(2, userId2);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "删除好友关系成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "FriendDAO", "删除好友关系失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库删除两个用户之间的所有消息记录
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 删除成功返回true，失败返回false
 */
bool FriendDAO::DeleteMessagesBetweenUsers(int userId1, int userId2){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "FriendDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "DELETE FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)"
        );
        pstmt->setInt(1, userId1);
        pstmt->setInt(2, userId2);
        pstmt->setInt(3, userId2);
        pstmt->setInt(4, userId1);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "删除消息记录成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "FriendDAO", "删除消息记录失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 在数据库中检查好友关系是否存在
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 存在返回true，不存在返回false
 */
bool FriendDAO::CheckFriendship(int userId1, int userId2){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "FriendDAO", "获取数据库连接失败");
        return false;
    }
    bool exists = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM friends WHERE user_id1 = ? AND user_id2 = ?"
        );
        pstmt->setInt(1, userId1);
        pstmt->setInt(2, userId2);
        sql::ResultSet* res = pstmt->executeQuery();  // 执行预处理语句（查询）
        if(res->next() && res->getInt(1) > 0){  // 检查查询结果是否为空且好友关系存在
            exists = true;
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "好友关系存在");
        }
        else{
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "好友关系不存在");
        }
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "FriendDAO", "检查好友关系失败: " + std::string(e.what())
        );
        exists = false;
    }
    dbManager->ReleaseConnection(conn);
    return exists;
}

/**
 * @brief 从数据库查询用户的好友列表（用户名和在线状态）
 * @param userId 用户ID
 * @param friends 输出参数：好友列表，每项为<用户名, 是否在线>
 * @return 查询成功返回true，失败返回false
 */
bool FriendDAO::GetFriendList(int userId, std::vector<std::pair<std::string, bool>>& friends){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "FriendDAO", "获取数据库连接失败");
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
        while(res->next()){  // 遍历查询下一行结果
            std::string username = res->getString("username");  // 获取用户名
            bool isOnline = res->getBoolean("is_online");  // 获取是否在线
            friends.push_back({username, isOnline});  // 将好友信息添加到列表
        }
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "FriendDAO", "查询好友列表成功");
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "FriendDAO", "查询好友列表失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}
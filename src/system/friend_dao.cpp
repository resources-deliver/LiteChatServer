#include "friend_dao.h"
#include <iostream>
#include <sstream>
#include <utility>

/**
 * @brief FriendDAO构造函数，初始化类内私有属性
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
 * @brief 向数据库插入好友关系
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 插入成功返回true，失败返回false
 */
bool FriendDAO::InsertFriend(int userId1, int userId2){
    if(userId1 > userId2){
        std::swap(userId1, userId2);
    }
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::InsertFriend]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "INSERT INTO friends (user_id1, user_id2) VALUES (" << userId1 << ", " << userId2 << ")";
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[FriendDAO::InsertFriend]向数据库插入好友关系失败, ExecuteUpdate结果失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[FriendDAO::InsertFriend]向数据库插入好友关系成功" << std::endl;
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
    if(userId1 > userId2){
        std::swap(userId1, userId2);
    }
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::DeleteFriend]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "DELETE FROM friends WHERE user_id1 = " << userId1 << " AND user_id2 = " << userId2;
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[FriendDAO::DeleteFriend]从数据库删除好友关系失败, ExecuteUpdate结果失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[FriendDAO::DeleteFriend]从数据库删除好友关系成功" << std::endl;
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::DeleteMessagesBetweenUsers]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "DELETE FROM messages WHERE (sender_id = " << userId1 << " AND receiver_id = " << userId2 << ") "
        << "OR (sender_id = " << userId2 << " AND receiver_id = " << userId1 << ")";
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[FriendDAO::DeleteMessagesBetweenUsers]从数据库删除两个用户之间的所有消息记录失败, ExecuteUpdate结果失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[FriendDAO::DeleteMessagesBetweenUsers]从数据库删除两个用户之间的所有消息记录成功" << std::endl;
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库检查两个用户是否为好友关系
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 是好友返回true，否则返回false
 */
bool FriendDAO::CheckFriendship(int userId1, int userId2){
    if(userId1 > userId2){
        std::swap(userId1, userId2);
    }
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::CheckFriendship]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "SELECT COUNT(*) FROM friends WHERE user_id1 = " << userId1 << " AND user_id2 = " << userId2;
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[FriendDAO::CheckFriendship]从数据库检查两个用户是否为好友关系失败, ExecuteQuery结果为空" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(result);  // 获取查询结果集的第一行
    if(!row){
        std::cerr << "[FriendDAO::CheckFriendship]从数据库检查两个用户是否为好友关系失败, ExecuteQuery结果首行为空" << std::endl;
        mysql_free_result(result);  // 释放查询结果集
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool exists = (std::stoi(row[0]) > 0);
    std::cout << "[FriendDAO::CheckFriendship]从数据库检查两个用户是否为好友关系成功" << std::endl;
    mysql_free_result(result);  // 释放查询结果集
    dbManager->ReleaseConnection(conn);
    return exists;
}

/**
 * @brief 从数据库查询用户的所有好友（用户名和在线状态）
 * @param userId 用户ID
 * @param friends 输出参数：好友列表，每项为<用户名, 是否在线>
 * @return 查询成功返回true，失败返回false
 */
bool FriendDAO::GetFriendList(int userId, std::vector<std::pair<std::string, bool>>& friends){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::GetFriendList]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "SELECT u.username, u.is_online FROM users u "
        << "INNER JOIN friends f ON (f.user_id1 = u.user_id OR f.user_id2 = u.user_id) "
        << "WHERE (f.user_id1 = " << userId << " OR f.user_id2 = " << userId << ") "
        << "AND u.user_id != " << userId;
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[FriendDAO::GetFriendList]从数据库查询用户的所有好友（用户名和在线状态）失败, ExecuteQuery结果为空" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        if(!row){
            std::cerr << "[FriendDAO::GetFriendList]从数据库查询用户的所有好友（用户名和在线状态）失败, ExecuteQuery结果首行为空" << std::endl;
            continue;
        }
        std::string friendUsername = row[0] ? row[0] : "";
        bool isOnline = (row[1] && std::stoi(row[1]) == 1);
        friends.push_back({friendUsername, isOnline});
    }
    std::cout << "[FriendDAO::GetFriendList]从数据库查询用户的所有好友（用户名和在线状态）成功" << std::endl;
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}
#include "friend_dao.h"
#include <iostream>
#include <cstring>

FriendDAO::FriendDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

FriendDAO::~FriendDAO(){}

/**
 * @brief 在数据库中添加好友关系
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @return 添加成功返回true，失败返回false
 */
bool FriendDAO::InsertFriend(int userId1, int userId2){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::InsertFriend]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "INSERT INTO friends (user_id1, user_id2) VALUES (?, ?)";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[FriendDAO::InsertFriend]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId1;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId2;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[FriendDAO::InsertFriend]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[FriendDAO::InsertFriend]添加好友关系失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[FriendDAO::InsertFriend]添加好友关系成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::DeleteFriend]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "DELETE FROM friends WHERE user_id1 = ? AND user_id2 = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[FriendDAO::DeleteFriend]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId1;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId2;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[FriendDAO::DeleteFriend]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[FriendDAO::DeleteFriend]删除好友关系失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[FriendDAO::DeleteFriend]删除好友关系成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    const char* sql = "DELETE FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?)";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[FriendDAO::DeleteMessagesBetweenUsers]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId1;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId2;
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &userId2;
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &userId1;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[FriendDAO::DeleteMessagesBetweenUsers]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[FriendDAO::DeleteMessagesBetweenUsers]删除消息记录失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[FriendDAO::DeleteMessagesBetweenUsers]删除消息记录成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::CheckFriendship]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT COUNT(*) FROM friends WHERE user_id1 = ? AND user_id2 = ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[FriendDAO::CheckFriendship]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId1;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId2;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[FriendDAO::CheckFriendship]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[FriendDAO::CheckFriendship]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
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
        std::cerr << "[FriendDAO::CheckFriendship]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[FriendDAO::CheckFriendship]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool exists = false;
    if(mysql_stmt_fetch(stmt) == 0 && count > 0){
        exists = true;
        std::cout << "[FriendDAO::CheckFriendship]好友关系存在" << std::endl;
    } else {
        std::cout << "[FriendDAO::CheckFriendship]好友关系不存在" << std::endl;
    }
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
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
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[FriendDAO::GetFriendList]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT u.username, u.is_online FROM users u "
        "INNER JOIN friends f ON (f.user_id1 = u.user_id OR f.user_id2 = u.user_id) "
        "WHERE (f.user_id1 = ? OR f.user_id2 = ?) AND u.user_id != ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[FriendDAO::GetFriendList]预编译语句失败" << std::endl;
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
        std::cerr << "[FriendDAO::GetFriendList]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[FriendDAO::GetFriendList]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
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
        std::cerr << "[FriendDAO::GetFriendList]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[FriendDAO::GetFriendList]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    while(mysql_stmt_fetch(stmt) == 0){
        std::string username(friendUsername, usernameLen);
        bool isOnline = (onlineVal == 1);
        friends.push_back({username, isOnline});
    }
    std::cout << "[FriendDAO::GetFriendList]查询好友列表成功" << std::endl;
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return true;
}
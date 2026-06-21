#include "message_dao.h"
#include <iostream>
#include <sstream>

/**
 * @brief MessageDAO构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 */
MessageDAO::MessageDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

/**
 * @brief MessageDAO析构函数
 */
MessageDAO::~MessageDAO(){}

/**
 * @brief 向数据库插入一条新消息
 * @param senderId 发送者用户ID
 * @param receiverId 接收者用户ID
 * @param content 消息内容
 * @return 插入成功返回true，失败返回false
 */
bool MessageDAO::InsertMessage(int senderId, int receiverId, const std::string& content){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::InsertMessage]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "INSERT INTO messages (sender_id, receiver_id, content) VALUES ("
        << senderId << ", " << receiverId << ", '" << content << "')";
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[MessageDAO::InsertMessage]向数据库插入一条新消息失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[MessageDAO::InsertMessage]向数据库插入一条新消息成功" << std::endl;
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库查询用户的所有未读消息
 * @param userId 用户ID
 * @param messages 输出参数：未读消息列表
 * @return 查询成功返回true，失败返回false
 */
bool MessageDAO::GetUnreadMessages(int userId, std::vector<Message>& messages){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::GetUnreadMessages]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
        << "u1.username AS sender_username, u2.username AS receiver_username "
        << "FROM messages m "
        << "INNER JOIN users u1 ON m.sender_id = u1.user_id "
        << "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
        << "WHERE m.receiver_id = " << userId << " AND m.is_read = 0 "
        << "ORDER BY m.send_time ASC";
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[MessageDAO::GetUnreadMessages]从数据库查询用户的所有未读消息失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        Message msg;
        msg.messageId = std::stoi(row[0]);
        msg.senderId = std::stoi(row[1]);
        msg.receiverId = std::stoi(row[2]);
        msg.content = row[3] ? row[3] : "";
        msg.sendTime = row[4] ? row[4] : "";
        msg.isRead = (row[5] && std::stoi(row[5]) == 1);
        msg.senderUsername = row[6] ? row[6] : "";
        msg.receiverUsername = row[7] ? row[7] : "";
        messages.push_back(msg);
    }
    std::cout << "[MessageDAO::GetUnreadMessages]从数据库查询用户的所有未读消息成功" << std::endl;
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 向数据库标记用户之间的未读消息为已读
 * @param senderId 发送者用户ID
 * @param receiverId 接收者用户ID
 * @return 更新成功返回true，失败返回false
 */
bool MessageDAO::MarkMessagesAsRead(int senderId, int receiverId){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "UPDATE messages SET is_read = 1 WHERE sender_id = " << senderId
        << " AND receiver_id = " << receiverId << " AND is_read = 0";
    bool result = dbManager->ExecuteUpdate(conn, sql.str());
    if(!result){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]向数据库标记用户之间的未读消息为已读失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    std::cout << "[MessageDAO::MarkMessagesAsRead]向数据库标记用户之间的未读消息为已读成功" << std::endl;
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库查询两个用户之间的历史消息（按时间升序，限制条数）
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @param count 查询条数限制
 * @param messages 输出参数：历史消息列表
 * @return 查询成功返回true，失败返回false
 */
bool MessageDAO::GetHistoryMessages(int userId1, int userId2, int count, std::vector<Message>& messages){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::GetHistoryMessages]获取数据库连接失败" << std::endl;
        return false;
    }
    std::ostringstream sql;
    sql << "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
        << "u1.username AS sender_username, u2.username AS receiver_username "
        << "FROM messages m "
        << "INNER JOIN users u1 ON m.sender_id = u1.user_id "
        << "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
        << "WHERE (m.sender_id = " << userId1 << " AND m.receiver_id = " << userId2 << ") "
        << "OR (m.sender_id = " << userId2 << " AND m.receiver_id = " << userId1 << ") "
        << "ORDER BY m.send_time ASC "
        << "LIMIT " << count;
    MYSQL_RES* result = dbManager->ExecuteQuery(conn, sql.str());
    if(!result){
        std::cerr << "[MessageDAO::GetHistoryMessages]从数据库查询两个用户之间的历史消息（按时间升序，限制条数）失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))){
        Message msg;
        msg.messageId = std::stoi(row[0]);
        msg.senderId = std::stoi(row[1]);
        msg.receiverId = std::stoi(row[2]);
        msg.content = row[3] ? row[3] : "";
        msg.sendTime = row[4] ? row[4] : "";
        msg.isRead = (row[5] && std::stoi(row[5]) == 1);
        msg.senderUsername = row[6] ? row[6] : "";
        msg.receiverUsername = row[7] ? row[7] : "";
        messages.push_back(msg);
    }
    std::cout << "[MessageDAO::GetHistoryMessages]从数据库查询两个用户之间的历史消息（按时间升序，限制条数）成功" << std::endl;
    mysql_free_result(result);
    dbManager->ReleaseConnection(conn);
    return true;
}
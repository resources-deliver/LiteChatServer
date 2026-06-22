#include "message_dao.h"
#include <iostream>
#include <cstring>
#include <cstdio>

MessageDAO::MessageDAO(DBManager* dbManager)
    : dbManager(dbManager)
{
}

MessageDAO::~MessageDAO(){}

/**
 * @brief 向数据库插入一条消息记录
 * @param senderId 发送者ID
 * @param receiverId 接收者ID
 * @param content 消息内容
 * @return 插入成功返回true，失败返回false
 */
bool MessageDAO::InsertMessage(int senderId, int receiverId, const std::string& content){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::InsertMessage]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "INSERT INTO messages (sender_id, receiver_id, content) VALUES (?, ?, ?)";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[MessageDAO::InsertMessage]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &senderId;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &receiverId;
    unsigned long contentLen = static_cast<unsigned long>(content.length());
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = const_cast<char*>(content.c_str());
    bind[2].buffer_length = contentLen;
    bind[2].length = &contentLen;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[MessageDAO::InsertMessage]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[MessageDAO::InsertMessage]插入消息到数据库失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[MessageDAO::InsertMessage]插入消息到数据库成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库查询用户的未读消息
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
    const char* sql = "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
        "u1.username AS sender_username, u2.username AS receiver_username "
        "FROM messages m "
        "INNER JOIN users u1 ON m.sender_id = u1.user_id "
        "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
        "WHERE m.receiver_id = ? AND m.is_read = 0 "
        "ORDER BY m.send_time ASC";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[MessageDAO::GetUnreadMessages]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[MessageDAO::GetUnreadMessages]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[MessageDAO::GetUnreadMessages]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int resultMsgId = 0;
    int resultSenderId = 0;
    int resultReceiverId = 0;
    char resultContent[4096];
    MYSQL_TIME resultSendTime;
    int resultIsRead = 0;
    char resultSenderUsername[256];
    char resultReceiverUsername[256];
    unsigned long contentLen = 0;
    unsigned long senderUsernameLen = 0;
    unsigned long receiverUsernameLen = 0;
    MYSQL_BIND result[8];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &resultMsgId;
    result[1].buffer_type = MYSQL_TYPE_LONG;
    result[1].buffer = &resultSenderId;
    result[2].buffer_type = MYSQL_TYPE_LONG;
    result[2].buffer = &resultReceiverId;
    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = resultContent;
    result[3].buffer_length = sizeof(resultContent);
    result[3].length = &contentLen;
    result[4].buffer_type = MYSQL_TYPE_DATETIME;
    result[4].buffer = &resultSendTime;
    result[5].buffer_type = MYSQL_TYPE_LONG;
    result[5].buffer = &resultIsRead;
    result[6].buffer_type = MYSQL_TYPE_STRING;
    result[6].buffer = resultSenderUsername;
    result[6].buffer_length = sizeof(resultSenderUsername);
    result[6].length = &senderUsernameLen;
    result[7].buffer_type = MYSQL_TYPE_STRING;
    result[7].buffer = resultReceiverUsername;
    result[7].buffer_length = sizeof(resultReceiverUsername);
    result[7].length = &receiverUsernameLen;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[MessageDAO::GetUnreadMessages]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[MessageDAO::GetUnreadMessages]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    while(mysql_stmt_fetch(stmt) == 0){
        Message msg;
        msg.messageId = resultMsgId;
        msg.senderId = resultSenderId;
        msg.receiverId = resultReceiverId;
        msg.content = std::string(resultContent, contentLen);
        char timeBuf[64];
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
            resultSendTime.year, resultSendTime.month, resultSendTime.day,
            resultSendTime.hour, resultSendTime.minute, resultSendTime.second);
        msg.sendTime = timeBuf;
        msg.isRead = (resultIsRead == 1);
        msg.senderUsername = std::string(resultSenderUsername, senderUsernameLen);
        msg.receiverUsername = std::string(resultReceiverUsername, receiverUsernameLen);
        messages.push_back(msg);
    }
    std::cout << "[MessageDAO::GetUnreadMessages]查询未读消息成功" << std::endl;
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return true;
}

/**
 * @brief 将两个用户之间未读的消息标记为已读
 * @param senderId 发送者ID
 * @param receiverId 接收者ID
 * @return 更新成功返回true，失败返回false
 */
bool MessageDAO::MarkMessagesAsRead(int senderId, int receiverId){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "UPDATE messages SET is_read = 1 WHERE sender_id = ? AND receiver_id = ? AND is_read = 0";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &senderId;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &receiverId;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    bool result = (mysql_stmt_execute(stmt) == 0);
    if(!result){
        std::cerr << "[MessageDAO::MarkMessagesAsRead]标记消息已读失败: " << mysql_stmt_error(stmt) << std::endl;
    } else {
        std::cout << "[MessageDAO::MarkMessagesAsRead]标记消息已读成功" << std::endl;
    }
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 从数据库查询两个用户之间的历史消息
 * @param userId1 用户ID1
 * @param userId2 用户ID2
 * @param count 查询消息数量
 * @param messages 输出参数：历史消息列表
 * @return 查询成功返回true，失败返回false
 */
bool MessageDAO::GetHistoryMessages(int userId1, int userId2, int count, std::vector<Message>& messages){
    MYSQL* conn = dbManager->GetConnection();
    if(!conn){
        std::cerr << "[MessageDAO::GetHistoryMessages]获取数据库连接失败" << std::endl;
        return false;
    }
    const char* sql = "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
        "u1.username AS sender_username, u2.username AS receiver_username "
        "FROM messages m "
        "INNER JOIN users u1 ON m.sender_id = u1.user_id "
        "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
        "WHERE (m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?) "
        "ORDER BY m.send_time ASC LIMIT ?";
    MYSQL_STMT* stmt = dbManager->PrepareStatement(conn, sql);
    if(!stmt){
        std::cerr << "[MessageDAO::GetHistoryMessages]预编译语句失败" << std::endl;
        dbManager->ReleaseConnection(conn);
        return false;
    }
    MYSQL_BIND bind[5];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &userId1;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &userId2;
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &userId2;
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &userId1;
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &count;
    if(mysql_stmt_bind_param(stmt, bind) != 0){
        std::cerr << "[MessageDAO::GetHistoryMessages]绑定参数失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_execute(stmt) != 0){
        std::cerr << "[MessageDAO::GetHistoryMessages]执行查询失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    int resultMsgId = 0;
    int resultSenderId = 0;
    int resultReceiverId = 0;
    char resultContent[4096];
    MYSQL_TIME resultSendTime;
    int resultIsRead = 0;
    char resultSenderUsername[256];
    char resultReceiverUsername[256];
    unsigned long contentLen = 0;
    unsigned long senderUsernameLen = 0;
    unsigned long receiverUsernameLen = 0;
    MYSQL_BIND result[8];
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONG;
    result[0].buffer = &resultMsgId;
    result[1].buffer_type = MYSQL_TYPE_LONG;
    result[1].buffer = &resultSenderId;
    result[2].buffer_type = MYSQL_TYPE_LONG;
    result[2].buffer = &resultReceiverId;
    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = resultContent;
    result[3].buffer_length = sizeof(resultContent);
    result[3].length = &contentLen;
    result[4].buffer_type = MYSQL_TYPE_DATETIME;
    result[4].buffer = &resultSendTime;
    result[5].buffer_type = MYSQL_TYPE_LONG;
    result[5].buffer = &resultIsRead;
    result[6].buffer_type = MYSQL_TYPE_STRING;
    result[6].buffer = resultSenderUsername;
    result[6].buffer_length = sizeof(resultSenderUsername);
    result[6].length = &senderUsernameLen;
    result[7].buffer_type = MYSQL_TYPE_STRING;
    result[7].buffer = resultReceiverUsername;
    result[7].buffer_length = sizeof(resultReceiverUsername);
    result[7].length = &receiverUsernameLen;
    if(mysql_stmt_bind_result(stmt, result) != 0){
        std::cerr << "[MessageDAO::GetHistoryMessages]绑定结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    if(mysql_stmt_store_result(stmt) != 0){
        std::cerr << "[MessageDAO::GetHistoryMessages]存储结果失败: " << mysql_stmt_error(stmt) << std::endl;
        dbManager->CloseStatement(stmt);
        dbManager->ReleaseConnection(conn);
        return false;
    }
    while(mysql_stmt_fetch(stmt) == 0){
        Message msg;
        msg.messageId = resultMsgId;
        msg.senderId = resultSenderId;
        msg.receiverId = resultReceiverId;
        msg.content = std::string(resultContent, contentLen);
        char timeBuf[64];
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d",
            resultSendTime.year, resultSendTime.month, resultSendTime.day,
            resultSendTime.hour, resultSendTime.minute, resultSendTime.second);
        msg.sendTime = timeBuf;
        msg.isRead = (resultIsRead == 1);
        msg.senderUsername = std::string(resultSenderUsername, senderUsernameLen);
        msg.receiverUsername = std::string(resultReceiverUsername, receiverUsernameLen);
        messages.push_back(msg);
    }
    std::cout << "[MessageDAO::GetHistoryMessages]查询历史消息成功" << std::endl;
    mysql_stmt_free_result(stmt);
    dbManager->CloseStatement(stmt);
    dbManager->ReleaseConnection(conn);
    return true;
}
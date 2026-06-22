#include "message_dao.h"
#include "server_logger.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

/**
 * @brief MessageDAO构造函数，初始化数据库管理器指针
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
 * @brief 向数据库插入一条消息记录
 * @param senderId 发送者ID
 * @param receiverId 接收者ID
 * @param content 消息内容
 * @return 插入成功返回true，失败返回false
 */
bool MessageDAO::InsertMessage(int senderId, int receiverId, const std::string& content){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "INSERT INTO messages (sender_id, receiver_id, content) VALUES (?, ?, ?)"
        );  // 创建预处理语句
        pstmt->setInt(1, senderId);  // 设置第一个参数位为发送者ID
        pstmt->setInt(2, receiverId);  // 设置第二个参数位为接收者ID
        pstmt->setString(3, content);  // 设置第三个参数位为消息内容
        pstmt->executeUpdate();  // 执行预处理语句（插入）
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "MessageDAO", "插入消息到数据库成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "MessageDAO", "插入消息到数据库失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageDAO", "获取数据库连接失败");
        return false;
    }
    bool result = true;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
            "u1.username AS sender_username, u2.username AS receiver_username "
            "FROM messages m "
            "INNER JOIN users u1 ON m.sender_id = u1.user_id "
            "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
            "WHERE m.receiver_id = ? AND m.is_read = 0 "
            "ORDER BY m.send_time ASC"
        );
        pstmt->setInt(1, userId);
        sql::ResultSet* res = pstmt->executeQuery();  // 执行预处理语句（查询）
        while(res->next()){  // 遍历查询下一行结果
            Message msg;
            msg.messageId = res->getInt("message_id");  // 获取消息ID
            msg.senderId = res->getInt("sender_id");  // 获取发送者ID
            msg.receiverId = res->getInt("receiver_id");  // 获取接收者ID
            msg.content = res->getString("content");  // 获取消息内容
            msg.sendTime = res->getString("send_time");  // 获取发送时间
            msg.isRead = res->getBoolean("is_read");  // 获取是否已读
            msg.senderUsername = res->getString("sender_username");  // 获取发送者用户名
            msg.receiverUsername = res->getString("receiver_username");  // 获取接收者用户名
            messages.push_back(msg);  // 将消息添加到列表
        }
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "MessageDAO", "查询未读消息成功");
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "MessageDAO", "查询未读消息失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}

/**
 * @brief 将两个用户之间未读的消息标记为已读
 * @param senderId 发送者ID
 * @param receiverId 接收者ID
 * @return 更新成功返回true，失败返回false
 */
bool MessageDAO::MarkMessagesAsRead(int senderId, int receiverId){
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageDAO", "获取数据库连接失败");
        return false;
    }
    bool result = false;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "UPDATE messages SET is_read = 1 WHERE sender_id = ? AND receiver_id = ? AND is_read = 0"
        );
        pstmt->setInt(1, senderId);
        pstmt->setInt(2, receiverId);
        pstmt->executeUpdate();
        result = true;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "MessageDAO", "执行标记消息已读成功");
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "MessageDAO", "执行标记消息已读失败: " + std::string(e.what())
        );
        result = false;
    }
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
    sql::Connection* conn = dbManager->GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageDAO", "获取数据库连接失败");
        return false;
    }
    bool result = true;
    try{
        sql::PreparedStatement* pstmt = conn->prepareStatement(
            "SELECT m.message_id, m.sender_id, m.receiver_id, m.content, m.send_time, m.is_read, "
            "u1.username AS sender_username, u2.username AS receiver_username "
            "FROM messages m "
            "INNER JOIN users u1 ON m.sender_id = u1.user_id "
            "INNER JOIN users u2 ON m.receiver_id = u2.user_id "
            "WHERE (m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?) "
            "ORDER BY m.send_time ASC LIMIT ?"
        );
        pstmt->setInt(1, userId1);
        pstmt->setInt(2, userId2);
        pstmt->setInt(3, userId2);
        pstmt->setInt(4, userId1);
        pstmt->setInt(5, count);
        sql::ResultSet* res = pstmt->executeQuery();
        while(res->next()){
            Message msg;
            msg.messageId = res->getInt("message_id");
            msg.senderId = res->getInt("sender_id");
            msg.receiverId = res->getInt("receiver_id");
            msg.content = res->getString("content");
            msg.sendTime = res->getString("send_time");
            msg.isRead = res->getBoolean("is_read");
            msg.senderUsername = res->getString("sender_username");
            msg.receiverUsername = res->getString("receiver_username");
            messages.push_back(msg);
        }
        ServerLogger::GetInstance().WriteLog(
            LogLevel::INFO, "MessageDAO",
            "查询历史消息成功, 共" + std::to_string(messages.size()) + "条消息"
        );
        delete res;
        delete pstmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "MessageDAO", "查询历史消息失败: " + std::string(e.what())
        );
        result = false;
    }
    dbManager->ReleaseConnection(conn);
    return result;
}
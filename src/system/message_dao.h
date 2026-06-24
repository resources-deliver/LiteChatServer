#ifndef MESSAGE_DAO_H
#define MESSAGE_DAO_H

#include "db_manager.h"
#include <string>
#include <vector>

/**
 * @brief 消息结构体，用于存储消息信息
 */
struct Message{
    int messageId;  // 消息ID
    int senderId;  // 发送者ID
    int receiverId;  // 接收者ID
    std::string senderUsername;  // 发送者用户名
    std::string receiverUsername;  // 接收者用户名
    std::string content;  // 消息内容
    std::string sendTime;  // 发送时间
    bool isRead;  // 已读状态
};

/**
 * @brief 消息数据访问类，负责消息相关的数据库操作
 */
class MessageDAO{
public:
    MessageDAO(DBManager* dbManager);
    ~MessageDAO();
    bool InsertMessage(int senderId, int receiverId, const std::string& content);
    bool GetUnreadMessages(int userId, std::vector<Message>& messages);
    bool MarkMessagesAsRead(int senderId, int receiverId);
    bool GetHistoryMessages(int userId1, int userId2, int count, std::vector<Message>& messages);

private:
    DBManager* dbManager;  // 数据库管理器指针
};

#endif // MESSAGE_DAO_H
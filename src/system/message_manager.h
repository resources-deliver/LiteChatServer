#ifndef MESSAGE_MANAGER_H
#define MESSAGE_MANAGER_H

#include <string>
#include <functional>
#include "db_manager.h"
#include "session_manager.h"
#include "user_dao.h"
#include "message_dao.h"
#include "user_manager.h"

/**
 * @brief 消息管理类，负责处理消息发送、历史查询、离线推送等业务逻辑
 */
class MessageManager{
public:
    MessageManager(DBManager* dbManager, SessionManager* sessionManager);
    ~MessageManager();
    Response HandleSendMessage(const Request& request);
    Response HandleHistoryMessages(const Request& request);
    void PushOfflineMessages(const std::string& username);
    bool ValidateMessageContent(const std::string& content);
    void SetSendCallback(std::function<bool(int, const std::string&)> callback);

private:
    bool ForwardMessage(int senderId, const std::string& senderUsername, int receiverId,
        const std::string& receiverUsername, const std::string& content, const std::string& sendTime);
    bool StoreMessage(int senderId, int receiverId, const std::string& content);
    void SendNotification(int receiverSocket, const std::string& senderUsername, const std::string& content);

private:
    DBManager* dbManager;  // 数据库管理器指针
    SessionManager* sessionManager;  // 会话管理器指针
    MessageDAO* messageDAO;  // 消息数据访问指针
    UserDAO* userDAO;  // 用户数据访问指针
    std::function<bool(int, const std::string&)> sendCallback;  // 发送回调函数指针
    int maxMessageSize;  // 最大消息体大小（默认64KB）
};

#endif // MESSAGE_MANAGER_H
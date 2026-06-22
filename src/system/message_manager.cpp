#include "message_manager.h"
#include "server_logger.h"
#include <iostream>
#include <cctype>
#include <jsoncpp/json/json.h>

/**
 * @brief MessageManager构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 * @param sessionManager 会话管理器指针
 */
MessageManager::MessageManager(DBManager* dbManager, SessionManager* sessionManager)
    : dbManager(dbManager)
    , sessionManager(sessionManager)
    , messageDAO(new MessageDAO(dbManager))
    , userDAO(new UserDAO(dbManager))
    , maxMessageSize(65536)
{
}

/**
 * @brief MessageManager析构函数，清理资源
 */
MessageManager::~MessageManager(){
    if(messageDAO){
        delete messageDAO;
    }
    if(userDAO){
        delete userDAO;
    }
}

/**
 * @brief 处理发送消息请求
 * @param request 发送消息请求结构体
 * @return 返回响应结构体
 */
Response MessageManager::HandleSendMessage(const Request& request){
    Response response;
    if(!ValidateMessageContent(request.content)){
        response.code = 4001;
        response.msg = "消息内容不能为空";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "发送消息内容为空");
        return response;
    }
    if(request.content.length() > static_cast<size_t>(maxMessageSize)){
        response.code = 4002;
        response.msg = "消息内容过长";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "发送消息内容过长,长度: " + std::to_string(request.content.length()));
        return response;
    }
    int senderId = 0;
    if(!userDAO->GetUserIdByUsername(request.username, senderId)){
        response.code = 4003;
        response.msg = "发送者不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "发送发送者不存在");
        return response;
    }
    int receiverId = 0;
    if(!userDAO->GetUserIdByUsername(request.targetUsername, receiverId)){
        response.code = 4003;
        response.msg = "接收方不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "发送接收方不存在");
        return response;
    }
    if(!StoreMessage(senderId, receiverId, request.content)){
        response.code = 5001;
        response.msg = "数据库操作失败";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "发送存储消息失败");
        return response;
    }
    bool isOnline = false;
    userDAO->GetUserOnlineStatus(receiverId, isOnline);
    if(isOnline){
        ClientSession* receiverSession = sessionManager->GetSessionByUsername(request.targetUsername);
        if(receiverSession && sendCallback){
            ForwardMessage(senderId, request.username, receiverId, request.targetUsername,
                request.content, "");
            SendNotification(receiverSession->GetSocket(), request.username, request.content);
        }
    }
    response.code = 0;
    response.msg = "发送成功";
    ServerLogger::GetInstance().WriteLog(
        LogLevel::INFO, "MessageManager", "发送消息发送成功：" + request.username + " -> " + request.targetUsername
    );
    return response;
}

/**
 * @brief 处理查询历史消息请求
 * @param request 查询历史消息请求结构体
 * @return 返回响应结构体
 */
Response MessageManager::HandleHistoryMessages(const Request& request){
    Response response;
    if(request.targetUsername.empty()){
        response.code = 1001;
        response.msg = "目标用户名不能为空";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "查询目标用户名不能为空");
        return response;
    }
    int userId1 = 0;
    if(!userDAO->GetUserIdByUsername(request.username, userId1)){
        response.code = 4003;
        response.msg = "用户不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "查询用户不存在");
        return response;
    }
    int userId2 = 0;
    if(!userDAO->GetUserIdByUsername(request.targetUsername, userId2)){
        response.code = 4003;
        response.msg = "目标用户不存在";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "查询目标用户不存在");
        return response;
    }
    int count = request.count;
    if(count <= 0 || count > 200){
        count = 50;
    }
    std::vector<Message> messages;
    if(!messageDAO->GetHistoryMessages(userId1, userId2, count, messages)){
        response.code = 5002;
        response.msg = "数据库查询失败";
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "查询数据库查询失败");
        return response;
    }
    Json::Value messagesArray(Json::arrayValue);
    for(auto& msg : messages){
        Json::Value msgObj;
        msgObj["sender"] = msg.senderUsername;
        msgObj["content"] = msg.content;
        msgObj["timestamp"] = msg.sendTime;
        msgObj["is_read"] = msg.isRead;
        messagesArray.append(msgObj);
    }
    Json::Value dataObj;
    dataObj["messages"] = messagesArray;
    Json::FastWriter writer;
    response.data = writer.write(dataObj);
    response.code = 0;
    response.msg = "查询成功";
    ServerLogger::GetInstance().WriteLog(
        LogLevel::INFO, "MessageManager", 
        "查询历史消息成功,用户名: " + request.username + " -> " + request.targetUsername + ", " + std::to_string(messages.size())
    );
    return response;
}

/**
 * @brief 推送离线消息给刚上线的用户
 * @param username 用户名
 */
void MessageManager::PushOfflineMessages(const std::string& username){
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(username, userId)){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "推送用户不存在");
        return;
    }
    std::vector<Message> messages;
    if(!messageDAO->GetUnreadMessages(userId, messages)){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "推送查询消息失败");
        return;
    }
    if(messages.empty()){
        return;
    }
    ClientSession* session = sessionManager->GetSessionByUsername(username);
    if(!session || !sendCallback){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "推送用户会话不存在或发送回调未设置");
        return;
    }
    int clientSocket = session->GetSocket();
    for(auto& msg : messages){
        ForwardMessage(msg.senderId, msg.senderUsername, msg.receiverId, msg.receiverUsername,
            msg.content, msg.sendTime);
        SendNotification(clientSocket, msg.senderUsername, msg.content);
    }
    ServerLogger::GetInstance().WriteLog(
        LogLevel::INFO, "MessageManager", "推送离线消息成功,用户名: " + username + ", " + std::to_string(messages.size())
    );
}

/**
 * @brief 校验消息内容是否合法
 * @param content 消息内容
 * @return 内容非空返回true，否则返回false
 */
bool MessageManager::ValidateMessageContent(const std::string& content){
    if(content.empty()){
        return false;
    }
    for(char c : content){
        if(!std::isspace(static_cast<unsigned char>(c))){
            return true;
        }
    }
    return false;
}

/**
 * @brief 设置发送数据的回调函数
 * @param callback 发送回调函数
 */
void MessageManager::SetSendCallback(std::function<bool(int, const std::string&)> callback){
    sendCallback = callback;
}

/**
 * @brief 转发消息给在线接收方
 * @param senderId 发送者ID
 * @param senderUsername 发送者用户名
 * @param receiverId 接收者ID
 * @param receiverUsername 接收者用户名
 * @param content 消息内容
 * @param sendTime 发送时间
 * @return 转发成功返回true，失败返回false
 */
bool MessageManager::ForwardMessage(int senderId, const std::string& senderUsername,
    int receiverId, const std::string& receiverUsername, const std::string& content,
    const std::string& sendTime){
    ClientSession* receiverSession = sessionManager->GetSessionByUsername(receiverUsername);
    if(!receiverSession){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "MessageManager", "转发接收方不在线");
        return false;
    }
    Json::Value forwardJson;
    forwardJson["type"] = "FORWARD_MSG";
    forwardJson["data"]["sender"] = senderUsername;
    forwardJson["data"]["content"] = content;
    forwardJson["data"]["timestamp"] = sendTime;
    Json::FastWriter writer;
    std::string forwardStr = writer.write(forwardJson);
    if(sendCallback){
        bool result = sendCallback(receiverSession->GetSocket(), forwardStr);
        if(result){
            messageDAO->MarkMessagesAsRead(senderId, receiverId);
        }
        return result;
    }
    return false;
}

/**
 * @brief 发送新消息通知给接收方
 * @param receiverSocket 接收方socket
 * @param senderUsername 发送者用户名
 * @param content 消息内容
 */
void MessageManager::SendNotification(int receiverSocket, const std::string& senderUsername,
    const std::string& content){
    std::string summary = content;
    if(summary.length() > 50){
        summary = summary.substr(0, 50) + "...";
    }
    Json::Value notifyJson;
    notifyJson["type"] = "MSG_NOTIFY";
    notifyJson["data"]["sender"] = senderUsername;
    notifyJson["data"]["summary"] = summary;
    Json::FastWriter writer;
    std::string notifyStr = writer.write(notifyJson);
    if(sendCallback){
        sendCallback(receiverSocket, notifyStr);
    }
    ServerLogger::GetInstance().WriteLog(
        LogLevel::INFO, "MessageManager", "发送消息通知成功,发送者: " + senderUsername
    );
}

/**
 * @brief 存储消息到数据库
 * @param senderId 发送者用户ID
 * @param receiverId 接收者用户ID
 * @param content 消息内容
 * @return 存储成功返回true，失败返回false
 */
bool MessageManager::StoreMessage(int senderId, int receiverId, const std::string& content){
    return messageDAO->InsertMessage(senderId, receiverId, content);
}
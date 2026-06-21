#include "friend_manager.h"
#include <iostream>
#include <jsoncpp/json/json.h>

/**
 * @brief FriendManager构造函数，初始化类内私有属性
 * @param dbManager 数据库管理器指针
 */
FriendManager::FriendManager(DBManager* dbManager)
    : dbManager(dbManager)
    , friendDAO(new FriendDAO(dbManager))
    , userDAO(new UserDAO(dbManager))
{
}

/**
 * @brief FriendManager析构函数，清理资源
 */
FriendManager::~FriendManager(){
    if(friendDAO){
        delete friendDAO;
    }
    if(userDAO){
        delete userDAO;
    }
}

/**
 * @brief 处理添加好友请求
 * @param request 添加好友请求结构体
 * @return 返回响应结构体
 */
Response FriendManager::HandleAddFriend(const Request& request){
    Response response;
    std::string targetUsername = request.targetUsername;
    if(targetUsername.empty()){
        targetUsername = request.username;
    }
    if(targetUsername.empty()){
        response.code = 1001;
        response.msg = "目标用户名不能为空";
        std::cout << "[FriendManager::HandleAddFriend]目标用户名不能为空" << std::endl;
        return response;
    }
    if(request.username == targetUsername){
        response.code = 3003;
        response.msg = "不能添加自己为好友";
        std::cout << "[FriendManager::HandleAddFriend]不能添加自己为好友" << std::endl;
        return response;
    }
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(request.username, userId)){
        response.code = 2003;
        response.msg = "用户不存在";
        std::cout << "[FriendManager::HandleAddFriend]用户不存在" << std::endl;
        return response;
    }
    int targetUserId = 0;
    if(!userDAO->GetUserIdByUsername(targetUsername, targetUserId)){
        response.code = 3001;
        response.msg = "目标用户不存在";
        std::cout << "[FriendManager::HandleAddFriend]目标用户不存在" << std::endl;
        return response;
    }
    if(friendDAO->CheckFriendship(userId, targetUserId)){
        response.code = 3002;
        response.msg = "已经是好友关系";
        std::cout << "[FriendManager::HandleAddFriend]已经是好友关系" << std::endl;
        return response;
    }
    if(!friendDAO->InsertFriend(userId, targetUserId)){
        response.code = 5001;
        response.msg = "数据库操作失败";
        std::cout << "[FriendManager::HandleAddFriend]数据库操作失败" << std::endl;
        return response;
    }
    response.code = 0;
    response.msg = "添加好友成功";
    std::cout << "[FriendManager::HandleAddFriend]添加好友成功：" << request.username << " -> " << targetUsername << std::endl;
    return response;
}

/**
 * @brief 处理删除好友请求（同时删除相关消息记录）
 * @param request 删除好友请求结构体
 * @return 返回响应结构体
 */
Response FriendManager::HandleDeleteFriend(const Request& request){
    Response response;
    std::string targetUsername = request.targetUsername;
    if(targetUsername.empty()){
        targetUsername = request.username;
    }
    if(targetUsername.empty()){
        response.code = 1001;
        response.msg = "目标用户名不能为空";
        std::cout << "[FriendManager::HandleDeleteFriend]目标用户名不能为空" << std::endl;
        return response;
    }
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(request.username, userId)){
        response.code = 2003;
        response.msg = "用户不存在";
        std::cout << "[FriendManager::HandleDeleteFriend]用户不存在" << std::endl;
        return response;
    }
    int targetUserId = 0;
    if(!userDAO->GetUserIdByUsername(targetUsername, targetUserId)){
        response.code = 3001;
        response.msg = "目标用户不存在";
        std::cout << "[FriendManager::HandleDeleteFriend]目标用户不存在" << std::endl;
        return response;
    }
    if(!friendDAO->CheckFriendship(userId, targetUserId)){
        response.code = 3004;
        response.msg = "不是好友关系";
        std::cout << "[FriendManager::HandleDeleteFriend]不是好友关系" << std::endl;
        return response;
    }
    if(!friendDAO->DeleteFriend(userId, targetUserId)){
        response.code = 5001;
        response.msg = "数据库操作失败";
        std::cout << "[FriendManager::HandleDeleteFriend]数据库操作失败" << std::endl;
        return response;
    }
    friendDAO->DeleteMessagesBetweenUsers(userId, targetUserId);
    response.code = 0;
    response.msg = "删除好友成功";
    std::cout << "[FriendManager::HandleDeleteFriend]删除好友成功：" << request.username << " -> " << targetUsername << "（含相关消息记录）" << std::endl;
    return response;
}

/**
 * @brief 处理好友列表查询请求
 * @param request 好友列表查询请求结构体
 * @return 返回响应结构体
 */
Response FriendManager::HandleFriendList(const Request& request){
    Response response;
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(request.username, userId)){
        response.code = 2003;
        response.msg = "用户不存在";
        std::cout << "[FriendManager::HandleFriendList]用户不存在" << std::endl;
        return response;
    }
    std::vector<std::pair<std::string, bool>> friends;
    if(!friendDAO->GetFriendList(userId, friends)){
        response.code = 5002;
        response.msg = "数据库查询失败";
        std::cout << "[FriendManager::HandleFriendList]数据库查询失败" << std::endl;
        return response;
    }
    Json::Value friendsArray(Json::arrayValue);
    for(auto& friendPair : friends){
        Json::Value friendObj;
        friendObj["username"] = friendPair.first;
        friendObj["status"] = friendPair.second ? "online" : "offline";
        friendsArray.append(friendObj);
    }
    Json::Value dataObj;
    dataObj["friends"] = friendsArray;
    Json::FastWriter writer;
    response.data = writer.write(dataObj);
    response.code = 0;
    response.msg = "查询成功";
    std::cout << "[FriendManager::HandleFriendList]查询好友列表成功,用户名: " << request.username << ", 好友数量: " << friends.size() << std::endl;
    return response;
}

/**
 * @brief 处理好友状态查询请求
 * @param request 好友状态查询请求结构体
 * @return 返回响应结构体
 */
Response FriendManager::HandleFriendStatus(const Request& request){
    Response response;
    std::string targetUsername = request.targetUsername;
    if(targetUsername.empty()){
        targetUsername = request.username;
    }
    if(targetUsername.empty()){
        response.code = 1001;
        response.msg = "目标用户名不能为空";
        std::cout << "[FriendManager::HandleFriendStatus]目标用户名不能为空" << std::endl;
        return response;
    }
    int targetUserId = 0;
    if(!userDAO->GetUserIdByUsername(targetUsername, targetUserId)){
        response.code = 3001;
        response.msg = "目标用户不存在";
        std::cout << "[FriendManager::HandleFriendStatus]目标用户不存在" << std::endl;
        return response;
    }
    bool isOnline = false;
    if(!userDAO->GetUserOnlineStatus(targetUserId, isOnline)){
        response.code = 5002;
        response.msg = "数据库查询失败";
        std::cout << "[FriendManager::HandleFriendStatus]数据库查询失败" << std::endl;
        return response;
    }
    Json::Value dataObj;
    dataObj["status"] = isOnline ? "online" : "offline";
    Json::FastWriter writer;
    response.data = writer.write(dataObj);
    response.code = 0;
    response.msg = isOnline ? "在线" : "离线";
    std::cout << "[FriendManager::HandleFriendStatus]查询好友状态成功,目标用户名: " << request.targetUsername << std::endl;
    return response;
}

/**
 * @brief 处理查询好友信息请求
 * @param request 查询好友信息请求结构体
 * @return 返回响应结构体
 */
Response FriendManager::HandleQueryFriend(const Request& request){
    Response response;
    std::string targetUsername = request.targetUsername;
    if(targetUsername.empty()){
        targetUsername = request.username;
    }
    if(targetUsername.empty()){
        response.code = 1001;
        response.msg = "目标用户名不能为空";
        std::cout << "[FriendManager::HandleQueryFriend]目标用户名不能为空" << std::endl;
        return response;
    }
    int targetUserId = 0;
    if(!userDAO->GetUserIdByUsername(targetUsername, targetUserId)){
        response.code = 3001;
        response.msg = "目标用户不存在";
        std::cout << "[FriendManager::HandleQueryFriend]目标用户不存在" << std::endl;
        return response;
    }
    bool isOnline = false;
    if(!userDAO->GetUserOnlineStatus(targetUserId, isOnline)){
        response.code = 5002;
        response.msg = "数据库查询失败";
        std::cout << "[FriendManager::HandleQueryFriend]数据库查询失败" << std::endl;
        return response;
    }
    Json::Value dataObj;
    dataObj["username"] = targetUsername;
    dataObj["status"] = isOnline ? "online" : "offline";
    Json::FastWriter writer;
    response.data = writer.write(dataObj);
    response.code = 0;
    response.msg = "查询成功";
    std::cout << "[FriendManager::HandleQueryFriend]查询好友信息成功,目标用户名: " << targetUsername << std::endl;
    return response;
}

/**
 * @brief 检查两个用户是否为好友关系
 * @param username1 用户名1
 * @param username2 用户名2
 * @return 是好友返回true，否则返回false
 */
bool FriendManager::CheckFriendship(const std::string& username1, const std::string& username2){
    int userId1 = 0;
    int userId2 = 0;
    if(!userDAO->GetUserIdByUsername(username1, userId1)){
        return false;
    }
    if(!userDAO->GetUserIdByUsername(username2, userId2)){
        return false;
    }
    return friendDAO->CheckFriendship(userId1, userId2);
}

/**
 * @brief 缓存用户的好友列表到内存（用户登录成功后调用）
 * @param username 用户名
 */
void FriendManager::CacheFriendList(const std::string& username){
    int userId = 0;
    if(!userDAO->GetUserIdByUsername(username, userId)){
        std::cout << "[FriendManager::CacheFriendList]用户不存在,用户名: " << username << std::endl;
        return;
    }
    std::vector<std::pair<std::string, bool>> friends;
    if(!friendDAO->GetFriendList(userId, friends)){
        std::cout << "[FriendManager::CacheFriendList]查询好友列表失败,用户名: " << username << std::endl;
        return;
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    std::vector<std::string> friendUsernames;
    for(auto& friendPair : friends){
        friendUsernames.push_back(friendPair.first);
    }
    cachedFriendLists[username] = friendUsernames;
    std::cout << "[FriendManager::CacheFriendList]缓存好友列表成功,用户名: " << username << ", 好友数量: " << friendUsernames.size() << std::endl;
}

/**
 * @brief 移除用户的好友列表缓存（用户下线时调用）
 * @param username 用户名
 */
void FriendManager::RemoveFriendCache(const std::string& username){
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cachedFriendLists.find(username);
    if(it != cachedFriendLists.end()){
        cachedFriendLists.erase(it);
        std::cout << "[FriendManager::RemoveFriendCache]移除好友列表缓存成功,用户名: " << username << std::endl;
    }
}

/**
 * @brief 获取缓存的好友列表（仅用户名）
 * @param username 用户名
 * @return 返回好友用户名列表，未缓存返回空列表
 */
std::vector<std::string> FriendManager::GetCachedFriendList(const std::string& username){
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cachedFriendLists.find(username);
    if(it != cachedFriendLists.end()){
        return it->second;
    }
    return std::vector<std::string>();
}
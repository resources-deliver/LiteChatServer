#ifndef FRIEND_MANAGER_H
#define FRIEND_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "db_manager.h"
#include "friend_dao.h"
#include "user_dao.h"
#include "user_manager.h"

/**
 * @brief 好友管理类，负责处理添加/删除好友、好友列表查询等业务逻辑
 */
class FriendManager{
public:
    FriendManager(DBManager* dbManager);
    ~FriendManager();
    Response HandleAddFriend(const Request& request);
    Response HandleDeleteFriend(const Request& request);
    Response HandleFriendList(const Request& request);
    Response HandleFriendStatus(const Request& request);
    Response HandleQueryFriend(const Request& request);
    bool CheckFriendship(const std::string& username1, const std::string& username2);
    void CacheFriendList(const std::string& username);
    void RemoveFriendCache(const std::string& username);
    std::vector<std::string> GetCachedFriendList(const std::string& username);

private:
    DBManager* dbManager;  // 数据库管理器指针
    FriendDAO* friendDAO;  // 好友数据访问指针
    UserDAO* userDAO;  // 用户数据访问指针
    std::unordered_map<std::string, std::vector<std::string>> cachedFriendLists;  // 在线用户好友列表缓存
    std::mutex cacheMutex;  // 缓存互斥锁
};

#endif // FRIEND_MANAGER_H
#ifndef FRIEND_DAO_H
#define FRIEND_DAO_H

#include "db_manager.h"
#include <string>
#include <vector>
#include <utility>

/**
 * @brief 好友数据访问类，负责好友相关的数据库操作
 */
class FriendDAO{
public:
    FriendDAO(DBManager* dbManager);
    ~FriendDAO();
    bool InsertFriend(int userId1, int userId2);
    bool DeleteFriend(int userId1, int userId2);
    bool DeleteMessagesBetweenUsers(int userId1, int userId2);
    bool CheckFriendship(int userId1, int userId2);
    bool GetFriendList(int userId, std::vector<std::pair<std::string, bool>>& friends);

private:
    DBManager* dbManager;  // 数据库管理器指针
};

#endif // FRIEND_DAO_H
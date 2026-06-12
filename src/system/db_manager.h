#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief 数据库管理器类，负责管理MySQL连接池和数据库操作
 */
class DBManager{
public:
    DBManager();
    ~DBManager();
    bool InitConnectionPool(int size = 10);
    MYSQL* GetConnection();
    void ReleaseConnection(MYSQL* conn);
    bool ValidateConnection();
    bool Reconnect();
    MYSQL_RES* ExecuteQuery(MYSQL* conn, const std::string& sql);
    bool ExecuteUpdate(MYSQL* conn, const std::string& sql);

private:
    MYSQL* CreateConnection();
    void DestroyConnection(MYSQL* conn);

private:
    std::queue<MYSQL*> connectionPool;
    int poolSize;
    std::mutex poolMutex;
    std::condition_variable poolCondition;
    std::string dbHost;
    int dbPort;
    std::string dbName;
    std::string dbUser;
    std::string dbPassword;
};

#endif // DB_MANAGER_H
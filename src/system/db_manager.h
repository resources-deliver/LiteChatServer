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
    std::queue<MYSQL*> connectionPool;  // MYSQL连接池队列
    int poolSize;  // 连接池大小
    std::mutex poolMutex;  // 连接池互斥锁
    std::condition_variable poolCondition;  // 连接池条件变量
    std::string dbHost;  // 数据库主机地址
    int dbPort;  // 数据库端口号
    std::string dbName;  // 数据库名称
    std::string dbUser;  // 数据库用户名
    std::string dbPassword;  // 数据库密码
};

#endif // DB_MANAGER_H
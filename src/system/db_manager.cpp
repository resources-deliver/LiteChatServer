#include "db_manager.h"
#include "server_logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

/**
 * @brief DBManager构造函数，初始化类内私有成员属性
 */
DBManager::DBManager()
    : driver(nullptr)
    , poolSize(0)
    , dbHost("tcp://127.0.0.1:3306")
    , dbPort(3306)
    , dbName("LiteChat_db")
    , dbUser("LiteChat_user")
    , dbPassword("LiteChat_password")
{
}

/**
 * @brief DBManager析构函数，释放连接池中所有连接资源
 */
DBManager::~DBManager(){
    std::lock_guard<std::mutex> lock(poolMutex);  // 加锁保护共享资源
    int shutDownCount = 0;  // 统计关闭的连接数量
    while(!connectionPool.empty()){
        sql::Connection* conn = connectionPool.front();  // 获取连接池首个连接
        connectionPool.pop();  // 从连接池中移除首个连接
        DestroyConnection(conn);
        shutDownCount++;
        if(shutDownCount == poolSize){
            ServerLogger::GetInstance().WriteLog(
                LogLevel::INFO, "DBManager", "数据库连接已关闭" + std::to_string(shutDownCount) + "个连接"
            );
        }
    }
}

/**
 * @brief 初始化连接池
 * @param size 连接池大小，默认10
 * @return 初始化成功返回true，失败返回false
 */
bool DBManager::InitConnectionPool(int size){
    poolSize = size;
    try{
        driver = sql::mysql::get_mysql_driver_instance();  // 获取MySQL驱动实例
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "DBManager", "获取MySQL驱动实例失败:" + std::string(e.what())
        );
        return false;
    }
    int createCount = 0;  // 统计创建的连接数量
    for(int i = 0; i < size; ++i){
        sql::Connection* conn = CreateConnection();
        if(!conn){
            ServerLogger::GetInstance().WriteLog(
                LogLevel::ERROR, "DBManager", "创建连接" + std::to_string(i + 1) + "失败"
            );
            return false;
        }
        createCount++;
        connectionPool.push(conn);  // 将新创建的连接添加到连接池队列中
    }
    if(createCount == poolSize){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::INFO, "DBManager", "数据库连接池已创建" + std::to_string(createCount) + "个连接"
        );
    }
    return ValidateConnection();
}

/**
 * @brief 从连接池获取可用连接
 * @return 返回连接指针，如果连接池耗尽则等待
 */
sql::Connection* DBManager::GetConnection(){
    std::unique_lock<std::mutex> lock(poolMutex);
    poolCondition.wait(lock, [this] { return !connectionPool.empty(); });  // 用条件变量等待连接池非空
    if(connectionPool.empty()){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "DBManager", "连接池为空，无法获取连接");
        return nullptr;
    }
    sql::Connection* conn = connectionPool.front();
    connectionPool.pop();
    return conn;
}

/**
 * @brief 将连接重新添加到连接池队列中
 * @param conn 要释放的连接指针
 */
void DBManager::ReleaseConnection(sql::Connection* conn){
    std::lock_guard<std::mutex> lock(poolMutex);
    connectionPool.push(conn);
    poolCondition.notify_one();  // 通知等待连接的线程
}

/**
 * @brief 验证连接池中的连接可用性
 * @return 连接可用返回true，失败返回false
 */
bool DBManager::ValidateConnection(){
    sql::Connection* conn = GetConnection();
    if(!conn){
        ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "DBManager", "获取数据库连接失败");
        return false;
    }
    bool valid = false;
    try{
        sql::Statement* stmt = conn->createStatement();  // 创建SQL语句对象
        sql::ResultSet* res = stmt->executeQuery("SELECT 1");  // 执行查询语句
        if(res->next()){  // 如果查询结果集有下一行数据
            valid = true;  // 连接可用
        }
        delete res;
        delete stmt;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "DBManager", "验证连接失败:" + std::string(e.what())
        );
        valid = false;
    }
    ReleaseConnection(conn);
    return valid;
}

/**
 * @brief 重连数据库（最多3次，间隔5秒）
 * @return 重连成功返回true，失败返回false
 */
bool DBManager::Reconnect(){
    for(int i = 0; i < 3; ++i){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::INFO, "DBManager", "尝试重连数据库连接(尝试" + std::to_string(i + 1) + "/ 3)..."
        );
        bool reconnectSuccess = InitConnectionPool(poolSize);
        if(reconnectSuccess){
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "DBManager", "数据库连接重连成功");
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "DBManager", "3次重连失败");
    return false;
}

/**
 * @brief 创建单个数据库连接
 * @return 返回连接指针，失败返回nullptr
 */
sql::Connection* DBManager::CreateConnection(){
    try{
        sql::Connection* conn = driver->connect(dbHost, dbUser, dbPassword);  // 创建数据库连接
        conn->setSchema(dbName);  // 设置数据库名称
        sql::Statement* stmt = conn->createStatement();
        stmt->execute("SET NAMES utf8mb4");  // 设置字符集为utf8mb4
        delete stmt;
        return conn;
    }
    catch(sql::SQLException& e){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "DBManager", "创建数据库连接失败:" + std::string(e.what())
        );
        return nullptr;
    }
}

/**
 * @brief 销毁数据库连接
 * @param conn 要销毁的连接指针
 */
void DBManager::DestroyConnection(sql::Connection* conn){
    if(conn){
        delete conn;
    }
}
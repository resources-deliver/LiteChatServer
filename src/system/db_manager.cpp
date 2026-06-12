#include "db_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief DBManager构造函数，初始化数据库连接参数
 */
DBManager::DBManager()
    : poolSize(0)
    , dbHost("127.0.0.1")
    , dbPort(3306)
    , dbName("LiteChat_db")
    , dbUser("LiteChat_user")
    , dbPassword("LiteChat_password")
{
}

/**
 * @brief DBManager析构函数，释放连接池中所有连接
 */
DBManager::~DBManager(){
    std::lock_guard<std::mutex> lock(poolMutex);
    while(!connectionPool.empty()){
        MYSQL* conn = connectionPool.front();
        connectionPool.pop();
        DestroyConnection(conn);
    }
}

/**
 * @brief 初始化数据库连接池
 * @param size 连接池大小，默认10
 * @return 初始化成功返回true，失败返回false
 */
bool DBManager::InitConnectionPool(int size){
    poolSize = size;
    for(int i = 0; i < size; ++i){
        MYSQL* conn = CreateConnection();
        if(!conn){
            std::cerr << "创建数据库连接 " << i + 1 << " 失败" << std::endl;
            return false;
        }
        connectionPool.push(conn);
    }
    return ValidateConnection();
}

/**
 * @brief 从连接池获取可用连接
 * @return 返回MySQL连接指针，如果连接池耗尽则等待
 */
MYSQL* DBManager::GetConnection(){
    std::unique_lock<std::mutex> lock(poolMutex);
    poolCondition.wait(lock, [this] { return !connectionPool.empty(); });
    
    MYSQL* conn = connectionPool.front();
    connectionPool.pop();
    return conn;
}

/**
 * @brief 释放连接（归还连接池）
 * @param conn 要释放的MySQL连接指针
 */
void DBManager::ReleaseConnection(MYSQL* conn){
    std::lock_guard<std::mutex> lock(poolMutex);
    connectionPool.push(conn);
    poolCondition.notify_one();
}

/**
 * @brief 验证数据库连接可用性
 * @return 连接可用返回true，失败返回false
 */
bool DBManager::ValidateConnection(){
    MYSQL* conn = GetConnection();
    if(!conn){
        return false;
    }
    
    bool valid = (mysql_query(conn, "SELECT 1") == 0);
    if(valid){
        MYSQL_RES* result = mysql_store_result(conn);
        if(result){
            mysql_free_result(result);
        }
        else{
            valid = false;
        }
    }
    ReleaseConnection(conn);
    return valid;
}

/**
 * @brief 重连数据库（最多3次，间隔5秒，超时30秒）
 * @return 重连成功返回true，失败返回false
 */
bool DBManager::Reconnect(){
    for(int i = 0; i < 3; ++i){
        std::cout << "尝试重连数据库连接 (尝试 " << i + 1 << "/3)..." << std::endl;
        if(InitConnectionPool(poolSize)){
            std::cout << "数据库连接重连成功" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::cerr << "Failed to reconnect to database after 3 attempts." << std::endl;
    return false;
}

/**
 * @brief 执行查询SQL
 * @param conn MySQL连接指针
 * @param sql SQL查询语句
 * @return 返回查询结果集，失败返回nullptr
 */
MYSQL_RES* DBManager::ExecuteQuery(MYSQL* conn, const std::string& sql){
    if(mysql_query(conn, sql.c_str())){
        std::cerr << "查询失败: " << mysql_error(conn) << std::endl;
        return nullptr;
    }
    return mysql_store_result(conn);
}

/**
 * @brief 执行更新SQL
 * @param conn MySQL连接指针
 * @param sql SQL更新语句
 * @return 执行成功返回true，失败返回false
 */
bool DBManager::ExecuteUpdate(MYSQL* conn, const std::string& sql){
    if(mysql_query(conn, sql.c_str())){
        std::cerr << "更新失败: " << mysql_error(conn) << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief 创建单个数据库连接
 * @return 返回MySQL连接指针，失败返回nullptr
 */
MYSQL* DBManager::CreateConnection(){
    MYSQL* conn = mysql_init(nullptr);
    if(!conn){
        std::cerr << "初始化数据库连接失败" << std::endl;
        return nullptr;
    }

    unsigned int timeout = 30;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    MYSQL* tryconn = mysql_real_connect(
        conn, 
        dbHost.c_str(), 
        dbUser.c_str(), 
        dbPassword.c_str(), 
        dbName.c_str(), 
        dbPort, 
        nullptr, 
        0
    );
    if(!tryconn){
        std::cerr << "连接数据库失败: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return nullptr;
    }

    mysql_set_character_set(conn, "utf8mb4");
    return conn;
}

/**
 * @brief 销毁数据库连接
 * @param conn 要销毁的MySQL连接指针
 */
void DBManager::DestroyConnection(MYSQL* conn){
    if(!conn){
        mysql_close(conn);
    }
}
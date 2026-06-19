#include "db_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief DBManager构造函数，初始化类内私有成员属性
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
 * @brief DBManager析构函数，释放连接池中所有MYSQL连接资源
 */
DBManager::~DBManager(){
    std::lock_guard<std::mutex> lock(poolMutex);  // 加锁使得共享资源同一时间只有一个线程访问
    int shutDownCount = 0;  // 记录关闭连接的次数
    while(!connectionPool.empty()){  // 若MYSQL连接池不为空
        MYSQL* conn = connectionPool.front();  // 获取连接池队头连接
        connectionPool.pop();  // 从连接池队列中移除队头连接
        DestroyConnection(conn);  // 关闭连接资源
        shutDownCount++;
        if(shutDownCount == poolSize){
            std::cout << "[DBManager::~DBManager]数据库连接已关闭" << shutDownCount << "个连接" << std::endl;
        }
    }
}

/**
 * @brief 初始化MYSQL连接池
 * @param size 连接池大小，默认10
 * @return 初始化成功返回true，失败返回false
 */
bool DBManager::InitConnectionPool(int size){
    poolSize = size;  // 初始化连接池大小
    int createCount = 0;  // 记录创建连接的次数
    for(int i = 0; i < size; ++i){  // 创建指定数量的MYSQL连接
        MYSQL* conn = CreateConnection();  // 创建MYSQL连接
        if(!conn){  // 如果创建连接失败
            std::cerr << "[DBManager::InitConnectionPool]创建MYSQL连接" << i + 1 << "失败" << std::endl;
            return false;
        }
        createCount++;
        connectionPool.push(conn);  // 将连接添加到连接池队列中
    }
    if(createCount == poolSize){
        std::cout << "[DBManager::InitConnectionPool]数据库连接池已创建" << createCount << "个连接" << std::endl;
    }
    return ValidateConnection();  // 验证连接池中的连接是否可用
}

/**
 * @brief 从连接池获取可用连接
 * @return 返回MySQL连接指针，如果连接池耗尽则等待
 */
MYSQL* DBManager::GetConnection(){
    std::unique_lock<std::mutex> lock(poolMutex);
    poolCondition.wait(lock, [this] { return !connectionPool.empty(); });  // 使用条件变量等待连接池有可用连接
    if(connectionPool.empty()){  // 如果连接池为空
        std::cerr << "[DBManager::GetConnection]连接池为空，无法获取连接" << std::endl;
        return nullptr;
    }
    MYSQL* conn = connectionPool.front();  // 获取连接池队头连接
    connectionPool.pop();  // 从连接池队列中移除队头连接
    return conn;
}

/**
 * @brief 释放连接
 * @param conn 要释放的MySQL连接指针
 */
void DBManager::ReleaseConnection(MYSQL* conn){
    std::lock_guard<std::mutex> lock(poolMutex);
    connectionPool.push(conn);  // 将连接添加到连接池队列中
    poolCondition.notify_one();  // 使用条件变量通知1个线程：等待连接池有可用连接的线程
}

/**
 * @brief 验证连接池中的连接可用性
 * @return 连接可用返回true，失败返回false
 */
bool DBManager::ValidateConnection(){
    MYSQL* conn = GetConnection();  // 从连接池获取可用连接
    if(!conn){  // 如果获取连接失败
        std::cerr << "[DBManager::ValidateConnection]获取数据库连接失败" << std::endl;
        return false;
    }
    bool valid = (mysql_query(conn, "SELECT 1") == 0);  // 执行查询"SELECT 1"语句
    if(valid){  // 如果查询成功
        MYSQL_RES* result = mysql_store_result(conn);  // 存储查询结果
        if(result){  // 如果存储查询结果成功
            mysql_free_result(result);  // 释放查询结果内存
        }
        else{
            std::cerr << "[DBManager::ValidateConnection]存储查询结果失败" << std::endl;
            valid = false;  // 标记查询无效
        }
    }
    ReleaseConnection(conn);  // 释放为了验证而获取的连接
    return valid;
}

/**
 * @brief 重连数据库（最多3次，间隔5秒，超时30秒）
 * @return 重连成功返回true，失败返回false
 */
bool DBManager::Reconnect(){
    for(int i = 0; i < 3; ++i){  // 尝试重连3次
        std::cout << "[DBManager::Reconnect]尝试重连数据库连接(尝试" << i + 1 << "/3)..." << std::endl;
        if(InitConnectionPool(poolSize)){  // 如果初始化连接池成功
            std::cout << "[DBManager::Reconnect]数据库连接重连成功" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));  // 间隔5秒
    }
    std::cerr << "[DBManager::Reconnect]3次重连失败" << std::endl;
    return false;
}

/**
 * @brief 执行查询SQL
 * @param conn MySQL连接指针
 * @param sql SQL查询语句
 * @return 返回查询结果集，失败返回nullptr
 */
MYSQL_RES* DBManager::ExecuteQuery(MYSQL* conn, const std::string& sql){
    if(mysql_query(conn, sql.c_str())){  // 如果执行查询SQL语句失败
        std::cerr << "[DBManager::ExecuteQuery]SQL语句查询失败:" << mysql_error(conn) << std::endl;
        return nullptr;
    }
    MYSQL_RES* result = mysql_store_result(conn);  // 存储查询结果
    return result;
}

/**
 * @brief 执行更新SQL
 * @param conn MySQL连接指针
 * @param sql SQL更新语句
 * @return 执行成功返回true，失败返回false
 */
bool DBManager::ExecuteUpdate(MYSQL* conn, const std::string& sql){
    if(mysql_query(conn, sql.c_str())){  // 如果执行更新SQL语句失败
        std::cerr << "[DBManager::ExecuteUpdate]SQL语句更新失败:" << mysql_error(conn) << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief 创建单个数据库连接
 * @return 返回MySQL连接指针，失败返回nullptr
 */
MYSQL* DBManager::CreateConnection(){
    MYSQL* conn = mysql_init(nullptr);  // 初始化MySQL连接
    if(!conn){  // 如果初始化连接失败
        std::cerr << "[DBManager::CreateConnection]初始化数据库连接失败" << std::endl;
        return nullptr;
    }
    int timeout = 30;  // 设置连接超时时间为30秒
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);  // 设置连接选项
    MYSQL* tryconn = mysql_real_connect(
        conn, 
        dbHost.c_str(), 
        dbUser.c_str(), 
        dbPassword.c_str(), 
        dbName.c_str(), 
        dbPort, 
        nullptr, 
        0
    );  // 尝试连接数据库
    if(!tryconn){  // 如果连接数据库失败
        std::cerr << "[DBManager::CreateConnection]连接数据库失败:" << mysql_error(conn) << std::endl;
        mysql_close(conn);  // 关闭连接
        return nullptr;
    }
    mysql_set_character_set(conn, "utf8mb4");  // 设置字符集为utf8mb4
    return conn;
}

/**
 * @brief 销毁数据库连接
 * @param conn 要销毁的MySQL连接指针
 */
void DBManager::DestroyConnection(MYSQL* conn){
    if(conn){  // 如果连接指针不为空
        mysql_close(conn);  // 关闭连接
    }
}
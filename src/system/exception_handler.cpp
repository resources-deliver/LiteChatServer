#include "exception_handler.h"
#include "db_manager.h"
#include "server_logger.h"
#include <sstream>
#include <thread>
#include <chrono>
#include <unistd.h>

/**
 * @brief 异常处理类构造函数
 * @param dbManager 数据库管理器指针
 */
ExceptionHandler::ExceptionHandler(DBManager* dbManager)
    : dbManager(dbManager)
{
}

/**
 * @brief 异常处理类析构函数，释放数据库管理器指针
 */
ExceptionHandler::~ExceptionHandler(){}

/**
 * @brief 处理通用异常，记录异常信息到日志
 * @param e 异常对象
 * @param context 异常发生的上下文描述
 */
void ExceptionHandler::HandleException(const std::exception& e, const std::string& context){
    std::ostringstream oss;  // 创建字符串流对象
    oss << "异常内容 [" << context << "]: " << e.what();  // 构建异常信息字符串
    ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "ExceptionHandler", oss.str());
}

/**
 * @brief 处理单个客户端异常，关闭客户端连接但不影响其他客户端
 * @param clientSocket 发生异常的客户端套接字
 * @param e 异常对象
 */
void ExceptionHandler::HandleClientException(int clientSocket, const std::exception& e){
    std::ostringstream oss;
    oss << "客户端套接字 [" << clientSocket << "] 异常: " << e.what() << ", 关闭连接";
    ServerLogger::GetInstance().WriteLog(LogLevel::WARNING, "ExceptionHandler", oss.str());
    close(clientSocket);
}

/**
 * @brief 处理数据库异常，尝试自动重连（最多3次，间隔5秒）
 * @param e 异常对象
 */
void ExceptionHandler::HandleDBException(const std::exception& e){
    std::ostringstream oss;
    oss << "数据库异常: " << e.what() << ", 尝试重连";
    ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "ExceptionHandler", oss.str());
    if(!dbManager){
        ServerLogger::GetInstance().WriteLog(
            LogLevel::ERROR, "ExceptionHandler", "数据库管理器指针为空，无法尝试重连"
        );
        return;
    }
    int maxRetries = 3;
    int retryInterval = 5;
    for(int i = 1; i <= maxRetries; i++){
        std::ostringstream oss;
        oss << "重连尝试 " << i << "/" << maxRetries;
        ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "ExceptionHandler", oss.str());
        bool reconn = dbManager->Reconnect();
        if(reconn){
            ServerLogger::GetInstance().WriteLog(LogLevel::INFO, "ExceptionHandler", "重连成功");
            return;
        }
        if(i < maxRetries){
            std::this_thread::sleep_for(std::chrono::seconds(retryInterval));
        }
    }
    ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "ExceptionHandler", "重连失败，最大重试次数已超过");
}

/**
 * @brief 处理网络异常，记录异常信息并尝试恢复
 * @param e 异常对象
 */
void ExceptionHandler::HandleNetworkException(const std::exception& e){
    std::ostringstream oss;
    oss << "网络异常: " << e.what();
    ServerLogger::GetInstance().WriteLog(LogLevel::ERROR, "ExceptionHandler", oss.str());
}
#ifndef SERVER_LOGGER_H
#define SERVER_LOGGER_H

#include <string>
#include <mutex>
#include <cstdio>

/**
 * @brief 日志级别枚举，用于表示日志的严重程度
 */
enum class LogLevel {DEBUG, INFO, WARNING, ERROR};

/**
 * @brief 服务器日志记录器类，负责多线程安全的日志写入、轮转和清理
 */
class ServerLogger{
public:
    ServerLogger();
    ~ServerLogger();
    void InitLogger(const std::string& logDir);
    void WriteLog(LogLevel level, const std::string& module, const std::string& message, const std::string& clientIP = "");
    void RotateLogFile();
    void CleanExpiredLogs(int retentionDays = 90);
    std::string GetTimestamp();
    std::string FilterSensitiveInfo(const std::string& message);

private:
    std::string logFilePath;  // 日志文件路径
    FILE* logFile;  // 日志文件指针
    size_t maxLogSize;  // 日志文件最大大小（字节）
    size_t currentLogSize;  // 当前日志文件大小（字节）
    std::mutex logMutex;  // 日志写入互斥锁
    int logRetentionDays;  // 日志文件保留天数
    std::string logDir;  // 日志目录
};

#endif // SERVER_LOGGER_H
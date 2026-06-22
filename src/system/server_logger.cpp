#include "server_logger.h"
#include <sys/stat.h>
#include <ctime>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <unistd.h>

/**
 * @brief 构造函数，初始化日志记录器
 */
ServerLogger::ServerLogger()
    : logFilePath("")
    , logFile(nullptr)
    , maxLogSize(10 * 1024 * 1024)
    , currentLogSize(0)
    , logRetentionDays(90)
    , logDir("")
{
}

/**
 * @brief 析构函数，关闭日志文件并释放资源
 */
ServerLogger::~ServerLogger(){
    if(logFile){
        fclose(logFile);
        logFile = nullptr;
    }
}

/**
 * @brief 初始化日志记录器，创建日志目录并打开日志文件
 * @param logDir 日志文件存放目录路径
 */
void ServerLogger::InitLogger(const std::string& logDir){
    this->logDir = logDir;
    struct stat st;
    if(stat(logDir.c_str(), &st) != 0){
        mkdir(logDir.c_str(), 0755);
    }
    RotateLogFile();
    CleanExpiredLogs(logRetentionDays);
}

/**
 * @brief 多线程安全地写入一条日志记录
 * @param level 日志级别
 * @param module 模块名称
 * @param message 日志内容
 * @param clientIP 客户端IP地址，可为空
 */
void ServerLogger::WriteLog(LogLevel level, const std::string& module, const std::string& message, const std::string& clientIP){
    std::lock_guard<std::mutex> lock(logMutex);
    if(!logFile){
        return;
    }
    std::string levelStr;
    switch(level){
        case LogLevel::DEBUG:
            levelStr = "DEBUG";
            break;
        case LogLevel::INFO:
            levelStr = "INFO";
            break;
        case LogLevel::WARNING:
            levelStr = "WARNING";
            break;
        case LogLevel::ERROR:
            levelStr = "ERROR";
            break;
    }
    std::string filteredMessage = FilterSensitiveInfo(message);
    std::string timestamp = GetTimestamp();
    std::ostringstream logEntry;
    logEntry << "[" << timestamp << "] [" << levelStr << "] [" << module << "]";
    if(!clientIP.empty()){
        logEntry << " [" << clientIP << "]";
    }
    logEntry << " " << filteredMessage << std::endl;
    std::string entry = logEntry.str();
    size_t written = fwrite(entry.c_str(), 1, entry.length(), logFile);
    fflush(logFile);
    currentLogSize += written;
    if(currentLogSize >= maxLogSize){
        RotateLogFile();
    }
}

/**
 * @brief 轮转日志文件，按日期命名新文件
 */
void ServerLogger::RotateLogFile(){
    if(logFile){
        fclose(logFile);
        logFile = nullptr;
    }
    time_t now = time(nullptr);
    struct tm* tmNow = localtime(&now);
    char dateBuf[32];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", tmNow);
    std::ostringstream filePath;
    filePath << logDir << "/server_log_" << dateBuf << ".txt";
    logFilePath = filePath.str();
    logFile = fopen(logFilePath.c_str(), "a");
    if(logFile){
        fseek(logFile, 0, SEEK_END);
        currentLogSize = ftell(logFile);
    }
    else{
        currentLogSize = 0;
    }
}

/**
 * @brief 清理超过保留天数的过期日志文件
 * @param retentionDays 日志保留天数，默认90天
 */
void ServerLogger::CleanExpiredLogs(int retentionDays){
    logRetentionDays = retentionDays;
    DIR* dir = opendir(logDir.c_str());
    if(!dir){
        return;
    }
    time_t now = time(nullptr);
    time_t cutoff = now - static_cast<time_t>(retentionDays) * 86400;
    struct dirent* entry;
    while((entry = readdir(dir)) != nullptr){
        std::string filename(entry->d_name);
        if(filename.find("server_log_") != 0 || filename.find(".txt") == std::string::npos) {
            continue;
        }
        std::string fullPath = logDir + "/" + filename;
        struct stat fileStat;
        if(stat(fullPath.c_str(), &fileStat) == 0){
            if(fileStat.st_mtime < cutoff){
                unlink(fullPath.c_str());
            }
        }
    }
    closedir(dir);
}

/**
 * @brief 获取当前时间戳字符串，格式为 yyyy-MM-dd HH:mm:ss
 * @return 格式化的时间戳字符串
 */
std::string ServerLogger::GetTimestamp(){
    time_t now = time(nullptr);
    struct tm* tmNow = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmNow);
    return std::string(buf);
}

/**
 * @brief 过滤日志中的敏感信息，将密码字段替换为 ***
 * @param message 原始日志消息
 * @return 过滤后的日志消息
 */
std::string ServerLogger::FilterSensitiveInfo(const std::string& message){
    std::string result = message;
    size_t pos = 0;
    while((pos = result.find("password", pos)) != std::string::npos){
        size_t colonPos = result.find(':', pos);
        if(colonPos != std::string::npos && colonPos + 1 < result.length()){
            if(result[colonPos + 1] == '"'){
                size_t valueStart = colonPos + 2;
                size_t valueEnd = result.find('"', valueStart);
                if(valueEnd != std::string::npos){
                    result.replace(valueStart, valueEnd - valueStart, "***");
                    pos = valueEnd + 1;
                    continue;
                }
            }
        }
        pos++;
    }
    return result;
}
#ifndef EXCEPTION_HANDLER_H
#define EXCEPTION_HANDLER_H

#include <string>
#include <exception>

class DBManager;

/**
 * @brief 异常处理类，负责处理服务器各类异常情况
 */
class ExceptionHandler{
public:
    ExceptionHandler(DBManager* dbManager);
    ~ExceptionHandler();
    void HandleException(const std::exception& e, const std::string& context);
    void HandleClientException(int clientSocket, const std::exception& e);
    void HandleDBException(const std::exception& e);
    void HandleNetworkException(const std::exception& e);

private:
    DBManager* dbManager;  // 数据库管理器指针
};

#endif // EXCEPTION_HANDLER_H
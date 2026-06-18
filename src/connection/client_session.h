#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <string>
#include <ctime>

/**
 * @brief 客户端会话类，存储客户端连接信息
*/
class ClientSession{
public:
    ClientSession();
    ClientSession(int socket, const std::string& ip, int port);
    ~ClientSession();
    int GetSocket() const;
    std::string GetIP() const;
    int GetPort() const;
    std::string GetUsername() const;
    time_t GetLastHeartbeat() const;
    bool GetIsConnected() const;
    void SetUsername(const std::string& username);
    void UpdateHeartbeat();
    void SetIsConnected(bool connected);

private:
    int socket;  // 客户端套接字描述符
    std::string ip;  // 客户端IP地址
    int port;  // 客户端端口号
    std::string username;  // 客户端用户名
    time_t lastHeartbeat;  // 客户端最后连接时间
    bool isConnected;  // 客户端连接状态
};

#endif // CLIENT_SESSION_H
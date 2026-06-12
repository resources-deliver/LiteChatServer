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
    int socket;
    std::string ip;
    int port;
    std::string username;
    time_t lastHeartbeat;
    bool isConnected;
};

#endif // CLIENT_SESSION_H
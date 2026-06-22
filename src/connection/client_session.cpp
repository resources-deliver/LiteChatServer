#include "client_session.h"

/**
 * @brief ClientSession默认构造函数，初始化类内私有属性
 */
ClientSession::ClientSession() 
    : socket(-1)
    , ip("")
    , port(0)
    , username("")
    , lastHeartbeat(0)
    , isConnected(false)
{
}

/**
 * @brief ClientSession带参数构造函数，初始化类内私有属性
 * @param socket 客户端socket文件描述符
 * @param ip 客户端IP地址
 * @param port 客户端端口
 */
ClientSession::ClientSession(int socket, const std::string& ip, int port)
    : socket(socket)
    , ip(ip)
    , port(port)
    , username("")
    , lastHeartbeat(time(nullptr))
    , isConnected(true)
{
}

/**
 * @brief ClientSession析构函数
 */
ClientSession::~ClientSession(){}

/**
 * @brief 获取客户端套接字描述符
 * @return 返回客户端socket文件描述符
 */
int ClientSession::GetSocket() const{
    return socket;
}

/**
 * @brief 获取客户端IP地址
 * @return 返回客户端IP地址字符串
 */
std::string ClientSession::GetIP() const{
    return ip;
}

/**
 * @brief 获取客户端端口
 * @return 返回客户端端口号
 */
int ClientSession::GetPort() const{
    return port;
}

/**
 * @brief 获取关联的用户名
 * @return 返回用户名，未登录时返回空字符串
 */
std::string ClientSession::GetUsername() const{
    return username;
}

/**
 * @brief 获取最后连接时间
 * @return 返回最后连接时间戳
 */
time_t ClientSession::GetLastHeartbeat() const{
    return lastHeartbeat;
}

/**
 * @brief 获取客户端连接状态
 * @return 返回客户端连接状态，true表示已连接，false表示未连接
 */
bool ClientSession::GetIsConnected() const{
    return isConnected;
}

/**
 * @brief 设置关联的用户名
 * @param username 用户名
 */
void ClientSession::SetUsername(const std::string& username){
    this->username = username;
}

/**
 * @brief 更新客户端连接时间
 */
void ClientSession::UpdateHeartbeat(){
    lastHeartbeat = time(nullptr);
}

/**
 * @brief 设置客户端连接状态
 * @param connected 连接状态
 */
void ClientSession::SetIsConnected(bool connected){
    isConnected = connected;
}
#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

/**
 * @brief Server构造函数，初始化服务器参数
*/
Server::Server()
    : serverSocket(-1)
    , serverIP("0.0.0.0")
    , serverPort(8886)
    , maxConnections(128)
    , currentConnections(0)
    , threadPool(nullptr)
    , dbManager(nullptr)
    , running(false)
{
}

/**
 * @brief Server析构函数，清理资源
 */
Server::~Server(){
    Stop();
    if(threadPool){
        delete threadPool;
    }
    if(dbManager){
        delete dbManager;
    }
}

/**
 * @brief 启动服务器，创建socket、绑定、监听
 * @return 启动成功返回true，失败返回false
 */
bool Server::Start(){
    threadPool = new ThreadPool(10);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        std::cerr << "创建socket失败" << std::endl;
        return false;
    }

    int opt = 1;
    int resetport = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(resetport == -1){
        std::cerr << "设置端口复用失败" << std::endl;
        close(serverSocket);
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(serverPort);

    int bindres = bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(bindres == -1){
        std::cerr << "绑定IP和端口失败" << std::endl;
        close(serverSocket);
        return false;
    }

    int listenres = listen(serverSocket, maxConnections);
    if(listenres == -1){
        std::cerr << "监听失败" << std::endl;
        close(serverSocket);
        return false;
    }

    std::cout << "服务器启动成功，监听端口" << serverPort << std::endl;
    running = true;
    return true;
}

/**
 * @brief 停止服务器，关闭socket
 */
void Server::Stop(){
    running = false;
    
    std::lock_guard<std::mutex> lock(sessionMutex);
    for(auto& pair : sessionMap){
        if(pair.second->GetSocket() != -1){
            shutdown(pair.second->GetSocket(), SHUT_RDWR);
        }
    }
    
    if(serverSocket != -1){
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        serverSocket = -1;
    }
}

/**
 * @brief 接受客户端连接循环
 */
void Server::AcceptConnections(){
    while(running){
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if(clientSocket == -1){
            if(!running){
                break;
            }
            std::cerr << "接受客户端连接失败" << std::endl;
            continue;
        }

        if(currentConnections.load() >= maxConnections){
            std::string response = PackageMessage("{\"type\":\"ERROR\",\"code\":5003,\"msg\":\"服务器繁忙\"}");
            send(clientSocket, response.c_str(), response.length(), 0);
            close(clientSocket);
            continue;
        }

        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        int clientPort = ntohs(clientAddr.sin_port);
        std::cout << "客户端连接成功：" << clientIP << ":" << clientPort << std::endl;

        ClientSession* session = new ClientSession(clientSocket, clientIP, clientPort);
        AddSession(clientSocket, session);
        currentConnections++;

        threadPool->SubmitTask([this, clientSocket](){
            HandleClient(clientSocket);
        });
    }
}

/**
 * @brief 处理单个客户端连接
 * @param clientSocket 客户端socket文件描述符
 */
void Server::HandleClient(int clientSocket){
    while(running){
        std::string data = ReceiveData(clientSocket);
        if(data.empty()){
            break;
        }

        // TODO: 解析JSON并分发到对应业务模块
        std::cout << "收到客户端数据：" << data << std::endl;
    }

    RemoveSession(clientSocket);
    currentConnections--;
    close(clientSocket);
    std::cout << "客户端断开连接" << std::endl;
}

/**
 * @brief 接收客户端数据（处理粘包）
 * @param clientSocket 客户端socket文件描述符
 * @return 返回接收到的消息体字符串
 */
std::string Server::ReceiveData(int clientSocket){
    char header[4];
    int bytesRead = recv(clientSocket, header, 4, 0);
    if(bytesRead <= 0){
        return "";
    }

    uint32_t messageLength = ParseMessageHeader(header);
    if(messageLength == 0 || messageLength > 65536){
        return "";
    }

    std::string messageBody(messageLength, '\0');
    int totalRead = 0;
    while(totalRead < messageLength){
        bytesRead = recv(clientSocket, &messageBody[totalRead], messageLength - totalRead, 0);
        if(bytesRead <= 0){
            return "";
        }
        totalRead += bytesRead;
    }

    return messageBody;
}

/**
 * @brief 发送数据到客户端
 * @param clientSocket 客户端socket文件描述符
 * @param data 要发送的数据字符串
 * @return 发送成功返回true，失败返回false
 */
bool Server::SendData(int clientSocket, const std::string& data){
    std::string packagedData = PackageMessage(data);
    int bytesSent = send(clientSocket, packagedData.c_str(), packagedData.length(), 0);
    return bytesSent > 0;
}

/**
 * @brief 解析4字节大端序消息头
 * @param header 4字节消息头
 * @return 返回消息体长度
 */
uint32_t Server::ParseMessageHeader(const char* header){
    uint32_t length = 0;
    length |= (static_cast<uint8_t>(header[0]) << 24);
    length |= (static_cast<uint8_t>(header[1]) << 16);
    length |= (static_cast<uint8_t>(header[2]) << 8);
    length |= static_cast<uint8_t>(header[3]);
    return length;
}

/**
 * @brief 封装消息（添加4字节大端序消息头）
 * @param body 消息体字符串
 * @return 返回封装后的完整消息
 */
std::string Server::PackageMessage(const std::string& body){
    uint32_t length = body.length();
    std::string header(4, '\0');
    header[0] = static_cast<char>((length >> 24) & 0xFF);
    header[1] = static_cast<char>((length >> 16) & 0xFF);
    header[2] = static_cast<char>((length >> 8) & 0xFF);
    header[3] = static_cast<char>(length & 0xFF);
    return header + body;
}

/**
 * @brief 添加会话到会话映射
 * @param clientSocket 客户端socket文件描述符
 * @param session 会话对象指针
 */
void Server::AddSession(int clientSocket, ClientSession* session){
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessionMap[clientSocket] = session;
}

/**
 * @brief 从会话映射移除会话
 * @param clientSocket 客户端socket文件描述符
 */
void Server::RemoveSession(int clientSocket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessionMap.find(clientSocket);
    if(it != sessionMap.end()){
        delete it->second;
        sessionMap.erase(it);
    }
}

/**
 * @brief 根据socket获取会话
 * @param clientSocket 客户端socket文件描述符
 * @return 返回会话对象指针，未找到返回nullptr
 */
ClientSession* Server::GetSession(int clientSocket){
    std::lock_guard<std::mutex> lock(sessionMutex);
    auto it = sessionMap.find(clientSocket);
    if(it != sessionMap.end()){
        return it->second;
    }
    return nullptr;
}

/**
 * @brief 根据用户名获取会话
 * @param username 用户名
 * @return 返回会话对象指针，未找到返回nullptr
 */
ClientSession* Server::GetSessionByUsername(const std::string& username){
    std::lock_guard<std::mutex> lock(sessionMutex);
    for(auto& pair : sessionMap){
        if(pair.second->GetUsername() == username){
            return pair.second;
        }
    }
    return nullptr;
}

/**
 * @brief 获取当前连接数
 * @return 返回当前连接数
 */
int Server::GetCurrentConnections(){
    return currentConnections.load();
}
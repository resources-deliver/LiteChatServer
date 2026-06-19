#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>
#include <jsoncpp/json/json.h>

/**
 * @brief Server构造函数，初始化类内私有属性
*/
Server::Server()
    : serverSocket(-1)
    , serverIP("0.0.0.0")
    , serverPort(8886)
    , maxConnections(128)
    , currentConnections(0)
    , threadPool(nullptr)
    , dbManager(nullptr)
    , sessionManager(nullptr)
    , userManager(nullptr)
    , heartbeatThread(nullptr)
    , running(false)
{
}

/**
 * @brief Server析构函数，清理资源
 */
Server::~Server(){
    Stop();  // 停止服务器运行
    if(threadPool){
        delete threadPool;
    }
    if(sessionManager){
        delete sessionManager;
    }
    if(userManager){
        delete userManager;
    }
    if(heartbeatThread){
        delete heartbeatThread;
    }
}

/**
 * @brief 初始化数据库管理器
 * @param dbMgr 数据库管理器指针
 */
void Server::SetDBManager(DBManager* dbMgr){
    dbManager = dbMgr;
}

/**
 * @brief 启动服务器，创建socket、绑定、监听
 * @return 启动成功返回true，失败返回false
 */
bool Server::Start(){
    if(!dbManager){
        std::cerr << "[Server::Start]数据库管理器未初始化" << std::endl;
        return false;
    }
    threadPool = new ThreadPool(10);
    sessionManager = new SessionManager();
    userManager = new UserManager(dbManager, sessionManager);
    userManager->SetSendCallback([this](int clientSocket, const std::string& data) -> bool{
        return this->SendData(clientSocket, data);
    });  // 发送数据到回调函数
    // 创建socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        std::cerr << "[Server::Start]创建socket失败" << std::endl;
        return false;
    }
    // 设置端口复用
    int opt = 1;
    int resetport = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(resetport == -1){
        std::cerr << "[Server::Start]设置端口复用失败" << std::endl;
        close(serverSocket);  // 关闭socket
        return false;
    }
    // 绑定服务端IP和端口
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(serverPort);
    int bindres = bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if(bindres == -1){
        std::cerr << "[Server::Start]绑定IP和端口失败" << std::endl;
        close(serverSocket);
        return false;
    }
    // 监听连接
    int listenres = listen(serverSocket, maxConnections);
    if(listenres == -1){
        std::cerr << "[Server::Start]监听失败" << std::endl;
        close(serverSocket);
        return false;
    }
    std::cout << "[Server::Start]服务器启动成功，监听端口" << serverPort << std::endl;
    running = true;
    StartHeartbeatCheck();
    return true;
}

/**
 * @brief 停止服务器，关闭socket
 */
void Server::Stop(){
    running = false;
    if(heartbeatThread && heartbeatThread->joinable()){
        heartbeatThread->join();
    }
    if(sessionManager){
        auto sessions = sessionManager->GetAllSessions();
        for(auto& pair : sessions){
            ClientSession* session = pair.second;
            if(session && !session->GetUsername().empty() && userManager){
                userManager->UpdateUserOnlineStatus(session->GetUsername(), false);
            }
            if(session->GetSocket() != -1){
                shutdown(session->GetSocket(), SHUT_RDWR);
            }
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
            std::cerr << "[Server::AcceptConnections]接受客户端连接失败" << std::endl;
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
        std::cout << "[Server::AcceptConnections]客户端连接成功：" << clientIP << ":" << clientPort << std::endl;
        ClientSession* session = new ClientSession(clientSocket, clientIP, clientPort);
        sessionManager->AddSession(clientSocket, session);
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

        DispatchRequest(clientSocket, data);
    }

    ClientSession* session = sessionManager->GetSession(clientSocket);
    if(session){
        if(!session->GetUsername().empty()){
            userManager->UpdateUserOnlineStatus(session->GetUsername(), false);
            userManager->NotifyFriendsStatusChange(session->GetUsername(), UserStatus::Offline);
        }

        sessionManager->RemoveSession(clientSocket);
        currentConnections--;
    }

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
    if(sessionManager){
        sessionManager->AddSession(clientSocket, session);
    }
    else{
        std::lock_guard<std::mutex> lock(sessionMutex);
        sessionMap[clientSocket] = session;
    }
}

/**
 * @brief 从会话映射移除会话
 * @param clientSocket 客户端socket文件描述符
 */
void Server::RemoveSession(int clientSocket){
    if(sessionManager){
        sessionManager->RemoveSession(clientSocket);
    }
    else{
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto it = sessionMap.find(clientSocket);
        if(it != sessionMap.end()){
            delete it->second;
            sessionMap.erase(it);
        }
    }
}

/**
 * @brief 根据socket获取会话
 * @param clientSocket 客户端socket文件描述符
 * @return 返回会话对象指针，未找到返回nullptr
 */
ClientSession* Server::GetSession(int clientSocket){
    if(sessionManager){
        return sessionManager->GetSession(clientSocket);
    }
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
    if(sessionManager){
        return sessionManager->GetSessionByUsername(username);
    }
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

/**
 * @brief 启动心跳检测线程
 */
void Server::StartHeartbeatCheck(){
    heartbeatThread = new std::thread(&Server::HeartbeatCheckThread, this);
}

/**
 * @brief 心跳检测线程函数，每60秒检测一次，超过180秒无心跳则断开
 */
void Server::HeartbeatCheckThread(){
    while(running){
        std::this_thread::sleep_for(std::chrono::seconds(60));
        
        auto sessions = sessionManager->GetAllSessions();
        time_t currentTime = time(nullptr);
        
        for(auto& pair : sessions){
            int socket = pair.first;
            ClientSession* session = pair.second;
            
            if(session->GetIsConnected() && !session->GetUsername().empty()){
                time_t lastHeartbeat = session->GetLastHeartbeat();
                if(difftime(currentTime, lastHeartbeat) > 180){
                    std::cout << "心跳超时，断开客户端：" << session->GetUsername() << std::endl;

                    userManager->UpdateUserOnlineStatus(session->GetUsername(), false);
                    userManager->NotifyFriendsStatusChange(session->GetUsername(), UserStatus::Offline);
                    
                    shutdown(socket, SHUT_RDWR);
                    sessionManager->RemoveSession(socket);
                    currentConnections--;
                }
            }
        }
    }
}

/**
 * @brief 分发客户端请求到对应业务模块
 * @param clientSocket 客户端socket
 * @param data JSON数据字符串
 */
void Server::DispatchRequest(int clientSocket, const std::string& data){
    Json::Reader reader;
    Json::Value root;
    
    if(!reader.parse(data, root)){
        std::string response = PackageMessage("{\"type\":\"ERROR\",\"code\":4001,\"msg\":\"JSON解析失败\"}");
        SendData(clientSocket, response);
        return;
    }
    
    std::string type = root["type"].asString();
    
    Json::Value dataNode = root["data"];
    
    Request request;
    request.type = type;
    request.clientSocket = clientSocket;
    
    if(dataNode.isNull()){
        if(root.isMember("username")){
            request.username = root["username"].asString();
        }
        if(root.isMember("password")){
            request.password = root["password"].asString();
        }
        if(root.isMember("newUsername")){
            request.newUsername = root["newUsername"].asString();
        }
        if(root.isMember("newPassword")){
            request.newPassword = root["newPassword"].asString();
        }
        if(root.isMember("verifyPassword")){
            request.verifyPassword = root["verifyPassword"].asString();
        }
    }
    else{
        if(dataNode.isMember("username")){
            request.username = dataNode["username"].asString();
        }
        if(dataNode.isMember("password")){
            request.password = dataNode["password"].asString();
        }
        if(dataNode.isMember("newUsername")){
            request.newUsername = dataNode["newUsername"].asString();
        }
        if(dataNode.isMember("newPassword")){
            request.newPassword = dataNode["newPassword"].asString();
        }
        if(dataNode.isMember("verifyPassword")){
            request.verifyPassword = dataNode["verifyPassword"].asString();
        }
    }
    
    Response response;
    
    if(!userManager){
        response.code = 5001;
        response.msg = "服务器内部错误";
    }
    else if(type == "REGISTER"){
        response = userManager->HandleRegister(request);
    }
    else if(type == "LOGIN"){
        response = userManager->HandleLogin(request);
    }
    else if(type == "UPDATE_USER"){
        response = userManager->HandleUpdateUser(request);
    }
    else if(type == "QUERY_STATUS"){
        response = userManager->HandleQueryStatus(request);
    }
    else if(type == "HEARTBEAT"){
        ClientSession* session = sessionManager->GetSession(clientSocket);
        if(session){
            session->UpdateHeartbeat();
        }
        response.code = 0;
        response.msg = "心跳正常";
    }
    else{
        response.code = 4002;
        response.msg = "未知的请求类型";
    }
    
    Json::Value jsonResponse;
    jsonResponse["type"] = type + "_RESPONSE";
    jsonResponse["code"] = response.code;
    jsonResponse["msg"] = response.msg;
    if(!response.data.empty()){
        jsonResponse["data"] = response.data;
    }
    
    Json::FastWriter writer;
    std::string responseStr = writer.write(jsonResponse);
    
    SendData(clientSocket, responseStr);
}
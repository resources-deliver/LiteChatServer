#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
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
    , friendManager(nullptr)
    , messageManager(nullptr)
    , heartbeatThread(nullptr)
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
    if(sessionManager){
        delete sessionManager;
    }
    if(userManager){
        delete userManager;
    }
    if(friendManager){
        delete friendManager;
    }
    if(messageManager){
        delete messageManager;
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
    // 初始化线程池、会话管理、用户管理
    threadPool = new ThreadPool(10);
    sessionManager = new SessionManager();
    userManager = new UserManager(dbManager, sessionManager);
    friendManager = new FriendManager(dbManager);
    messageManager = new MessageManager(dbManager, sessionManager);
    // 发送数据到回调函数
    userManager->SetSendCallback(
        [this](int clientSocket, const std::string& data){
            return this->SendData(clientSocket, data);
        }
    );
    messageManager->SetSendCallback(
        [this](int clientSocket, const std::string& data){
            return this->SendData(clientSocket, data);
        }
    );
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
        close(serverSocket);
        return false;
    }
    // 设置服务端IP和端口（网络字节序）
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(serverPort);
    // 绑定服务端IP和端口
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
    // 设置服务器为运行状态、启动心跳检查线程
    std::cout << "[Server::Start]服务器启动成功，监听端口" << serverPort << std::endl;
    running = true;
    StartHeartbeatCheck();
    return true;
}

/**
 * @brief 停止服务器，关闭socket
 */
void Server::Stop(){
    running = false;  // 设置服务器为非运行状态
    if(heartbeatThread && heartbeatThread->joinable()){  // 如果线程还在运行
        heartbeatThread->join();  // 则等待线程结束
    }
    if(sessionManager){  // 如果有会话在运行
        auto sessions = sessionManager->GetAllSessions();  // 获取所有会话
        for(auto& pair : sessions){  // 遍历所有会话
            ClientSession* session = pair.second;
            if(session && !session->GetUsername().empty() && userManager){  // 如果会话存在且有用户名且有用户管理器
                userManager->UpdateUserOnlineStatus(session->GetUsername(), false);  // 则更新用户在线状态
                if(friendManager){
                    friendManager->RemoveFriendCache(session->GetUsername());  // 移除好友列表缓存
                }
            }
            if(session->GetSocket() != -1){  // 如果会话存在socket
                shutdown(session->GetSocket(), SHUT_RDWR);  // 关闭socket
            }
        }
    }
    if(serverSocket != -1){  // 如果服务器socket未关闭
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
        serverSocket = -1;
    }
}

/**
 * @brief 接受客户端连接循环
 */
void Server::AcceptConnections(){
    while(running){  // 如果服务器处于运行状态
        struct sockaddr_in clientAddr;  // 客户端地址结构体
        socklen_t clientLen = sizeof(clientAddr);  // 客户端地址长度
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);  // 接受客户端连接
        if(clientSocket == -1){
            if(!running){  // 如果服务器已停止
                close(clientSocket);  // 关闭客户端socket
                break;
            }
            std::cerr << "[Server::AcceptConnections]接受客户端连接失败" << std::endl;
            continue;
        }
        if(currentConnections.load() >= maxConnections){  // 如果当前连接数已达上限
            std::string response = PackageMessage("{\"type\":\"ERROR\",\"code\":5003,\"msg\":\"服务器繁忙\"}");
            send(clientSocket, response.c_str(), response.length(), 0);  // 发送错误响应
            close(clientSocket);  // 关闭客户端socket
            continue;
        }
        int flag = 1;
        setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        std::string clientIP = inet_ntoa(clientAddr.sin_addr);  // 将客户端IP转为主机字节序
        int clientPort = ntohs(clientAddr.sin_port);  // 将客户端端口转为主机字节序
        ClientSession* session = new ClientSession(clientSocket, clientIP, clientPort);  // 创建新会话
        sessionManager->AddSession(clientSocket, session);  // 添加会话到会话管理器
        std::cout << "[Server::AcceptConnections]客户端连接成功：" << clientIP << ":" << clientPort << std::endl;
        currentConnections++;  // 增加当前连接数
        threadPool->SubmitTask([this, clientSocket](){ HandleClient(clientSocket); });  // 提交处理任务到线程池
    }
}

/**
 * @brief 处理单个客户端连接
 * @param clientSocket 客户端socket文件描述符
 */
void Server::HandleClient(int clientSocket){
    while(running){  // 如果服务器处于运行状态
        std::string data = ReceiveData(clientSocket);  // 接收客户端数据
        if(data.empty()){  // 如果收到空数据
            std::cout << "[Server::HandleClient]收到空数据" << std::endl;
            break;
        }
        DispatchRequest(clientSocket, data);  // 分发客户端的请求到各个业务模块
    }
    ClientSession* session = sessionManager->GetSession(clientSocket);  // 获取客户端会话
    if(session){  // 如果会话存在
        if(!session->GetUsername().empty()){  // 如果有用户名
            userManager->UpdateUserOnlineStatus(session->GetUsername(), false);  // 更新用户在线状态
            userManager->NotifyFriendsStatusChange(session->GetUsername(), UserStatus::Offline);  // 通知好友状态改变
            if(friendManager){
                friendManager->RemoveFriendCache(session->GetUsername());  // 移除好友列表缓存
            }
        }
        sessionManager->RemoveSession(clientSocket);  // 移除会话
        currentConnections--;  // 减少当前连接数
    }
    std::cout << "[Server::HandleClient]客户端断开连接" << std::endl;
    close(clientSocket);  // 关闭客户端socket
}

/**
 * @brief 接收客户端数据（处理粘包）
 * @param clientSocket 客户端socket文件描述符
 * @return 返回接收到的消息体字符串
 */
std::string Server::ReceiveData(int clientSocket){
    // 接收4字节消息头
    char header[4];
    int bytesRead = recv(clientSocket, header, 4, 0);
    if(bytesRead <= 0){
        std::cerr << "[Server::ReceiveData]接收消息头失败,bytesRead: " << bytesRead << ", errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        return "";
    }
    // 解析消息头长度
    uint32_t messageLength = ParseMessageHeader(header);
    if(messageLength == 0 || messageLength > 65536){
        std::cerr << "[Server::ReceiveData]消息头长度无效" << std::endl;
        return "";
    }
    // 接收消息体
    std::string messageBody(messageLength, '\0');
    int totalRead = 0;
    while(totalRead < messageLength){
        bytesRead = recv(clientSocket, &messageBody[totalRead], messageLength - totalRead, 0);
        if(bytesRead <= 0){
            std::cerr << "[Server::ReceiveData]接收消息体失败" << std::endl;
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
    // 封装消息、发送消息
    std::string packagedData = PackageMessage(data);
    int totalSent = 0;
    int totalLength = packagedData.length();
    while(totalSent < totalLength){
        int bytesSent = send(clientSocket, packagedData.c_str() + totalSent, totalLength - totalSent, 0);
        if(bytesSent <= 0){
            std::cerr << "[Server::SendData]发送数据失败,errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
            return false;
        }
        totalSent += bytesSent;
    }
    std::cout << "[Server::SendData]发送数据成功,长度: " << totalSent << std::endl;
    return true;
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
 * @brief 心跳检测线程函数，每30秒检测一次，超过10分钟客户端无操作则断开
 */
void Server::HeartbeatCheckThread(){
    while(running){  // 若服务器为运行状态
        std::this_thread::sleep_for(std::chrono::seconds(30));  // 休眠30秒
        auto sessions = sessionManager->GetAllSessions();  // 获取所有会话
        time_t currentTime = time(nullptr);  // 获取当前时间
        for(auto& pair : sessions){  // 遍历所有会话
            int socket = pair.first;  // 获取会话socket
            ClientSession* session = pair.second;  // 获取会话对象
            if(session->GetIsConnected() && !session->GetUsername().empty()){  // 若会话已连接且有用户名
                // 若会话最后心跳时间与当前时间差超过10分钟
                if(difftime(currentTime, session->GetLastHeartbeat()) > 600){
                    std::cout << "[Server::HeartbeatCheckThread]客户端10分钟无响应, 心跳超时, 断开客户端: " << session->GetUsername() << std::endl;
                    userManager->UpdateUserOnlineStatus(session->GetUsername(), false);  // 更新用户在线状态为离线
                    userManager->NotifyFriendsStatusChange(session->GetUsername(), UserStatus::Offline);  // 通知好友状态改变
                    if(friendManager){
                        friendManager->RemoveFriendCache(session->GetUsername());  // 移除好友列表缓存
                    }
                    shutdown(socket, SHUT_RDWR);  // 关闭会话套接字
                    sessionManager->RemoveSession(socket);  // 移除会话
                    currentConnections--;  // 减少连接数
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
        std::string response = "{\"type\":\"ERROR\",\"code\":4001,\"msg\":\"JSON解析失败\"}";
        SendData(clientSocket, response);
        return;
    }
    std::string type = root["type"].asString();
    Json::Value dataNode = root["data"];
    ClientSession* session = sessionManager->GetSession(clientSocket);
    if(session){
        session->UpdateHeartbeat();
    }
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
        if(root.isMember("targetUsername")){
            request.targetUsername = root["targetUsername"].asString();
        }
        if(root.isMember("content")){
            request.content = root["content"].asString();
        }
        if(root.isMember("count")){
            request.count = root["count"].asInt();
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
        if(dataNode.isMember("targetUsername")){
            request.targetUsername = dataNode["targetUsername"].asString();
        }
        if(dataNode.isMember("content")){
            request.content = dataNode["content"].asString();
        }
        if(dataNode.isMember("count")){
            request.count = dataNode["count"].asInt();
        }
    }
    if(request.username.empty()){
        auto session = sessionManager->GetSession(clientSocket);
        if(session && !session->GetUsername().empty()){
            request.username = session->GetUsername();
            std::cout << "[Server::DispatchRequest]从会话中获取用户名：" << request.username << std::endl;
        }
    }
    else{
        auto session = sessionManager->GetSession(clientSocket);
        if(session && !session->GetUsername().empty()){
            if(request.targetUsername.empty()){
                request.targetUsername = request.username;
            }
            request.username = session->GetUsername();
            std::cout << "[Server::DispatchRequest]从会话中获取用户名：" << request.username << std::endl;
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
        if(response.code == 0 && friendManager){
            friendManager->CacheFriendList(request.username);
        }
        if(response.code == 0 && messageManager){
            messageManager->PushOfflineMessages(request.username);
        }
    }
    else if(type == "UPDATE_USER"){
        response = userManager->HandleUpdateUser(request);
    }
    else if(type == "QUERY_STATUS"){
        response = userManager->HandleQueryStatus(request);
    }
    else if(type == "ADD_FRIEND"){
        if(friendManager){
            response = friendManager->HandleAddFriend(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "DEL_FRIEND"){
        if(friendManager){
            response = friendManager->HandleDeleteFriend(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "FRIEND_LIST"){
        if(friendManager){
            response = friendManager->HandleFriendList(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "FRIEND_STATUS"){
        if(friendManager){
            response = friendManager->HandleFriendStatus(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "QUERY_FRIEND"){
        if(friendManager){
            response = friendManager->HandleQueryFriend(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "SEND_MSG"){
        if(messageManager){
            response = messageManager->HandleSendMessage(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "HISTORY_MSG"){
        if(messageManager){
            response = messageManager->HandleHistoryMessages(request);
        }
        else{
            response.code = 5001;
            response.msg = "服务器内部错误";
        }
    }
    else if(type == "HEARTBEAT"){
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
        Json::Reader dataReader;
        Json::Value dataValue;
        if(dataReader.parse(response.data, dataValue)){
            jsonResponse["data"] = dataValue;
        }
        else{
            jsonResponse["data"] = response.data;
        }
    }
    Json::FastWriter writer;
    std::string responseStr = writer.write(jsonResponse);
    if(SendData(clientSocket, responseStr)){
        std::cout << "[Server::DispatchRequest]响应已发送, 类型: " << (type + "_RESPONSE") << std::endl;
    }
}
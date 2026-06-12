#include "server.h"
#include "db_manager.h"
#include <iostream>
#include <signal.h>
#include <thread>

static Server* g_server = nullptr;

/**
 * @brief 信号处理函数，优雅关闭服务器
 * @param signal 信号编号
 */
void SignalHandler(int signal){
    std::cout << "接收到信号：" << signal << "，正在关闭服务器..." << std::endl;
    if(g_server){
        g_server->Stop();
    }
}

/**
 * @brief 程序入口函数
 * @return 程序退出码
 */
int main(){
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    DBManager dbManager;
    if(!dbManager.InitConnectionPool(10)){
        std::cerr << "数据库连接池初始化失败，服务器拒绝启动" << std::endl;
        return 1;
    }
    std::cout << "数据库连接池初始化成功" << std::endl;

    Server server;
    g_server = &server;

    if(!server.Start()){
        std::cerr << "服务器启动失败" << std::endl;
        return 1;
    }

    std::thread acceptThread(&Server::AcceptConnections, &server);
    acceptThread.join();

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}
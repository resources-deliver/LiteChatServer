# LiteChat Server
轻量级即时通讯服务器端程序，基于 C++11 开发，采用 TCP Socket + MySQL 架构，为 [LiteChatClient](https://github.com/resources-deliver/LiteChatClient) 聊天通信软件提供高性能、多用户并发的后端服务。

## 项目简介
LiteChat 是一款轻量级聊天通信软件，采用客户端/服务器（C/S）架构设计。本项目为服务器端程序，负责处理多客户端连接、用户管理、好友管理、消息转发与存储等核心业务逻辑。

- **客户端**：Windows 下基于 Qt6 Widgets 框架开发
- **服务器端**：Linux 下基于 C++11 原生 Socket 开发（本项目）
- **数据库**：MySQL 8.0+

## 功能特性
### 用户管理
- 用户注册（MD5 加密密码存储）
- 用户登录（支持顶号，强制旧连接下线）
- 用户信息修改（用户名/密码，需验证身份）
- 用户在线状态查询与实时推送

### 好友管理
- 添加/删除好友（双向关系维护）
- 好友列表查询（含在线状态，支持缓存加速）
- 好友状态实时通知（上下线自动推送）

### 消息通信
- 文本消息发送与转发（在线实时转发，离线存储待推送）
- 历史消息查询（支持指定条数，默认50条，最多200条）
- 离线消息推送（用户上线后自动推送未读消息）
- 新消息通知（含消息摘要）

### 系统功能
- TCP 粘包处理（4字节大端序消息头 + 消息体）
- 线程池管理（避免频繁创建销毁线程）
- 数据库连接池（互斥锁 + 条件变量，线程安全）
- 心跳检测（每30秒检测，超时10分钟自动断开）
- 服务器状态监控（每60秒采集在线用户数、连接数、线程池使用率、数据库状态）
- 日志记录（按日期轮转，超10MB自动轮转，超90天自动清理，过滤敏感信息）
- 异常处理（单客户端异常不影响其他客户端，数据库异常自动重连）

## 技术栈
| 技术领域 | 技术选型 |
|---------|---------|
| 编程语言 | C++11 |
| 构建工具 | CMake 3.10+ |
| 网络通信 | TCP Socket（IPv4） |
| 多线程 | C++11 线程库（thread、mutex、condition_variable、atomic） |
| 数据库 | MySQL 8.0+ |
| 数据库接口 | MySQL Connector/C++ |
| 数据加密 | MD5（OpenSSL EVP API） |
| JSON 解析 | JsonCpp |
| 字符编码 | UTF-8（utf8mb4） |

## 项目结构
```
LiteChatServer/
├── CMakeLists.txt                  # CMake 构建配置
├── README.md                       # 项目说明文档
├── .gitignore                      # Git 忽略规则
├── docs/                           # 设计文档
│   ├── Whole/                      # 总体设计文档
│   │   ├── SRS.md                  # 软件需求规格说明书
│   │   ├── HLD.md                  # 概要设计说明书
│   │   ├── LLD.md                  # 详细设计说明书
│   │   └── Stage.md                # 多阶段开发计划
│   └── DealStage/                  # 处理流程文档
│       ├── ClientDealStage.md      # 客户端处理逻辑
│       └── ServerDealStage.md      # 服务器端处理逻辑
└── src/
    ├── main.cpp                    # 程序入口
    ├── connection/                 # 网络连接层
    │   ├── server.h/.cpp           # 服务器主类（核心调度）
    │   └── client_session.h/.cpp   # 客户端会话对象
    ├── system/                     # 系统/业务逻辑层
    │   ├── db_manager.h/.cpp       # MySQL 连接池管理
    │   ├── thread_pool.h/.cpp      # 线程池
    │   ├── session_manager.h/.cpp  # 会话管理器
    │   ├── user_dao.h/.cpp         # 用户数据访问层
    │   ├── user_manager.h/.cpp     # 用户业务逻辑
    │   ├── friend_dao.h/.cpp       # 好友数据访问层
    │   ├── friend_manager.h/.cpp   # 好友业务逻辑
    │   ├── message_dao.h/.cpp      # 消息数据访问层
    │   ├── message_manager.h/.cpp  # 消息业务逻辑
    │   ├── server_logger.h/.cpp    # 日志系统（单例模式）
    │   ├── server_monitor.h/.cpp   # 服务器监控
    │   └── exception_handler.h/.cpp # 异常处理
    └── sql/                        # 数据库脚本
        ├── init.sql                # 数据库初始化脚本
        ├── SQLCommand1.sql         # SQL 命令参考（终端操作）
        └── SQLCommand2.sql         # 备选表结构设计
```

## 架构分层
```
┌─────────────────────────────────────┐
│         网络通信层（connection/）    │
│  TCP Socket 管理、连接处理、消息收发  │
├─────────────────────────────────────┤
│         业务处理层（system/）        │
│  UserManager、FriendManager、        │
│  MessageManager、SessionManager     │
├─────────────────────────────────────┤
│         数据访问层（system/）        │
│  UserDAO、FriendDAO、MessageDAO     │
├─────────────────────────────────────┤
│         基础设施层（system/）        │
│  DBManager、ThreadPool、            │
│  ServerLogger、ServerMonitor        │
└─────────────────────────────────────┘
```

## 环境要求
### 运行环境
| 项目 | 要求 |
|------|------|
| 操作系统 | Ubuntu 20.04 及以上 |
| 数据库 | MySQL 8.0 及以上 |
| 网络 | 开放 8886 端口（客户端连接）和 3306 端口（MySQL 访问） |

### 编译依赖
```bash
# 基础编译工具
sudo apt-get install build-essential cmake pkg-config
# MySQL 客户端库
sudo apt-get install libmysqlclient-dev
# MySQL Connector/C++
sudo apt-get install libmysqlcppconn-dev
# OpenSSL 开发库
sudo apt-get install libssl-dev
# JsonCpp 开发库
sudo apt-get install libjsoncpp-dev
```

## 构建与运行
### 1. 初始化数据库
```bash
# 使用 root 用户执行初始化脚本
mysql -u root -p < src/sql/init.sql
```

脚本会自动创建：
- 数据库 `LiteChat_db`
- 数据库用户 `LiteChat_user`（密码 `LiteChat_password`）
- 三张核心表：`users`（用户）、`friends`（好友关系）、`messages`（消息）

### 2. 编译项目
```bash
# 创建构建目录
mkdir -p build && cd build
# 配置 CMake
cmake ..
# 编译
make -j$(nproc)
```

### 3. 运行服务器
```bash
# 在项目根目录下运行
./build/litechat_server
```

服务默认监听 `0.0.0.0:8886`，日志输出到 `./logs/` 目录。

### 4. 停止服务器
按 `Ctrl+C` 发送 SIGINT 信号，服务器将优雅关闭（等待线程结束、更新用户离线状态、关闭所有连接）。

## 通信协议
### 消息格式
```
+----------------+------------------+
| 4字节消息头(大端序) | 消息体(JSON)  |
+----------------+------------------+
```

- 消息头：4字节无符号整数（大端序），表示消息体的字节长度
- 消息体：UTF-8 编码的 JSON 字符串
- 消息体最大长度：65536 字节（64KB）

### 请求类型
| Type | 说明 | 处理模块 |
|------|------|---------|
| REGISTER | 用户注册 | UserManager |
| LOGIN | 用户登录 | UserManager |
| UPDATE_USER | 修改用户信息 | UserManager |
| QUERY_STATUS | 查询用户状态 | UserManager |
| ADD_FRIEND | 添加好友 | FriendManager |
| DEL_FRIEND | 删除好友 | FriendManager |
| FRIEND_LIST | 查询好友列表 | FriendManager |
| FRIEND_STATUS | 查询好友状态 | FriendManager |
| QUERY_FRIEND | 查询好友信息 | FriendManager |
| SEND_MSG | 发送消息 | MessageManager |
| HISTORY_MSG | 查询历史消息 | MessageManager |
| HEARTBEAT | 心跳检测 | Server |

### 服务端推送类型
| Type | 说明 |
|------|------|
| STATUS_NOTIFY | 好友状态变更通知 |
| FORWARD_MSG | 转发消息 |
| MSG_NOTIFY | 新消息通知 |
| KICKED | 账号被顶号通知 |

## 数据库设计
### users（用户表）
| 字段 | 类型 | 说明 |
|------|------|------|
| user_id | INT AUTO_INCREMENT | 用户ID，主键 |
| username | VARCHAR(20) UNIQUE | 用户名 |
| password_hash | VARCHAR(32) | MD5加密密码 |
| create_time | DATETIME | 创建时间 |
| update_time | DATETIME | 更新时间 |
| is_online | TINYINT(1) | 在线状态（0=离线，1=在线） |
| last_login_time | DATETIME | 最后登录时间 |

### friends（好友关系表）
| 字段 | 类型 | 说明 |
|------|------|------|
| friendship_id | INT AUTO_INCREMENT | 关系ID，主键 |
| user_id1 | INT | 用户ID1 |
| user_id2 | INT | 用户ID2 |

> 好友关系为双向对等，通过 `(user_id1, user_id2)` 唯一约束保证不重复。

### messages（消息表）
| 字段 | 类型 | 说明 |
|------|------|------|
| message_id | INT AUTO_INCREMENT | 消息ID，主键 |
| sender_id | INT | 发送者ID |
| receiver_id | INT | 接收者ID |
| content | TEXT | 消息内容 |
| send_time | DATETIME | 发送时间 |
| is_read | TINYINT(1) | 已读状态（0=未读，1=已读） |

## 配置说明
主要配置项位于源码中，可根据需要修改：

| 配置项 | 位置 | 默认值 |
|--------|------|--------|
| 服务器端口 | `server.cpp` | 8886 |
| 最大连接数 | `server.cpp` | 128 |
| 线程池大小 | `server.cpp` | 10 |
| 数据库地址 | `db_manager.cpp` | tcp://127.0.0.1:3306 |
| 数据库名 | `db_manager.cpp` | LiteChat_db |
| 数据库用户 | `db_manager.cpp` | LiteChat_user |
| 数据库密码 | `db_manager.cpp` | LiteChat_password |
| 连接池大小 | `main.cpp` | 10 |
| 心跳检测间隔 | `server.cpp` | 30秒 |
| 心跳超时时间 | `server.cpp` | 600秒（10分钟） |
| 监控采集间隔 | `server_monitor.cpp` | 60秒 |
| 日志轮转大小 | `server_logger.cpp` | 10MB |
| 日志保留天数 | `server_logger.cpp` | 90天 |
| 最大消息体 | `message_manager.cpp` | 65536字节 |

## 开发阶段
项目按五个阶段迭代开发，当前已完成全部阶段：

1. **第一阶段**：基础通信框架与底层支撑（Server、ThreadPool、ClientSession、DBManager）
2. **第二阶段**：用户管理模块（UserManager、SessionManager、心跳检测）
3. **第三阶段**：好友管理模块（FriendManager、好友列表缓存）
4. **第四阶段**：消息通信模块（MessageManager、离线消息推送）
5. **第五阶段**：系统功能完善（ServerLogger、ServerMonitor、ExceptionHandler）

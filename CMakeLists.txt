cmake_minimum_required(VERSION 3.10)
project(LiteChatServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找MySQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

# 查找OpenSSL
find_package(OpenSSL REQUIRED)

# 查找JsonCpp
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 包含目录
include_directories(${MYSQL_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src/system)
include_directories(${CMAKE_SOURCE_DIR}/src/connection)

# 源文件
set(SOURCES
    src/main.cpp
    src/system/db_manager.cpp
    src/system/thread_pool.cpp
    src/system/user_dao.cpp
    src/system/session_manager.cpp
    src/system/user_manager.cpp
    src/connection/server.cpp
    src/connection/client_session.cpp
)

# 创建可执行文件
add_executable(litechat_server ${SOURCES})

# 链接库
target_link_libraries(litechat_server 
    ${MYSQL_LIBRARIES} 
    pthread
    ${OPENSSL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
)
target_compile_options(litechat_server PRIVATE ${MYSQL_CFLAGS_OTHER} ${JSONCPP_CFLAGS_OTHER})
# CMakeLists.txt in Net directory

# 添加源文件
set(SOURCE_FILES
    acceptor.cpp
    buffer.cpp
    channel.cpp
    epoller.cpp
    eventloop.cpp
    loopthreadpool.cpp
    tcpconnection.cpp
    tcpserver.cpp
)

# 添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 添加库
add_library(netlib ${SOURCE_FILES})

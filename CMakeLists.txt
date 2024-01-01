# CMakeLists.txt in webserver directory

cmake_minimum_required(VERSION 2.8)
project(WebServer)

# 添加可执行文件
add_executable(server main.cpp)

# 设置构建类型为 Debug，同时添加调试信息
set(CMAKE_BUILD_TYPE Debug)

# 添加额外的编译选项，包括 GDB 调试选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 添加子目录，进入Net,Http目录继续构建
add_subdirectory(Net)
add_subdirectory(Http)

# 链接子目录生成的库到可执行文件
target_link_libraries(server pthread netlib httplib)

# 添加头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Http)



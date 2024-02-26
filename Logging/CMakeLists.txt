# CMakeLists.txt in Log directory

# 添加源文件
set(SOURCE_FILES
    logstream.cpp
    logging.cpp
    logfile.cpp
    asynclogging.cpp
)

# 添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Net)
include_directories(${PROJECT_SOURCE_DIR}/Timer)

# 添加库
add_library(loglib ${SOURCE_FILES})

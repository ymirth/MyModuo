# CMakeLists.txt in Net directory

# 添加源文件
set(SOURCE_FILES
        rbtreetimer.cpp
)

# 添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Net)
include_directories(${PROJECT_SOURCE_DIR}/Logging)
# 添加库
add_library(timerlib ${SOURCE_FILES})

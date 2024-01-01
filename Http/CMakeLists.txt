# CMakeLists.txt in Http directory

# 添加源文件
set(SOURCE_FILES
        httpresponse.cpp
        httprequest.cpp
        httpserver.cpp
        httprequestparser.cpp
)

# 添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/Net)

# 添加库
add_library(httplib ${SOURCE_FILES})

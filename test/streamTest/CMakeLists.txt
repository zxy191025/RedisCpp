# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(testStream)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
add_compile_options(-Wall -Wextra -O0 -g)

# 设置动态库默认属性
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#自动链接当前目录下的.so
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# 查找源文件
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

# 添加头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/redis/base
)


add_executable(testStream ${SOURCE_FILES})

# 链接外部库
target_link_libraries(testStream
    pthread
    redis_base
    # 添加其他需要链接的库
)

# 设置安装目标
install(TARGETS testStream
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
    ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
)
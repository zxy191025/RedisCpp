# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(zmallocTest)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
add_compile_options(-Wall -Wextra -O0 -g)

# 设置动态库默认属性
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找源文件
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

# 添加头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/redis/base
)


add_executable(zmallocTest ${SOURCE_FILES})

# 链接外部库
target_link_libraries(zmallocTest
    pthread
    redis_base
    # 添加其他需要链接的库
)

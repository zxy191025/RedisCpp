# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(redis_base)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
add_compile_options(-g -O0)

# 设置动态库默认属性
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找源文件
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

# 添加头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}
)

# 创建动态库
add_library(redis_base SHARED ${SOURCE_FILES})

# 设置动态库版本
set_target_properties(redis_base PROPERTIES
    VERSION 1.0.0.0
    SOVERSION 1
)

# 链接外部库
target_link_libraries(redis_base
    pthread
    # 添加其他需要链接的库
)

# 设置安装目标
install(TARGETS redis_base
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
    ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
    RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/redisCpp
)

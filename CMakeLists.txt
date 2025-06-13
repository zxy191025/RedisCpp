cmake_minimum_required(VERSION 2.6)

project(redis C CXX)

set(CMAKE_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# only build examples if this is the main project
option(REDIS_BUILD_EXAMPLES "Build redis examples" OFF)
option(REDIS_BUILD_TESTS "Build redis tests" ON)

set(CXX_FLAGS "-Wall")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CXX_FLAGS "${CXX_FLAGS} -O3 -march=native -flto")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CXX_FLAGS "${CXX_FLAGS} -g -O0 -fsanitize=address")
endif()


string(REPLACE ";" " " CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/bin/${CMAKE_BUILD_TYPE})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out/bin/${CMAKE_BUILD_TYPE})


include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(redis/base)

if(REDIS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(REDIS_BUILD_TESTS)
  add_subdirectory(test)
endif()

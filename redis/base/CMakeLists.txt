set(redisBase
  #zmalloc.cc

  )

add_library(redis_base ${redisBase})
#target_link_libraries(muduo_base pthread rt)

install(TARGETS redis_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/redis/base)

if(REDIS_BUILD_EXAMPLES)
  #add_subdirectory(tests)
endif()

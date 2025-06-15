option(zmallocTest "zmalloc tests" ON)
if(zmallocTest)
    add_subdirectory(zmallocTest)
endif()

option(sdsTest "sds tests" ON)
if(sdsTest)
    add_subdirectory(sdsTest)
endif()
option(zmallocTest "zmallocTest" ON)
if(zmallocTest)
    add_subdirectory(zmallocTest)
endif()

option(sdsTest "sdsTest" ON)
if(sdsTest)
    add_subdirectory(sdsTest)
endif()

option(adlistTest "adlistTest" ON)
if(adlistTest)
    add_subdirectory(adlistTest)
endif()

option(randomNumGeneratorTest "randomNumGeneratorTest" ON)
if(randomNumGeneratorTest)
    add_subdirectory(randomNumGeneratorTest)
endif()

option(dictTest "dictTest" ON)
if(dictTest)
    add_subdirectory(dictTest)
endif()

option(toolFuncTest "toolFuncTest" ON)
if(toolFuncTest)
    add_subdirectory(toolFuncTest)
endif()

option(ziplistTest "ziplistTest" ON)
if(ziplistTest)
    add_subdirectory(ziplistTest)
endif()

option(zskiplistTest "zskiplistTest" ON)
if(zskiplistTest)
    add_subdirectory(zskiplistTest)
endif()

option(zsetTest "zsetTest" ON)
if(zsetTest)
    add_subdirectory(zsetTest)
endif()

option(raxTest "raxTest" ON)
if(raxTest)
    add_subdirectory(raxTest)
endif()


option(listPackTest "listPackTest" ON)
if(listPackTest)
    add_subdirectory(listPackTest)
endif()
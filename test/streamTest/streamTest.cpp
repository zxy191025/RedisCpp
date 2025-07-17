/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/01
 * All rights reserved. No one may copy or transfer.
 * Description: zskiplist test program
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "define.h"
#include "zmallocDf.h"
#include "stream.h"
#include "redisObject.h"
#include "sds.h"

using namespace REDIS_BASE;
#define UNUSED(x) (void)(x)

int __failed_tests = 0;
int __test_num = 0;
#define test_cond(descr,_c) do { \
    __test_num++; printf("%d - %s: ", __test_num, descr); \
    if(_c) printf("PASSED\n"); else {printf("FAILED\n"); __failed_tests++;} \
} while(0)

#define test_report() do { \
    printf("%d tests, %d passed, %d failed\n", __test_num, \
                    __test_num-__failed_tests, __failed_tests); \
    if (__failed_tests) { \
        printf("=== WARNING === We have failed tests here...\n"); \
        exit(1); \
    } \
} while(0)

int main(int argc, char **argv)
{
    streamCreate creator;

    // 测试 streamNew 函数
    stream *s = creator.streamNew();
    test_cond("Test streamNew function", s != nullptr);

    // 测试 streamLength 函数
    robj mockObj;
    mockObj.ptr = s;
    unsigned long length = creator.streamLength(&mockObj);
    test_cond("Test streamLength function", length == 0);

    // 测试 streamCreateCG 函数
    streamID startID;
    startID.ms = 0;
    startID.seq = 0;
    streamCG *cg = creator.streamCreateCG(s, "test_group", strlen("test_group"), &startID);
    test_cond("Test streamCreateCG function", cg != nullptr);


    // 测试 streamLookupCG 函数 
    sdsCreate sdsC;
    sds groupname = sdsC.sdsnew("test_group");
    streamCG *foundCG = creator.streamLookupCG(s, groupname);
    test_cond("Test streamLookupCG function", foundCG == cg);
    sdsC.sdsfree(groupname);
    
    // 测试 streamCompareID 函数
    streamID id1 = {1, 1};
    streamID id2 = {1, 2};
    int compareResult = creator.streamCompareID(&id1, &id2);
    test_cond("Test streamCompareID function", compareResult < 0);

    // 测试 streamIncrID 函数
    streamID idToIncr = {1, 1};
    int incrResult = creator.streamIncrID(&idToIncr);
    test_cond("Test streamIncrID function", incrResult == 0 && idToIncr.seq == 2);

    // 释放资源
    creator.freeStream(s);

    // 输出测试报告
    test_report();

    return 0;
}
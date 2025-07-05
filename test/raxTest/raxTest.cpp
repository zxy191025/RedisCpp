/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/01
 * All rights reserved. No one may copy or transfer.
 * Description: zset test program
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <cmath>
#include "zmallocDf.h"
#include "rax.h"
using namespace REDIS_BASE;


// 定义测试宏
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

int main()
{
    raxCreate raxCreator;

    // 测试 raxNew 方法
    rax *rt = raxCreator.raxNew();
    test_cond("raxNew should return a non-null pointer", rt != nullptr);

    // 测试 raxInsert 方法
    unsigned char key[] = "test_key";
    size_t keyLen = strlen(reinterpret_cast<const char*>(key));
    void *data = reinterpret_cast<void*>(123);
    void *old = nullptr;
    int insertResult = raxCreator.raxInsert(rt, key, keyLen, data, &old);
    test_cond("raxInsert should return 1 for successful insertion", insertResult == 1);

    
    // 测试 raxFind 方法
    void *foundData = raxCreator.raxFind(rt, key, keyLen);
    test_cond("raxFind should return the inserted data", foundData == data);

    // 测试 raxInsert 方法
    unsigned char key2[] = "test_key_2";
    size_t keyLen2 = strlen(reinterpret_cast<const char*>(key2));
    void *data2 = reinterpret_cast<void*>(123);
    void *old2 = nullptr;
    int insertResult2 = raxCreator.raxInsert(rt, key2, keyLen2, data2, &old2);
    test_cond("raxInsert should return 1 for successful insertion", insertResult2 == 1);

    // 测试 raxRemove 方法
    int removeResult = raxCreator.raxRemove(rt, key, keyLen, NULL);
    test_cond("raxRemove should return 1 for successful removal", removeResult == 1);

    // // 再次测试 raxFind 方法，确保键已被移除
    // foundData = raxCreator.raxFind(rt, key, keyLen);
    // test_cond("raxFind should return nullptr after removal", foundData == nullptr);

    // 测试 raxFree 方法
    raxCreator.raxFree(rt);
    // 这里无法直接测试 raxFree 的正确性，但可以假设它不会崩溃
    test_cond("raxFree should not crash", true);

    // 输出测试报告
    test_report();
    
    return 0;
}
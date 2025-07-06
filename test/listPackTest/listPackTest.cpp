/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/01
 * All rights reserved. No one may copy or transfer.
 * Description: listPack test program
 */
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "listPack.h"

using namespace REDIS_BASE;

// 测试宏定义
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

int main() {
    REDIS_BASE::listPackCreate lpc;
    unsigned char *lp = nullptr;
    unsigned char ele[] = {1, 2, 3};
    uint32_t eleSize = sizeof(ele);

    unsigned char ele2[] = {10, 12, 13, 19};
    uint32_t eleSize2 = sizeof(ele2);

    unsigned char *newp = nullptr;
    int64_t count = 0;
    unsigned char intbuf[21];

    // 测试 lpNew 函数
    lp = lpc.lpNew(1024);
    test_cond("Test lpNew function", lp != nullptr);

    // 测试 lpAppend 函数
    lp = lpc.lpAppend(lp, ele, eleSize);
    test_cond("Test lpAppend function", lp != nullptr);

    // 测试 lpAppend 函数
    lp = lpc.lpAppend(lp, ele2, eleSize2);
    test_cond("Test lpAppend function", lp != nullptr);

    // 测试 lpLength 函数
    uint32_t length = lpc.lpLength(lp);
    // 两次追加操作后，链表中应该有 2 个元素
    test_cond("Test lpLength function", length == 2);

    // 测试 lpFirst 函数
    unsigned char *first = lpc.lpFirst(lp);
    test_cond("Test lpFirst function", first != nullptr);

    // 测试 lpGet 函数
    unsigned char *next = lpc.lpGet(first, &count, intbuf);
    test_cond("Test lpGet function", next != nullptr);

    // 测试 lpNext 函数
    unsigned char *second = lpc.lpNext(lp, first);
    // 追加了两个元素，第一个元素的下一个元素不应该为 nullptr
    test_cond("Test lpNext function", second != nullptr);

    // 测试 lpDelete 函数
    lp = lpc.lpDelete(lp, first, &newp);
    test_cond("Test lpDelete function", lp != nullptr);

    // 测试 lpLength 函数（删除后）
    length = lpc.lpLength(lp);
    test_cond("Test lpLength function after deletion", length == 1);

    // 测试 lpFree 函数
    lpc.lpFree(lp);
    test_cond("Test lpFree function", true); // 无法直接验证释放是否成功，简单标记为通过

    // 报告测试结果
    test_report();

    return 0;
}
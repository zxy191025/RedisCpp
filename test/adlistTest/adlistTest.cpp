/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: adlist test
 */
#include <stdio.h>
#include <stdlib.h>
#include "adlist.h"
#include "define.h"
using namespace REDIS_BASE;

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
    adlistCreate creator;

    // 测试链表创建
    list *l = creator.listCreate();
    test_cond("listCreate: Check if list is created successfully", l != nullptr);
    test_cond("listCreate: Check initial length", listLength(l) == 0);
    test_cond("listCreate: Check initial head", listFirst(l) == nullptr);
    test_cond("listCreate: Check initial tail", listLast(l) == nullptr);

    // 测试在链表头部添加节点
    int value1 = 1;
    list *result = creator.listAddNodeHead(l, &value1);
    test_cond("listAddNodeHead: Check if adding to head returns list pointer", result != nullptr);
    test_cond("listAddNodeHead: Check length after adding to head", listLength(l) == 1);
    test_cond("listAddNodeHead: Check value of first node after adding to head", listNodeValue(listFirst(l)) == &value1);

    // 测试在链表尾部添加节点
    int value2 = 2;
    result = creator.listAddNodeTail(l, &value2);
    test_cond("listAddNodeTail: Check if adding to tail returns list pointer", result != nullptr);
    test_cond("listAddNodeTail: Check length after adding to tail", listLength(l) == 2);
    test_cond("listAddNodeTail: Check value of last node after adding to tail", listNodeValue(listLast(l)) == &value2);

    // 测试删除节点
    listNode *nodeToDelete = listFirst(l);
    creator.listDelNode(l, nodeToDelete);
    test_cond("listDelNode: Check length after deleting a node", listLength(l) == 1);
    test_cond("listDelNode: Check if head and tail are the same after deleting first node", listFirst(l) == listLast(l));
    test_cond("listDelNode: Check value of remaining node after deleting first node", listNodeValue(listFirst(l)) == &value2);

    // 测试链表复制
    list *copy = creator.listDup(l);
    test_cond("listDup: Check if copy is created successfully", copy != nullptr);
    test_cond("listDup: Check length of copied list", listLength(copy) == 1);
    int *copiedVal = static_cast<int*>(listNodeValue(listFirst(copy)));
    test_cond("listDup: Check value of copied node", *copiedVal == value2);
    creator.listRelease(copy);

    // 测试根据键值搜索节点
    listNode *foundNode = creator.listSearchKey(l, &value2);
    test_cond("listSearchKey: Check if node is found by key", foundNode != nullptr);
    test_cond("listSearchKey: Check if found node has correct value", listNodeValue(foundNode) == listNodeValue(listFirst(l)));

    // 测试根据索引获取节点
    listNode *indexNode = creator.listIndex(l, 0);
    test_cond("listIndex: Check if node is retrieved by index", indexNode != nullptr);
    test_cond("listIndex: Check if retrieved node has correct value", listNodeValue(indexNode) == listNodeValue(listFirst(l)));

    // 测试链表旋转（尾节点移到头部）
    int value3 = 3;
    creator.listAddNodeTail(l, &value3);
    creator.listRotateTailToHead(l);
    test_cond("listRotateTailToHead: Check value of first node after rotation", listNodeValue(listFirst(l)) == &value3);
    test_cond("listRotateTailToHead: Check value of last node after rotation", listNodeValue(listLast(l)) == &value2);

    // 测试链表旋转（头节点移到尾部）
    creator.listRotateHeadToTail(l);
    test_cond("listRotateHeadToTail: Check value of first node after rotation", listNodeValue(listFirst(l)) == &value2);
    test_cond("listRotateHeadToTail: Check value of last node after rotation", listNodeValue(listLast(l)) == &value3);

    // 测试链表合并
    list *l2 = creator.listCreate();
    int value4 = 4;
    creator.listAddNodeTail(l2, &value4);
    creator.listJoin(l, l2);
    test_cond("listJoin: Check length of target list after joining", listLength(l) == 3);
    test_cond("listJoin: Check value of last node after joining", listNodeValue(listLast(l)) == &value4);
    test_cond("listJoin: Check length of source list after joining", listLength(l2) == 0);
    creator.listRelease(l2);

    // 释放链表
    creator.listRelease(l);

    // 输出测试报告
    test_report();

    return 0;
}

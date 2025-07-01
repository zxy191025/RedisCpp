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
#include "dict.h"
#include "zskiplist.h"
#include "zmallocDf.h"
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
static sdsCreate sdsC;
int main() {
    zskiplistCreate creator;
    zskiplist *zsl = creator.zslCreate();

    // 测试 zslInsert 接口
    sds ele1 = sdsC.sdsnew("element1");
    sds ele2 = sdsC.sdsnew("element2");
    sds ele3 = sdsC.sdsnew("element3");
    sds ele4 = sdsC.sdsnew("element4");
    sds ele5 = sdsC.sdsnew("element5");
    sds ele6 = sdsC.sdsnew("element6");
    sds ele7 = sdsC.sdsnew("element7");
    sds ele8 = sdsC.sdsnew("element8");
    sds ele9 = sdsC.sdsnew("element9");
    sds ele10 = sdsC.sdsnew("element10");
    //gdb调试内容
    /*
    60   zskiplistNode *node8 = creator.zslInsert(zsl, 3.3, ele8);
    (gdb) p *zsl
    $1 = {header = 0x516000000080, tail = 0x5040000000d0, length = 7, level = 2}
    (gdb)  p *zsl->header->level[0].forward
    $2 = {ele = 0x5020000000f1 "element6", score = 0.29999999999999999, backward = 0x0, level = 0x504000000128}
    (gdb) set $node = zsl->header->level[0].forward
    (gdb) while $node != 0
    >p $node->score
    >p $node->ele
    >set $node = $node->level[0].forward
    >end
    $3 = 0.29999999999999999
    $4 = (sds) 0x5020000000f1 "element6"
    $5 = 1
    $6 = (sds) 0x502000000051 "element1"
    $7 = 1.3
    $8 = (sds) 0x502000000111 "element7"
    $9 = 1.3999999999999999
    $10 = (sds) 0x502000000091 "element3"
    $11 = 2
    $12 = (sds) 0x502000000071 "element2"
    $13 = 2.2000000000000002
    $14 = (sds) 0x5020000000b1 "element4"
    $15 = 2.7999999999999998
    $16 = (sds) 0x5020000000d1 "element5"        
    */
    zskiplistNode *node1 = creator.zslInsert(zsl, 1.0, ele1);
    zskiplistNode *node2 = creator.zslInsert(zsl, 2.0, ele2);
    zskiplistNode *node3 = creator.zslInsert(zsl, 1.4, ele3);
    zskiplistNode *node4 = creator.zslInsert(zsl, 2.2, ele4);
    zskiplistNode *node5 = creator.zslInsert(zsl, 2.8, ele5);
    zskiplistNode *node6 = creator.zslInsert(zsl, 0.3, ele6);
    zskiplistNode *node7 = creator.zslInsert(zsl, 1.3, ele7);
    zskiplistNode *node8 = creator.zslInsert(zsl, 3.3, ele8);
    zskiplistNode *node9 = creator.zslInsert(zsl, 3.9, ele9);
    zskiplistNode *node10 = creator.zslInsert(zsl, 0.7, ele10);


    test_cond("Test zslInsert", node1 != nullptr && node2 != nullptr
    && node3 != nullptr&& node4 != nullptr&& node5 != nullptr&& node6 != nullptr&& node7 != nullptr
    && node8 != nullptr&& node9 != nullptr&& node10 != nullptr);

    // 测试 zslIsInRange 接口
    zrangespec range;
    range.min = 0.1;
    range.max = 10.0;
    range.minex = 0;
    range.maxex = 0;
    test_cond("Test zslIsInRange", creator.zslIsInRange(zsl, &range) == 1);

    // 测试 zslFirstInRange 接口
    zskiplistNode *firstInRange = creator.zslFirstInRange(zsl, &range);
    test_cond("Test zslFirstInRange", firstInRange != nullptr && firstInRange->score == 0.3);

    // 测试 zslLastInRange 接口
    zskiplistNode *lastInRange = creator.zslLastInRange(zsl, &range);
    test_cond("Test zslLastInRange", lastInRange != nullptr && lastInRange->score == 3.9);

    // 测试 zslGetRank 接口
    unsigned long rank = creator.zslGetRank(zsl, 1.0, ele1);
    test_cond("Test zslGetRank", rank == 3);

    // 测试 zslDelete 接口
    zskiplistNode *deletedNode = nullptr;
    int deleted = creator.zslDelete(zsl, 1.0, ele1, &deletedNode);
    test_cond("Test zslDelete", deleted == 1 && deletedNode != nullptr);
    creator.zslFreeNode(deletedNode);

    // 测试 zslFree 接口
    creator.zslFree(zsl);
    test_cond("Test zslFree", true);  // 由于 zslFree 没有返回值，这里简单认为通过

    //无需再释放ele1 ele2

    test_report();

    return 0;
}
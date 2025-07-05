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
#include "dict.h"
#include "zskiplist.h"
#include "ziplist.h"
#include "zmallocDf.h"
#include "sds.h"
#include "zset.h"
#include "redisObject.h"
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

ziplistCreate ziplistCreateInst;
zskiplistCreate zskiplistCreateInst;
dictionaryCreate dictionaryCreateInst;
sdsCreate sdsCreateInst;

// 简单的辅助函数，用于创建 robj 对象
robj* createZsetObject() {
    robj *zobj = static_cast<robj*>(zmalloc(sizeof(robj)));
    zobj->type = OBJ_ZSET;
    zobj->encoding = OBJ_ENCODING_ZIPLIST;
    zobj->ptr = ziplistCreateInst.ziplistNew();
    return zobj;
}

// 简单的辅助函数，用于销毁 robj 对象
void destroyZsetObject(robj *zobj) {
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        zfree(zobj->ptr); 
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs = (zset*)zobj->ptr;
        zskiplistCreateInst.zslFree(zs->zsl);
        dictionaryCreateInst.dictRelease(zs->dictl);
        zfree(zs);
    }
    zfree(zobj);
}

int main() {
     zsetCreate zsetCreator;
    
    printf("=== Starting ZSET Tests ===\n");
    
    // 测试 zsetLength 函数 - 空集合
    {
        robj *zobj = createZsetObject();
        test_cond("zsetLength on empty set", zsetCreator.zsetLength(zobj) == 0);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetAdd 和 zsetLength 函数
    {
        robj *zobj = createZsetObject();
        sds ele1 = sdsCreateInst.sdsnew("member1");
        sds ele2 = sdsCreateInst.sdsnew("member2");
        
        int out_flags;
        double newscore;
        
        // 添加元素
        test_cond("zsetAdd member1", zsetCreator.zsetAdd(zobj, 10.0, ele1, 0, &out_flags, &newscore));
        test_cond("zsetAdd member2", zsetCreator.zsetAdd(zobj, 20.0, ele2, 0, &out_flags, &newscore));
        
        // 验证长度
        test_cond("zsetLength after adds", zsetCreator.zsetLength(zobj) == 2);
        
        // 测试重复添加
        test_cond("zsetAdd existing member1", zsetCreator.zsetAdd(zobj, 15.0, ele1, 0, &out_flags, &newscore));
        test_cond("zsetLength after update", zsetCreator.zsetLength(zobj) == 2);
        
        sdsCreateInst.sdsfree(ele1);
        sdsCreateInst.sdsfree(ele2);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetScore 函数
    {
        robj *zobj1 = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("member");
        double score;
        int out_flags;
        // 添加元素
        zsetCreator.zsetAdd(zobj1, 100.5, ele, 0, &out_flags, &score);
         sds ele2 = sdsCreateInst.sdsnew("non_exist");
        // 查询分数
        test_cond("zsetScore existing member", zsetCreator.zsetScore(zobj1, ele, &score) && score == 100.5);
        test_cond("zsetScore non-existing member", !zsetCreator.zsetScore(zobj1, ele2, &score));
        
        sdsCreateInst.sdsfree(ele);
        sdsCreateInst.sdsfree(ele2);
        destroyZsetObject(zobj1);
    }
    
    // 测试 zsetRank 函数
    {
        robj *zobj = createZsetObject();
        sds ele1 = sdsCreateInst.sdsnew("member1");
        sds ele2 = sdsCreateInst.sdsnew("member2");
        sds ele3 = sdsCreateInst.sdsnew("member3");
        
        double score;
        int out_flags;
        // 添加元素
        zsetCreator.zsetAdd(zobj, 10.0, ele1, 0, &out_flags, &score);
        zsetCreator.zsetAdd(zobj, 30.0, ele2, 0, &out_flags, &score);
        zsetCreator.zsetAdd(zobj, 20.0, ele3, 0, &out_flags, &score);
        
        // 测试升序排名
        test_cond("zsetRank asc member1", zsetCreator.zsetRank(zobj, ele1, 0) == 0);
        test_cond("zsetRank asc member3", zsetCreator.zsetRank(zobj, ele3, 0) == 1);
        test_cond("zsetRank asc member2", zsetCreator.zsetRank(zobj, ele2, 0) == 2);
        
        // 测试降序排名
        test_cond("zsetRank desc member2", zsetCreator.zsetRank(zobj, ele2, 1) == 0);
        test_cond("zsetRank desc member3", zsetCreator.zsetRank(zobj, ele3, 1) == 1);
        test_cond("zsetRank desc member1", zsetCreator.zsetRank(zobj, ele1, 1) == 2);
        
        sdsCreateInst.sdsfree(ele1);
        sdsCreateInst.sdsfree(ele2);
        sdsCreateInst.sdsfree(ele3);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetDel 函数
    {
        robj *zobj = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("to_delete");
        
        double score;
        int out_flags;
        // 添加元素
        zsetCreator.zsetAdd(zobj, 50.0, ele, 0, &out_flags, &score);
        test_cond("zsetLength before delete", zsetCreator.zsetLength(zobj) == 1);
        
        // 删除元素
        test_cond("zsetDel existing", zsetCreator.zsetDel(zobj, ele));
        test_cond("zsetLength after delete", zsetCreator.zsetLength(zobj) == 0);
        
        // 测试删除不存在的元素
        test_cond("zsetDel non-existing", !zsetCreator.zsetDel(zobj, ele));
        
        sdsCreateInst.sdsfree(ele);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetConvert 函数
    {
        robj *zobj = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("convert_test");
        
        double score1;
        int out_flags1;
        // 添加元素
        zsetCreator.zsetAdd(zobj, 10.0, ele, 0,&out_flags1, &score1);
        test_cond("Initial encoding is ziplist", zobj->encoding == OBJ_ENCODING_ZIPLIST);
        
        // 转换为跳跃表编码
        zsetCreator.zsetConvert(zobj, OBJ_ENCODING_SKIPLIST);
        test_cond("Encoding after convert is skiplist", zobj->encoding == OBJ_ENCODING_SKIPLIST);
        
        // 验证操作仍然有效
        double score;
        test_cond("zsetScore after convert", zsetCreator.zsetScore(zobj, ele, &score) && score == 10.0);
        
        // 转换回压缩列表
        zsetCreator.zsetConvert(zobj, OBJ_ENCODING_ZIPLIST);
        test_cond("Encoding after convert back is ziplist", zobj->encoding == OBJ_ENCODING_ZIPLIST);
        
        sdsCreateInst.sdsfree(ele);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetConvertToZiplistIfNeeded 函数
    {
        robj *zobj = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("test");
        
        double score;
        int out_flags;

        // 添加元素
        zsetCreator.zsetAdd(zobj, 10.0, ele, 0, &out_flags, &score);
        
        // 转换为跳跃表
        zsetCreator.zsetConvert(zobj, OBJ_ENCODING_SKIPLIST);
        test_cond("Encoding before conversion attempt", zobj->encoding == OBJ_ENCODING_SKIPLIST);
        
        // 尝试转换回压缩列表（由于元素太少，应该保持跳跃表）
        zsetCreator.zsetConvertToZiplistIfNeeded(zobj, 100, 1000);
        test_cond("Encoding after conversion attempt (skiplist)", zobj->encoding == OBJ_ENCODING_SKIPLIST);
        
        // 添加大量元素以满足转换条件（模拟）
        for (int i = 0; i < 100; i++) {
            sds key = sdsCreateInst.sdsnewlen("element", 7);
            key = sdsCreateInst.sdscatprintf(key, "%d", i);
            double score3;
            int out_flags3;
            zsetCreator.zsetAdd(zobj, (double)i, key, 0, &out_flags3, &score3);
            sdsCreateInst.sdsfree(key);
        }
        
        // 再次尝试转换（假设元素大小和数量满足条件）
        zsetCreator.zsetConvertToZiplistIfNeeded(zobj, 10, 10);
        test_cond("Encoding after conversion attempt (ziplist)", zobj->encoding == OBJ_ENCODING_ZIPLIST);
        
        sdsCreateInst.sdsfree(ele);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetDup 函数
    {
        robj *src = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("dup_test");
        
        double score1;
        int out_flags1;
        // 添加元素
        zsetCreator.zsetAdd(src, 100.0, ele, 0, &out_flags1, &score1);
        
        // 复制对象
        robj *dup = zsetCreator.zsetDup(src);
        test_cond("zsetDup creates non-null object", dup != NULL);
        test_cond("Dup encoding matches source", dup->encoding == src->encoding);
        
        // 验证内容相同
        double score;
        test_cond("zsetScore on dup", zsetCreator.zsetScore(dup, ele, &score) && score == 100.0);
        
        // 释放资源
        destroyZsetObject(src);
        destroyZsetObject(dup);
        sdsCreateInst.sdsfree(ele);
    }
    
    // 测试 zsetZiplistValidateIntegrity 函数
    {
        robj *zobj = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("validate_test");
        
        double score;
        int out_flags;
        // 添加元素
        zsetCreator.zsetAdd(zobj, 10.0, ele, 0, &out_flags, &score);
        
        // 验证完整性（初始状态应该有效）
        unsigned char *zl = (unsigned char*)zobj->ptr;
        size_t size = ziplistCreateInst.ziplistBlobLen(zl);
        test_cond("zsetZiplistValidateIntegrity initial", zsetCreator.zsetZiplistValidateIntegrity(zl, size, 0));
        
        // 模拟损坏（仅用于测试，实际代码不应这样做）
        // 注意：这会导致内存损坏，仅用于测试目的
        // zl[0] = 0xFF; // 取消注释此行会导致验证失败
        // test_cond("zsetZiplistValidateIntegrity after corruption", !zsetCreator.zsetZiplistValidateIntegrity(zl, size, 0));
        
        sdsCreateInst.sdsfree(ele);
        destroyZsetObject(zobj);
    }
    
    // 测试 zsetRemoveFromSkiplist 函数
    {
        robj *zobj = createZsetObject();
        sds ele = sdsCreateInst.sdsnew("remove_test");
        double score;
        int out_flags;
        // 添加元素
        zsetCreator.zsetAdd(zobj, 20.0, ele, 0, &out_flags, &score);
        
        // 转换为跳跃表编码
        zsetCreator.zsetConvert(zobj, OBJ_ENCODING_SKIPLIST);
        zset *zs = (zset*)zobj->ptr;
        
        // 删除元素
        test_cond("zsetRemoveFromSkiplist existing", zsetCreator.zsetRemoveFromSkiplist(zs, ele));
        test_cond("zsetLength after skiplist remove", zsetCreator.zsetLength(zobj) == 0);
        
        // 尝试删除不存在的元素
        test_cond("zsetRemoveFromSkiplist non-existing", !zsetCreator.zsetRemoveFromSkiplist(zs, ele));
        
        sdsCreateInst.sdsfree(ele);
        destroyZsetObject(zobj);
    }
    
    // 输出测试报告
    test_report();
    
    return 0;
}
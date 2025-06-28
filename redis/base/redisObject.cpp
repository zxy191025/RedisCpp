/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: redisObject implementation
 */
#include "redisObject.h"
#include "zmallocDf.h"
#include "zset.h"
redisObjectCreate::redisObjectCreate()
{
    zskiplistCreateInstance = static_cast<zskiplistCreate *>(zmalloc(sizeof(zskiplistCreate)));
    //serverAssert(zskiplistCreateInstance != NULL);
    dictionaryCreateInstance = static_cast<dictionaryCreate *>(zmalloc(sizeof(dictionaryCreate)));
    //serverAssert(dictionaryCreateInstance != NULL);
    zsetCreateInstance = static_cast<zsetCreate *>(zmalloc(sizeof(zsetCreate)));
    //serverAssert(zsetCreateInstance != NULL);
}
redisObjectCreate::~redisObjectCreate()
{
    zfree(zskiplistCreateInstance);
    zfree(dictionaryCreateInstance);
    zfree(zsetCreateInstance);
}
/**
 * 创建一个Redis对象(robj)。
 * Redis使用robj结构表示所有键值对的值，支持多种数据类型。
 * 
 * @param type 对象类型(如REDIS_STRING, REDIS_LIST, REDIS_SET等)
 * @param ptr 指向对象实际数据的指针
 * @return 新创建的Redis对象
 */
robj *redisObjectCreate::createObject(int type, void *ptr) 
{
    robj *o =static_cast<robj*>(zmalloc(sizeof(*o)));
    o->type = type;
    o->encoding = OBJ_ENCODING_RAW;
    o->ptr = ptr;
    o->refcount = 1;
    //delete by zhenjia.zhao
    /* Set the LRU to the current lruclock (minutes resolution), or
     * alternatively the LFU counter. */
    // if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
    //     o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
    // } else {
    //     o->lru = LRU_CLOCK();
    // }
    return o;
}

/**
 * 创建一个有序集合(zset)对象。
 * 有序集合同时使用哈希表和跳跃表实现，确保成员唯一性并支持按分数排序。
 * 
 * @return 新创建的有序集合对象
 */
robj *redisObjectCreate::createZsetObject(void)
{
    zset *zs = static_cast<zset*>(zmalloc(sizeof(*zs)));
    robj *o;

    zs->dictl = dictionaryCreateInstance->dictCreate(&zsetCreateInstance->zsetDictType,NULL);
    zs->zsl = zskiplistCreateInstance->zslCreate();
    o = createObject(OBJ_ZSET,zs);
    o->encoding = OBJ_ENCODING_SKIPLIST;
    return o;
}
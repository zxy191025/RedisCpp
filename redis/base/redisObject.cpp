/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: redisObject（或称为 Redis 对象系统）是实现多态数据类型的核心组件，
 * 它定义了 Redis 中所有键值对的基本结构。通过 redisObject，
 * Redis 能够在同一个数据库中存储不同类型（如字符串、列表、哈希、集合、有序集合）的值，并对这些值执行类型特定的操作。
 */
#include "redisObject.h"
#include "zskiplist.h"
#include "ziplist.h"
#include "toolFunc.h"
#include "sds.h"
#include "zmallocDf.h"
#include "zset.h"
#include "intset.h"
#include "quicklist.h"
#include <string.h>
#include "debugDf.h"
#include "module.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
sdsCreate sdsCreateInst;
dictionaryCreate dictionaryCreateInst;
redisObjectCreate::redisObjectCreate()
{
    zskiplistCreateInstance = static_cast<zskiplistCreate *>(zmalloc(sizeof(zskiplistCreate)));
    serverAssert(zskiplistCreateInstance != NULL);
    dictionaryCreateInstance = static_cast<dictionaryCreate *>(zmalloc(sizeof(dictionaryCreate)));
    serverAssert(dictionaryCreateInstance != NULL);
    zsetCreateInstance = static_cast<zsetCreate *>(zmalloc(sizeof(zsetCreate)));
    serverAssert(zsetCreateInstance != NULL);
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
    serverAssert(sdsCreateInstance != NULL);
    toolFuncInstance = static_cast<toolFunc *>(zmalloc(sizeof(toolFunc)));
    serverAssert(toolFuncInstance != NULL);
    ziplistCreateInstance = static_cast<ziplistCreate *>(zmalloc(sizeof(ziplistCreate)));
    serverAssert(ziplistCreateInstance != NULL);
    intsetCreateInstance = static_cast<intsetCreate*>(zmalloc(sizeof(intsetCreate)));
    serverAssert(intsetCreateInstance != NULL);
    quicklistCreateInstance = static_cast<quicklistCreate*>(zmalloc(sizeof(quicklistCreate)));
    serverAssert(quicklistCreateInstance != NULL);

}
redisObjectCreate::~redisObjectCreate()
{
    zfree(zskiplistCreateInstance);
    zfree(dictionaryCreateInstance);
    zfree(zsetCreateInstance);
    zfree(toolFuncInstance);
    zfree(sdsCreateInstance);
    zfree(ziplistCreateInstance);
    zfree(intsetCreateInstance);
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

void redisObjectCreate::createSharedObjects(void)
{
     int j;
    /* Shared command responses */
    shared.crlf = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("\r\n"));
    shared.ok = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("+OK\r\n"));
    shared.emptybulk = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("$0\r\n\r\n"));
    shared.czero = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(":0\r\n"));
    shared.cone = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(":1\r\n"));
    shared.emptyarray = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("*0\r\n"));
    shared.pong = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("+PONG\r\n"));
    shared.queued = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("+QUEUED\r\n"));
    shared.emptyscan = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("*2\r\n$1\r\n0\r\n*0\r\n"));
    shared.space = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(" "));
    shared.colon = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(":"));
    shared.plus = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("+"));

    /* Shared command error responses */
    shared.wrongtypeerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n"));
    shared.err = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("-ERR\r\n"));
    shared.nokeyerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-ERR no such key\r\n"));
    shared.syntaxerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-ERR syntax error\r\n"));
    shared.sameobjecterr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-ERR source and destination objects are the same\r\n"));
    shared.outofrangeerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-ERR index out of range\r\n"));
    shared.noscripterr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-NOSCRIPT No matching script. Please use EVAL.\r\n"));
    shared.loadingerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-LOADING Redis is loading the dataset in memory\r\n"));
    shared.slowscripterr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-BUSY Redis is busy running a script. You can only call SCRIPT KILL or SHUTDOWN NOSAVE.\r\n"));
    shared.masterdownerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-MASTERDOWN Link with MASTER is down and replica-serve-stale-data is set to 'no'.\r\n"));
    shared.bgsaveerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-MISCONF Redis is configured to save RDB snapshots, but it is currently not able to persist on disk. Commands that may modify the data set are disabled, because this instance is configured to report errors during writes if RDB snapshotting fails (stop-writes-on-bgsave-error option). Please check the Redis logs for details about the RDB error.\r\n"));
    shared.roslaveerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-READONLY You can't write against a read only replica.\r\n"));
    shared.noautherr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-NOAUTH Authentication required.\r\n"));
    shared.oomerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-OOM command not allowed when used memory > 'maxmemory'.\r\n"));
    shared.execaborterr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-EXECABORT Transaction discarded because of previous errors.\r\n"));
    shared.noreplicaserr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-NOREPLICAS Not enough good replicas to write.\r\n"));
    shared.busykeyerr = createObject(OBJ_STRING,sdsCreateInstance->sdsnew(
        "-BUSYKEY Target key name already exists.\r\n"));

    /* The shared NULL depends on the protocol version. */
    shared.null[0] = NULL;
    shared.null[1] = NULL;
    shared.null[2] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("$-1\r\n"));
    shared.null[3] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("_\r\n"));

    shared.nullarray[0] = NULL;
    shared.nullarray[1] = NULL;
    shared.nullarray[2] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("*-1\r\n"));
    shared.nullarray[3] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("_\r\n"));

    shared.emptymap[0] = NULL;
    shared.emptymap[1] = NULL;
    shared.emptymap[2] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("*0\r\n"));
    shared.emptymap[3] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("%0\r\n"));

    shared.emptyset[0] = NULL;
    shared.emptyset[1] = NULL;
    shared.emptyset[2] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("*0\r\n"));
    shared.emptyset[3] = createObject(OBJ_STRING,sdsCreateInstance->sdsnew("~0\r\n"));
    for (j = 0; j < PROTO_SHARED_SELECT_CMDS; j++) {
        char dictid_str[64];
        int dictid_len;

        dictid_len = toolFuncInstance->ll2string(dictid_str,sizeof(dictid_str),j);
        shared.select[j] = createObject(OBJ_STRING,
            sdsCreateInstance->sdscatprintf(sdsCreateInstance->sdsempty(),
                "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                dictid_len, dictid_str));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);

    /* Shared command names */
    shared.del = createStringObject("DEL",3);
    shared.unlink = createStringObject("UNLINK",6);
    shared.rpop = createStringObject("RPOP",4);
    shared.lpop = createStringObject("LPOP",4);
    shared.lpush = createStringObject("LPUSH",5);
    shared.rpoplpush = createStringObject("RPOPLPUSH",9);
    shared.lmove = createStringObject("LMOVE",5);
    shared.blmove = createStringObject("BLMOVE",6);
    shared.zpopmin = createStringObject("ZPOPMIN",7);
    shared.zpopmax = createStringObject("ZPOPMAX",7);
    shared.multi = createStringObject("MULTI",5);
    shared.exec = createStringObject("EXEC",4);
    shared.hset = createStringObject("HSET",4);
    shared.srem = createStringObject("SREM",4);
    shared.xgroup = createStringObject("XGROUP",6);
    shared.xclaim = createStringObject("XCLAIM",6);
    shared.script = createStringObject("SCRIPT",6);
    shared.replconf = createStringObject("REPLCONF",8);
    shared.pexpireat = createStringObject("PEXPIREAT",9);
    shared.pexpire = createStringObject("PEXPIRE",7);
    shared.persist = createStringObject("PERSIST",7);
    shared.set = createStringObject("SET",3);
    shared.eval = createStringObject("EVAL",4);

    /* Shared command argument */
    shared.left = createStringObject("left",4);
    shared.right = createStringObject("right",5);
    shared.pxat = createStringObject("PXAT", 4);
    shared.px = createStringObject("PX",2);
    shared.time = createStringObject("TIME",4);
    shared.retrycount = createStringObject("RETRYCOUNT",10);
    shared.force = createStringObject("FORCE",5);
    shared.justid = createStringObject("JUSTID",6);
    shared.lastid = createStringObject("LASTID",6);
    shared.default_username = createStringObject("default",7);
    shared.ping = createStringObject("ping",4);
    shared.setid = createStringObject("SETID",5);
    shared.keepttl = createStringObject("KEEPTTL",7);
    shared.load = createStringObject("LOAD",4);
    shared.createconsumer = createStringObject("CREATECONSUMER",14);
    shared.getack = createStringObject("GETACK",6);
    shared.special_asterick = createStringObject("*",1);
    shared.special_equals = createStringObject("=",1);
    shared.redacted = makeObjectShared(createStringObject("(redacted)",10));

    for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
        shared.integers[j] =
            makeObjectShared(createObject(OBJ_STRING,(void*)(long)j));
        shared.integers[j]->encoding = OBJ_ENCODING_INT;
    }
    for (j = 0; j < OBJ_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] = createObject(OBJ_STRING,
            sdsCreateInstance->sdscatprintf(sdsCreateInstance->sdsempty(),"*%d\r\n",j));
        shared.bulkhdr[j] = createObject(OBJ_STRING,
            sdsCreateInstance->sdscatprintf(sdsCreateInstance->sdsempty(),"$%d\r\n",j));
    }

    shared.minstring = sdsCreateInstance->sdsnew("minstring");
    shared.maxstring = sdsCreateInstance->sdsnew("maxstring");
}

/**
 * 共享对象，增加引用计数并返回原对象指针
 * 
 * @param o 要共享的对象
 * @return 返回增加引用计数后的对象指针
 */
robj *redisObjectCreate::makeObjectShared(robj *o)
{
    serverAssert(o->refcount == 1);
    o->refcount = OBJ_SHARED_REFCOUNT;
    return o;
}

/**
 * 创建一个原始字符串对象（未经过编码优化）
 * 
 * @param ptr 字符串内容指针
 * @param len 字符串长度
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createRawStringObject(const char *ptr, size_t len)
{
    return createObject(OBJ_STRING, sdsCreateInstance->sdsnewlen(ptr,len));
}

/**
 * 创建一个嵌入式字符串对象（用于短字符串的优化表示）
 * 
 * @param ptr 字符串内容指针
 * @param len 字符串长度
 * @return 返回新创建的嵌入式字符串对象
 */
robj *redisObjectCreate::createEmbeddedStringObject(const char *ptr, size_t len)
{
    robj *o =static_cast<robj*>(zmalloc(sizeof(robj)+sizeof(struct sdshdr8)+len+1));
    struct sdshdr8 *sh = static_cast<struct sdshdr8 *>((void*)(o+1));

    o->type = OBJ_STRING;
    o->encoding = OBJ_ENCODING_EMBSTR;
    o->ptr = sh+1;
    o->refcount = 1;
    //delete by zhenjia.zhao
    // if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
    //     o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
    // } else {
    //     o->lru = LRU_CLOCK();
    // }

    sh->len = len;
    sh->alloc = len;
    sh->flags = SDS_TYPE_8;
    if (ptr == SDS_NOINIT)
        sh->buf[len] = '\0';
    else if (ptr) {
        memcpy(sh->buf,ptr,len);
        sh->buf[len] = '\0';
    } else {
        memset(sh->buf,0,len+1);
    }
    return o;
}

/**
 * 创建字符串对象（自动选择合适的编码方式）
 * 
 * @param ptr 字符串内容指针
 * @param len 字符串长度
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createStringObject(const char *ptr, size_t len)
{
    if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT)
        return createEmbeddedStringObject(ptr,len);
    else
        return createRawStringObject(ptr,len);
}

/**
 * 尝试创建原始字符串对象（内存不足时可能返回NULL）
 * 
 * @param ptr 字符串内容指针
 * @param len 字符串长度
 * @return 成功时返回新创建的字符串对象，失败时返回NULL
 */
robj *redisObjectCreate::tryCreateRawStringObject(const char *ptr, size_t len)
{
    sds str = sdsCreateInstance->sdstrynewlen(ptr,len);
    if (!str) return NULL;
    return createObject(OBJ_STRING, str);
}

/**
 * 尝试创建字符串对象（内存不足时可能返回NULL）
 * 
 * @param ptr 字符串内容指针
 * @param len 字符串长度
 * @return 成功时返回新创建的字符串对象，失败时返回NULL
 */
robj *redisObjectCreate::tryCreateStringObject(const char *ptr, size_t len)
{
    if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT)
        return createEmbeddedStringObject(ptr,len);
    else
        return tryCreateRawStringObject(ptr,len);
}

/**
 * 从长整型创建字符串对象（可指定是否为值对象）
 * 
 * @param value 长整型值
 * @param valueobj 是否作为值对象创建
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createStringObjectFromLongLongWithOptions(long long value, int valueobj)
{
   robj *o;
    //delete by zhenjia.zhao
    // if (server.maxmemory == 0 ||
    //     !(server.maxmemory_policy & MAXMEMORY_FLAG_NO_SHARED_INTEGERS))
    // {
    //     /* If the maxmemory policy permits, we can still return shared integers
    //      * even if valueobj is true. */
    //     valueobj = 0;
    // }

    if (value >= 0 && value < OBJ_SHARED_INTEGERS && valueobj == 0) {
        incrRefCount(shared.integers[value]);
        o = shared.integers[value];
    } else {
        if (value >= LONG_MIN && value <= LONG_MAX) {
            o = createObject(OBJ_STRING, NULL);
            o->encoding = OBJ_ENCODING_INT;
            o->ptr = (void*)((long)value);
        } else {
            o = createObject(OBJ_STRING,sdsCreateInstance->sdsfromlonglong(value));
        }
    }
    return o;
}

/**
 * 从长整型创建字符串对象
 * 
 * @param value 长整型值
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createStringObjectFromLongLong(long long value)
{
    return createStringObjectFromLongLongWithOptions(value,0);
}

/**
 * 从长整型创建用于存储值的字符串对象
 * 
 * @param value 长整型值
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createStringObjectFromLongLongForValue(long long value)
{
    return createStringObjectFromLongLongWithOptions(value,1);
}

/**
 * 从长双精度浮点型创建字符串对象
 * 
 * @param value 长双精度浮点值
 * @param humanfriendly 是否使用人类友好的格式
 * @return 返回新创建的字符串对象
 */
robj *redisObjectCreate::createStringObjectFromLongDouble(long double value, int humanfriendly)
{
    char buf[MAX_LONG_DOUBLE_CHARS];
    int len = toolFuncInstance->ld2string(buf,sizeof(buf),value,humanfriendly? LD_STR_HUMAN: LD_STR_AUTO);
    return createStringObject(buf,len);
}

/**
 * 复制字符串对象
 * 
 * @param o 要复制的字符串对象
 * @return 返回新创建的字符串对象副本
 */
robj *redisObjectCreate::dupStringObject(const robj *o)
{
    robj *d;

    serverAssert(o->type == OBJ_STRING);

    switch(o->encoding) {
    case OBJ_ENCODING_RAW:
        return createRawStringObject(static_cast<const char*>(o->ptr),sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)));
    case OBJ_ENCODING_EMBSTR:
        return createEmbeddedStringObject(static_cast<const char*>(o->ptr),sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)));
    case OBJ_ENCODING_INT:
        d = createObject(OBJ_STRING, NULL);
        d->encoding = OBJ_ENCODING_INT;
        d->ptr = o->ptr;
        return d;
    default:
        serverPanic("Wrong encoding.");
        break;
    }
}

/**
 * 创建快速列表对象
 * 
 * @return 返回新创建的快速列表对象
 */
robj *redisObjectCreate::createQuicklistObject(void)
{
    quicklist *l = quicklistCreateInstance->quicklistCrt();
    robj *o = createObject(OBJ_LIST,l);
    o->encoding = OBJ_ENCODING_QUICKLIST;
    return o;
}

/**
 * 创建压缩列表对象
 * 
 * @return 返回新创建的压缩列表对象
 */
robj *redisObjectCreate::createZiplistObject(void)
{
    unsigned char *zl = ziplistCreateInstance->ziplistNew();
    robj *o = createObject(OBJ_LIST,zl);
    o->encoding = OBJ_ENCODING_ZIPLIST;
    return o;
}

/**
 * 创建集合对象
 * 
 * @return 返回新创建的集合对象
 */
robj *redisObjectCreate::createSetObject(void)
{
    dict *d = dictionaryCreateInstance->dictCreate(&setDictType,NULL);
    robj *o = createObject(OBJ_SET,d);
    o->encoding = OBJ_ENCODING_HT;
    return o;
}

/**
 * 创建整数集合对象
 * 
 * @return 返回新创建的整数集合对象
 */
robj *redisObjectCreate::createIntsetObject(void)
{
    intset *is = intsetCreateInstance->intsetNew();
    robj *o = createObject(OBJ_SET,is);
    o->encoding = OBJ_ENCODING_INTSET;
    return o;
}

/**
 * 创建哈希对象
 * 
 * @return 返回新创建的哈希对象
 */
robj *redisObjectCreate::createHashObject(void)
{
    unsigned char *zl = ziplistCreateInstance->ziplistNew();
    robj *o = createObject(OBJ_HASH, zl);
    o->encoding = OBJ_ENCODING_ZIPLIST;
    return o;
}

/**
 * 创建基于压缩列表的有序集合对象
 * 
 * @return 返回新创建的有序集合对象
 */
robj *redisObjectCreate::createZsetZiplistObject(void)
{
    unsigned char *zl = ziplistCreateInstance->ziplistNew();
    robj *o = createObject(OBJ_ZSET,zl);
    o->encoding = OBJ_ENCODING_ZIPLIST;
    return o;
}

/**
 * 创建流对象
 * 
 * @return 返回新创建的流对象
 */
robj *redisObjectCreate::createStreamObject(void)
{
    // stream *s = streamNew();
    // robj *o = createObject(OBJ_STREAM,s);
    // o->encoding = OBJ_ENCODING_STREAM;
    // return o;
}

/**
 * 创建模块对象
 * 
 * @param mt 模块类型
 * @param value 模块值
 * @return 返回新创建的模块对象
 */
robj *redisObjectCreate::createModuleObject(moduleType *mt, void *value)
{
    moduleValue *mv =static_cast<moduleValue*>(zmalloc(sizeof(*mv)));
    mv->type = mt;
    mv->value = value;
    return createObject(OBJ_MODULE,mv);
}

/**
 * 释放字符串对象
 * 
 * @param o 要释放的字符串对象
 */
void redisObjectCreate::freeStringObject(robj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW) {
        sdsCreateInstance->sdsfree(static_cast<sds>(o->ptr));
    }
}

/**
 * 释放列表对象
 * 
 * @param o 要释放的列表对象
 */
void redisObjectCreate::freeListObject(robj *o)
{
    if (o->encoding == OBJ_ENCODING_QUICKLIST) 
    {
        quicklistCreateInstance->quicklistRelease(static_cast<quicklist *>(o->ptr));
    } 
    else 
    {
        serverPanic("Unknown list encoding type");
    }
}

/**
 * 释放集合对象
 * 
 * @param o 要释放的集合对象
 */
void redisObjectCreate::freeSetObject(robj *o)
{
    switch (o->encoding) {
    case OBJ_ENCODING_HT:
        dictionaryCreateInstance->dictRelease((dict*) o->ptr);
        break;
    case OBJ_ENCODING_INTSET:
        zfree(o->ptr);
        break;
    default:
        serverPanic("Unknown set encoding type");
    }
}

/**
 * 释放有序集合对象
 * 
 * @param o 要释放的有序集合对象
 */
void redisObjectCreate::freeZsetObject(robj *o)
{
    zset *zs;
    switch (o->encoding) {
    case OBJ_ENCODING_SKIPLIST:
        zs =static_cast<zset*>(o->ptr);
        dictionaryCreateInstance->dictRelease(zs->dictl);
        zskiplistCreateInstance->zslFree(zs->zsl);
        zfree(zs);
        break;
    case OBJ_ENCODING_ZIPLIST:
        zfree(o->ptr);
        break;
    default:
        serverPanic("Unknown sorted set encoding");
    }
}

/**
 * 释放哈希对象
 * 
 * @param o 要释放的哈希对象
 */
void redisObjectCreate::freeHashObject(robj *o)
{
    switch (o->encoding) {
    case OBJ_ENCODING_HT:
        dictionaryCreateInstance->dictRelease((dict*) o->ptr);
        break;
    case OBJ_ENCODING_ZIPLIST:
        zfree(o->ptr);
        break;
    default:
        serverPanic("Unknown hash encoding type");
        break;
    }
}

/**
 * 释放模块对象
 * 
 * @param o 要释放的模块对象
 */
void redisObjectCreate::freeModuleObject(robj *o)
{
    moduleValue *mv =static_cast<moduleValue*>(o->ptr);
    mv->type->free(mv->value);
    zfree(mv);
}

/**
 * 释放流对象
 * 
 * @param o 要释放的流对象
 */
void redisObjectCreate::freeStreamObject(robj *o)
{
    //freeStream(o->ptr);
}

/**
 * 增加对象引用计数
 * 
 * @param o 要增加引用计数的对象
 */
void redisObjectCreate::incrRefCount(robj *o)
{
    if (o->refcount < OBJ_FIRST_SPECIAL_REFCOUNT) {
        o->refcount++;
    } else {
        if (o->refcount == OBJ_SHARED_REFCOUNT) {
            /* Nothing to do: this refcount is immutable. */
        } else if (o->refcount == OBJ_STATIC_REFCOUNT) {
            serverPanic("You tried to retain an object allocated in the stack");
        }
    }
}

/**
 * 减少对象引用计数（引用计数为0时释放对象）
 * 
 * @param o 要减少引用计数的对象
 */
void redisObjectCreate::decrRefCount(robj *o)
{
    if (o->refcount == 1) {
        switch(o->type) {
        case OBJ_STRING: freeStringObject(o); break;
        case OBJ_LIST: freeListObject(o); break;
        case OBJ_SET: freeSetObject(o); break;
        case OBJ_ZSET: freeZsetObject(o); break;
        case OBJ_HASH: freeHashObject(o); break;
        case OBJ_MODULE: freeModuleObject(o); break;
        // case OBJ_STREAM: freeStreamObject(o); break;
        default: serverPanic("Unknown object type"); break;
        }
        zfree(o);
    } else {
        if (o->refcount <= 0) serverPanic("decrRefCount against refcount <= 0");
        if (o->refcount != OBJ_SHARED_REFCOUNT) o->refcount--;
    }
}


/**
 * 减少对象引用计数（用于回调函数）
 * 
 * @param o 要减少引用计数的对象
 */
void redisObjectCreate::decrRefCountVoid(void *o)
{
    decrRefCount(static_cast<robj*>(o));
}

/**
 * 检查对象类型是否符合预期
 * 
 * @param c 客户端上下文
 * @param o 要检查的对象
 * @param type 期望的对象类型
 * @return 如果类型匹配返回1，否则返回0并向客户端发送错误信息
 */
int redisObjectCreate::checkType(client *c, robj *o, int type)
{
    /* A NULL is considered an empty key */
    // if (o && o->type != type) {
    //     addReplyErrorObject(c,shared.wrongtypeerr);
    //     return 1;
    // }
    return 0;
}

/**
 * 检查SDS字符串是否可以表示为长整型
 * 
 * @param s 要检查的SDS字符串
 * @param llval 用于存储转换后的长整型值的指针
 * @return 如果可以表示为长整型返回1，否则返回0
 */
int redisObjectCreate::isSdsRepresentableAsLongLong(sds s, long long *llval)
{
    return toolFuncInstance->string2ll(s,sdsCreateInstance->sdslen(s),llval) ? C_OK : C_ERR;
}

/**
 * 检查对象是否可以表示为长整型
 * 
 * @param o 要检查的对象
 * @param llval 用于存储转换后的长整型值的指针
 * @return 如果可以表示为长整型返回1，否则返回0
 */
int redisObjectCreate::isObjectRepresentableAsLongLong(robj *o, long long *llval)
{
    //serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
    if (o->encoding == OBJ_ENCODING_INT) {
        if (llval) *llval = (long) o->ptr;
        return C_OK;
    } else {
        return isSdsRepresentableAsLongLong(static_cast<sds>(o->ptr),llval);
    }
}

/**
 * 必要时修剪字符串对象（优化内存使用）
 * 
 * @param o 要修剪的字符串对象
 */
void redisObjectCreate::trimStringObjectIfNeeded(robj *o)
{
    if (o->encoding == OBJ_ENCODING_RAW &&
        sdsCreateInstance->sdsavail(static_cast<const char*>(o->ptr)) > sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr))/10)
    {
        o->ptr = sdsCreateInstance->sdsRemoveFreeSpace(static_cast<sds>(o->ptr));
    }
}

/**
 * 尝试对对象进行编码优化
 * 
 * @param o 要优化的对象
 * @return 返回优化后的对象（可能是原对象，也可能是新对象）
 */
robj *redisObjectCreate::tryObjectEncoding(robj *o)
{
    long value;
    sds s = static_cast<sds>(o->ptr);
    size_t len;

    /* Make sure this is a string object, the only type we encode
     * in this function. Other types use encoded memory efficient
     * representations but are handled by the commands implementing
     * the type. */
    //serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);

    /* We try some specialized encoding only for objects that are
     * RAW or EMBSTR encoded, in other words objects that are still
     * in represented by an actually array of chars. */
    if (!sdsEncodedObject(o)) return o;

    /* It's not safe to encode shared objects: shared objects can be shared
     * everywhere in the "object space" of Redis and may end in places where
     * they are not handled. We handle them only as values in the keyspace. */
     if (o->refcount > 1) return o;

    /* Check if we can represent this string as a long integer.
     * Note that we are sure that a string larger than 20 chars is not
     * representable as a 32 nor 64 bit integer. */
    len = sdsCreateInstance->sdslen(s);
    if (len <= 20 && toolFuncInstance->string2l(s,len,&value)) {
        /* This object is encodable as a long. Try to use a shared object.
         * Note that we avoid using shared integers when maxmemory is used
         * because every object needs to have a private LRU field for the LRU
         * algorithm to work well. */
        //delete by zhenjia.zhao
         if (/*(server.maxmemory == 0 ||*/
            /* !(server.maxmemory_policy & MAXMEMORY_FLAG_NO_SHARED_INTEGERS)) &&*/
            value >= 0 &&
            value < OBJ_SHARED_INTEGERS)
        {
            decrRefCount(o);
            incrRefCount(shared.integers[value]);
            return shared.integers[value];
        } else {
            if (o->encoding == OBJ_ENCODING_RAW) {
                sdsCreateInstance->sdsfree(static_cast<sds>(o->ptr));
                o->encoding = OBJ_ENCODING_INT;
                o->ptr = (void*) value;
                return o;
            } else if (o->encoding == OBJ_ENCODING_EMBSTR) {
                decrRefCount(o);
                return createStringObjectFromLongLongForValue(value);
            }
        }
    }

    /* If the string is small and is still RAW encoded,
     * try the EMBSTR encoding which is more efficient.
     * In this representation the object and the SDS string are allocated
     * in the same chunk of memory to save space and cache misses. */
    if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT) {
        robj *emb;

        if (o->encoding == OBJ_ENCODING_EMBSTR) return o;
        emb = createEmbeddedStringObject(s,sdsCreateInstance->sdslen(s));
        decrRefCount(o);
        return emb;
    }

    /* We can't encode the object...
     *
     * Do the last try, and at least optimize the SDS string inside
     * the string object to require little space, in case there
     * is more than 10% of free space at the end of the SDS string.
     *
     * We do that only for relatively large strings as this branch
     * is only entered if the length of the string is greater than
     * OBJ_ENCODING_EMBSTR_SIZE_LIMIT. */
    trimStringObjectIfNeeded(o);

    /* Return the original object. */
    return o;
}

/**
 * 获取对象的解码形式（如果对象被编码）
 * 
 * @param o 要解码的对象
 * @return 返回解码后的对象（需要调用者释放）
 */
robj *redisObjectCreate::getDecodedObject(robj *o)
{
    robj *dec;

    if (sdsEncodedObject(o)) {
        incrRefCount(o);
        return o;
    }
    if (o->type == OBJ_STRING && o->encoding == OBJ_ENCODING_INT) {
        char buf[32];

        toolFuncInstance->ll2string(buf,32,(long)o->ptr);
        dec = createStringObject(buf,strlen(buf));
        return dec;
    } else {
        serverPanic("Unknown encoding type");
    }
}

/**
 * 比较两个字符串对象（带标志位）
 * 
 * @param a 第一个字符串对象
 * @param b 第二个字符串对象
 * @param flags 比较标志位
 * @return 如果相同返回1，否则返回0
 */
int redisObjectCreate::compareStringObjectsWithFlags(robj *a, robj *b, int flags)
{
    serverAssertWithInfo(NULL,a,a->type == OBJ_STRING && b->type == OBJ_STRING);
    char bufa[128], bufb[128], *astr, *bstr;
    size_t alen, blen, minlen;

    if (a == b) return 0;
    if (sdsEncodedObject(a)) {
        astr =static_cast<char*>(a->ptr);
        alen = sdsCreateInstance->sdslen(astr);
    } else {
        alen = toolFuncInstance->ll2string(bufa,sizeof(bufa),(long) a->ptr);
        astr = bufa;
    }
    if (sdsEncodedObject(b)) {
        bstr =static_cast<char*>(b->ptr);
        blen = sdsCreateInstance->sdslen(bstr);
    } else {
        blen = toolFuncInstance->ll2string(bufb,sizeof(bufb),(long) b->ptr);
        bstr = bufb;
    }
    if (flags & REDIS_COMPARE_COLL) {
        return strcoll(astr,bstr);
    } else {
        int cmp;

        minlen = (alen < blen) ? alen : blen;
        cmp = memcmp(astr,bstr,minlen);
        if (cmp == 0) return alen-blen;
        return cmp;
    }
}

/**
 * 比较两个字符串对象
 * 
 * @param a 第一个字符串对象
 * @param b 第二个字符串对象
 * @return 如果相同返回1，否则返回0
 */
int redisObjectCreate::compareStringObjects(robj *a, robj *b)
{
    return compareStringObjectsWithFlags(a,b,REDIS_COMPARE_BINARY);
}

/**
 * 按语言环境比较两个字符串对象
 * 
 * @param a 第一个字符串对象
 * @param b 第二个字符串对象
 * @return 如果a小于b返回负值，如果相等返回0，如果a大于b返回正值
 */
int redisObjectCreate::collateStringObjects(robj *a, robj *b)
{
    return compareStringObjectsWithFlags(a,b,REDIS_COMPARE_COLL);
}

/**
 * 判断两个字符串对象是否相等
 * 
 * @param a 第一个字符串对象
 * @param b 第二个字符串对象
 * @return 如果相等返回1，否则返回0
 */
int redisObjectCreate::equalStringObjects(robj *a, robj *b)
{
    if (a->encoding == OBJ_ENCODING_INT &&
        b->encoding == OBJ_ENCODING_INT){
        /* If both strings are integer encoded just check if the stored
         * long is the same. */
        return a->ptr == b->ptr;
    } else {
        return compareStringObjects(a,b) == 0;
    }
}

/**
 * 获取字符串对象的长度
 * 
 * @param o 字符串对象
 * @return 返回字符串的长度
 */
size_t redisObjectCreate::stringObjectLen(robj *o)
{
    //serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
    if (sdsEncodedObject(o)) {
        return sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr));
    } else {
        return toolFuncInstance->sdigits10((long)o->ptr);
    }
}

/**
 * 从对象中获取双精度浮点值
 * 
 * @param o 对象指针
 * @param target 用于存储结果的双精度浮点指针
 * @return 成功时返回C_OK，失败时返回C_ERR
 */
int redisObjectCreate::getDoubleFromObject(const robj *o, double *target)
{
    double value;

    if (o == NULL) {
        value = 0;
    } else {
        serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
        if (sdsEncodedObject(o)) {
            if (!toolFuncInstance->string2d(static_cast<const char*>(o->ptr), sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)), &value))
                return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    *target = value;
    return C_OK;
}

/**
 * 从对象中获取双精度浮点值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的双精度浮点指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getDoubleFromObjectOrReply(client *c, robj *o, double *target, const char *msg)
{
    // double value;
    // if (getDoubleFromObject(o, &value) != C_OK) {
    //     if (msg != NULL) {
    //         addReplyError(c,(char*)msg);
    //     } else {
    //         addReplyError(c,"value is not a valid float");
    //     }
    //     return C_ERR;
    // }
    // *target = value;
    // return C_OK;
}

/**
 * 从对象中获取长双精度浮点值
 * 
 * @param o 对象指针
 * @param target 用于存储结果的长双精度浮点指针
 * @return 成功时返回C_OK，失败时返回C_ERR
 */
int redisObjectCreate::getLongDoubleFromObject(robj *o, long double *target)
{
    long double value;

    if (o == NULL) {
        value = 0;
    } else {
        //serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
        if (sdsEncodedObject(o)) {
            if (!toolFuncInstance->string2ld(static_cast<const char*>(o->ptr), sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)), &value))
                return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    *target = value;
    return C_OK;
}

/**
 * 从对象中获取长双精度浮点值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的长双精度浮点指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getLongDoubleFromObjectOrReply(client *c, robj *o, long double *target, const char *msg)
{
    // long double value;
    // if (getLongDoubleFromObject(o, &value) != C_OK) {
    //     if (msg != NULL) {
    //         addReplyError(c,(char*)msg);
    //     } else {
    //         addReplyError(c,"value is not a valid float");
    //     }
    //     return C_ERR;
    // }
    // *target = value;
    // return C_OK;
}

/**
 * 从对象中获取长整型值
 * 
 * @param o 对象指针
 * @param target 用于存储结果的长整型指针
 * @return 成功时返回C_OK，失败时返回C_ERR
 */
int redisObjectCreate::getLongLongFromObject(robj *o, long long *target)
{
    long long value;

    if (o == NULL) {
        value = 0;
    } else {
        serverAssertWithInfo(NULL,o,o->type == OBJ_STRING);
        if (sdsEncodedObject(o)) {
            if (toolFuncInstance->string2ll(static_cast<const char*>(o->ptr),sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)),&value) == 0) return C_ERR;
        } else if (o->encoding == OBJ_ENCODING_INT) {
            value = (long)o->ptr;
        } else {
            serverPanic("Unknown string encoding");
        }
    }
    if (target) *target = value;
    return C_OK;
}

/**
 * 从对象中获取长整型值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的长整型指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getLongLongFromObjectOrReply(client *c, robj *o, long long *target, const char *msg)
{
    // long long value;
    // if (getLongLongFromObject(o, &value) != C_OK) {
    //     if (msg != NULL) {
    //         addReplyError(c,(char*)msg);
    //     } else {
    //         addReplyError(c,"value is not an integer or out of range");
    //     }
    //     return C_ERR;
    // }
    // *target = value;
    // return C_OK;
}

/**
 * 从对象中获取长整型值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的长整型指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getLongFromObjectOrReply(client *c, robj *o, long *target, const char *msg)
{
    // long long value;

    // if (getLongLongFromObjectOrReply(c, o, &value, msg) != C_OK) return C_ERR;
    // if (value < LONG_MIN || value > LONG_MAX) {
    //     if (msg != NULL) {
    //         addReplyError(c,(char*)msg);
    //     } else {
    //         addReplyError(c,"value is out of range");
    //     }
    //     return C_ERR;
    // }
    // *target = value;
    // return C_OK;
}

/**
 * 从对象中获取指定范围内的长整型值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param min 最小值
 * @param max 最大值
 * @param target 用于存储结果的长整型指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getRangeLongFromObjectOrReply(client *c, robj *o, long min, long max, long *target, const char *msg)
{
    // if (getLongFromObjectOrReply(c, o, target, msg) != C_OK) return C_ERR;
    // if (*target < min || *target > max) {
    //     if (msg != NULL) {
    //         addReplyError(c,(char*)msg);
    //     } else {
    //         addReplyErrorFormat(c,"value is out of range, value must between %ld and %ld", min, max);
    //     }
    //     return C_ERR;
    // }
    // return C_OK;
}

/**
 * 从对象中获取正长整型值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的长整型指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getPositiveLongFromObjectOrReply(client *c, robj *o, long *target, const char *msg)
{
    // if (msg) {
    //     return getRangeLongFromObjectOrReply(c, o, 0, LONG_MAX, target, msg);
    // } else {
    //     return getRangeLongFromObjectOrReply(c, o, 0, LONG_MAX, target, "value is out of range, must be positive");
    // }
}

/**
 * 从对象中获取整型值，失败时向客户端发送错误信息
 * 
 * @param c 客户端上下文
 * @param o 对象指针
 * @param target 用于存储结果的整型指针
 * @param msg 错误消息
 * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
 */
int redisObjectCreate::getIntFromObjectOrReply(client *c, robj *o, int *target, const char *msg)
{
    // long value;

    // if (getRangeLongFromObjectOrReply(c, o, INT_MIN, INT_MAX, &value, msg) != C_OK)
    //     return C_ERR;

    // *target = value;
    // return C_OK;
}

/**
 * 获取编码类型的字符串表示
 * 
 * @param encoding 编码类型
 * @return 返回编码类型的字符串表示
 */
char *redisObjectCreate::strEncoding(int encoding)
{
    switch(encoding) {
    case OBJ_ENCODING_RAW: return "raw";
    case OBJ_ENCODING_INT: return "int";
    case OBJ_ENCODING_HT: return "hashtable";
    case OBJ_ENCODING_QUICKLIST: return "quicklist";
    case OBJ_ENCODING_ZIPLIST: return "ziplist";
    case OBJ_ENCODING_INTSET: return "intset";
    case OBJ_ENCODING_SKIPLIST: return "skiplist";
    case OBJ_ENCODING_EMBSTR: return "embstr";
    case OBJ_ENCODING_STREAM: return "stream";
    default: return "unknown";
    }
}

/**
 * 计算流基数树的内存使用量
 * 
 * @param rax 基数树指针
 * @return 返回内存使用量（字节）
 */
size_t redisObjectCreate::streamRadixTreeMemoryUsage(rax *rax)
{
    // size_t size;
    // size = rax->numele * sizeof(streamID);
    // size += rax->numnodes * sizeof(raxNode);
    // /* Add a fixed overhead due to the aux data pointer, children, ... */
    // size += rax->numnodes * sizeof(long)*30;
    // return size;
}

/**
 * 计算对象的内存占用大小
 * 
 * @param o 对象指针
 * @param sample_size 样本大小
 * @return 返回对象占用的内存大小（字节）
 */
size_t redisObjectCreate::objectComputeSize(robj *o, size_t sample_size)
{
    sds ele, ele2;
    dict *d;
    dictIterator *di;
    struct dictEntry *de;
    size_t asize = 0, elesize = 0, samples = 0;

    if (o->type == OBJ_STRING) {
        if(o->encoding == OBJ_ENCODING_INT) {
            asize = sizeof(*o);
        } else if(o->encoding == OBJ_ENCODING_RAW) {
            //asize = sdsZmallocSize(o->ptr)+sizeof(*o);
        } else if(o->encoding == OBJ_ENCODING_EMBSTR) {
            asize = sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr))+2+sizeof(*o);
        } else {
            serverPanic("Unknown string encoding");
        }
    } else if (o->type == OBJ_LIST) {
        if (o->encoding == OBJ_ENCODING_QUICKLIST) {
            quicklist *ql =static_cast<quicklist*>(o->ptr);
            quicklistNode *node = ql->head;
            asize = sizeof(*o)+sizeof(quicklist);
            do {
                elesize += sizeof(quicklistNode)+ziplistCreateInstance->ziplistBlobLen(node->zl);
                samples++;
            } while ((node = node->next) && samples < sample_size);
            asize += (double)elesize/samples*ql->len;
        } else if (o->encoding == OBJ_ENCODING_ZIPLIST) {
            asize = sizeof(*o)+ziplistCreateInstance->ziplistBlobLen(static_cast<unsigned char*>(o->ptr));
        } else {
            serverPanic("Unknown list encoding");
         }
    } else if (o->type == OBJ_SET) {
        if (o->encoding == OBJ_ENCODING_HT) {
            d =static_cast<dict*>(o->ptr);
            di = dictionaryCreateInstance->dictGetIterator(d);
            asize = sizeof(*o)+sizeof(dict)+(sizeof(struct dictEntry*)*dictSlots(d));
            while((de = dictionaryCreateInstance->dictNext(di)) != NULL && samples < sample_size) {
                ele = static_cast<sds>(dictGetKey(de));
                //elesize += sizeof(struct dictEntry) + sdsZmallocSize(ele);
                samples++;
            }
            dictionaryCreateInstance->dictReleaseIterator(di);
            if (samples) asize += (double)elesize/samples*dictSize(d);
        } else if (o->encoding == OBJ_ENCODING_INTSET) {
            intset *is =static_cast<intset*>(o->ptr);
            asize = sizeof(*o)+sizeof(*is)+(size_t)is->encoding*is->length;
        } else {
            serverPanic("Unknown set encoding");
        }
    } else if (o->type == OBJ_ZSET) {
        if (o->encoding == OBJ_ENCODING_ZIPLIST) {
            asize = sizeof(*o)+(ziplistCreateInstance->ziplistBlobLen(static_cast<unsigned char*>(o->ptr)));
        } else if (o->encoding == OBJ_ENCODING_SKIPLIST) {
            d = ((zset*)o->ptr)->dictl;
            zskiplist *zsl = ((zset*)o->ptr)->zsl;
            zskiplistNode *znode = zsl->header->level[0].forward;
            asize = sizeof(*o)+sizeof(zset)+sizeof(zskiplist)+sizeof(dict)+
                    (sizeof(struct dictEntry*)*dictSlots(d))+
                    zmalloc_size(zsl->header);
            while(znode != NULL && samples < sample_size) {
                //elesize += sdsZmallocSize(znode->ele);
                elesize += sizeof(struct dictEntry) + zmalloc_size(znode);
                samples++;
                znode = znode->level[0].forward;
            }
            if (samples) asize += (double)elesize/samples*dictSize(d);
        } else {
            serverPanic("Unknown sorted set encoding");
        }
    } else if (o->type == OBJ_HASH) {
        if (o->encoding == OBJ_ENCODING_ZIPLIST) {
            asize = sizeof(*o)+(ziplistCreateInstance->ziplistBlobLen(static_cast<unsigned char*>(o->ptr)));
        } else if (o->encoding == OBJ_ENCODING_HT) {
            d =static_cast<dict*>(o->ptr) ;
            di = dictionaryCreateInstance->dictGetIterator(d);
            asize = sizeof(*o)+sizeof(dict)+(sizeof(struct dictEntry*)*dictSlots(d));
            while((de = dictionaryCreateInstance->dictNext(di)) != NULL && samples < sample_size) {
                ele = static_cast<sds>(dictGetKey(de));
                ele2 = static_cast<sds>(dictGetVal(de));
                //elesize += sdsZmallocSize(ele) + sdsZmallocSize(ele2);
                elesize += sizeof(struct dictEntry);
                samples++;
            }
            dictionaryCreateInstance->dictReleaseIterator(di);
            if (samples) asize += (double)elesize/samples*dictSize(d);
        } else {
            serverPanic("Unknown hash encoding");
        }
     } //else if (o->type == OBJ_STREAM) {
    //     stream *s = o->ptr;
    //     asize = sizeof(*o)+sizeof(*s);
    //     asize += streamRadixTreeMemoryUsage(s->rax);

    //     /* Now we have to add the listpacks. The last listpack is often non
    //      * complete, so we estimate the size of the first N listpacks, and
    //      * use the average to compute the size of the first N-1 listpacks, and
    //      * finally add the real size of the last node. */
    //     raxIterator ri;
    //     raxStart(&ri,s->rax);
    //     raxSeek(&ri,"^",NULL,0);
    //     size_t lpsize = 0, samples = 0;
    //     while(samples < sample_size && raxNext(&ri)) {
    //         unsigned char *lp = ri.data;
    //         lpsize += lpBytes(lp);
    //         samples++;
    //     }
    //     if (s->rax->numele <= samples) {
    //         asize += lpsize;
    //     } else {
    //         if (samples) lpsize /= samples; /* Compute the average. */
    //         asize += lpsize * (s->rax->numele-1);
    //         /* No need to check if seek succeeded, we enter this branch only
    //          * if there are a few elements in the radix tree. */
    //         raxSeek(&ri,"$",NULL,0);
    //         raxNext(&ri);
    //         asize += lpBytes(ri.data);
    //     }
    //     raxStop(&ri);

    //     /* Consumer groups also have a non trivial memory overhead if there
    //      * are many consumers and many groups, let's count at least the
    //      * overhead of the pending entries in the groups and consumers
    //      * PELs. */
    //     if (s->cgroups) {
    //         raxStart(&ri,s->cgroups);
    //         raxSeek(&ri,"^",NULL,0);
    //         while(raxNext(&ri)) {
    //             streamCG *cg = ri.data;
    //             asize += sizeof(*cg);
    //             asize += streamRadixTreeMemoryUsage(cg->pel);
    //             asize += sizeof(streamNACK)*raxSize(cg->pel);

    //             /* For each consumer we also need to add the basic data
    //              * structures and the PEL memory usage. */
    //             raxIterator cri;
    //             raxStart(&cri,cg->consumers);
    //             raxSeek(&cri,"^",NULL,0);
    //             while(raxNext(&cri)) {
    //                 streamConsumer *consumer = cri.data;
    //                 asize += sizeof(*consumer);
    //                 asize += sdslen(consumer->name);
    //                 asize += streamRadixTreeMemoryUsage(consumer->pel);
    //                 /* Don't count NACKs again, they are shared with the
    //                  * consumer group PEL. */
    //             }
    //             raxStop(&cri);
    //         }
    //         raxStop(&ri);
    //     }
    // } else if (o->type == OBJ_MODULE) {
    //     moduleValue *mv = o->ptr;
    //     moduleType *mt = mv->type;
    //     if (mt->mem_usage != NULL) {
    //         asize = mt->mem_usage(mv->value);
    //     } else {
    //         asize = 0;
    //     }
    // } else {
    //     serverPanic("Unknown object type");
    // }
    return asize;
}

/**
 * 释放内存开销数据结构
 * 
 * @param mh 内存开销数据结构指针
 */
void redisObjectCreate::freeMemoryOverheadData(struct redisMemOverhead *mh)
{
    zfree(mh->db);
    zfree(mh);
}

/**
 * 获取内存开销数据结构
 * 
 * @return 返回内存开销数据结构指针
 */
struct redisMemOverhead *redisObjectCreate::getMemoryOverheadData(void)
{
    int j;
    size_t mem_total = 0;
    size_t mem = 0;
    size_t zmalloc_used = zmalloc_used_memory();
    struct redisMemOverhead *mh = static_cast<struct redisMemOverhead *>(zcalloc(sizeof(*mh)));

    mh->total_allocated = zmalloc_used;
    //mh->startup_allocated = server.initial_memory_usage;
    //mh->peak_allocated = server.stat_peak_memory;
    // mh->total_frag =
    //     (float)server.cron_malloc_stats.process_rss / server.cron_malloc_stats.zmalloc_used;
    // mh->total_frag_bytes =
    //     server.cron_malloc_stats.process_rss - server.cron_malloc_stats.zmalloc_used;
    // mh->allocator_frag =
    //     (float)server.cron_malloc_stats.allocator_active / server.cron_malloc_stats.allocator_allocated;
    // mh->allocator_frag_bytes =
    //     server.cron_malloc_stats.allocator_active - server.cron_malloc_stats.allocator_allocated;
    // mh->allocator_rss =
    //     (float)server.cron_malloc_stats.allocator_resident / server.cron_malloc_stats.allocator_active;
    // mh->allocator_rss_bytes =
    //     server.cron_malloc_stats.allocator_resident - server.cron_malloc_stats.allocator_active;
    // mh->rss_extra =
    //     (float)server.cron_malloc_stats.process_rss / server.cron_malloc_stats.allocator_resident;
    // mh->rss_extra_bytes =
    //     server.cron_malloc_stats.process_rss - server.cron_malloc_stats.allocator_resident;

    // mem_total += server.initial_memory_usage;

    mem = 0;
    // if (server.repl_backlog)
    //     mem += zmalloc_size(server.repl_backlog);
    mh->repl_backlog = mem;
    mem_total += mem;

    /* Computing the memory used by the clients would be O(N) if done
     * here online. We use our values computed incrementally by
     * clientsCronTrackClientsMemUsage(). */
    // mh->clients_slaves = server.stat_clients_type_memory[CLIENT_TYPE_SLAVE];
    // mh->clients_normal = server.stat_clients_type_memory[CLIENT_TYPE_MASTER]+
    //                      server.stat_clients_type_memory[CLIENT_TYPE_PUBSUB]+
    //                      server.stat_clients_type_memory[CLIENT_TYPE_NORMAL];
    mem_total += mh->clients_slaves;
    mem_total += mh->clients_normal;

    mem = 0;
    // if (server.aof_state != AOF_OFF) {
    //     mem += sdsZmallocSize(server.aof_buf);
    //     mem += aofRewriteBufferSize();
    // }
    mh->aof_buffer = mem;
    mem_total+=mem;

    //mem = server.lua_scripts_mem;
    // mem += dictSize(server.lua_scripts) * sizeof(dictEntry) +
    //     dictSlots(server.lua_scripts) * sizeof(dictEntry*);
    // mem += dictSize(server.repl_scriptcache_dict) * sizeof(dictEntry) +
    //     dictSlots(server.repl_scriptcache_dict) * sizeof(dictEntry*);
    // if (listLength(server.repl_scriptcache_fifo) > 0) {
    //     mem += listLength(server.repl_scriptcache_fifo) * (sizeof(listNode) +
    //         sdsZmallocSize(listNodeValue(listFirst(server.repl_scriptcache_fifo))));
    // }
    mh->lua_caches = mem;
    mem_total+=mem;

    // for (j = 0; j < server.dbnum; j++) {
    //     redisDb *db = server.db+j;
    //     long long keyscount = dictSize(db->dict);
    //     if (keyscount==0) continue;

    //     mh->total_keys += keyscount;
    //     mh->db = zrealloc(mh->db,sizeof(mh->db[0])*(mh->num_dbs+1));
    //     mh->db[mh->num_dbs].dbid = j;

    //     mem = dictSize(db->dict) * sizeof(dictEntry) +
    //           dictSlots(db->dict) * sizeof(dictEntry*) +
    //           dictSize(db->dict) * sizeof(robj);
    //     mh->db[mh->num_dbs].overhead_ht_main = mem;
    //     mem_total+=mem;

    //     mem = dictSize(db->expires) * sizeof(dictEntry) +
    //           dictSlots(db->expires) * sizeof(dictEntry*);
    //     mh->db[mh->num_dbs].overhead_ht_expires = mem;
    //     mem_total+=mem;

    //     mh->num_dbs++;
    // }

    mh->overhead_total = mem_total;
    mh->dataset = zmalloc_used - mem_total;
    mh->peak_perc = (float)zmalloc_used*100/mh->peak_allocated;

    /* Metrics computed after subtracting the startup memory from
     * the total memory. */
    size_t net_usage = 1;
    if (zmalloc_used > mh->startup_allocated)
        net_usage = zmalloc_used - mh->startup_allocated;
    mh->dataset_perc = (float)mh->dataset*100/net_usage;
    mh->bytes_per_key = mh->total_keys ? (net_usage / mh->total_keys) : 0;

    return mh;
}

/**
 * 将SDS字符串追加到结果缓冲区
 * 
 * @param result 结果缓冲区指针
 * @param str 要追加的字符串
 */
void redisObjectCreate::inputCatSds(void *result, const char *str)
{
    /* result is actually a (sds *), so re-cast it here */
    sds *info = (sds *)result;
    *info = sdsCreateInstance->sdscat(*info, str);
}

/**
 * 获取内存诊断报告
 * 
 * @return 返回内存诊断报告的SDS字符串
 */
sds redisObjectCreate::getMemoryDoctorReport(void)
{
    int empty = 0;          /* Instance is empty or almost empty. */
    int big_peak = 0;       /* Memory peak is much larger than used mem. */
    int high_frag = 0;      /* High fragmentation. */
    int high_alloc_frag = 0;/* High allocator fragmentation. */
    int high_proc_rss = 0;  /* High process rss overhead. */
    int high_alloc_rss = 0; /* High rss overhead. */
    int big_slave_buf = 0;  /* Slave buffers are too big. */
    int big_client_buf = 0; /* Client buffers are too big. */
    int many_scripts = 0;   /* Script cache has too many scripts. */
    int num_reports = 0;
    struct redisMemOverhead *mh = getMemoryOverheadData();

    if (mh->total_allocated < (1024*1024*5)) {
        empty = 1;
        num_reports++;
    } else {
        /* Peak is > 150% of current used memory? */
        if (((float)mh->peak_allocated / mh->total_allocated) > 1.5) {
            big_peak = 1;
            num_reports++;
        }

        /* Fragmentation is higher than 1.4 and 10MB ?*/
        if (mh->total_frag > 1.4 && mh->total_frag_bytes > 10<<20) {
            high_frag = 1;
            num_reports++;
        }

        /* External fragmentation is higher than 1.1 and 10MB? */
        if (mh->allocator_frag > 1.1 && mh->allocator_frag_bytes > 10<<20) {
            high_alloc_frag = 1;
            num_reports++;
        }

        /* Allocator rss is higher than 1.1 and 10MB ? */
        if (mh->allocator_rss > 1.1 && mh->allocator_rss_bytes > 10<<20) {
            high_alloc_rss = 1;
            num_reports++;
        }

        /* Non-Allocator rss is higher than 1.1 and 10MB ? */
        if (mh->rss_extra > 1.1 && mh->rss_extra_bytes > 10<<20) {
            high_proc_rss = 1;
            num_reports++;
        }

        /* Clients using more than 200k each average? */
        // long numslaves = listLength(server.slaves);
        // long numclients = listLength(server.clients)-numslaves;
        // if (mh->clients_normal / numclients > (1024*200)) {
        //     big_client_buf = 1;
        //     num_reports++;
        // }

        // /* Slaves using more than 10 MB each? */
        // if (numslaves > 0 && mh->clients_slaves / numslaves > (1024*1024*10)) {
        //     big_slave_buf = 1;
        //     num_reports++;
        // }

        // /* Too many scripts are cached? */
        // if (dictSize(server.lua_scripts) > 1000) {
        //     many_scripts = 1;
        //     num_reports++;
        // }
    }

    sds s;
    if (num_reports == 0) {
        s = sdsCreateInstance->sdsnew(
        "Hi Sam, I can't find any memory issue in your instance. "
        "I can only account for what occurs on this base.\n");
    } else if (empty == 1) {
        s = sdsCreateInstance->sdsnew(
        "Hi Sam, this instance is empty or is using very little memory, "
        "my issues detector can't be used in these conditions. "
        "Please, leave for your mission on Earth and fill it with some data. "
        "The new Sam and I will be back to our programming as soon as I "
        "finished rebooting.\n");
    } else {
        s = sdsCreateInstance->sdsnew("Sam, I detected a few issues in this Redis instance memory implants:\n\n");
        if (big_peak) {
            s = sdsCreateInstance->sdscat(s," * Peak memory: In the past this instance used more than 150% the memory that is currently using. The allocator is normally not able to release memory after a peak, so you can expect to see a big fragmentation ratio, however this is actually harmless and is only due to the memory peak, and if the Redis instance Resident Set Size (RSS) is currently bigger than expected, the memory will be used as soon as you fill the Redis instance with more data. If the memory peak was only occasional and you want to try to reclaim memory, please try the MEMORY PURGE command, otherwise the only other option is to shutdown and restart the instance.\n\n");
        }
        if (high_frag) {
            s = sdsCreateInstance->sdscatprintf(s," * High total RSS: This instance has a memory fragmentation and RSS overhead greater than 1.4 (this means that the Resident Set Size of the Redis process is much larger than the sum of the logical allocations Redis performed). This problem is usually due either to a large peak memory (check if there is a peak memory entry above in the report) or may result from a workload that causes the allocator to fragment memory a lot. If the problem is a large peak memory, then there is no issue. Otherwise, make sure you are using the Jemalloc allocator and not the default libc malloc. Note: The currently used allocator is \"%s\".\n\n", ZMALLOC_LIB);
        }
        if (high_alloc_frag) {
            s = sdsCreateInstance->sdscatprintf(s," * High allocator fragmentation: This instance has an allocator external fragmentation greater than 1.1. This problem is usually due either to a large peak memory (check if there is a peak memory entry above in the report) or may result from a workload that causes the allocator to fragment memory a lot. You can try enabling 'activedefrag' config option.\n\n");
        }
        if (high_alloc_rss) {
            s = sdsCreateInstance->sdscatprintf(s," * High allocator RSS overhead: This instance has an RSS memory overhead is greater than 1.1 (this means that the Resident Set Size of the allocator is much larger than the sum what the allocator actually holds). This problem is usually due to a large peak memory (check if there is a peak memory entry above in the report), you can try the MEMORY PURGE command to reclaim it.\n\n");
        }
        if (high_proc_rss) {
            s = sdsCreateInstance->sdscatprintf(s," * High process RSS overhead: This instance has non-allocator RSS memory overhead is greater than 1.1 (this means that the Resident Set Size of the Redis process is much larger than the RSS the allocator holds). This problem may be due to Lua scripts or Modules.\n\n");
        }
        if (big_slave_buf) {
            s = sdsCreateInstance->sdscat(s," * Big replica buffers: The replica output buffers in this instance are greater than 10MB for each replica (on average). This likely means that there is some replica instance that is struggling receiving data, either because it is too slow or because of networking issues. As a result, data piles on the master output buffers. Please try to identify what replica is not receiving data correctly and why. You can use the INFO output in order to check the replicas delays and the CLIENT LIST command to check the output buffers of each replica.\n\n");
        }
        if (big_client_buf) {
            s = sdsCreateInstance->sdscat(s," * Big client buffers: The clients output buffers in this instance are greater than 200K per client (on average). This may result from different causes, like Pub/Sub clients subscribed to channels bot not receiving data fast enough, so that data piles on the Redis instance output buffer, or clients sending commands with large replies or very large sequences of commands in the same pipeline. Please use the CLIENT LIST command in order to investigate the issue if it causes problems in your instance, or to understand better why certain clients are using a big amount of memory.\n\n");
        }
        if (many_scripts) {
            s = sdsCreateInstance->sdscat(s," * Many scripts: There seem to be many cached scripts in this instance (more than 1000). This may be because scripts are generated and `EVAL`ed, instead of being parameterized (with KEYS and ARGV), `SCRIPT LOAD`ed and `EVALSHA`ed. Unless `SCRIPT FLUSH` is called periodically, the scripts' caches may end up consuming most of your memory.\n\n");
        }
        s = sdsCreateInstance->sdscat(s,"I'm here to keep you safe, Sam. I want to help you.\n");
    }
    freeMemoryOverheadData(mh);
    return s;
}

/**
 * 设置对象的LRU或LFU信息
 * 
 * @param val 对象指针
 * @param lfu_freq LFU频率值
 * @param lru_idle LRU空闲时间
 * @param lru_clock 当前LRU时钟
 * @param lru_multiplier LRU乘数
 * @return 设置成功返回1，失败返回0
 */
int redisObjectCreate::objectSetLRUOrLFU(robj *val, long long lfu_freq, long long lru_idle, long long lru_clock, int lru_multiplier)
{
    // if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
    //     if (lfu_freq >= 0) {
    //         serverAssert(lfu_freq <= 255);
    //         val->lru = (LFUGetTimeInMinutes()<<8) | lfu_freq;
    //         return 1;
    //     }
    // } else if (lru_idle >= 0) {
    //     /* Provided LRU idle time is in seconds. Scale
    //      * according to the LRU clock resolution this Redis
    //      * instance was compiled with (normally 1000 ms, so the
    //      * below statement will expand to lru_idle*1000/1000. */
    //     lru_idle = lru_idle*lru_multiplier/LRU_CLOCK_RESOLUTION;
    //     long lru_abs = lru_clock - lru_idle; /* Absolute access time. */
    //     /* If the LRU field underflows (since LRU it is a wrapping
    //      * clock), the best we can do is to provide a large enough LRU
    //      * that is half-way in the circlular LRU clock we use: this way
    //      * the computed idle time for this object will stay high for quite
    //      * some time. */
    //     if (lru_abs < 0)
    //         lru_abs = (lru_clock+(LRU_CLOCK_MAX/2)) % LRU_CLOCK_MAX;
    //     val->lru = lru_abs;
    //     return 1;
    // }
    // return 0;
}

/**
 * 查找并返回键对应的对象
 * 
 * @param c 客户端上下文
 * @param key 键对象
 * @return 如果找到返回对象指针，否则返回NULL
 */
robj *redisObjectCreate::objectCommandLookup(client *c, robj *key)
{
    //return lookupKeyReadWithFlags(c->db,key,LOOKUP_NOTOUCH|LOOKUP_NONOTIFY);
}

/**
 * 查找并返回键对应的对象，未找到时向客户端发送回复
 * 
 * @param c 客户端上下文
 * @param key 键对象
 * @param reply 未找到时发送的回复对象
 * @return 如果找到返回对象指针，否则返回NULL并发送回复
 */
robj *redisObjectCreate::objectCommandLookupOrReply(client *c, robj *key, robj *reply)
{
    // robj *o = objectCommandLookup(c,key);
    // if (!o) SentReplyOnKeyMiss(c, reply);
    // return o;
}

/**
 * 处理OBJECT命令
 * 
 * @param c 客户端上下文
 */
void redisObjectCreate::objectCommand(client *c)
{
//     robj *o;

//     if (c->argc == 2 && !strcasecmp(c->argv[1]->ptr,"help")) {
//         const char *help[] = {
// "ENCODING <key>",
// "    Return the kind of internal representation used in order to store the value",
// "    associated with a <key>.",
// "FREQ <key>",
// "    Return the access frequency index of the <key>. The returned integer is",
// "    proportional to the logarithm of the recent access frequency of the key.",
// "IDLETIME <key>",
// "    Return the idle time of the <key>, that is the approximated number of",
// "    seconds elapsed since the last access to the key.",
// "REFCOUNT <key>",
// "    Return the number of references of the value associated with the specified",
// "    <key>.",
// NULL
//         };
//         addReplyHelp(c, help);
//     } else if (!strcasecmp(c->argv[1]->ptr,"refcount") && c->argc == 3) {
//         if ((o = objectCommandLookupOrReply(c,c->argv[2],shared.null[c->resp]))
//                 == NULL) return;
//         addReplyLongLong(c,o->refcount);
//     } else if (!strcasecmp(c->argv[1]->ptr,"encoding") && c->argc == 3) {
//         if ((o = objectCommandLookupOrReply(c,c->argv[2],shared.null[c->resp]))
//                 == NULL) return;
//         addReplyBulkCString(c,strEncoding(o->encoding));
//     } else if (!strcasecmp(c->argv[1]->ptr,"idletime") && c->argc == 3) {
//         if ((o = objectCommandLookupOrReply(c,c->argv[2],shared.null[c->resp]))
//                 == NULL) return;
//         if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
//             addReplyError(c,"An LFU maxmemory policy is selected, idle time not tracked. Please note that when switching between policies at runtime LRU and LFU data will take some time to adjust.");
//             return;
//         }
//         addReplyLongLong(c,estimateObjectIdleTime(o)/1000);
//     } else if (!strcasecmp(c->argv[1]->ptr,"freq") && c->argc == 3) {
//         if ((o = objectCommandLookupOrReply(c,c->argv[2],shared.null[c->resp]))
//                 == NULL) return;
//         if (!(server.maxmemory_policy & MAXMEMORY_FLAG_LFU)) {
//             addReplyError(c,"An LFU maxmemory policy is not selected, access frequency not tracked. Please note that when switching between policies at runtime LRU and LFU data will take some time to adjust.");
//             return;
//         }
//         /* LFUDecrAndReturn should be called
//          * in case of the key has not been accessed for a long time,
//          * because we update the access time only
//          * when the key is read or overwritten. */
//         addReplyLongLong(c,LFUDecrAndReturn(o));
//     } else {
//         addReplySubcommandSyntaxError(c);
//     }
}

/**
 * 处理MEMORY命令
 * 
 * @param c 客户端上下文
 */
void redisObjectCreate::memoryCommand(client *c)
{
//     if (!strcasecmp(c->argv[1]->ptr,"help") && c->argc == 2) {
//         const char *help[] = {
// "DOCTOR",
// "    Return memory problems reports.",
// "MALLOC-STATS"
// "    Return internal statistics report from the memory allocator.",
// "PURGE",
// "    Attempt to purge dirty pages for reclamation by the allocator.",
// "STATS",
// "    Return information about the memory usage of the server.",
// "USAGE <key> [SAMPLES <count>]",
// "    Return memory in bytes used by <key> and its value. Nested values are",
// "    sampled up to <count> times (default: 5).",
// NULL
//         };
//         addReplyHelp(c, help);
//     } else if (!strcasecmp(c->argv[1]->ptr,"usage") && c->argc >= 3) {
//         dictEntry *de;
//         long long samples = OBJ_COMPUTE_SIZE_DEF_SAMPLES;
//         for (int j = 3; j < c->argc; j++) {
//             if (!strcasecmp(c->argv[j]->ptr,"samples") &&
//                 j+1 < c->argc)
//             {
//                 if (getLongLongFromObjectOrReply(c,c->argv[j+1],&samples,NULL)
//                      == C_ERR) return;
//                 if (samples < 0) {
//                     addReplyErrorObject(c,shared.syntaxerr);
//                     return;
//                 }
//                 if (samples == 0) samples = LLONG_MAX;
//                 j++; /* skip option argument. */
//             } else {
//                 addReplyErrorObject(c,shared.syntaxerr);
//                 return;
//             }
//         }
//         if ((de = dictFind(c->db->dict,c->argv[2]->ptr)) == NULL) {
//             addReplyNull(c);
//             return;
//         }
//         size_t usage = objectComputeSize(dictGetVal(de),samples);
//         usage += sdsZmallocSize(dictGetKey(de));
//         usage += sizeof(dictEntry);
//         addReplyLongLong(c,usage);
//     } else if (!strcasecmp(c->argv[1]->ptr,"stats") && c->argc == 2) {
//         struct redisMemOverhead *mh = getMemoryOverheadData();

//         addReplyMapLen(c,25+mh->num_dbs);

//         addReplyBulkCString(c,"peak.allocated");
//         addReplyLongLong(c,mh->peak_allocated);

//         addReplyBulkCString(c,"total.allocated");
//         addReplyLongLong(c,mh->total_allocated);

//         addReplyBulkCString(c,"startup.allocated");
//         addReplyLongLong(c,mh->startup_allocated);

//         addReplyBulkCString(c,"replication.backlog");
//         addReplyLongLong(c,mh->repl_backlog);

//         addReplyBulkCString(c,"clients.slaves");
//         addReplyLongLong(c,mh->clients_slaves);

//         addReplyBulkCString(c,"clients.normal");
//         addReplyLongLong(c,mh->clients_normal);

//         addReplyBulkCString(c,"aof.buffer");
//         addReplyLongLong(c,mh->aof_buffer);

//         addReplyBulkCString(c,"lua.caches");
//         addReplyLongLong(c,mh->lua_caches);

//         for (size_t j = 0; j < mh->num_dbs; j++) {
//             char dbname[32];
//             snprintf(dbname,sizeof(dbname),"db.%zd",mh->db[j].dbid);
//             addReplyBulkCString(c,dbname);
//             addReplyMapLen(c,2);

//             addReplyBulkCString(c,"overhead.hashtable.main");
//             addReplyLongLong(c,mh->db[j].overhead_ht_main);

//             addReplyBulkCString(c,"overhead.hashtable.expires");
//             addReplyLongLong(c,mh->db[j].overhead_ht_expires);
//         }

//         addReplyBulkCString(c,"overhead.total");
//         addReplyLongLong(c,mh->overhead_total);

//         addReplyBulkCString(c,"keys.count");
//         addReplyLongLong(c,mh->total_keys);

//         addReplyBulkCString(c,"keys.bytes-per-key");
//         addReplyLongLong(c,mh->bytes_per_key);

//         addReplyBulkCString(c,"dataset.bytes");
//         addReplyLongLong(c,mh->dataset);

//         addReplyBulkCString(c,"dataset.percentage");
//         addReplyDouble(c,mh->dataset_perc);

//         addReplyBulkCString(c,"peak.percentage");
//         addReplyDouble(c,mh->peak_perc);

//         addReplyBulkCString(c,"allocator.allocated");
//         addReplyLongLong(c,server.cron_malloc_stats.allocator_allocated);

//         addReplyBulkCString(c,"allocator.active");
//         addReplyLongLong(c,server.cron_malloc_stats.allocator_active);

//         addReplyBulkCString(c,"allocator.resident");
//         addReplyLongLong(c,server.cron_malloc_stats.allocator_resident);

//         addReplyBulkCString(c,"allocator-fragmentation.ratio");
//         addReplyDouble(c,mh->allocator_frag);

//         addReplyBulkCString(c,"allocator-fragmentation.bytes");
//         addReplyLongLong(c,mh->allocator_frag_bytes);

//         addReplyBulkCString(c,"allocator-rss.ratio");
//         addReplyDouble(c,mh->allocator_rss);

//         addReplyBulkCString(c,"allocator-rss.bytes");
//         addReplyLongLong(c,mh->allocator_rss_bytes);

//         addReplyBulkCString(c,"rss-overhead.ratio");
//         addReplyDouble(c,mh->rss_extra);

//         addReplyBulkCString(c,"rss-overhead.bytes");
//         addReplyLongLong(c,mh->rss_extra_bytes);

//         addReplyBulkCString(c,"fragmentation"); /* this is the total RSS overhead, including fragmentation */
//         addReplyDouble(c,mh->total_frag); /* it is kept here for backwards compatibility */

//         addReplyBulkCString(c,"fragmentation.bytes");
//         addReplyLongLong(c,mh->total_frag_bytes);

//         freeMemoryOverheadData(mh);
//     } else if (!strcasecmp(c->argv[1]->ptr,"malloc-stats") && c->argc == 2) {
// #if defined(USE_JEMALLOC)
//         sds info = sdsempty();
//         je_malloc_stats_print(inputCatSds, &info, NULL);
//         addReplyVerbatim(c,info,sdslen(info),"txt");
//         sdsfree(info);
// #else
//         addReplyBulkCString(c,"Stats not supported for the current allocator");
// #endif
//     } else if (!strcasecmp(c->argv[1]->ptr,"doctor") && c->argc == 2) {
//         sds report = getMemoryDoctorReport();
//         addReplyVerbatim(c,report,sdslen(report),"txt");
//         sdsfree(report);
//     } else if (!strcasecmp(c->argv[1]->ptr,"purge") && c->argc == 2) {
//         if (jemalloc_purge() == 0)
//             addReply(c, shared.ok);
//         else
//             addReplyError(c, "Error purging dirty pages");
//     } else {
//         addReplySubcommandSyntaxError(c);
//     }
}

/**
 * 计算SDS字符串的哈希值。
 * 用于字典的哈希函数回调。
 * 
 * @param key 指向SDS字符串的指针
 * @return 计算得到的哈希值
 */
uint64_t redisObjectCreate::dictSdsHash(const void *key)
{
    return dictionaryCreateInst.dictGenHashFunction((unsigned char*)key, sdsCreateInst.sdslen((char*)key));
}
/**
 * 比较两个SDS字符串键是否相等。
 * 用于字典的键比较回调函数。
 * 
 * @param privdata 私有数据（未使用）
 * @param key1 第一个键
 * @param key2 第二个键
 * @return 相等返回1，否则返回0
 */
int redisObjectCreate::dictSdsKeyCompare(void *privdata, const void *key1, const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);
    l1 = sdsCreateInst.sdslen((sds)key1);
    l2 = sdsCreateInst.sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}
/**
 * 释放SDS字符串的内存
 * @param privdata 私有数据（未使用）
 * @param val 待释放的SDS字符串指针
 */
void redisObjectCreate::dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    sdsCreateInst.sdsfree(static_cast<char*>(val));
}

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
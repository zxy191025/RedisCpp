/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: redisObject implementation
 */
#include "zskiplist.h"
#include "ziplist.h"
#include "toolFunc.h"
#include "sds.h"
#include "redisObject.h"
#include "zmallocDf.h"
#include "zset.h"
#include <string.h>
redisObjectCreate::redisObjectCreate()
{
    zskiplistCreateInstance = static_cast<zskiplistCreate *>(zmalloc(sizeof(zskiplistCreate)));
    //serverAssert(zskiplistCreateInstance != NULL);
    dictionaryCreateInstance = static_cast<dictionaryCreate *>(zmalloc(sizeof(dictionaryCreate)));
    //serverAssert(dictionaryCreateInstance != NULL);
    zsetCreateInstance = static_cast<zsetCreate *>(zmalloc(sizeof(zsetCreate)));
    //serverAssert(zsetCreateInstance != NULL);
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
    //serverAssert(sdsCreateInstance != NULL);
    toolFuncInstance = static_cast<toolFunc *>(zmalloc(sizeof(toolFunc)));

    ziplistCreateInstance = static_cast<ziplistCreate *>(zmalloc(sizeof(ziplistCreate)));
}
redisObjectCreate::~redisObjectCreate()
{
    zfree(zskiplistCreateInstance);
    zfree(dictionaryCreateInstance);
    zfree(zsetCreateInstance);
    zfree(toolFuncInstance);
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
    //serverAssert(o->refcount == 1);
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
    //delete by zhenjia.zhao
    // robj *o = zmalloc(sizeof(robj)+sizeof(struct sdshdr8)+len+1);
    // struct sdshdr8 *sh = (void*)(o+1);

    // o->type = OBJ_STRING;
    // o->encoding = OBJ_ENCODING_EMBSTR;
    // o->ptr = sh+1;
    // o->refcount = 1;
    // if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
    //     o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
    // } else {
    //     o->lru = LRU_CLOCK();
    // }

    // sh->len = len;
    // sh->alloc = len;
    // sh->flags = SDS_TYPE_8;
    // if (ptr == SDS_NOINIT)
    //     sh->buf[len] = '\0';
    // else if (ptr) {
    //     memcpy(sh->buf,ptr,len);
    //     sh->buf[len] = '\0';
    // } else {
    //     memset(sh->buf,0,len+1);
    // }
    // return o;
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
    //delete by zhenjia.zhao
//    robj *o;

//     if (server.maxmemory == 0 ||
//         !(server.maxmemory_policy & MAXMEMORY_FLAG_NO_SHARED_INTEGERS))
//     {
//         /* If the maxmemory policy permits, we can still return shared integers
//          * even if valueobj is true. */
//         valueobj = 0;
//     }

//     if (value >= 0 && value < OBJ_SHARED_INTEGERS && valueobj == 0) {
//         incrRefCount(shared.integers[value]);
//         o = shared.integers[value];
//     } else {
//         if (value >= LONG_MIN && value <= LONG_MAX) {
//             o = createObject(OBJ_STRING, NULL);
//             o->encoding = OBJ_ENCODING_INT;
//             o->ptr = (void*)((long)value);
//         } else {
//             o = createObject(OBJ_STRING,sdsCreateInstance->sdsfromlonglong(value));
//         }
//     }
//     return o;
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

    //serverAssert(o->type == OBJ_STRING);

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
        //serverPanic("Wrong encoding.");
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
    //delete by zhenjia.zhao
    // quicklist *l = quicklistCreate();
    // robj *o = createObject(OBJ_LIST,l);
    // o->encoding = OBJ_ENCODING_QUICKLIST;
    // return o;
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
    // dict *d = dictionaryCreateInstance->dictCreate(&setDictType,NULL);
    // robj *o = createObject(OBJ_SET,d);
    // o->encoding = OBJ_ENCODING_HT;
    // return o;
}

/**
 * 创建整数集合对象
 * 
 * @return 返回新创建的整数集合对象
 */
robj *redisObjectCreate::createIntsetObject(void)
{

}

/**
 * 创建哈希对象
 * 
 * @return 返回新创建的哈希对象
 */
robj *redisObjectCreate::createHashObject(void)
{

}

/**
 * 创建基于压缩列表的有序集合对象
 * 
 * @return 返回新创建的有序集合对象
 */
robj *redisObjectCreate::createZsetZiplistObject(void)
{

}

/**
 * 创建流对象
 * 
 * @return 返回新创建的流对象
 */
robj *redisObjectCreate::createStreamObject(void)
{

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

}

/**
 * 释放字符串对象
 * 
 * @param o 要释放的字符串对象
 */
void redisObjectCreate::freeStringObject(robj *o)
{

}

/**
 * 释放列表对象
 * 
 * @param o 要释放的列表对象
 */
void redisObjectCreate::freeListObject(robj *o)
{

}

/**
 * 释放集合对象
 * 
 * @param o 要释放的集合对象
 */
void redisObjectCreate::freeSetObject(robj *o)
{

}

/**
 * 释放有序集合对象
 * 
 * @param o 要释放的有序集合对象
 */
void redisObjectCreate::freeZsetObject(robj *o)
{

}

/**
 * 释放哈希对象
 * 
 * @param o 要释放的哈希对象
 */
void redisObjectCreate::freeHashObject(robj *o)
{

}

/**
 * 释放模块对象
 * 
 * @param o 要释放的模块对象
 */
void redisObjectCreate::freeModuleObject(robj *o)
{

}

/**
 * 释放流对象
 * 
 * @param o 要释放的流对象
 */
void redisObjectCreate::freeStreamObject(robj *o)
{

}

/**
 * 增加对象引用计数
 * 
 * @param o 要增加引用计数的对象
 */
void redisObjectCreate::incrRefCount(robj *o)
{

}

/**
 * 减少对象引用计数（引用计数为0时释放对象）
 * 
 * @param o 要减少引用计数的对象
 */
void redisObjectCreate::decrRefCount(robj *o)
{

}


/**
 * 减少对象引用计数（用于回调函数）
 * 
 * @param o 要减少引用计数的对象
 */
void redisObjectCreate::decrRefCountVoid(void *o)
{

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

}

/**
 * 必要时修剪字符串对象（优化内存使用）
 * 
 * @param o 要修剪的字符串对象
 */
void redisObjectCreate::trimStringObjectIfNeeded(robj *o)
{

}

/**
 * 尝试对对象进行编码优化
 * 
 * @param o 要优化的对象
 * @return 返回优化后的对象（可能是原对象，也可能是新对象）
 */
robj *redisObjectCreate::tryObjectEncoding(robj *o)
{

}

/**
 * 获取对象的解码形式（如果对象被编码）
 * 
 * @param o 要解码的对象
 * @return 返回解码后的对象（需要调用者释放）
 */
robj *redisObjectCreate::getDecodedObject(robj *o)
{

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

}

/**
 * 获取字符串对象的长度
 * 
 * @param o 字符串对象
 * @return 返回字符串的长度
 */
size_t redisObjectCreate::stringObjectLen(robj *o)
{

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

}

/**
 * 获取编码类型的字符串表示
 * 
 * @param encoding 编码类型
 * @return 返回编码类型的字符串表示
 */
char *redisObjectCreate::strEncoding(int encoding)
{

}

/**
 * 计算流基数树的内存使用量
 * 
 * @param rax 基数树指针
 * @return 返回内存使用量（字节）
 */
size_t redisObjectCreate::streamRadixTreeMemoryUsage(rax *rax)
{

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

}

/**
 * 释放内存开销数据结构
 * 
 * @param mh 内存开销数据结构指针
 */
void redisObjectCreate::freeMemoryOverheadData(struct redisMemOverhead *mh)
{

}

/**
 * 获取内存开销数据结构
 * 
 * @return 返回内存开销数据结构指针
 */
struct redisMemOverhead *redisObjectCreate::getMemoryOverheadData(void)
{

}

/**
 * 将SDS字符串追加到结果缓冲区
 * 
 * @param result 结果缓冲区指针
 * @param str 要追加的字符串
 */
void redisObjectCreate::inputCatSds(void *result, const char *str)
{

}

/**
 * 获取内存诊断报告
 * 
 * @return 返回内存诊断报告的SDS字符串
 */
sds redisObjectCreate::getMemoryDoctorReport(void)
{

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

}

/**
 * 处理OBJECT命令
 * 
 * @param c 客户端上下文
 */
void redisObjectCreate::objectCommand(client *c)
{

}

/**
 * 处理MEMORY命令
 * 
 * @param c 客户端上下文
 */
void redisObjectCreate::memoryCommand(client *c)
{

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
    sdsCreate sdsCreateInst;
    dictionaryCreate dictionaryCreateInst;
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
    sdsCreate sdsCreateInst;
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
    sdsCreate sdsCreateInstancel;
    sdsCreateInstancel.sdsfree(static_cast<char*>(val));
}
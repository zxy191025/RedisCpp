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
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
    //serverAssert(sdsCreateInstance != NULL);
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

void redisObjectCreate::createSharedObjects(void)
{
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
    //delete by zhenjia.zhao
    // for (j = 0; j < PROTO_SHARED_SELECT_CMDS; j++) {
    //     char dictid_str[64];
    //     int dictid_len;

    //     dictid_len = ll2string(dictid_str,sizeof(dictid_str),j);
    //     shared.select[j] = createObject(OBJ_STRING,
    //         sdscatprintf(sdsempty(),
    //             "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
    //             dictid_len, dictid_str));
    // }
    // shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    // shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    // shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    // shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    // shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    // shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);

    // /* Shared command names */
    // shared.del = createStringObject("DEL",3);
    // shared.unlink = createStringObject("UNLINK",6);
    // shared.rpop = createStringObject("RPOP",4);
    // shared.lpop = createStringObject("LPOP",4);
    // shared.lpush = createStringObject("LPUSH",5);
    // shared.rpoplpush = createStringObject("RPOPLPUSH",9);
    // shared.lmove = createStringObject("LMOVE",5);
    // shared.blmove = createStringObject("BLMOVE",6);
    // shared.zpopmin = createStringObject("ZPOPMIN",7);
    // shared.zpopmax = createStringObject("ZPOPMAX",7);
    // shared.multi = createStringObject("MULTI",5);
    // shared.exec = createStringObject("EXEC",4);
    // shared.hset = createStringObject("HSET",4);
    // shared.srem = createStringObject("SREM",4);
    // shared.xgroup = createStringObject("XGROUP",6);
    // shared.xclaim = createStringObject("XCLAIM",6);
    // shared.script = createStringObject("SCRIPT",6);
    // shared.replconf = createStringObject("REPLCONF",8);
    // shared.pexpireat = createStringObject("PEXPIREAT",9);
    // shared.pexpire = createStringObject("PEXPIRE",7);
    // shared.persist = createStringObject("PERSIST",7);
    // shared.set = createStringObject("SET",3);
    // shared.eval = createStringObject("EVAL",4);

    // /* Shared command argument */
    // shared.left = createStringObject("left",4);
    // shared.right = createStringObject("right",5);
    // shared.pxat = createStringObject("PXAT", 4);
    // shared.px = createStringObject("PX",2);
    // shared.time = createStringObject("TIME",4);
    // shared.retrycount = createStringObject("RETRYCOUNT",10);
    // shared.force = createStringObject("FORCE",5);
    // shared.justid = createStringObject("JUSTID",6);
    // shared.lastid = createStringObject("LASTID",6);
    // shared.default_username = createStringObject("default",7);
    // shared.ping = createStringObject("ping",4);
    // shared.setid = createStringObject("SETID",5);
    // shared.keepttl = createStringObject("KEEPTTL",7);
    // shared.load = createStringObject("LOAD",4);
    // shared.createconsumer = createStringObject("CREATECONSUMER",14);
    // shared.getack = createStringObject("GETACK",6);
    // shared.special_asterick = createStringObject("*",1);
    // shared.special_equals = createStringObject("=",1);
    // shared.redacted = makeObjectShared(createStringObject("(redacted)",10));

    // for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
    //     shared.integers[j] =
    //         makeObjectShared(createObject(OBJ_STRING,(void*)(long)j));
    //     shared.integers[j]->encoding = OBJ_ENCODING_INT;
    // }
    // for (j = 0; j < OBJ_SHARED_BULKHDR_LEN; j++) {
    //     shared.mbulkhdr[j] = createObject(OBJ_STRING,
    //         sdscatprintf(sdsempty(),"*%d\r\n",j));
    //     shared.bulkhdr[j] = createObject(OBJ_STRING,
    //         sdscatprintf(sdsempty(),"$%d\r\n",j));
    // }

    shared.minstring = sdsCreateInstance->sdsnew("minstring");
    shared.maxstring = sdsCreateInstance->sdsnew("maxstring");
}
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: redisObject implementation
 */
#ifndef REDIS_BASE_REDISOBJECT_H
#define REDIS_BASE_REDISOBJECT_H
class zsetCreate;
class zskiplistCreate;
class dictionaryCreate;
class client;
class sdsCreate;
struct redisObject;
typedef struct redisObject robj;
class rax;
struct RedisModuleType;
typedef struct RedisModuleType moduleType;
class toolFunc;
class ziplistCreate;

#define OBJ_ENCODING_RAW 0     /* Raw representation */
#define OBJ_ENCODING_INT 1     /* Encoded as integer */
#define OBJ_ENCODING_HT 2      /* Encoded as hash table */
#define OBJ_ENCODING_ZIPMAP 3  /* Encoded as zipmap */
#define OBJ_ENCODING_LINKEDLIST 4 /* No longer used: old list encoding. */
#define OBJ_ENCODING_ZIPLIST 5 /* Encoded as ziplist */
#define OBJ_ENCODING_INTSET 6  /* Encoded as intset */
#define OBJ_ENCODING_SKIPLIST 7  /* Encoded as skiplist */
#define OBJ_ENCODING_EMBSTR 8  /* Embedded sds string encoding */
#define OBJ_ENCODING_QUICKLIST 9 /* Encoded as linked list of ziplists */
#define OBJ_ENCODING_STREAM 10 /* Encoded as a radix tree of listpacks */

#define LRU_BITS 24
#define LRU_CLOCK_MAX ((1<<LRU_BITS)-1) /* Max value of obj->lru */
#define LRU_CLOCK_RESOLUTION 1000 /* LRU clock resolution in ms */

#define OBJ_SHARED_REFCOUNT INT_MAX     /* Global object never destroyed. */
#define OBJ_STATIC_REFCOUNT (INT_MAX-1) /* Object allocated in the stack. */
#define OBJ_FIRST_SPECIAL_REFCOUNT OBJ_STATIC_REFCOUNT

/* Redis maxmemory strategies. Instead of using just incremental number
 * for this defines, we use a set of flags so that testing for certain
 * properties common to multiple policies is faster. */
#define MAXMEMORY_FLAG_LRU (1<<0)
#define MAXMEMORY_FLAG_LFU (1<<1)
#define MAXMEMORY_FLAG_ALLKEYS (1<<2)
#define MAXMEMORY_FLAG_NO_SHARED_INTEGERS \
    (MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_LFU)

#define MAXMEMORY_VOLATILE_LRU ((0<<8)|MAXMEMORY_FLAG_LRU)
#define MAXMEMORY_VOLATILE_LFU ((1<<8)|MAXMEMORY_FLAG_LFU)
#define MAXMEMORY_VOLATILE_TTL (2<<8)
#define MAXMEMORY_VOLATILE_RANDOM (3<<8)
#define MAXMEMORY_ALLKEYS_LRU ((4<<8)|MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_LFU ((5<<8)|MAXMEMORY_FLAG_LFU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_RANDOM ((6<<8)|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_NO_EVICTION (7<<8)

#define PROTO_SHARED_SELECT_CMDS 10
#define OBJ_SHARED_INTEGERS 10000
#define OBJ_SHARED_BULKHDR_LEN 32

#define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
#define REDIS_COMPARE_BINARY (1<<0)
#define REDIS_COMPARE_COLL (1<<1)
#define OBJ_COMPUTE_SIZE_DEF_SAMPLES 5 /* Default sample size. */
typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS; /* LRU time (relative to global lru_clock) or
                            * LFU data (least significant 8 bits frequency
                            * and most significant 16 bits access time). */
    int refcount;
    void *ptr;
} robj;

#ifndef __DICT_H_DICTTYPE
#define __DICT_H_DICTTYPE
/**
 * 字典类型特定函数集合 - 用于自定义字典行为
 */
typedef struct dictType {
    uint64_t (*hashFunction)(const void *key);         // 哈希函数
    void *(*keyDup)(void *privdata, const void *key);  // 键复制函数
    void *(*valDup)(void *privdata, const void *obj);  // 值复制函数
    int (*keyCompare)(void *privdata, const void *key1, const void *key2); // 键比较函数
    void (*keyDestructor)(void *privdata, void *key);  // 键销毁函数
    void (*valDestructor)(void *privdata, void *obj);  // 值销毁函数
    int (*expandAllowed)(size_t moreMem, double usedRatio); // 扩容允许判断函数
} dictType;
#endif

class redisObjectCreate
{
public:
    redisObjectCreate();
    ~redisObjectCreate();

public:
    /**
     *  初始化 Redis 服务器运行时所需的共享对象池。
     */
    void createSharedObjects(void) ;

    /**
     * 创建一个Redis对象(robj)。
     * Redis使用robj结构表示所有键值对的值，支持多种数据类型。
     * 
     * @param type 对象类型(如REDIS_STRING, REDIS_LIST, REDIS_SET等)
     * @param ptr 指向对象实际数据的指针
     * @return 新创建的Redis对象
     */
    robj *createObject(int type, void *ptr);

    /**
     * 创建一个有序集合(zset)对象。
     * 有序集合同时使用哈希表和跳跃表实现，确保成员唯一性并支持按分数排序。
     * 
     * @return 新创建的有序集合对象
     */
    robj *createZsetObject(void);

public:
    /**
     * 共享对象，增加引用计数并返回原对象指针
     * 
     * @param o 要共享的对象
     * @return 返回增加引用计数后的对象指针
     */
    robj *makeObjectShared(robj *o);

    /**
     * 创建一个原始字符串对象（未经过编码优化）
     * 
     * @param ptr 字符串内容指针
     * @param len 字符串长度
     * @return 返回新创建的字符串对象
     */
    robj *createRawStringObject(const char *ptr, size_t len);

    /**
     * 创建一个嵌入式字符串对象（用于短字符串的优化表示）
     * 
     * @param ptr 字符串内容指针
     * @param len 字符串长度
     * @return 返回新创建的嵌入式字符串对象
     */
    robj *createEmbeddedStringObject(const char *ptr, size_t len);

    /**
     * 创建字符串对象（自动选择合适的编码方式）
     * 
     * @param ptr 字符串内容指针
     * @param len 字符串长度
     * @return 返回新创建的字符串对象
     */
    robj *createStringObject(const char *ptr, size_t len);

    /**
     * 尝试创建原始字符串对象（内存不足时可能返回NULL）
     * 
     * @param ptr 字符串内容指针
     * @param len 字符串长度
     * @return 成功时返回新创建的字符串对象，失败时返回NULL
     */
    robj *tryCreateRawStringObject(const char *ptr, size_t len);

    /**
     * 尝试创建字符串对象（内存不足时可能返回NULL）
     * 
     * @param ptr 字符串内容指针
     * @param len 字符串长度
     * @return 成功时返回新创建的字符串对象，失败时返回NULL
     */
    robj *tryCreateStringObject(const char *ptr, size_t len);

    /**
     * 从长整型创建字符串对象（可指定是否为值对象）
     * 
     * @param value 长整型值
     * @param valueobj 是否作为值对象创建
     * @return 返回新创建的字符串对象
     */
    robj *createStringObjectFromLongLongWithOptions(long long value, int valueobj);

    /**
     * 从长整型创建字符串对象
     * 
     * @param value 长整型值
     * @return 返回新创建的字符串对象
     */
    robj *createStringObjectFromLongLong(long long value);

    /**
     * 从长整型创建用于存储值的字符串对象
     * 
     * @param value 长整型值
     * @return 返回新创建的字符串对象
     */
    robj *createStringObjectFromLongLongForValue(long long value);

    /**
     * 从长双精度浮点型创建字符串对象
     * 
     * @param value 长双精度浮点值
     * @param humanfriendly 是否使用人类友好的格式
     * @return 返回新创建的字符串对象
     */
    robj *createStringObjectFromLongDouble(long double value, int humanfriendly);

    /**
     * 复制字符串对象
     * 
     * @param o 要复制的字符串对象
     * @return 返回新创建的字符串对象副本
     */
    robj *dupStringObject(const robj *o);

    /**
     * 创建快速列表对象
     * 
     * @return 返回新创建的快速列表对象
     */
    robj *createQuicklistObject(void);

    /**
     * 创建压缩列表对象
     * 
     * @return 返回新创建的压缩列表对象
     */
    robj *createZiplistObject(void);

    /**
     * 创建集合对象
     * 
     * @return 返回新创建的集合对象
     */
    robj *createSetObject(void);

    /**
     * 创建整数集合对象
     * 
     * @return 返回新创建的整数集合对象
     */
    robj *createIntsetObject(void);

    /**
     * 创建哈希对象
     * 
     * @return 返回新创建的哈希对象
     */
    robj *createHashObject(void);

    /**
     * 创建基于压缩列表的有序集合对象
     * 
     * @return 返回新创建的有序集合对象
     */
    robj *createZsetZiplistObject(void);

    /**
     * 创建流对象
     * 
     * @return 返回新创建的流对象
     */
    robj *createStreamObject(void);

    /**
     * 创建模块对象
     * 
     * @param mt 模块类型
     * @param value 模块值
     * @return 返回新创建的模块对象
     */
    robj *createModuleObject(moduleType *mt, void *value);

    /**
     * 释放字符串对象
     * 
     * @param o 要释放的字符串对象
     */
    void freeStringObject(robj *o);

    /**
     * 释放列表对象
     * 
     * @param o 要释放的列表对象
     */
    void freeListObject(robj *o);

    /**
     * 释放集合对象
     * 
     * @param o 要释放的集合对象
     */
    void freeSetObject(robj *o);

    /**
     * 释放有序集合对象
     * 
     * @param o 要释放的有序集合对象
     */
    void freeZsetObject(robj *o);

    /**
     * 释放哈希对象
     * 
     * @param o 要释放的哈希对象
     */
    void freeHashObject(robj *o);

    /**
     * 释放模块对象
     * 
     * @param o 要释放的模块对象
     */
    void freeModuleObject(robj *o);

    /**
     * 释放流对象
     * 
     * @param o 要释放的流对象
     */
    void freeStreamObject(robj *o);

    /**
     * 增加对象引用计数
     * 
     * @param o 要增加引用计数的对象
     */
    void incrRefCount(robj *o);

    /**
     * 减少对象引用计数（引用计数为0时释放对象）
     * 
     * @param o 要减少引用计数的对象
     */
    void decrRefCount(robj *o);

    /**
     * 减少对象引用计数（用于回调函数）
     * 
     * @param o 要减少引用计数的对象
     */
    void decrRefCountVoid(void *o);

    /**
     * 检查对象类型是否符合预期
     * 
     * @param c 客户端上下文
     * @param o 要检查的对象
     * @param type 期望的对象类型
     * @return 如果类型匹配返回1，否则返回0并向客户端发送错误信息
     */
    int checkType(client *c, robj *o, int type);

    /**
     * 检查SDS字符串是否可以表示为长整型
     * 
     * @param s 要检查的SDS字符串
     * @param llval 用于存储转换后的长整型值的指针
     * @return 如果可以表示为长整型返回1，否则返回0
     */
    int isSdsRepresentableAsLongLong(sds s, long long *llval);

    /**
     * 检查对象是否可以表示为长整型
     * 
     * @param o 要检查的对象
     * @param llval 用于存储转换后的长整型值的指针
     * @return 如果可以表示为长整型返回1，否则返回0
     */
    int isObjectRepresentableAsLongLong(robj *o, long long *llval);

    /**
     * 必要时修剪字符串对象（优化内存使用）
     * 
     * @param o 要修剪的字符串对象
     */
    void trimStringObjectIfNeeded(robj *o);

    /**
     * 尝试对对象进行编码优化
     * 
     * @param o 要优化的对象
     * @return 返回优化后的对象（可能是原对象，也可能是新对象）
     */
    robj *tryObjectEncoding(robj *o);

    /**
     * 获取对象的解码形式（如果对象被编码）
     * 
     * @param o 要解码的对象
     * @return 返回解码后的对象（需要调用者释放）
     */
    robj *getDecodedObject(robj *o);

    /**
     * 比较两个字符串对象（带标志位）
     * 
     * @param a 第一个字符串对象
     * @param b 第二个字符串对象
     * @param flags 比较标志位
     * @return 如果相同返回1，否则返回0
     */
    int compareStringObjectsWithFlags(robj *a, robj *b, int flags);

    /**
     * 比较两个字符串对象
     * 
     * @param a 第一个字符串对象
     * @param b 第二个字符串对象
     * @return 如果相同返回1，否则返回0
     */
    int compareStringObjects(robj *a, robj *b);

    /**
     * 按语言环境比较两个字符串对象
     * 
     * @param a 第一个字符串对象
     * @param b 第二个字符串对象
     * @return 如果a小于b返回负值，如果相等返回0，如果a大于b返回正值
     */
    int collateStringObjects(robj *a, robj *b);

    /**
     * 判断两个字符串对象是否相等
     * 
     * @param a 第一个字符串对象
     * @param b 第二个字符串对象
     * @return 如果相等返回1，否则返回0
     */
    int equalStringObjects(robj *a, robj *b);

    /**
     * 获取字符串对象的长度
     * 
     * @param o 字符串对象
     * @return 返回字符串的长度
     */
    size_t stringObjectLen(robj *o);

    /**
     * 从对象中获取双精度浮点值
     * 
     * @param o 对象指针
     * @param target 用于存储结果的双精度浮点指针
     * @return 成功时返回C_OK，失败时返回C_ERR
     */
    int getDoubleFromObject(const robj *o, double *target);

    /**
     * 从对象中获取双精度浮点值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的双精度浮点指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getDoubleFromObjectOrReply(client *c, robj *o, double *target, const char *msg);

    /**
     * 从对象中获取长双精度浮点值
     * 
     * @param o 对象指针
     * @param target 用于存储结果的长双精度浮点指针
     * @return 成功时返回C_OK，失败时返回C_ERR
     */
    int getLongDoubleFromObject(robj *o, long double *target);

    /**
     * 从对象中获取长双精度浮点值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的长双精度浮点指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getLongDoubleFromObjectOrReply(client *c, robj *o, long double *target, const char *msg);

    /**
     * 从对象中获取长整型值
     * 
     * @param o 对象指针
     * @param target 用于存储结果的长整型指针
     * @return 成功时返回C_OK，失败时返回C_ERR
     */
    int getLongLongFromObject(robj *o, long long *target);

    /**
     * 从对象中获取长整型值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的长整型指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getLongLongFromObjectOrReply(client *c, robj *o, long long *target, const char *msg);

    /**
     * 从对象中获取长整型值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的长整型指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getLongFromObjectOrReply(client *c, robj *o, long *target, const char *msg);

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
    int getRangeLongFromObjectOrReply(client *c, robj *o, long min, long max, long *target, const char *msg);

    /**
     * 从对象中获取正长整型值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的长整型指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getPositiveLongFromObjectOrReply(client *c, robj *o, long *target, const char *msg);

    /**
     * 从对象中获取整型值，失败时向客户端发送错误信息
     * 
     * @param c 客户端上下文
     * @param o 对象指针
     * @param target 用于存储结果的整型指针
     * @param msg 错误消息
     * @return 成功时返回C_OK，失败时返回C_ERR并发送错误信息
     */
    int getIntFromObjectOrReply(client *c, robj *o, int *target, const char *msg);

    /**
     * 获取编码类型的字符串表示
     * 
     * @param encoding 编码类型
     * @return 返回编码类型的字符串表示
     */
    char *strEncoding(int encoding);

    /**
     * 计算流基数树的内存使用量
     * 
     * @param rax 基数树指针
     * @return 返回内存使用量（字节）
     */
    size_t streamRadixTreeMemoryUsage(rax *rax);

    /**
     * 计算对象的内存占用大小
     * 
     * @param o 对象指针
     * @param sample_size 样本大小
     * @return 返回对象占用的内存大小（字节）
     */
    size_t objectComputeSize(robj *o, size_t sample_size);

    /**
     * 释放内存开销数据结构
     * 
     * @param mh 内存开销数据结构指针
     */
    void freeMemoryOverheadData(struct redisMemOverhead *mh);

    /**
     * 获取内存开销数据结构
     * 
     * @return 返回内存开销数据结构指针
     */
    struct redisMemOverhead *getMemoryOverheadData(void);

    /**
     * 将SDS字符串追加到结果缓冲区
     * 
     * @param result 结果缓冲区指针
     * @param str 要追加的字符串
     */
    void inputCatSds(void *result, const char *str);

    /**
     * 获取内存诊断报告
     * 
     * @return 返回内存诊断报告的SDS字符串
     */
    sds getMemoryDoctorReport(void);

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
    int objectSetLRUOrLFU(robj *val, long long lfu_freq, long long lru_idle, long long lru_clock, int lru_multiplier);

    /**
     * 查找并返回键对应的对象
     * 
     * @param c 客户端上下文
     * @param key 键对象
     * @return 如果找到返回对象指针，否则返回NULL
     */
    robj *objectCommandLookup(client *c, robj *key);

    /**
     * 查找并返回键对应的对象，未找到时向客户端发送回复
     * 
     * @param c 客户端上下文
     * @param key 键对象
     * @param reply 未找到时发送的回复对象
     * @return 如果找到返回对象指针，否则返回NULL并发送回复
     */
    robj *objectCommandLookupOrReply(client *c, robj *key, robj *reply);

    /**
     * 处理OBJECT命令
     * 
     * @param c 客户端上下文
     */
    void objectCommand(client *c);

    /**
     * 处理MEMORY命令
     * 
     * @param c 客户端上下文
     */
    void memoryCommand(client *c);

        /**
     * 计算SDS字符串的哈希值。
     * 用于字典的哈希函数回调。
     * 
     * @param key 指向SDS字符串的指针
     * @return 计算得到的哈希值
     */
    static uint64_t dictSdsHash(const void *key);
    /**
     * 比较两个SDS字符串键是否相等。
     * 用于字典的键比较回调函数。
     * 
     * @param privdata 私有数据（未使用）
     * @param key1 第一个键
     * @param key2 第二个键
     * @return 相等返回1，否则返回0
     */
    static int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2);
    /**
     * 释放SDS字符串的内存
     * @param privdata 私有数据（未使用）
     * @param val 待释放的SDS字符串指针
     */
    static void dictSdsDestructor(void *privdata, void *val);

public:
    struct sharedObjectsStruct 
    {
        robj *crlf, *ok, *err, *emptybulk, *czero, *cone, *pong, *space,
        *colon, *queued, *null[4], *nullarray[4], *emptymap[4], *emptyset[4],
        *emptyarray, *wrongtypeerr, *nokeyerr, *syntaxerr, *sameobjecterr,
        *outofrangeerr, *noscripterr, *loadingerr, *slowscripterr, *bgsaveerr,
        *masterdownerr, *roslaveerr, *execaborterr, *noautherr, *noreplicaserr,
        *busykeyerr, *oomerr, *plus, *messagebulk, *pmessagebulk, *subscribebulk,
        *unsubscribebulk, *psubscribebulk, *punsubscribebulk, *del, *unlink,
        *rpop, *lpop, *lpush, *rpoplpush, *lmove, *blmove, *zpopmin, *zpopmax,
        *emptyscan, *multi, *exec, *left, *right, *hset, *srem, *xgroup, *xclaim,  
        *script, *replconf, *eval, *persist, *set, *pexpireat, *pexpire, 
        *time, *pxat, *px, *retrycount, *force, *justid, 
        *lastid, *ping, *setid, *keepttl, *load, *createconsumer,
        *getack, *special_asterick, *special_equals, *default_username, *redacted,
        *select[PROTO_SHARED_SELECT_CMDS],
        *integers[OBJ_SHARED_INTEGERS],
        *mbulkhdr[OBJ_SHARED_BULKHDR_LEN], /* "*<value>\r\n" */
        *bulkhdr[OBJ_SHARED_BULKHDR_LEN];  /* "$<value>\r\n" */
        sds minstring, maxstring;
    } shared;
    
    dictType setDictType = {
        dictSdsHash,               /* hash function */
        NULL,                      /* key dup */
        NULL,                      /* val dup */
        dictSdsKeyCompare,         /* key compare */
        dictSdsDestructor,         /* key destructor */
        NULL                       /* val destructor */
    };


private:
    zskiplistCreate* zskiplistCreateInstance;
    dictionaryCreate* dictionaryCreateInstance;
    zsetCreate* zsetCreateInstance;
    sdsCreate *sdsCreateInstance;
    toolFunc* toolFuncInstance;
    ziplistCreate *ziplistCreateInstance;
};

#endif
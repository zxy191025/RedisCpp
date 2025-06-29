/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: dict implementation
 */

#ifndef __DICT_H
#define __DICT_H
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#define DICT_OK 0
#define DICT_ERR 1
#define DICT_NOTUSED(V) ((void) V)
class randomNumGenerator;
/**
 * 哈希表节点结构 - 存储键值对
 */
typedef struct dictEntry {
    void *key;                // 键指针（可存储任意类型）
    union {                   // 值的联合类型（节省空间）
        void *val;            // 通用指针值
        uint64_t u64;         // 无符号64位整数
        int64_t s64;          // 有符号64位整数
        double d;             // 双精度浮点数
    } v;
    struct dictEntry *next;   // 指向下一个节点的指针（处理哈希冲突）
} dictEntry;

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


/**
 * 哈希表结构 - 每个字典包含两个用于渐进式rehash
 */
typedef struct dictht {
    dictEntry **table;        // 哈希表数组（桶数组）
    unsigned long size;       // 哈希表大小（桶数量）
    unsigned long sizemask;   // 大小掩码（用于计算索引：size-1）
    unsigned long used;       // 已使用节点数量
} dictht;

/**
 * 字典结构 - 核心数据结构
 */
typedef struct dict {
    dictType *type;           // 字典类型（定义回调函数）
    void *privdata;           // 私有数据（传递给回调函数）
    dictht ht[2];             // 两个哈希表（用于渐进式rehash）
    long rehashidx;           // 渐进式rehash索引（-1表示未进行rehash）
    int16_t pauserehash;      // 暂停rehash标记（>0时暂停，<0表示编码错误）
} dict;

/**
 * 字典迭代器 - 用于遍历字典
 */
typedef struct dictIterator {
    dict *d;                  // 关联的字典
    long index;               // 当前桶索引
    int table;                // 当前哈希表索引（0或1）
    int safe;                 // 是否为安全迭代器（1=安全，0=不安全）
    dictEntry *entry;         // 当前节点
    dictEntry *nextEntry;     // 下一个节点（防止迭代过程中被修改）
    long long fingerprint;    // 迭代器指纹（用于检测不安全操作）
} dictIterator;

/**
 * 字典扫描回调函数类型
 */
typedef void (dictScanFunction)(void *privdata, const dictEntry *de);
typedef void (dictScanBucketFunction)(void *privdata, dictEntry **bucketref);
/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* 哈希表初始大小 - 必须为2的幂 */
#define DICT_HT_INITIAL_SIZE     4

/* ------------------------------- 宏定义 ------------------------------------*/

/**
 * 释放字典条目的值 - 调用自定义值析构函数
 * @param d 字典指针
 * @param entry 字典条目指针
 */
#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

/**
 * 设置字典条目的值 - 支持自定义值复制
 * @param d 字典指针
 * @param entry 字典条目指针
 * @param _val_ 值指针
 */
#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

/**
 * 设置字典条目的有符号整数值（直接存储，无需复制）
 * @param entry 字典条目指针
 * @param _val_ 64位有符号整数值
 */
#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)

/**
 * 设置字典条目的无符号整数值（直接存储，无需复制）
 * @param entry 字典条目指针
 * @param _val_ 64位无符号整数值
 */
#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

/**
 * 设置字典条目的双精度浮点数值（直接存储，无需复制）
 * @param entry 字典条目指针
 * @param _val_ 双精度浮点数值
 */
#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)

/**
 * 释放字典条目的键 - 调用自定义键析构函数
 * @param d 字典指针
 * @param entry 字典条目指针
 */
#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)

/**
 * 设置字典条目的键 - 支持自定义键复制
 * @param d 字典指针
 * @param entry 字典条目指针
 * @param _key_ 键指针
 */
#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

/**
 * 比较两个键是否相等 - 优先使用自定义比较函数
 * @param d 字典指针
 * @param key1 第一个键指针
 * @param key2 第二个键指针
 * @return 相等返回非零值，不等返回0
 */
#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))

/**
 * 计算键的哈希值 - 调用自定义哈希函数
 * @param d 字典指针
 * @param key 键指针
 * @return 哈希值
 */
#define dictHashKey(d, key) (d)->type->hashFunction(key)

/**
 * 获取字典条目的键
 * @param he 字典条目指针
 * @return 键指针
 */
#define dictGetKey(he) ((he)->key)

/**
 * 获取字典条目的值（通用指针形式）
 * @param he 字典条目指针
 * @return 值指针
 */
#define dictGetVal(he) ((he)->v.val)

/**
 * 获取字典条目的有符号整数值
 * @param he 字典条目指针
 * @return 64位有符号整数值
 */
#define dictGetSignedIntegerVal(he) ((he)->v.s64)

/**
 * 获取字典条目的无符号整数值
 * @param he 字典条目指针
 * @return 64位无符号整数值
 */
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)

/**
 * 获取字典条目的双精度浮点数值
 * @param he 字典条目指针
 * @return 双精度浮点数值
 */
#define dictGetDoubleVal(he) ((he)->v.d)

/**
 * 获取字典的总槽位数（两个哈希表的槽位之和）
 * @param d 字典指针
 * @return 总槽位数
 */
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)

/**
 * 获取字典的元素数量（两个哈希表的元素之和）
 * @param d 字典指针
 * @return 元素数量
 */
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)

/**
 * 判断字典是否正在进行rehash
 * @param d 字典指针
 * @return 正在rehash返回非零值，否则返回0
 */
#define dictIsRehashing(d) ((d)->rehashidx != -1)

/**
 * 暂停字典的rehash操作
 * @param d 字典指针
 */
#define dictPauseRehashing(d) (d)->pauserehash++

/**
 * 恢复字典的rehash操作
 * @param d 字典指针
 */
#define dictResumeRehashing(d) (d)->pauserehash--

class dictionaryCreate  // 类名修改为dictionaryCreate以避免与dict冲突
{
public:
    dictionaryCreate();
    ~dictionaryCreate();
public:
        #if ULONG_MAX >= 0xffffffffffffffff
        #define randomULong() ((unsigned long) genrand64->genrand64_int64())
        #else
        #define randomULong() genrand64.random()
        #endif
        /**
         * 创建新字典
         * @param type 字典类型（定义回调函数）
         * @param privDataPtr 私有数据指针（传递给回调函数）
         * @return 成功返回字典指针，失败返回NULL
         */
        dict *dictCreate(dictType *type, void *privDataPtr);

        /**
         * 扩展字典到指定大小（强制）
         * @param d 字典指针
         * @param size 目标大小（必须为2的幂）
         * @return 成功返回DICT_OK，失败返回DICT_ERR
         */
        int dictExpand(dict *d, unsigned long size);

        /**
         * 尝试扩展字典（仅在需要时且允许时扩展）
         * @param d 字典指针
         * @param size 目标大小（必须为2的幂）
         * @return 成功返回DICT_OK，无需扩展或失败返回DICT_ERR
         */
        int dictTryExpand(dict *d, unsigned long size);

        /**
         * 添加键值对（键不存在时）
         * @param d 字典指针
         * @param key 键指针
         * @param val 值指针
         * @return 成功返回DICT_OK，键已存在返回DICT_ERR
         */
        int dictAdd(dict *d, void *key, void *val);

        /**
         * 仅添加键（不设置值），用于自定义值初始化
         * @param d 字典指针
         * @param key 键指针
         * @param existing 输出参数：若键已存在，返回现有条目指针
         * @return 新条目指针（键不存在）或NULL（键已存在）
         */
        dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing);

        /**
         * 添加键或查找现有键
         * @param d 字典指针
         * @param key 键指针
         * @return 若键存在返回现有条目，否则创建新条目并返回
         */
        dictEntry *dictAddOrFind(dict *d, void *key);

        /**
         * 添加或替换键值对（键存在时替换值）
         * @param d 字典指针
         * @param key 键指针
         * @param val 值指针
         * @return 成功返回DICT_OK，失败返回DICT_ERR
         */
        int dictReplace(dict *d, void *key, void *val);

        /**
         * 删除指定键（自动释放资源）
         * @param d 字典指针
         * @param key 键指针
         * @return 成功返回DICT_OK，键不存在返回DICT_ERR
         */
        int dictDelete(dict *d, const void *key);

        /**
         * 解除键的链接（不释放资源，用于手动管理内存）
         * @param ht 字典指针
         * @param key 键指针
         * @return 若键存在返回条目指针，否则返回NULL
         */
        dictEntry *dictUnlink(dict *ht, const void *key);

        /**
         * 释放unlink返回的条目
         * @param d 字典指针
         * @param he 条目指针
         */
        void dictFreeUnlinkedEntry(dict *d, dictEntry *he);

        /**
         * 释放字典及其所有资源
         * @param d 字典指针
         */
        void dictRelease(dict *d);

        /**
         * 查找指定键
         * @param d 字典指针
         * @param key 键指针
         * @return 若键存在返回条目指针，否则返回NULL
         */
        dictEntry * dictFind(dict *d, const void *key);

        /**
         * 获取指定键的值
         * @param d 字典指针
         * @param key 键指针
         * @return 若键存在返回值指针，否则返回NULL
         */
        void *dictFetchValue(dict *d, const void *key);

        /**
         * 调整字典大小为刚好容纳所有元素（优化内存）
         * @param d 字典指针
         * @return 成功返回DICT_OK，失败返回DICT_ERR
         */
        int dictResize(dict *d);

        /**
         * 获取字典迭代器（不安全版本）
         * @param d 字典指针
         * @return 迭代器指针
         */
        dictIterator *dictGetIterator(dict *d);

        /**
         * 获取安全迭代器（允许迭代时修改字典）
         * @param d 字典指针
         * @return 安全迭代器指针
         */
        dictIterator *dictGetSafeIterator(dict *d);

        /**
         * 获取迭代器的下一个元素
         * @param iter 迭代器指针
         * @return 下一个条目指针，遍历结束返回NULL
         */
        dictEntry *dictNext(dictIterator *iter);

        /**
         * 释放迭代器
         * @param iter 迭代器指针
         */
        void dictReleaseIterator(dictIterator *iter);

        /**
         * 随机获取一个字典条目（快速但分布不均）
         * @param d 字典指针
         * @return 随机条目指针，字典为空返回NULL
         */
        dictEntry *dictGetRandomKey(dict *d);

        /**
         * 随机获取一个字典条目（更均匀的随机分布）
         * @param d 字典指针
         * @return 随机条目指针，字典为空返回NULL
         */
        dictEntry *dictGetFairRandomKey(dict *d);

        /**
         * 随机获取多个字典条目
         * @param d 字典指针
         * @param des 存储结果的数组
         * @param count 需要获取的条目数量
         * @return 实际获取的条目数量
         */
        unsigned int dictGetSomeKeys(dict *d, dictEntry **des, unsigned int count);

        /**
         * 获取字典统计信息
         * @param buf 存储信息的缓冲区
         * @param bufsize 缓冲区大小
         * @param d 字典指针
         */
        void dictGetStats(char *buf, size_t bufsize, dict *d);

        /**
         * 生成通用哈希值（二进制敏感）
         * @param key 键指针
         * @param len 键长度（字节）
         * @return 哈希值
         */
        uint64_t dictGenHashFunction(const void *key, int len);

        /**
         * 生成大小写不敏感的哈希值（字符串专用）
         * @param buf 字符串缓冲区
         * @param len 字符串长度（字节）
         * @return 哈希值
         */
        uint64_t dictGenCaseHashFunction(const unsigned char *buf, int len);

        /**
         * 清空字典但保留结构（可指定回调函数）
         * @param d 字典指针
         * @param callback 清空时调用的回调函数（可为NULL）
         */
        void dictEmpty(dict *d, void(callback)(void*));

        /**
         * 启用字典自动扩容
         */
        void dictEnableResize(void);

        /**
         * 禁用字典自动扩容
         */
        void dictDisableResize(void);

        /**
         * 执行n步渐进式rehash
         * @param d 字典指针
         * @param n 执行的rehash步数
         * @return 仍需rehash返回1，已完成返回0
         */
        int dictRehash(dict *d, int n);

        /**
         * 在指定毫秒内执行rehash
         * @param d 字典指针
         * @param ms 执行时间（毫秒）
         * @return 仍需rehash返回1，已完成返回0
         */
        int dictRehashMilliseconds(dict *d, int ms);

        /**
         * 设置哈希函数种子（影响哈希结果）
         * @param seed 种子数组（8字节）
         */
        void dictSetHashFunctionSeed(uint8_t *seed);

        /**
         * 获取哈希函数种子
         * @return 种子数组指针
         */
        uint8_t *dictGetHashFunctionSeed(void);

        /**
         * 迭代扫描字典（无需锁，支持增量）
         * @param d 字典指针
         * @param v 迭代游标（初始为0）
         * @param fn 条目处理回调函数
         * @param bucketfn 桶处理回调函数（可为NULL）
         * @param privdata 传递给回调函数的私有数据
         * @return 下一次迭代的游标值，0表示遍历结束
         */
        unsigned long dictScan(dict *d, unsigned long v, dictScanFunction *fn, dictScanBucketFunction *bucketfn, void *privdata);

        /**
         * 计算键的哈希值
         * @param d 字典指针
         * @param key 键指针
         * @return 哈希值
         */
        uint64_t dictGetHash(dict *d, const void *key);

        /**
         * 通过指针和哈希值查找条目引用（内部使用）
         * @param d 字典指针
         * @param oldptr 键指针
         * @param hash 哈希值
         * @return 条目引用指针，未找到返回NULL
         */
        dictEntry **dictFindEntryRefByPtrAndHash(dict *d, const void *oldptr, uint64_t hash);

        long long timeInMilliseconds(void); 
private:
        /*
        * 执行字典的一步渐进式rehash操作
        * 每次调用仅处理一个哈希表索引位置
        * 参数：
        *   d：指向待rehash的字典指针
        */
        void _dictRehashStep(dict *d);

        /*
        * 判断字典是否允许进行扩展操作
        * 会检查当前内存使用情况、负载因子等条件
        * 参数：
        *   d：指向目标字典的指针
        * 返回值：
        *   允许扩展返回1，否则返回0
        */
        int dictTypeExpandAllowed(dict *d);

        /*
        * 根据需要扩展字典大小
        * 当字典达到负载因子或为空时触发扩展
        * 参数：
        *   d：指向待检查和扩展的字典指针
        * 返回值：
        *   成功返回DICT_OK，失败返回DICT_ERR
        */
        int _dictExpandIfNeeded(dict *d);

        /*
        * 重置哈希表结构为初始状态
        * 清空所有表项并重置大小参数
        * 参数：
        *   ht：指向待重置的哈希表指针
        */
        void _dictReset(dictht *ht);

        /*
        * 初始化字典结构
        * 设置字典类型、私有数据并初始化哈希表
        * 参数：
        *   d：指向待初始化的字典指针
        *   type：指向字典类型特定函数的结构体指针
        *   privDataPtr：私有数据指针
        * 返回值：
        *   成功返回DICT_OK，失败返回DICT_ERR
        */
        int _dictInit(dict *d, dictType *type, void *privDataPtr);

        /*
        * 扩展或创建字典哈希表
        * 参数：
        *   d：指向目标字典的指针
        *   size：期望的哈希表大小
        *   malloc_failed：可选的失败标志指针，失败时被设置为1
        * 返回值：
        *   成功返回DICT_OK，失败返回DICT_ERR
        */
        int _dictExpand(dict *d, unsigned long size, int* malloc_failed);

        /*
        * 清空指定哈希表中的所有元素
        * 可选执行回调函数处理每个元素
        * 参数：
        *   d：指向字典的指针
        *   ht：指向待清空的哈希表指针
        *   callback：可选的元素处理回调函数
        * 返回值：
        *   成功返回DICT_OK，失败返回DICT_ERR
        */
        int _dictClear(dict *d, dictht *ht, void(callback)(void *));

        /*
        * 生成字典的指纹（唯一标识）
        * 基于字典当前状态生成64位哈希值
        * 参数：
        *   d：指向目标字典的指针
        * 返回值：
        *   字典的指纹值
        */
        long long dictFingerprint(dict *d);

        /*
        * 计算大于或等于给定值的最小2的幂
        * 用于确定合适的哈希表大小
        * 参数：
        *   size：输入值
        * 返回值：
        *   大于或等于size的最小2的幂
        */
        unsigned long _dictNextPower(unsigned long size);

        /*
        * 查找或计算键在哈希表中的索引位置
        * 参数：
        *   d：指向字典的指针
        *   key：待查找的键
        *   hash：键的哈希值
        *   existing：可选的指针，用于存储已存在的键的条目
        * 返回值：
        *   可用索引位置，若键已存在则返回-1
        */
        long _dictKeyIndex(dict *d, const void *key, uint64_t hash, dictEntry **existing);

        /*
        * 生成哈希表的统计信息
        * 参数：
        *   buf：存储统计信息的缓冲区
        *   bufsize：缓冲区大小
        *   ht：指向目标哈希表的指针
        *   tableid：哈希表标识
        * 返回值：
        *   写入缓冲区的字节数
        */
        size_t _dictGetStatsHt(char *buf, size_t bufsize, dictht *ht, int tableid);

        /*
        * 从字典中删除指定键的条目
        * 参数：
        *   d：指向目标字典的指针
        *   key：待删除的键
        *   nofree：是否不释放键值内存(1表示不释放)
        * 返回值：
        *   指向被删除条目的指针，不存在则返回NULL
        */
        dictEntry *dictGenericDelete(dict *d, const void *key, int nofree);

        /*
        * 使用SipHash算法计算输入数据的哈希值
        * 参数：
        *   in：输入数据指针
        *   inlen：输入数据长度
        *   k：16字节的密钥
        * 返回值：
        *   64位哈希值
        */
        uint64_t siphash(const uint8_t *in, const size_t inlen, const uint8_t *k);

        /*
        * 使用SipHash算法计算不区分大小写的哈希值
        * 参数：
        *   in：输入数据指针
        *   inlen：输入数据长度
        *   k：16字节的密钥
        * 返回值：
        *   64位哈希值
        */
        uint64_t siphash_nocase(const uint8_t *in, const size_t inlen, const uint8_t *k);
private:
    randomNumGenerator *genrand64;  
};
/**
 * 预定义字典类型：
 * - dictTypeHeapStringCopyKey: 键复制，值为字符串
 * - dictTypeHeapStrings: 键和值均为字符串
 * - dictTypeHeapStringCopyKeyValue: 键和值均复制
 */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif
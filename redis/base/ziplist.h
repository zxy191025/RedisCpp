/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: ziplist.h 是实现 压缩列表（ZipList） 的核心头文件。
 * 压缩列表是一种特殊的紧凑型数组，设计用于在内存中高效存储少量键值对，被 Redis 用作 列表（List）、哈希（Hash） 和 有序集合（Sorted Set） 的底层实现之一
 */
#ifndef REDIS_BASE_ZIPLIST_H
#define REDIS_BASE_ZIPLIST_H
#include "define.h"
#include <cstddef>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
/**
 * ziplistEntry - 表示压缩列表中的一个条目
 * 
 * 压缩列表中的每个条目可以是字符串或整数，使用联合表示：
 * - 当存储字符串时：sval 指向字符串数据，slen 表示字符串长度
 * - 当存储整数时：sval 为 NULL，lval 存储整数值
 */
typedef struct {
    unsigned char *sval;    // 字符串值指针，字符串存在时有效
    unsigned int slen;     // 字符串长度
    long long lval;        // 整数值，字符串不存在时有效
} ziplistEntry;

/* 我们使用此函数获取压缩列表(ziplist)条目的相关信息。
 * 注意：这并非数据的实际编码方式，而是为了便于操作，由函数填充后提供的信息结构。 */
typedef struct zlentry {
    unsigned int prevrawlensize; /* 编码前一个条目长度所需的字节数 */
    unsigned int prevrawlen;     /* 前一个条目的原始长度 */
    unsigned int lensize;        /* 编码当前条目类型/长度所需的字节数
                                    例如：字符串有1、2或5字节的头部
                                    整数始终使用1字节 */
    unsigned int len;            /* 表示实际条目内容所需的字节数
                                    对于字符串，这就是字符串长度
                                    对于整数，根据数值范围不同，可能是1、2、3、4、8或0字节(4位立即数) */
    unsigned int headersize;     /* prevrawlensize + lensize之和，即头部总大小 */
    unsigned char encoding;      /* 根据条目编码类型设置为ZIP_STR_*或ZIP_INT_*
                                    但对于4位立即数整数，此值可能处于特定范围，需要进行范围检查 */
    unsigned char *p;            /* 指向条目起始位置的指针
                                    即指向存储前一个条目长度的字段 */
} zlentry;

typedef int (*ziplistValidateEntryCB)(unsigned char* p, void* userdata);

/**
 * ziplistCreate - 压缩列表操作类
 * 
 * 提供了创建、操作和管理压缩列表的接口。压缩列表是一种
 * 内存高效的数据结构，用于存储小量的字符串或整数元素。
 */
class ziplistCreate
{
public:
    ziplistCreate() = default;
    ~ziplistCreate() = default;
public:
    /**
     * 创建一个新的空压缩列表
     * @return 返回指向新创建的压缩列表的指针
     */
    unsigned char *ziplistNew(void);

    /**
     * 合并两个压缩列表
     * @param first 指向第一个压缩列表指针的指针，合并后会被修改
     * @param second 指向第二个压缩列表指针的指针，合并后会被释放
     * @return 返回合并后的压缩列表指针
     */
    unsigned char *ziplistMerge(unsigned char **first, unsigned char **second);

    /**
     * 在压缩列表的头部或尾部添加一个新元素
     * @param zl 压缩列表指针
     * @param s 要添加的元素数据
     * @param slen 元素数据长度
     * @param where 添加位置（ZIPLIST_HEAD 或 ZIPLIST_TAIL）
     * @return 返回更新后的压缩列表指针
     */
    unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where);

    /**
     * 根据索引获取压缩列表中的元素
     * @param zl 压缩列表指针
     * @param index 元素索引（负数表示从尾部开始计算）
     * @return 返回指向元素的指针，如果索引无效则返回 NULL
     */
    unsigned char *ziplistIndex(unsigned char *zl, int index);

    /**
     * 获取压缩列表中指定元素的下一个元素
     * @param zl 压缩列表指针
     * @param p 指向当前元素的指针
     * @return 返回指向下一个元素的指针，如果没有下一个元素则返回 NULL
     */
    unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);

    /**
     * 获取压缩列表中指定元素的前一个元素
     * @param zl 压缩列表指针
     * @param p 指向当前元素的指针
     * @return 返回指向前一个元素的指针，如果没有前一个元素则返回 NULL
     */
    unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);

    /**
     * 解析压缩列表中的元素数据
     * @param p 指向元素的指针
     * @param sval 输出参数，存储元素的字符串值指针
     * @param slen 输出参数，存储元素的字符串长度
     * @param lval 输出参数，存储元素的整数值
     * @return 如果元素是字符串返回 1，如果是整数返回 0
     */
    unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval);

    /**
     * 在指定位置插入一个新元素
     * @param zl 压缩列表指针
     * @param p 指向插入位置的指针
     * @param s 要插入的元素数据
     * @param slen 元素数据长度
     * @return 返回更新后的压缩列表指针
     */
    unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);

    /**
     * 删除压缩列表中的指定元素
     * @param zl 压缩列表指针
     * @param p 指向要删除元素指针的指针，删除后会被更新为下一个元素
     * @return 返回更新后的压缩列表指针
     */
    unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p);

    /**
     * 从指定索引开始删除指定数量的元素
     * @param zl 压缩列表指针
     * @param index 开始删除的索引
     * @param num 要删除的元素数量
     * @return 返回更新后的压缩列表指针
     */
    unsigned char *ziplistDeleteRange(unsigned char *zl, int index, unsigned int num);

    /**
     * 替换压缩列表中的指定元素
     * @param zl 压缩列表指针
     * @param p 指向要替换元素的指针
     * @param s 新元素数据
     * @param slen 新元素数据长度
     * @return 返回更新后的压缩列表指针
     */
    unsigned char *ziplistReplace(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);

    /**
     * 比较压缩列表中的元素与给定值
     * @param p 指向压缩列表中元素的指针
     * @param s 要比较的值
     * @param slen 要比较的值的长度
     * @return 如果相等返回 1，否则返回 0
     */
    unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen);

    /**
     * 在压缩列表中查找指定值
     * @param zl 压缩列表指针
     * @param p 开始查找的位置指针
     * @param vstr 要查找的值
     * @param vlen 要查找的值的长度
     * @param skip 查找时跳过的元素数量（用于查找重复值）
     * @return 返回找到的元素指针，如果未找到则返回 NULL
     */
    unsigned char *ziplistFind(unsigned char *zl, unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip);

    /**
     * 获取压缩列表的元素数量
     * @param zl 压缩列表指针
     * @return 返回压缩列表中的元素数量
     */
    unsigned int ziplistLen(unsigned char *zl);

    /**
     * 获取压缩列表的总字节长度
     * @param zl 压缩列表指针
     * @return 返回压缩列表占用的总字节数
     */
    size_t ziplistBlobLen(unsigned char *zl);

    /**
     * 打印压缩列表的详细信息（用于调试）
     * @param zl 压缩列表指针
     */
    void ziplistRepr(unsigned char *zl);

    /**
     * 验证压缩列表的完整性
     * @param zl 压缩列表指针
     * @param size 压缩列表大小
     * @param deep 是否进行深度验证
     * @param entry_cb 条目验证回调函数
     * @param cb_userdata 传递给回调函数的用户数据
     * @return 验证成功返回1，失败返回0
     */
    int ziplistValidateIntegrity(unsigned char *zl, size_t size, int deep,
                                ziplistValidateEntryCB entry_cb, void *cb_userdata);

    /**
     * 从压缩列表中随机获取一对键值（用于哈希表）
     * @param zl 压缩列表指针
     * @param total_count 总元素数量
     * @param key 输出参数，存储随机获取的键
     * @param val 输出参数，存储随机获取的值
     */
    void ziplistRandomPair(unsigned char *zl, unsigned long total_count, ziplistEntry *key, ziplistEntry *val);

    /**
     * 从压缩列表中随机获取多对键值（用于哈希表）
     * @param zl 压缩列表指针
     * @param count 要获取的键值对数量
     * @param keys 输出参数数组，存储随机获取的键
     * @param vals 输出参数数组，存储随机获取的值
     */
    void ziplistRandomPairs(unsigned char *zl, unsigned int count, ziplistEntry *keys, ziplistEntry *vals);

    /**
     * 从压缩列表中随机获取唯一的多对键值（用于哈希表）
     * @param zl 压缩列表指针
     * @param count 要获取的键值对数量
     * @param keys 输出参数数组，存储随机获取的键
     * @param vals 输出参数数组，存储随机获取的值
     * @return 实际获取到的唯一键值对数量
     */
    unsigned int ziplistRandomPairsUnique(unsigned char *zl, unsigned int count, ziplistEntry *keys, ziplistEntry *vals);

    /**
     * 检查压缩列表是否有足够空间添加指定大小的数据
     * @param zl 压缩列表指针
     * @param add 需要添加的字节数
     * @return 如果安全返回1，否则返回0
     */
    int ziplistSafeToAdd(unsigned char* zl, size_t add);

    /**
     * 获取指定编码所需的长度字段字节数
     * 
     * @param encoding 编码类型（如ZIP_STR_*或ZIP_INT_*）
     * @return 存储长度所需的字节数
     */
    unsigned int zipEncodingLenSize(unsigned char encoding);

    /**
     * 获取指定整数编码所需的字节数
     * 
     * @param encoding 整数编码类型（如ZIP_INT_8B, ZIP_INT_16B等）
     * @return 存储该类型整数所需的字节数
     */
    unsigned int zipIntSize(unsigned char encoding);

    /**
     * 存储压缩列表项的编码和原始长度
     * 
     * @param p 目标缓冲区指针
     * @param encoding 编码类型
     * @param rawlen 原始数据长度
     * @return 写入的字节数
     */
    unsigned int zipStoreEntryEncoding(unsigned char *p, unsigned char encoding, unsigned int rawlen);

    /**
     * 以大长度格式存储前一项的长度
     * 
     * @param p 目标缓冲区指针
     * @param len 前一项长度值
     * @return 成功返回1，失败返回0
     */
    int zipStorePrevEntryLengthLarge(unsigned char *p, unsigned int len);

    /**
     * 存储前一项的长度（自动选择合适的编码）
     * 
     * @param p 目标缓冲区指针
     * @param len 前一项长度值
     * @return 写入的字节数
     */
    unsigned int zipStorePrevEntryLength(unsigned char *p, unsigned int len);

    /**
     * 计算存储前一项长度所需的字节数差异
     * 
     * @param p 当前项指针
     * @param len 新的前一项长度
     * @return 字节数差异，用于级联更新判断
     */
    int zipPrevLenByteDiff(unsigned char *p, unsigned int len);

    /**
     * 尝试为数据选择最优编码方式
     * 
     * @param entry 数据条目指针
     * @param entrylen 数据长度
     * @param v 若为整数，存储解析后的整数值
     * @param encoding 输出最优编码类型
     * @return 成功返回1，失败返回0
     */
    int zipTryEncoding(unsigned char *entry, unsigned int entrylen, long long *v, unsigned char *encoding);

    /**
     * 按指定编码保存整数值
     * 
     * @param p 目标缓冲区指针
     * @param value 整数值
     * @param encoding 编码类型
     */
    void zipSaveInteger(unsigned char *p, int64_t value, unsigned char encoding);

    /**
     * 按指定编码加载整数值
     * 
     * @param p 源缓冲区指针
     * @param encoding 编码类型
     * @return 解析后的整数值
     */
    int64_t zipLoadInteger(unsigned char *p, unsigned char encoding);

    /**
     * 将zlentry结构保存到压缩列表
     * 
     * @param p 目标缓冲区指针
     * @param e 要保存的zlentry结构
     */
    void zipEntry(unsigned char *p, zlentry *e);

    /**
     * 安全地将zlentry保存到压缩列表（带边界检查）
     * 
     * @param zl 压缩列表指针
     * @param zlbytes 压缩列表总字节数
     * @param p 目标位置指针
     * @param e 要保存的zlentry结构
     * @param validate_prevlen 是否验证前一项长度
     * @return 成功返回1，失败返回0
     */
    int zipEntrySafe(unsigned char* zl, size_t zlbytes, unsigned char *p, zlentry *e, int validate_prevlen);

    /**
     * 安全获取压缩列表项的原始长度（带边界检查）
     * 
     * @param zl 压缩列表指针
     * @param zlbytes 压缩列表总字节数
     * @param p 项指针
     * @return 原始长度，若越界返回0
     */
    unsigned int zipRawEntryLengthSafe(unsigned char* zl, size_t zlbytes, unsigned char *p);

    /**
     * 获取压缩列表项的原始长度
     * 
     * @param p 项指针
     * @return 原始长度
     */
    unsigned int zipRawEntryLength(unsigned char *p);

    /**
     * 断言压缩列表项的有效性（用于调试）
     * 
     * @param zl 压缩列表指针
     * @param zlbytes 压缩列表总字节数
     * @param p 项指针
     */
    void zipAssertValidEntry(unsigned char* zl, size_t zlbytes, unsigned char *p);

    /**
     * 调整压缩列表大小
     * 
     * @param zl 压缩列表指针
     * @param len 新的大小
     * @return 调整后的压缩列表指针
     */
    unsigned char *ziplistResize(unsigned char *zl, size_t len);

    /**
     * 处理压缩列表的级联更新（内部函数）
     * 
     * @param zl 压缩列表指针
     * @param p 触发级联更新的位置
     * @return 更新后的压缩列表指针
     */
    unsigned char *__ziplistCascadeUpdate(unsigned char *zl, unsigned char *p);

    /**
     * 删除压缩列表中的项（内部函数）
     * 
     * @param zl 压缩列表指针
     * @param p 起始删除位置
     * @param num 要删除的项数
     * @return 更新后的压缩列表指针
     */
    unsigned char *__ziplistDelete(unsigned char *zl, unsigned char *p, unsigned int num);

    /**
     * 在压缩列表中插入项（内部函数）
     * 
     * @param zl 压缩列表指针
     * @param p 插入位置
     * @param s 要插入的数据
     * @param slen 数据长度
     * @return 更新后的压缩列表指针
     */
    unsigned char *__ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);

    /**
     * 保存值到ziplistEntry结构
     * 
     * @param val 数据指针
     * @param len 数据长度
     * @param lval 整数值（如果适用）
     * @param dest 目标zlentry结构
     */
    void ziplistSaveValue(unsigned char *val, unsigned int len, long long lval, ziplistEntry *dest);
    /**
     * 解码并设置 ziplist 中前一个节点的长度信息
     * 
     * 该函数用于解析 ziplist 中存储的前一个节点的长度，并将其写入指定的指针位置。
     * ziplist 使用可变长度编码来存储前一个节点的长度，以节省内存空间。
     * 
     * @param ptr           指向 ziplist 中存储前一个节点长度信息的指针
     * @param prevlensize   前一个节点长度信息占用的字节数（1 或 5）
     * @param prevlen       前一个节点的实际长度值
     * 
     * @note 该函数会根据 prevlensize 的值，将 prevlen 以不同的编码方式写入 ptr 指向的位置：
     *       - 若 prevlensize 为 1，则写入 1 字节（适用于长度 < 254 的情况）
     *       - 若 prevlensize 为 5，则写入 5 字节（首字节为 0xFE，后四字节为实际长度）
     * 
     * @see zipEncodePrevLength() - 用于编码前一个节点长度的互补函数
     * @see zipPrevLenByteDiff()  - 计算前一个节点长度编码所需字节数的变化
     */
    void zipDecodePrevlen(unsigned char *ptr, unsigned int prevlensize,unsigned int prevlen);
    
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
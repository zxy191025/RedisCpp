/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/27
 * All rights reserved. No one may copy or transfer.
 * Description: 有序集合是 Redis 提供的一种高级数据结构，它结合了哈希表和跳跃表（Skip List）的特性，能够高效地实现按分数排序和快速查找。
 */
#ifndef REDIS_BASE_ZSET_H
#define REDIS_BASE_ZSET_H
#include "define.h"
#include "dict.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class sdsCreate;
class ziplistCreate;
class toolFunc;
class zskiplistCreate;
class redisObjectCreate;
class redisObject;
typedef class redisObject robj;
typedef struct zset {
    dict *dictl;
    zskiplist *zsl;
} zset;
class zsetCreate
{
public:
    zsetCreate();
    ~zsetCreate();

    //有序集合（zset）通用操作
public:
    /**
     * 获取有序集合的元素数量
     * @param zobj 有序集合对象指针
     * @return 返回元素数量
     */
    unsigned long zsetLength(const robj *zobj);

    /**
     * 转换有序集合的编码方式（如压缩列表与跳跃表互转）
     * @param zobj 有序集合对象指针
     * @param encoding 目标编码类型（OBJ_ENCODING_ZIPLIST 或 OBJ_ENCODING_SKIPLIST）
     */
    void zsetConvert(robj *zobj, int encoding);

    /**
     * 当满足条件时自动将有序集合转换为压缩列表编码
     * @param zobj 有序集合对象指针
     * @param maxelelen 元素最大长度阈值
     * @param totelelen 集合总长度阈值
     */
    void zsetConvertToZiplistIfNeeded(robj *zobj, size_t maxelelen, size_t totelelen);

    /**
     * 获取有序集合中成员的分数
     * @param zobj 有序集合对象指针
     * @param member 待查询的成员名称
     * @param score 输出参数，存储查询到的分数值
     * @return 成员存在返回1，不存在返回0
     */
    int zsetScore(robj *zobj, sds member, double *score);

    /**
     * 向有序集合中添加或更新成员的分数
     * @param zobj 有序集合对象指针
     * @param score 新分数值
     * @param ele 成员名称
     * @param in_flags 输入标志（如ZADD_IN_FLAG_NX表示不更新已存在成员）
     * @param out_flags 输出标志（如ZADD_OUT_FLAG_ADDED表示成功添加）
     * @param newscore 输出参数，存储最终生效的分数值
     * @return 操作成功返回1，失败返回0
     */
    int zsetAdd(robj *zobj, double score, sds ele, int in_flags, int *out_flags, double *newscore);

    /**
     * 获取有序集合中成员的排名（支持升序/降序）
     * @param zobj 有序集合对象指针
     * @param ele 成员名称
     * @param reverse 排序方向（1表示降序，0表示升序）
     * @return 返回成员排名（从0开始），成员不存在返回-1
     */
    long zsetRank(robj *zobj, sds ele, int reverse);

    /**
     * 从有序集合中删除成员
     * @param zobj 有序集合对象指针
     * @param ele 待删除的成员名称
     * @return 成功删除返回1，成员不存在返回0
     */
    int zsetDel(robj *zobj, sds ele);

    /**
     * 复制有序集合对象
     * @param o 源有序集合对象指针
     * @return 返回新复制的有序集合对象指针
     */
    robj *zsetDup(robj *o);

    //其他辅助函数
    /**
     * 验证压缩列表编码的有序集合完整性
     * @param zl 压缩列表指针
     * @param size 压缩列表大小（字节数）
     * @param deep 是否进行深度验证（验证每个元素内容）
     * @return 验证通过返回1，发现错误返回0
     */
    int zsetZiplistValidateIntegrity(unsigned char *zl, size_t size, int deep);

    /**
     * 从有序集合中删除指定元素。
     * 
     * @param zs 指向有序集合的指针
     * @param ele 要删除的元素(SDS字符串)
     * @return 成功删除返回1，未找到元素返回0
     */
    int zsetRemoveFromSkiplist(zset *zs, sds ele);

    /**
     * 验证压缩列表表示的有序集合的完整性
     * 用于内部一致性检查和调试
     * @param p 压缩列表的指针
     * @param userdata 用户数据，包含验证所需的上下文
     * @return 验证通过返回1，否则返回0
     */
    static int _zsetZiplistValidateIntegrity(unsigned char *p, void *userdata);

    //压缩列表（Ziplist）操作
public:
    /**
     * 向压缩列表中插入元素和分数
     * @param zl 目标压缩列表指针
     * @param ele 待插入的元素字符串
     * @param score 待插入的分数值
     * @return 返回新的压缩列表指针（可能已重新分配内存）
     */
    unsigned char *zzlInsert(unsigned char *zl, sds ele, double score);

    /**
     * 从压缩列表中获取元素的分数值
     * @param sptr 指向分数存储位置的指针（通过zzlNext/zzlPrev获取）
     * @return 返回解析出的分数值
     */
    double zzlGetScore(unsigned char *sptr);

    /**
     * 移动到压缩列表的下一个元素
     * @param zl 压缩列表指针
     * @param eptr 输出参数，指向当前元素的指针
     * @param sptr 输出参数，指向当前元素分数的指针
     */
    void zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

    /**
     * 移动到压缩列表的前一个元素
     * @param zl 压缩列表指针
     * @param eptr 输出参数，指向当前元素的指针
     * @param sptr 输出参数，指向当前元素分数的指针
     */
    void zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

    /**
     * 获取压缩列表中分数范围内的第一个元素
     * @param zl 压缩列表指针
     * @param range 分数范围规范结构体，包含min/max及边界规则
     * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
     */
    unsigned char *zzlFirstInRange(unsigned char *zl, zrangespec *range);

    /**
     * 获取压缩列表中分数范围内的最后一个元素
     * @param zl 压缩列表指针
     * @param range 分数范围规范结构体，包含min/max及边界规则
     * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
     */
    unsigned char *zzlLastInRange(unsigned char *zl, zrangespec *range);

    /**
     * 获取压缩列表中字典序范围内的第一个元素
     * @param zl 压缩列表指针
     * @param range 字典序范围规范结构体
     * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
     */
    unsigned char *zzlFirstInLexRange(unsigned char *zl, zlexrangespec *range);

    /**
     * 获取压缩列表中字典序范围内的最后一个元素
     * @param zl 压缩列表指针
     * @param range 字典序范围规范结构体
     * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
     */
    unsigned char *zzlLastInLexRange(unsigned char *zl, zlexrangespec *range);

    /**
     * 在跳跃表(zskiplist)的底层链表中插入一个新元素
     * 
     * @param zl        指向跳跃表底层压缩列表的指针
     * @param eptr      插入位置的指针（NULL表示插入到尾部）
     * @param ele       要插入的元素值（SDS字符串）
     * @param score     元素的分数（排序依据）
     * @return          插入新元素后的压缩列表指针
     */
    unsigned char *zzlInsertAt(unsigned char *zl, unsigned char *eptr, sds ele, double score);

    /**
     * 比较压缩列表中的元素与指定字符串
     * 
     * @param eptr      指向压缩列表中元素的指针
     * @param cstr      要比较的目标字符串
     * @param clen      目标字符串的长度
     * @return          比较结果：0表示相等，非0表示不相等
     */
    int zzlCompareElements(unsigned char *eptr, unsigned char *cstr, unsigned int clen);

    /**
     * 将压缩列表中的字符串转换为double类型数值
     * 
     * @param vstr      指向字符串值的指针
     * @param vlen      字符串的长度
     * @return          转换后的double值
     */
    double zzlStrtod(unsigned char *vstr, unsigned int vlen);

    /**
     * 检查压缩列表中的元素是否在指定范围内
     * 
     * @param zl        指向压缩列表的指针
     * @param range     范围规范结构体指针
     * @return          1表示元素在范围内，0表示不在范围内
     */
    int zzlIsInRange(unsigned char *zl, zrangespec *range);

    /**
     * Redis有序集合(zset)底层实现相关的核心数据结构和函数。
     * 有序集合同时使用哈希表(dict)和跳跃表(zskiplist)实现，
     * 以支持O(1)时间复杂度的成员查找和O(logN)的范围操作。
     */

    /**
     * 计算压缩列表(ziplist)的长度。
     * 
     * @param zl 指向压缩列表的指针
     * @return 压缩列表中的元素数量
     */
    unsigned int zzlLength(unsigned char *zl);

    /**
     * 从压缩列表中提取元素对象
     * @param sptr 指向元素存储位置的指针
     * @return 返回解析出的字符串对象
     */
    sds ziplistGetObject(unsigned char *sptr);

    /**
     * 在压缩列表中查找指定元素。
     * 
     * @param zl 指向压缩列表的指针
     * @param ele 要查找的元素(SDS字符串)
     * @param score 用于存储找到元素的分数(如果不为NULL)
     * @return 指向元素的指针，如果未找到则返回NULL
     */
    unsigned char *zzlFind(unsigned char *zl, sds ele, double *score);

    /**
     * 从压缩列表中删除指定元素。
     * 
     * @param zl 指向压缩列表的指针
     * @param eptr 指向要删除元素的指针
     * @return 删除元素后的压缩列表指针
     */
    unsigned char *zzlDelete(unsigned char *zl, unsigned char *eptr);


    /**
     * 判断压缩列表表示的有序集合是否在字典序范围内
     * @param zl 压缩列表指针
     * @param range 字典序范围规范
     * @return 若在范围内返回1，否则返回0
     */
    int zzlIsInLexRange(unsigned char *zl, zlexrangespec *range);

    /**
     * 检查压缩列表中的元素是否大于等于字典序最小值
     * @param p 压缩列表中元素的指针
     * @param spec 字典序范围规范
     * @return 满足条件返回1，否则返回0
     */
    int zzlLexValueGteMin(unsigned char *p, zlexrangespec *spec);

    /**
     * 检查压缩列表中的元素是否小于等于字典序最大值
     * @param p 压缩列表中元素的指针
     * @param spec 字典序范围规范
     * @return 满足条件返回1，否则返回0
     */
    int zzlLexValueLteMax(unsigned char *p, zlexrangespec *spec);

  //跳跃表（zskiplist）操作
public:

    /**
     * 解析有序集合的字典序范围项
     * @param item 待解析的对象
     * @param dest 输出参数，存储解析后的SDS字符串
     * @param ex 输出参数，存储是否为排他范围标记
     * @return 成功返回1，失败返回0
     */
    int zslParseLexRangeItem(robj *item, sds *dest, int *ex);

    /**
     * 判断有序集合节点是否在指定字典序范围内
     * @param zsl 有序集合指针
     * @param range 字典序范围规范
     * @return 若在范围内返回1，否则返回0
     */
    int zslIsInLexRange(zskiplist *zsl, zlexrangespec *range);

    /**
     * 解析字典序范围参数（如"[min" "[max"）
     * @param min 最小值对象（字符串）
     * @param max 最大值对象（字符串）
     * @param spec 输出参数，存储解析后的范围规范结构体
     * @return 解析成功返回1，参数格式错误返回0
     */
    int zslParseLexRange(robj *min, robj *max, zlexrangespec *spec);

    /**
     * 释放字典序范围规范结构体占用的内存
     * @param spec 待释放的范围规范结构体指针
     */
    void zslFreeLexRange(zlexrangespec *spec);

    /**
     * 获取跳跃表中字典序范围内的第一个节点
     * @param zsl 目标跳跃表指针
     * @param range 字典序范围规范结构体
     * @return 返回符合条件的第一个节点指针，若无匹配则返回NULL
     */
    zskiplistNode *zslFirstInLexRange(zskiplist *zsl, zlexrangespec *range);

    /**
     * 获取跳跃表中字典序范围内的最后一个节点
     * @param zsl 目标跳跃表指针
     * @param range 字典序范围规范结构体
     * @return 返回符合条件的最后一个节点指针，若无匹配则返回NULL
     */
    zskiplistNode *zslLastInLexRange(zskiplist *zsl, zlexrangespec *range);

    /**
     * 判断元素是否大于等于字典序最小值
     * @param value 待判断的元素字符串
     * @param spec 字典序范围规范结构体
     * @return 满足条件返回1，否则返回0
     */
    int zslLexValueGteMin(sds value, zlexrangespec *spec);

    /**
     * 判断元素是否小于等于字典序最大值
     * @param value 待判断的元素字符串
     * @param spec 字典序范围规范结构体
     * @return 满足条件返回1，否则返回0
     */
    int zslLexValueLteMax(sds value, zlexrangespec *spec);

    /**
     * 更新跳跃表中元素的分数。
     * 如果分数变化导致元素排序位置改变，会调整元素在跳跃表中的位置。
     * 
     * @param zsl 指向跳跃表的指针
     * @param curscore 当前分数
     * @param ele 元素(SDS字符串)
     * @param newscore 新分数
     * @return 更新后的跳跃表节点指针
     */
    zskiplistNode *zslUpdateScore(zskiplist *zsl, double curscore, sds ele, double newscore);

public:

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
     * 计算SDS字符串的哈希值。
     * 用于字典的哈希函数回调。
     * 
     * @param key 指向SDS字符串的指针
     * @return 计算得到的哈希值
     */
    static uint64_t dictSdsHash(const void *key);

    /**
     * 判断字典是否需要调整大小。
     * 
     * @param dict 指向字典的指针
     * @return 如果需要调整返回1，否则返回0
     */
    int htNeedsResize(dict *dict);
    
    /**
     * 有序集合使用的字典类型定义。
     * 定义了字典操作所需的回调函数，包括哈希函数、键比较函数等。
     * 注意：键和值的内存管理由跳跃表负责，因此dup和destructor回调为NULL。
     */
    dictType zsetDictType = {
        dictSdsHash,               /* 哈希函数 */
        NULL,                      /* 键复制函数（由跳跃表管理） */
        NULL,                      /* 值复制函数（由跳跃表管理） */
        dictSdsKeyCompare,         /* 键比较函数 */
        NULL,                      /* 键销毁函数（由跳跃表管理） */
        NULL,                      /* 值销毁函数（由跳跃表管理） */
        NULL                       /* 扩容判断函数（使用默认策略） */
    };

    /**
     * 定义哈希类型的字典配置
     * 包含了哈希函数、键值比较和析构函数等回调
     * 用于处理以SDS字符串为键值的哈希表
     */
    dictType hashDictType = {
            dictSdsHash,                /* 哈希函数：计算SDS字符串的哈希值 */
            NULL,                       /* 键复制函数：使用默认内存分配 */
            NULL,                       /* 值复制函数：使用默认内存分配 */
            dictSdsKeyCompare,          /* 键比较函数：比较两个SDS字符串 */
            dictSdsDestructor,          /* 键析构函数：释放SDS内存 */
            dictSdsDestructor,          /* 值析构函数：释放SDS内存 */
            NULL                        /* 扩展允许函数：使用默认扩展策略 */
        };

    /**
     * 按字典序比较两个SDS字符串
     * @param a 第一个SDS字符串
     * @param b 第二个SDS字符串
     * @return 比较结果：a<b返回负数，a==b返回0，a>b返回正数
     */
    int sdscmplex(sds a, sds b);


    /**
     * 释放SDS字符串的内存
     * @param privdata 私有数据（未使用）
     * @param val 待释放的SDS字符串指针
     */
    static void dictSdsDestructor(void *privdata, void *val);
private:
    sdsCreate *sdsCreateInstance;
    ziplistCreate *ziplistCreateInstance;
    toolFunc* toolFuncInstance;
    zskiplistCreate* zskiplistCreateInstance;
    dictionaryCreate* dictionaryCreateInstance;
    redisObjectCreate* redisObjectCreateInstance;
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
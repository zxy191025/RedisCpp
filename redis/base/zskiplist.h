/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: zskiplist implementation
 */
#ifndef REDIS_BASE_ZSKIPLIST_H
#define REDIS_BASE_ZSKIPLIST_H
#include "dict.h"
typedef char *sds;
class sdsCreate;
#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */

typedef struct zskiplistNode {
    sds ele;               // 存储的元素字符串
    double score;          // 排序分数
    struct zskiplistNode *backward; // 指向前一个节点的指针
    struct zskiplistLevel {
        struct zskiplistNode *forward; // 指向下一个节点的指针
        unsigned long span;            // 两节点间的元素数量
    } level[];                         // 层级数组（动态长度）
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail; // 头节点和尾节点指针
    unsigned long length;                // 节点数量（不含头节点）
    int level;                           // 最大层级数
} zskiplist;

typedef struct {
    double min, max;    // 范围的最小值和最大值
    int minex, maxex;   // 边界是否排除（1=排除，0=包含）
} zrangespec;

typedef struct {
    sds min, max;       // 范围的最小和最大字符串
    int minex, maxex;   // 边界是否排除（1=排除，0=包含）
} zlexrangespec;

// typedef struct zset {
//     dict *dict;
//     zskiplist *zsl;
// } zset;

class zskiplistCreate
{
public:
    zskiplistCreate();
    ~zskiplistCreate();
//跳跃表（Skiplist）操作
public:
    /**
     * 创建一个新的跳跃表数据结构
     * @return 返回指向新创建跳跃表的指针
     */
    zskiplist *zslCreate(void);

    /**
     * 释放跳跃表及其所有节点占用的内存
     * @param zsl 指向待释放跳跃表的指针
     */
    void zslFree(zskiplist *zsl);

    /**
     * 向跳跃表中插入新节点
     * @param zsl 目标跳跃表指针
     * @param score 新节点的排序分数
     * @param ele 新节点存储的元素字符串
     * @return 返回新插入节点的指针
     */
    zskiplistNode *zslInsert(zskiplist *zsl, double score, sds ele);

    /**
     * 从跳跃表中删除指定节点
     * @param zsl 目标跳跃表指针
     * @param score 待删除节点的分数
     * @param ele 待删除节点的元素值
     * @param node 若不为NULL，用于存储被删除节点的指针（可用于后续操作）
     * @return 成功删除返回1，未找到节点返回0
     */
    int zslDelete(zskiplist *zsl, double score, sds ele, zskiplistNode **node);

    /**
     * 获取分数范围内的第一个节点（按升序排列）
     * @param zsl 目标跳跃表指针
     * @param range 分数范围规范结构体，包含min/max及边界规则
     * @return 返回符合范围的第一个节点指针，若无匹配则返回NULL
     */
    zskiplistNode *zslFirstInRange(zskiplist *zsl, zrangespec *range);

    /**
     * 获取分数范围内的最后一个节点（按升序排列）
     * @param zsl 目标跳跃表指针
     * @param range 分数范围规范结构体，包含min/max及边界规则
     * @return 返回符合范围的最后一个节点指针，若无匹配则返回NULL
     */
    zskiplistNode *zslLastInRange(zskiplist *zsl, zrangespec *range);

    /**
     * 判断值是否大于等于范围最小值
     * @param value 待判断的分数值
     * @param spec 分数范围规范结构体
     * @return 满足条件返回1，否则返回0
     */
    int zslValueGteMin(double value, zrangespec *spec);

    /**
     * 判断值是否小于等于范围最大值
     * @param value 待判断的分数值
     * @param spec 分数范围规范结构体
     * @return 满足条件返回1，否则返回0
     */
    int zslValueLteMax(double value, zrangespec *spec);

    /**
     * 获取节点在跳跃表中的排名（从0开始，按升序计算）
     * @param zsl 目标跳跃表指针
     * @param score 节点的分数
     * @param o 节点的元素值
     * @return 返回节点的排名，未找到节点返回0
     */
    unsigned long zslGetRank(zskiplist *zsl, double score, sds o);

// //压缩列表（Ziplist）操作
// public:
//     /**
//      * 向压缩列表中插入元素和分数
//      * @param zl 目标压缩列表指针
//      * @param ele 待插入的元素字符串
//      * @param score 待插入的分数值
//      * @return 返回新的压缩列表指针（可能已重新分配内存）
//      */
//     unsigned char *zzlInsert(unsigned char *zl, sds ele, double score);

//     /**
//      * 从压缩列表中获取元素的分数值
//      * @param sptr 指向分数存储位置的指针（通过zzlNext/zzlPrev获取）
//      * @return 返回解析出的分数值
//      */
//     double zzlGetScore(unsigned char *sptr);

//     /**
//      * 移动到压缩列表的下一个元素
//      * @param zl 压缩列表指针
//      * @param eptr 输出参数，指向当前元素的指针
//      * @param sptr 输出参数，指向当前元素分数的指针
//      */
//     void zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

//     /**
//      * 移动到压缩列表的前一个元素
//      * @param zl 压缩列表指针
//      * @param eptr 输出参数，指向当前元素的指针
//      * @param sptr 输出参数，指向当前元素分数的指针
//      */
//     void zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr);

//     /**
//      * 获取压缩列表中分数范围内的第一个元素
//      * @param zl 压缩列表指针
//      * @param range 分数范围规范结构体，包含min/max及边界规则
//      * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
//      */
//     unsigned char *zzlFirstInRange(unsigned char *zl, zrangespec *range);

//     /**
//      * 获取压缩列表中分数范围内的最后一个元素
//      * @param zl 压缩列表指针
//      * @param range 分数范围规范结构体，包含min/max及边界规则
//      * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
//      */
//     unsigned char *zzlLastInRange(unsigned char *zl, zrangespec *range);


    
// //有序集合（zset）通用操作
// public:
//     /**
//      * 获取有序集合的元素数量
//      * @param zobj 有序集合对象指针
//      * @return 返回元素数量
//      */
//     unsigned long zsetLength(const robj *zobj);

//     /**
//      * 转换有序集合的编码方式（如压缩列表与跳跃表互转）
//      * @param zobj 有序集合对象指针
//      * @param encoding 目标编码类型（OBJ_ENCODING_ZIPLIST 或 OBJ_ENCODING_SKIPLIST）
//      */
//     void zsetConvert(robj *zobj, int encoding);

//     /**
//      * 当满足条件时自动将有序集合转换为压缩列表编码
//      * @param zobj 有序集合对象指针
//      * @param maxelelen 元素最大长度阈值
//      * @param totelelen 集合总长度阈值
//      */
//     void zsetConvertToZiplistIfNeeded(robj *zobj, size_t maxelelen, size_t totelelen);

//     /**
//      * 获取有序集合中成员的分数
//      * @param zobj 有序集合对象指针
//      * @param member 待查询的成员名称
//      * @param score 输出参数，存储查询到的分数值
//      * @return 成员存在返回1，不存在返回0
//      */
//     int zsetScore(robj *zobj, sds member, double *score);

//     /**
//      * 向有序集合中添加或更新成员的分数
//      * @param zobj 有序集合对象指针
//      * @param score 新分数值
//      * @param ele 成员名称
//      * @param in_flags 输入标志（如ZADD_IN_FLAG_NX表示不更新已存在成员）
//      * @param out_flags 输出标志（如ZADD_OUT_FLAG_ADDED表示成功添加）
//      * @param newscore 输出参数，存储最终生效的分数值
//      * @return 操作成功返回1，失败返回0
//      */
//     int zsetAdd(robj *zobj, double score, sds ele, int in_flags, int *out_flags, double *newscore);

//     /**
//      * 获取有序集合中成员的排名（支持升序/降序）
//      * @param zobj 有序集合对象指针
//      * @param ele 成员名称
//      * @param reverse 排序方向（1表示降序，0表示升序）
//      * @return 返回成员排名（从0开始），成员不存在返回-1
//      */
//     long zsetRank(robj *zobj, sds ele, int reverse);

//     /**
//      * 从有序集合中删除成员
//      * @param zobj 有序集合对象指针
//      * @param ele 待删除的成员名称
//      * @return 成功删除返回1，成员不存在返回0
//      */
//     int zsetDel(robj *zobj, sds ele);

//     /**
//      * 复制有序集合对象
//      * @param o 源有序集合对象指针
//      * @return 返回新复制的有序集合对象指针
//      */
//     robj *zsetDup(robj *o);

// //字典序（Lexicographical）范围操作
// public:
//     /**
//      * 解析字典序范围参数（如"[min" "[max"）
//      * @param min 最小值对象（字符串）
//      * @param max 最大值对象（字符串）
//      * @param spec 输出参数，存储解析后的范围规范结构体
//      * @return 解析成功返回1，参数格式错误返回0
//      */
//     int zslParseLexRange(robj *min, robj *max, zlexrangespec *spec);

//     /**
//      * 释放字典序范围规范结构体占用的内存
//      * @param spec 待释放的范围规范结构体指针
//      */
//     void zslFreeLexRange(zlexrangespec *spec);

//     /**
//      * 获取跳跃表中字典序范围内的第一个节点
//      * @param zsl 目标跳跃表指针
//      * @param range 字典序范围规范结构体
//      * @return 返回符合条件的第一个节点指针，若无匹配则返回NULL
//      */
//     zskiplistNode *zslFirstInLexRange(zskiplist *zsl, zlexrangespec *range);

//     /**
//      * 获取跳跃表中字典序范围内的最后一个节点
//      * @param zsl 目标跳跃表指针
//      * @param range 字典序范围规范结构体
//      * @return 返回符合条件的最后一个节点指针，若无匹配则返回NULL
//      */
//     zskiplistNode *zslLastInLexRange(zskiplist *zsl, zlexrangespec *range);

//     /**
//      * 获取压缩列表中字典序范围内的第一个元素
//      * @param zl 压缩列表指针
//      * @param range 字典序范围规范结构体
//      * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
//      */
//     unsigned char *zzlFirstInLexRange(unsigned char *zl, zlexrangespec *range);

//     /**
//      * 获取压缩列表中字典序范围内的最后一个元素
//      * @param zl 压缩列表指针
//      * @param range 字典序范围规范结构体
//      * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
//      */
//     unsigned char *zzlLastInLexRange(unsigned char *zl, zlexrangespec *range);

//     /**
//      * 判断元素是否大于等于字典序最小值
//      * @param value 待判断的元素字符串
//      * @param spec 字典序范围规范结构体
//      * @return 满足条件返回1，否则返回0
//      */
//     int zslLexValueGteMin(sds value, zlexrangespec *spec);

//     /**
//      * 判断元素是否小于等于字典序最大值
//      * @param value 待判断的元素字符串
//      * @param spec 字典序范围规范结构体
//      * @return 满足条件返回1，否则返回0
//      */
//     int zslLexValueLteMax(sds value, zlexrangespec *spec);

// //其他辅助函数
// public:
//     /**
//      * 验证压缩列表编码的有序集合完整性
//      * @param zl 压缩列表指针
//      * @param size 压缩列表大小（字节数）
//      * @param deep 是否进行深度验证（验证每个元素内容）
//      * @return 验证通过返回1，发现错误返回0
//      */
//     int zsetZiplistValidateIntegrity(unsigned char *zl, size_t size, int deep);

//     /**
//      * 处理ZPOP系列命令（如ZPOPMIN/ZPOPMAX）
//      * @param c 客户端连接对象
//      * @param keyv 键名数组
//      * @param keyc 键名数量
//      * @param where 弹出方向（ZPOP_MIN或ZPOP_MAX）
//      * @param emitkey 是否返回键名（用于多键操作）
//      * @param countarg 弹出数量参数对象
//      */
//     void genericZpopCommand(client *c, robj **keyv, int keyc, int where, int emitkey, robj *countarg);

//     /**
//      * 从压缩列表中提取元素对象
//      * @param sptr 指向元素存储位置的指针
//      * @return 返回解析出的字符串对象
//      */
//     sds ziplistGetObject(unsigned char *sptr);


public:
    /**
     * 创建一个新的跳跃表节点
     * 
     * @param level 节点的层级数（随机生成，通常为1~32之间的整数）
     * @param score 节点的排序分数
     * @param ele 节点存储的元素（字符串）
     * @return 返回新创建的节点指针，失败时返回NULL
     */
    zskiplistNode *zslCreateNode(int level, double score, sds ele);

    /**
     * 释放跳跃表节点占用的内存
     * 
     * @param node 待释放的节点指针
     */
    void zslFreeNode(zskiplistNode *node);

    /**
     * 检查跳跃表中是否存在分数在指定范围内的节点
     * 
     * @param zsl 目标跳跃表
     * @param range 分数范围规范（包含min、max及边界标志）
     * @return 存在符合条件的节点返回1，否则返回0
     */
    int zslIsInRange(zskiplist *zsl, zrangespec *range);

    /**
     * 生成随机层级数（用于新节点插入）
     * 
     * @return 返回1~ZSKIPLIST_MAXLEVEL之间的随机整数，服从幂次分布
     */
    int zslRandomLevel(void);

    /**
     * 从跳跃表中删除指定节点
     * 
     * @param zsl 目标跳跃表
     * @param x 待删除的节点
     * @param update 预先记录的层级更新数组（由zslInsert或查找操作生成）
     */
    void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update);
    
private:
    sdsCreate *sdsCreateInstance;
};
#endif
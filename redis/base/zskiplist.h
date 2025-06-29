/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: zskiplist implementation
 */
#ifndef REDIS_BASE_ZSKIPLIST_H
#define REDIS_BASE_ZSKIPLIST_H
#include "dict.h"
#include <cstddef>
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
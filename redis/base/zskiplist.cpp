/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: zskiplist implementation
 */
#include "zskiplist.h"
#include "zmallocDf.h"
#include "sds.h"
zskiplistCreate::zskiplistCreate()
{
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
}
zskiplistCreate::~zskiplistCreate()
{
    zfree(sdsCreateInstance);
}
/**
 * 创建一个新的跳跃表节点
 * 
 * @param level 节点的层级数（随机生成，通常为1~32之间的整数）
 * @param score 节点的排序分数
 * @param ele 节点存储的元素（字符串）
 * @return 返回新创建的节点指针，失败时返回NULL
 */
zskiplistNode *zskiplistCreate::zslCreateNode(int level, double score, sds ele) 
{
    zskiplistNode *zn = static_cast<zskiplistNode *>(zmalloc(sizeof(*zn)+level*sizeof(struct zskiplistNode::zskiplistLevel)));
    zn->score = score;
    zn->ele = ele;
    return zn;
}


/**
 * 释放跳跃表节点占用的内存
 * 
 * @param node 待释放的节点指针
 */
void zskiplistCreate::zslFreeNode(zskiplistNode *node) 
{
    sdsCreateInstance->sdsfree(node->ele);
    zfree(node);
}

/**
 * 检查跳跃表中是否存在分数在指定范围内的节点
 * 
 * @param zsl 目标跳跃表
 * @param range 分数范围规范（包含min、max及边界标志）
 * @return 存在符合条件的节点返回1，否则返回0
 */
int zskiplistCreate::zslIsInRange(zskiplist *zsl, zrangespec *range) 
{
    zskiplistNode *x;

    /* Test for ranges that will always be empty. */
    if (range->min > range->max ||
            (range->min == range->max && (range->minex || range->maxex)))
        return 0;
    x = zsl->tail;
    if (x == NULL || !zslValueGteMin(x->score,range))
        return 0;
    x = zsl->header->level[0].forward;
    if (x == NULL || !zslValueLteMax(x->score,range))
        return 0;
    return 1;
}

/**
 * 生成随机层级数（用于新节点插入）
 * 
 * @return 返回1~ZSKIPLIST_MAXLEVEL之间的随机整数，服从幂次分布
 */
int zskiplistCreate::zslRandomLevel(void) 
{
    int level = 1;
    while ((random()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/**
 * 从跳跃表中删除指定节点
 * 
 * @param zsl 目标跳跃表
 * @param x 待删除的节点
 * @param update 预先记录的层级更新数组（由zslInsert或查找操作生成）
 */
void zskiplistCreate::zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) 
{
    int i;
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        zsl->tail = x->backward;
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

/**
 * 创建一个新的跳跃表数据结构
 * @return 返回指向新创建跳跃表的指针
 */
zskiplist *zskiplistCreate::zslCreate(void)
{
    int j;
    zskiplist *zsl;

    zsl = static_cast<zskiplist *>(zmalloc(sizeof(*zsl)));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,NULL);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
        zsl->header->level[j].span = 0;
    }
    zsl->header->backward = NULL;
    zsl->tail = NULL;
    return zsl;
}

/**
 * 释放跳跃表及其所有节点占用的内存
 * @param zsl 指向待释放跳跃表的指针
 */
void zskiplistCreate::zslFree(zskiplist *zsl)
{
    zskiplistNode *node = zsl->header->level[0].forward, *next;

    zfree(zsl->header);
    while(node) {
        next = node->level[0].forward;
        zslFreeNode(node);
        node = next;
    }
    zfree(zsl);
}

/**
 * 向跳跃表中插入新节点
 * @param zsl 目标跳跃表指针
 * @param score 新节点的排序分数
 * @param ele 新节点存储的元素字符串
 * @return 返回新插入节点的指针
 */
zskiplistNode *zskiplistCreate::zslInsert(zskiplist *zsl, double score, sds ele)
{
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    unsigned int rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    //serverAssert(!isnan(score));//delete by zhenjia.zhao 2025/06/27 后期要加
    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward &&
                (x->level[i].forward->score < score ||
                    (x->level[i].forward->score == score &&
                    sdsCreateInstance->sdscmp(x->level[i].forward->ele,ele) < 0)))
        {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the element is not already inside, since we allow duplicated
     * scores, reinserting the same element should never happen since the
     * caller of zslInsert() should test in the hash table if the element is
     * already inside or not. */
    level = zslRandomLevel();
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = zsl->header;
            update[i]->level[i].span = zsl->length;
        }
        zsl->level = level;
    }
    x = zslCreateNode(level,score,ele);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < zsl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == zsl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        zsl->tail = x;
    zsl->length++;
    return x;
}

/**
 * 从跳跃表中删除指定节点
 * @param zsl 目标跳跃表指针
 * @param score 待删除节点的分数
 * @param ele 待删除节点的元素值
 * @param node 若不为NULL，用于存储被删除节点的指针（可用于后续操作）
 * @return 成功删除返回1，未找到节点返回0
 */
int zskiplistCreate::zslDelete(zskiplist *zsl, double score, sds ele, zskiplistNode **node)
{
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                (x->level[i].forward->score < score ||
                    (x->level[i].forward->score == score &&
                     sdsCreateInstance->sdscmp(x->level[i].forward->ele,ele) < 0)))
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score && sdsCreateInstance->sdscmp(x->ele,ele) == 0) {
        zslDeleteNode(zsl, x, update);
        if (!node)
            zslFreeNode(x);
        else
            *node = x;
        return 1;
    }
    return 0; /* not found */
}

/**
 * 获取分数范围内的第一个节点（按升序排列）
 * @param zsl 目标跳跃表指针
 * @param range 分数范围规范结构体，包含min/max及边界规则
 * @return 返回符合范围的第一个节点指针，若无匹配则返回NULL
 */
zskiplistNode *zskiplistCreate::zslFirstInRange(zskiplist *zsl, zrangespec *range)
{
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl,range)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward &&
            !zslValueGteMin(x->level[i].forward->score,range))
                x = x->level[i].forward;
    }

    /* This is an inner range, so the next node cannot be NULL. */
    x = x->level[0].forward;
    //serverAssert(x != NULL);//delete by zhenjia.zhao 2025/06/27 后期要加

    /* Check if score <= max. */
    if (!zslValueLteMax(x->score,range)) return NULL;
    return x;
}

/**
 * 获取分数范围内的最后一个节点（按升序排列）
 * @param zsl 目标跳跃表指针
 * @param range 分数范围规范结构体，包含min/max及边界规则
 * @return 返回符合范围的最后一个节点指针，若无匹配则返回NULL
 */
zskiplistNode *zskiplistCreate::zslLastInRange(zskiplist *zsl, zrangespec *range)
{
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl,range)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward &&
            zslValueLteMax(x->level[i].forward->score,range))
                x = x->level[i].forward;
    }

    /* This is an inner range, so this node cannot be NULL. */
    //serverAssert(x != NULL);//delete by zhenjia.zhao 2025/06/27 后期要加

    /* Check if score >= min. */
    if (!zslValueGteMin(x->score,range)) return NULL;
    return x;
}

/**
 * 判断值是否大于等于范围最小值
 * @param value 待判断的分数值
 * @param spec 分数范围规范结构体
 * @return 满足条件返回1，否则返回0
 */
int zskiplistCreate::zslValueGteMin(double value, zrangespec *spec)
{
    return spec->minex ? (value > spec->min) : (value >= spec->min);
}

/**
 * 判断值是否小于等于范围最大值
 * @param value 待判断的分数值
 * @param spec 分数范围规范结构体
 * @return 满足条件返回1，否则返回0
 */
int zskiplistCreate::zslValueLteMax(double value, zrangespec *spec)
{
    return spec->maxex ? (value < spec->max) : (value <= spec->max);
}

/**
 * 获取节点在跳跃表中的排名（从0开始，按升序计算）
 * @param zsl 目标跳跃表指针
 * @param score 节点的分数
 * @param o 节点的元素值
 * @return 返回节点的排名，未找到节点返回0
 */
unsigned long zskiplistCreate::zslGetRank(zskiplist *zsl, double score, sds o)
{
    zskiplistNode *x;
    unsigned long rank = 0;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
            (x->level[i].forward->score < score ||
                (x->level[i].forward->score == score &&
                sdsCreateInstance->sdscmp(x->level[i].forward->ele,o) <= 0))) {
            rank += x->level[i].span;
            x = x->level[i].forward;
        }

        /* x might be equal to zsl->header, so test if obj is non-NULL */
        if (x->ele && sdsCreateInstance->sdscmp(x->ele,o) == 0) {
            return rank;
        }
    }
    return 0;
}
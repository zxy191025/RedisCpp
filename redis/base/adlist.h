/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: Linked list implementation
 */
#ifndef __ADLIST_H__
#define __ADLIST_H__

typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

typedef struct listIter {
    listNode *next;
    int direction;
} listIter;

typedef struct list {
    listNode *head;
    listNode *tail;
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);
    unsigned long len;
} list;

#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))

#define listGetDupMethod(l) ((l)->dup)
#define listGetFreeMethod(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

class adlistCreate
{
public:
    adlistCreate() = default;
    ~adlistCreate() = default;
public:
    /**
     * 创建一个新链表
     * @return 成功返回链表指针，失败返回NULL
     * 
     * 注意：创建后需要通过listSetDupMethod()、listSetFreeMethod()等设置回调函数
     */
    list *listCreate(void);

    /**
     * 释放整个链表及其所有节点
     * @param list 要释放的链表指针
     * 
     * 操作步骤：
     * 1. 遍历链表，对每个节点调用free方法释放值
     * 2. 释放节点本身
     * 3. 释放链表结构
     */
    void listRelease(list *list);

    /**
     * 清空链表中的所有节点，但保留链表结构
     * @param list 要清空的链表指针
     * 
     * 与listRelease的区别：此函数保留链表结构，仅删除所有节点
     */
    void listEmpty(list *list);

    /**
     * 在链表头部添加新节点
     * @param list 目标链表
     * @param value 新节点的值(存储为void*)
     * @return 成功返回链表指针，失败返回NULL
     */
    list *listAddNodeHead(list *list, void *value);

    /**
     * 在链表尾部添加新节点
     * @param list 目标链表
     * @param value 新节点的值(存储为void*)
     * @return 成功返回链表指针，失败返回NULL
     */
    list *listAddNodeTail(list *list, void *value);

    /**
     * 在指定节点前后插入新节点
     * @param list 目标链表
     * @param old_node 参考节点
     * @param value 新节点的值
     * @param after 插入位置：1表示在old_node后，0表示在old_node前
     * @return 成功返回链表指针，失败返回NULL
     */
    list *listInsertNode(list *list, listNode *old_node, void *value, int after);

    /**
     * 删除指定节点
     * @param list 目标链表
     * @param node 要删除的节点
     * 
     * 注意：会调用free方法释放节点的值
     */
    void listDelNode(list *list, listNode *node);

    /**
     * 获取链表迭代器
     * @param list 目标链表
     * @param direction 迭代方向：AL_START_HEAD(从头开始) 或 AL_START_TAIL(从尾开始)
     * @return 迭代器指针，失败返回NULL
     */
    listIter *listGetIterator(list *list, int direction);

    /**
     * 获取迭代器的下一个节点，并移动迭代器位置
     * @param iter 迭代器指针
     * @return 下一个节点指针，迭代结束返回NULL
     */
    listNode *listNext(listIter *iter);

    /**
     * 释放迭代器资源
     * @param iter 要释放的迭代器指针
     */
    void listReleaseIterator(listIter *iter);

    /**
     * 复制整个链表(深拷贝)
     * @param orig 原链表
     * @return 新链表指针，失败返回NULL
     * 
     * 注意：会调用dup方法复制节点的值，如果未设置则直接复制指针
     */
    list *listDup(list *orig);

    /**
     * 根据键值搜索节点
     * @param list 目标链表
     * @param key 搜索键值
     * @return 找到返回节点指针，未找到返回NULL
     * 
     * 注意：使用listSetMatchMethod()设置的匹配函数进行比较
     */
    listNode *listSearchKey(list *list, void *key);

    /**
     * 根据索引获取节点
     * @param list 目标链表
     * @param index 索引值：
     *              - 正数：从头部开始(0表示头节点)
     *              - 负数：从尾部开始(-1表示尾节点)
     * @return 找到返回节点指针，超出范围返回NULL
     */
    listNode *listIndex(list *list, long index);

    /**
     * 重置迭代器为从头开始
     * @param list 目标链表
     * @param li 迭代器指针
     */
    void listRewind(list *list, listIter *li);

    /**
     * 重置迭代器为从尾开始
     * @param list 目标链表
     * @param li 迭代器指针
     */
    void listRewindTail(list *list, listIter *li);

    /**
     * 将尾节点移动到头部(链表旋转)
     * @param list 目标链表
     */
    void listRotateTailToHead(list *list);

    /**
     * 将头节点移动到尾部(链表旋转)
     * @param list 目标链表
     */
    void listRotateHeadToTail(list *list);

    /**
     * 将另一个链表合并到当前链表尾部
     * @param l 目标链表(合并后o会被清空但保留结构)
     * @param o 要合并的源链表
     * 
     * 注意：合并后o链表会被清空但不会释放，需要调用者手动释放
     */
    void listJoin(list *l, list *o);
};
#endif
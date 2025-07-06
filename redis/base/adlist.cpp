/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: adlist（A Generic Doubly Linked List）是一个通用的双向链表实现，提供了高效的节点插入、删除和遍历操作。
 * 它是 Redis 核心数据结构之一，被广泛用于实现列表类型（如 LIST 数据类型）、发布订阅系统、客户端连接管理等功能。
 */

#include <stdlib.h>
#include "adlist.h"
#include "zmallocDf.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
/**
 * 创建一个新链表
 * @return 成功返回链表指针，失败返回NULL
 * 
 * 注意：创建后需要通过listSetDupMethod()、listSetFreeMethod()等设置回调函数
 */
list *adlistCreate::listCreate(void)
{
    struct list *listl;

    if ((listl = static_cast<list*>(zmalloc(sizeof(*listl)))) == NULL)
        return NULL;
    listl->head = listl->tail = NULL;
    listl->len = 0;
    listl->dup = NULL;
    listl->free = NULL;
    listl->match = NULL;
    return listl;
}
/**
 * 清空链表中的所有节点，但保留链表结构
 * @param list 要清空的链表指针
 * 
 * 与listRelease的区别：此函数保留链表结构，仅删除所有节点
 */
/* Remove all the elements from the list without destroying the list itself. */
void adlistCreate::listEmpty(list *list)
{
    unsigned long len;
    listNode *current, *next;

    current = list->head;
    len = list->len;
    while(len--) {
        next = current->next;
        if (list->free) list->free(current->value);
        zfree(current);
        current = next;
    }
    list->head = list->tail = NULL;
    list->len = 0;
}
/**
 * 释放整个链表及其所有节点
 * @param list 要释放的链表指针
 * 
 * 操作步骤：
 * 1. 遍历链表，对每个节点调用free方法释放值
 * 2. 释放节点本身
 * 3. 释放链表结构
 */
/* Free the whole list.
 *
 * This function can't fail. */
void adlistCreate::listRelease(list *list)
{
    listEmpty(list);
    zfree(list);
}
/**
 * 在链表头部添加新节点
 * @param list 目标链表
 * @param value 新节点的值(存储为void*)
 * @return 成功返回链表指针，失败返回NULL
 */
/* Add a new node to the list, to head, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
list *adlistCreate::listAddNodeHead(list *list, void *value)
{
    listNode *node;
    if ((node = static_cast<listNode*>(zmalloc(sizeof(*node)))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = NULL;
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    list->len++;
    return list;
}
/**
 * 在链表尾部添加新节点
 * @param list 目标链表
 * @param value 新节点的值(存储为void*)
 * @return 成功返回链表指针，失败返回NULL
 */
/* Add a new node to the list, to tail, containing the specified 'value'
 * pointer as value.
 *
 * On error, NULL is returned and no operation is performed (i.e. the
 * list remains unaltered).
 * On success the 'list' pointer you pass to the function is returned. */
list *adlistCreate::listAddNodeTail(list *list, void *value)
{
    listNode *node;

    if ((node = static_cast<listNode*>(zmalloc(sizeof(*node)))) == NULL)
        return NULL;
    node->value = value;
    if (list->len == 0) {
        list->head = list->tail = node;
        node->prev = node->next = NULL;
    } else {
        node->prev = list->tail;
        node->next = NULL;
        list->tail->next = node;
        list->tail = node;
    }
    list->len++;
    return list;
}
/**
 * 在指定节点前后插入新节点
 * @param list 目标链表
 * @param old_node 参考节点
 * @param value 新节点的值
 * @param after 插入位置：1表示在old_node后，0表示在old_node前
 * @return 成功返回链表指针，失败返回NULL
 */
list *adlistCreate::listInsertNode(list *list, listNode *old_node, void *value, int after) {
    listNode *node;

    if ((node = static_cast<listNode*>(zmalloc(sizeof(*node)))) == NULL)
        return NULL;
    node->value = value;
    if (after) {
        node->prev = old_node;
        node->next = old_node->next;
        if (list->tail == old_node) {
            list->tail = node;
        }
    } else {
        node->next = old_node;
        node->prev = old_node->prev;
        if (list->head == old_node) {
            list->head = node;
        }
    }
    if (node->prev != NULL) {
        node->prev->next = node;
    }
    if (node->next != NULL) {
        node->next->prev = node;
    }
    list->len++;
    return list;
}
/**
 * 删除指定节点
 * @param list 目标链表
 * @param node 要删除的节点
 * 
 * 注意：会调用free方法释放节点的值
 */
/* Remove the specified node from the specified list.
 * It's up to the caller to free the private value of the node.
 *
 * This function can't fail. */
void adlistCreate::listDelNode(list *list, listNode *node)
{
    if (node->prev)
        node->prev->next = node->next;
    else
        list->head = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        list->tail = node->prev;
    if (list->free) list->free(node->value);
    zfree(node);
    list->len--;
}
/**
 * 获取链表迭代器
 * @param list 目标链表
 * @param direction 迭代方向：AL_START_HEAD(从头开始) 或 AL_START_TAIL(从尾开始)
 * @return 迭代器指针，失败返回NULL
 */
/* Returns a list iterator 'iter'. After the initialization every
 * call to listNext() will return the next element of the list.
 *
 * This function can't fail. */
listIter *adlistCreate::listGetIterator(list *list, int direction)
{
    listIter *iter;

    if ((iter = static_cast<listIter*>(zmalloc(sizeof(*iter)))) == NULL) return NULL;
    if (direction == AL_START_HEAD)
        iter->next = list->head;
    else
        iter->next = list->tail;
    iter->direction = direction;
    return iter;
}
/**
 * 释放迭代器资源
 * @param iter 要释放的迭代器指针
 */
/* Release the iterator memory */
void adlistCreate::listReleaseIterator(listIter *iter) {
    zfree(iter);
}
/**
 * 重置迭代器为从头开始
 * @param list 目标链表
 * @param li 迭代器指针
 */
/* Create an iterator in the list private iterator structure */
void adlistCreate::listRewind(list *list, listIter *li) {
    li->next = list->head;
    li->direction = AL_START_HEAD;
}
/**
 * 重置迭代器为从尾开始
 * @param list 目标链表
 * @param li 迭代器指针
 */
void adlistCreate::listRewindTail(list *list, listIter *li) {
    li->next = list->tail;
    li->direction = AL_START_TAIL;
}
/**
 * 获取迭代器的下一个节点，并移动迭代器位置
 * @param iter 迭代器指针
 * @return 下一个节点指针，迭代结束返回NULL
 */
/* Return the next element of an iterator.
 * It's valid to remove the currently returned element using
 * listDelNode(), but not to remove other elements.
 *
 * The function returns a pointer to the next element of the list,
 * or NULL if there are no more elements, so the classical usage
 * pattern is:
 *
 * iter = listGetIterator(list,<direction>);
 * while ((node = listNext(iter)) != NULL) {
 *     doSomethingWith(listNodeValue(node));
 * }
 *
 * */
listNode *adlistCreate::listNext(listIter *iter)
{
    listNode *current = iter->next;

    if (current != NULL) {
        if (iter->direction == AL_START_HEAD)
            iter->next = current->next;
        else
            iter->next = current->prev;
    }
    return current;
}
/**
 * 复制整个链表(深拷贝)
 * @param orig 原链表
 * @return 新链表指针，失败返回NULL
 * 
 * 注意：会调用dup方法复制节点的值，如果未设置则直接复制指针
 */
/* Duplicate the whole list. On out of memory NULL is returned.
 * On success a copy of the original list is returned.
 *
 * The 'Dup' method set with listSetDupMethod() function is used
 * to copy the node value. Otherwise the same pointer value of
 * the original node is used as value of the copied node.
 *
 * The original list both on success or error is never modified. */
list *adlistCreate::listDup(list *orig)
{
    list *copy;
    listIter iter;
    listNode *node;

    if ((copy = listCreate()) == NULL)
        return NULL;
    copy->dup = orig->dup;
    copy->free = orig->free;
    copy->match = orig->match;
    listRewind(orig, &iter);
    while((node = listNext(&iter)) != NULL) {
        void *value;

        if (copy->dup) {
            value = copy->dup(node->value);
            if (value == NULL) {
                listRelease(copy);
                return NULL;
            }
        } else
            value = node->value;
        if (listAddNodeTail(copy, value) == NULL) {
            listRelease(copy);
            return NULL;
        }
    }
    return copy;
}
/**
 * 根据键值搜索节点
 * @param list 目标链表
 * @param key 搜索键值
 * @return 找到返回节点指针，未找到返回NULL
 * 
 * 注意：使用listSetMatchMethod()设置的匹配函数进行比较
 */
/* Search the list for a node matching a given key.
 * The match is performed using the 'match' method
 * set with listSetMatchMethod(). If no 'match' method
 * is set, the 'value' pointer of every node is directly
 * compared with the 'key' pointer.
 *
 * On success the first matching node pointer is returned
 * (search starts from head). If no matching node exists
 * NULL is returned. */
listNode *adlistCreate::listSearchKey(list *list, void *key)
{
    listIter iter;
    listNode *node;

    listRewind(list, &iter);
    while((node = listNext(&iter)) != NULL) {
        if (list->match) {
            if (list->match(node->value, key)) {
                return node;
            }
        } else {
            if (key == node->value) {
                return node;
            }
        }
    }
    return NULL;
}
/**
 * 根据索引获取节点
 * @param list 目标链表
 * @param index 索引值：
 *              - 正数：从头部开始(0表示头节点)
 *              - 负数：从尾部开始(-1表示尾节点)
 * @return 找到返回节点指针，超出范围返回NULL
 */
/* Return the element at the specified zero-based index
 * where 0 is the head, 1 is the element next to head
 * and so on. Negative integers are used in order to count
 * from the tail, -1 is the last element, -2 the penultimate
 * and so on. If the index is out of range NULL is returned. */
listNode *adlistCreate::listIndex(list *list, long index) {
    listNode *n;

    if (index < 0) {
        index = (-index)-1;
        n = list->tail;
        while(index-- && n) n = n->prev;
    } else {
        n = list->head;
        while(index-- && n) n = n->next;
    }
    return n;
}
/**
 * 将尾节点移动到头部(链表旋转)
 * @param list 目标链表
 */
/* Rotate the list removing the tail node and inserting it to the head. */
void adlistCreate::listRotateTailToHead(list *list) {
    if (listLength(list) <= 1) return;

    /* Detach current tail */
    listNode *tail = list->tail;
    list->tail = tail->prev;
    list->tail->next = NULL;
    /* Move it as head */
    list->head->prev = tail;
    tail->prev = NULL;
    tail->next = list->head;
    list->head = tail;
}
/**
 * 将头节点移动到尾部(链表旋转)
 * @param list 目标链表
 */
/* Rotate the list removing the head node and inserting it to the tail. */
void adlistCreate::listRotateHeadToTail(list *list) {
    if (listLength(list) <= 1) return;

    listNode *head = list->head;
    /* Detach current head */
    list->head = head->next;
    list->head->prev = NULL;
    /* Move it as tail */
    list->tail->next = head;
    head->next = NULL;
    head->prev = list->tail;
    list->tail = head;
}
/**
 * 将另一个链表合并到当前链表尾部
 * @param l 目标链表(合并后o会被清空但保留结构)
 * @param o 要合并的源链表
 * 
 * 注意：合并后o链表会被清空但不会释放，需要调用者手动释放
 */
/* Add all the elements of the list 'o' at the end of the
 * list 'l'. The list 'other' remains empty but otherwise valid. */
void adlistCreate::listJoin(list *l, list *o) {
    if (o->len == 0) return;

    o->head->prev = l->tail;

    if (l->tail)
        l->tail->next = o->head;
    else
        l->head = o->head;

    l->tail = o->tail;
    l->len += o->len;

    /* Setup other as an empty list. */
    o->head = o->tail = NULL;
    o->len = 0;
}
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/30
 * All rights reserved. No one may copy or transfer.
 * Description: Quicklist 是一种双向链表与压缩列表（ziplist）结合的数据结构，用于高效存储和操作列表类型（如 LIST 数据类型）。
 * 它平衡了内存效率和操作性能，是 Redis 列表的默认底层实现。
 */
#include <string.h>
#include "quicklist.h"
#include "zmallocDf.h"
#include "config.h"
#include "ziplist.h"
#include "toolFunc.h"
#include <assert.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
#if defined(REDIS_TEST) || defined(REDIS_TEST_VERBOSE)
#include <stdio.h> /* for printf (debug printing), snprintf (genstr) */
#endif

#ifndef REDIS_STATIC
#define REDIS_STATIC static
#endif
static ziplistCreate ziplistCreatel;
/* Optimization levels for size-based filling.
 * Note that the largest possible limit is 16k, so even if each record takes
 * just one byte, it still won't overflow the 16 bit count field. */
static const size_t optimization_level[] = {4096, 8192, 16384, 32768, 65536};

/* Maximum size in bytes of any multi-element ziplist.
 * Larger values will live in their own isolated ziplists.
 * This is used only if we're limited by record count. when we're limited by
 * size, the maximum limit is bigger, but still safe.
 * 8k is a recommended / default size limit */
#define SIZE_SAFETY_LIMIT 8192

/* Minimum ziplist size in bytes for attempting compression. */
#define MIN_COMPRESS_BYTES 48

/* Minimum size reduction in bytes to store compressed quicklistNode data.
 * This also prevents us from storing compression if the compression
 * resulted in a larger size than the original data. */
#define MIN_COMPRESS_IMPROVE 8

/* If not verbose testing, remove all debug printing. */
#ifndef REDIS_TEST_VERBOSE
#define D(...)
#else
#define D(...)                                                                 \
    do {                                                                       \
        printf("%s:%s:%d:\t", __FILE__, __func__, __LINE__);                   \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
    } while (0)
#endif

/* Bookmarks forward declarations */
#define QL_MAX_BM ((1 << QL_BM_BITS)-1)

/* Simple way to give quicklistEntry structs default values with one call. */
#define initEntry(e)                                                           \
    do {                                                                       \
        (e)->zi = (e)->value = NULL;                                           \
        (e)->longval = -123456789;                                             \
        (e)->quicklistl = NULL;                                                 \
        (e)->node = NULL;                                                      \
        (e)->offset = 123456789;                                               \
        (e)->sz = 0;                                                           \
    } while (0)

quicklistCreate::quicklistCreate()
{
    ziplistCreateInstance = static_cast<ziplistCreate *>(zmalloc(sizeof(ziplistCreate)));
    toolFuncInstance = static_cast<toolFunc*>(zmalloc(sizeof(toolFunc)));
}

quicklistCreate::~quicklistCreate()
{
    zfree(ziplistCreateInstance);
    zfree(toolFuncInstance);
}

/**
 * 创建一个新的 quicklist
 * 
 * @return 返回新创建的 quicklist 指针，失败时返回 NULL
 */
quicklist *quicklistCreate::quicklistCrt(void)
{
    struct quicklist *quicklistl;
    quicklistl =static_cast<quicklist*>(zmalloc(sizeof(quicklist)));
    quicklistl->head = quicklistl->tail = NULL;
    quicklistl->len = 0;
    quicklistl->count = 0;
    quicklistl->compress = 0;
    quicklistl->fill = -2;
    quicklistl->bookmark_count = 0;
    return quicklistl;
}

/**
 * 使用指定参数创建一个新的 quicklist
 * 
 * @param fill     每个节点的填充因子
 * @param compress 压缩深度
 * @return 返回新创建的 quicklist 指针，失败时返回 NULL
 */
quicklist *quicklistCreate::quicklistNew(int fill, int compress)
{
    quicklist *quicklist = quicklistCrt();
    quicklistSetOptions(quicklist, fill, compress);
    return quicklist;
}

/**
 * 设置 quicklist 的压缩深度
 * 
 * @param quicklist 目标 quicklist
 * @param depth     压缩深度值
 */
#define COMPRESS_MAX ((1 << QL_COMP_BITS)-1)
void quicklistCreate::quicklistSetCompressDepth(quicklist *quicklist, int depth)
{
    if (depth > COMPRESS_MAX) {
        depth = COMPRESS_MAX;
    } else if (depth < 0) {
        depth = 0;
    }
    quicklist->compress = depth;
}

/**
 * 设置 quicklist 节点的填充因子
 * 
 * @param quicklist 目标 quicklist
 * @param fill      填充因子值
 */
#define FILL_MAX ((1 << (QL_FILL_BITS-1))-1)
void quicklistCreate::quicklistSetFill(quicklist *quicklist, int fill)
{
    if (fill > FILL_MAX) {
        fill = FILL_MAX;
    } else if (fill < -5) {
        fill = -5;
    }
    quicklist->fill = fill;
}

/**
 * 一次性设置 quicklist 的填充因子和压缩深度
 * 
 * @param quicklist 目标 quicklist
 * @param fill      填充因子值
 * @param depth     压缩深度值
 */
void quicklistCreate::quicklistSetOptions(quicklist *quicklist, int fill, int depth)
{
    quicklistSetFill(quicklist, fill);
    quicklistSetCompressDepth(quicklist, depth);
}

/**
 * 释放 quicklist 占用的内存
 * 
 * @param quicklist 要释放的 quicklist 指针
 */
void quicklistCreate::quicklistRelease(quicklist *quicklist)
{
    unsigned long len;
    quicklistNode *current, *next;

    current = quicklist->head;
    len = quicklist->len;
    while (len--) {
        next = current->next;

        zfree(current->zl);
        quicklist->count -= current->count;

        zfree(current);

        quicklist->len--;
        current = next;
    }
    quicklistBookmarksClear(quicklist);
    zfree(quicklist);
}

/**
 * 在 quicklist 头部插入一个元素
 * 
 * @param quicklist 目标 quicklist
 * @param value     要插入的值指针
 * @param sz        值的大小（字节）
 * @return 成功返回 1，失败返回 0
 */                                                                       
int quicklistCreate::quicklistPushHead(quicklist *quicklist, void *value, const size_t sz)
{
    quicklistNode *orig_head = quicklist->head;
    assert(sz < UINT32_MAX); /* TODO: add support for quicklist nodes that are sds encoded (not zipped) */
    if (likely(_quicklistNodeAllowInsert(quicklist->head, quicklist->fill, sz))) 
    {
        quicklist->head->zl = ziplistCreateInstance->ziplistPush(quicklist->head->zl, static_cast<unsigned char*>(value), sz, ZIPLIST_HEAD);
        quicklistNodeUpdateSz(quicklist->head);
    } else 
    {
        quicklistNode *node = quicklistCreateNode();
        node->zl = ziplistCreateInstance->ziplistPush(ziplistCreateInstance->ziplistNew(), static_cast<unsigned char*>(value), sz, ZIPLIST_HEAD);

        quicklistNodeUpdateSz(node);
        _quicklistInsertNodeBefore(quicklist, quicklist->head, node);
    }
    quicklist->count++;
    quicklist->head->count++;
    return (orig_head != quicklist->head);
}

/**
 * 在 quicklist 尾部插入一个元素
 * 
 * @param quicklist 目标 quicklist
 * @param value     要插入的值指针
 * @param sz        值的大小（字节）
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistPushTail(quicklist *quicklist, void *value, const size_t sz)
{
    quicklistNode *orig_tail = quicklist->tail;
    assert(sz < UINT32_MAX); /* TODO: add support for quicklist nodes that are sds encoded (not zipped) */
    if (likely(
            _quicklistNodeAllowInsert(quicklist->tail, quicklist->fill, sz))) {
        quicklist->tail->zl =
            ziplistCreateInstance->ziplistPush(quicklist->tail->zl,static_cast<unsigned char*>(value), sz, ZIPLIST_TAIL);
        quicklistNodeUpdateSz(quicklist->tail);
    } else {
        quicklistNode *node = quicklistCreateNode();
        node->zl = ziplistCreateInstance->ziplistPush(ziplistCreateInstance->ziplistNew(), static_cast<unsigned char*>(value), sz, ZIPLIST_TAIL);

        quicklistNodeUpdateSz(node);
        _quicklistInsertNodeAfter(quicklist, quicklist->tail, node);
    }
    quicklist->count++;
    quicklist->tail->count++;
    return (orig_tail != quicklist->tail);
}

/**
 * 在 quicklist 指定位置插入一个元素
 * 
 * @param quicklist 目标 quicklist
 * @param value     要插入的值指针
 * @param sz        值的大小（字节）
 * @param where     插入位置（QL_HEAD 或 QL_TAIL）
 */
void quicklistCreate::quicklistPush(quicklist *quicklist, void *value, const size_t sz, int where)
{
    if (where == QUICKLIST_HEAD) {
        quicklistPushHead(quicklist, value, sz);
    } else if (where == QUICKLIST_TAIL) {
        quicklistPushTail(quicklist, value, sz);
    }
}

/**
 * 将一个 ziplist 追加到 quicklist 尾部
 * 
 * @param quicklist 目标 quicklist
 * @param zl        要追加的 ziplist 指针
 */
void quicklistCreate::quicklistAppendZiplist(quicklist *quicklist, unsigned char *zl)
{
    quicklistNode *node = quicklistCreateNode();

    node->zl = zl;
    node->count = ziplistCreateInstance->ziplistLen(node->zl);
    node->sz = ziplistCreateInstance->ziplistBlobLen(zl);

    _quicklistInsertNodeAfter(quicklist, quicklist->tail, node);
    quicklist->count += node->count;
}

/**
 * 从 ziplist 中提取值并追加到 quicklist
 * 
 * @param quicklist 目标 quicklist
 * @param zl        源 ziplist 指针
 * @return 返回操作后的 quicklist 指针
 */
quicklist *quicklistCreate::quicklistAppendValuesFromZiplist(quicklist *quicklist, unsigned char *zl)
{
    unsigned char *value;
    unsigned int sz;
    long long longval;
    char longstr[32] = {0};

    unsigned char *p = ziplistCreateInstance->ziplistIndex(zl, 0);
    while (ziplistCreateInstance->ziplistGet(p, &value, &sz, &longval)) {
        if (!value) {
            /* Write the longval as a string so we can re-add it */
            sz = toolFuncInstance->ll2string(longstr, sizeof(longstr), longval);
            value = (unsigned char *)longstr;
        }
        quicklistPushTail(quicklist, value, sz);
        p = ziplistCreateInstance->ziplistNext(zl, p);
    }
    zfree(zl);
    return quicklist;
}

/**
 * 从 ziplist 创建一个新的 quicklist
 * 
 * @param fill     填充因子
 * @param compress 压缩深度
 * @param zl       源 ziplist 指针
 * @return 返回新创建的 quicklist 指针
 */
quicklist *quicklistCreate::quicklistCreateFromZiplist(int fill, int compress, unsigned char *zl)
{
    return quicklistAppendValuesFromZiplist(quicklistNew(fill, compress), zl);
}

/**
 * 在指定节点后插入一个元素
 * 
 * @param quicklist 目标 quicklist
 * @param node      参考节点
 * @param value     要插入的值指针
 * @param sz        值的大小（字节）
 */
void quicklistCreate::quicklistInsertAfter(quicklist *quicklist, quicklistEntry *node, void *value, const size_t sz)
{
    _quicklistInsert(quicklist, node, value, sz, 1);
}

/**
 * 在指定节点前插入一个元素
 * 
 * @param quicklist 目标 quicklist
 * @param node      参考节点
 * @param value     要插入的值指针
 * @param sz        值的大小（字节）
 */
void quicklistCreate::quicklistInsertBefore(quicklist *quicklist, quicklistEntry *node, void *value, const size_t sz)
{
    _quicklistInsert(quicklist, node, value, sz, 0);
}

/**
 * 删除迭代器当前指向的元素
 * 
 * @param iter  迭代器指针
 * @param entry 要删除的元素
 */
void quicklistCreate::quicklistDelEntry(quicklistIter *iter, quicklistEntry *entry)
{
    quicklistNode *prev = entry->node->prev;
    quicklistNode *next = entry->node->next;
    int deleted_node = quicklistDelIndex((quicklist *)entry->quicklistl,
                                         entry->node, &entry->zi);

    /* after delete, the zi is now invalid for any future usage. */
    iter->zi = NULL;

    /* If current node is deleted, we must update iterator node and offset. */
    if (deleted_node) {
        if (iter->direction == AL_START_HEAD) {
            iter->current = next;
            iter->offset = 0;
        } else if (iter->direction == AL_START_TAIL) {
            iter->current = prev;
            iter->offset = -1;
        }
    }
    /* else if (!deleted_node), no changes needed.
     * we already reset iter->zi above, and the existing iter->offset
     * doesn't move again because:
     *   - [1, 2, 3] => delete offset 1 => [1, 3]: next element still offset 1
     *   - [1, 2, 3] => delete offset 0 => [2, 3]: next element still offset 0
     *  if we deleted the last element at offet N and now
     *  length of this ziplist is N-1, the next call into
     *  quicklistNext() will jump to the next node. */
}

/**
 * 替换指定索引位置的元素
 * 
 * @param quicklist 目标 quicklist
 * @param index     要替换的元素索引
 * @param data      新数据指针
 * @param sz        新数据大小（字节）
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistReplaceAtIndex(quicklist *quicklist, long index, void *data, int sz)
{
    quicklistEntry entry;
    if (likely(quicklistIndex(quicklist, index, &entry))) {
        /* quicklistIndex provides an uncompressed node */
        entry.node->zl = ziplistCreateInstance->ziplistReplace(entry.node->zl, entry.zi,static_cast<unsigned char*>(data), sz);
        quicklistNodeUpdateSz(entry.node);
        quicklistCompress(quicklist, entry.node);
        return 1;
    } else {
        return 0;
    }
}

/**
 * 删除指定范围内的元素
 * 
 * @param quicklist 目标 quicklist
 * @param start     起始索引
 * @param stop      结束索引
 * @return 删除的元素数量
 */
int quicklistCreate::quicklistDelRange(quicklist *quicklist, const long start, const long stop)
{
    if (stop <= 0)
        return 0;

    unsigned long extent = stop; /* range is inclusive of start position */

    if (start >= 0 && extent > (quicklist->count - start)) {
        /* if requesting delete more elements than exist, limit to list size. */
        extent = quicklist->count - start;
    } else if (start < 0 && extent > (unsigned long)(-start)) {
        /* else, if at negative offset, limit max size to rest of list. */
        extent = -start; /* c.f. LREM -29 29; just delete until end. */
    }

    quicklistEntry entry;
    if (!quicklistIndex(quicklist, start, &entry))
        return 0;

    D("Quicklist delete request for start %ld, count %ld, extent: %ld", start,
      count, extent);
    quicklistNode *node = entry.node;

    /* iterate over next nodes until everything is deleted. */
    while (extent) {
        quicklistNode *next = node->next;

        unsigned long del;
        int delete_entire_node = 0;
        if (entry.offset == 0 && extent >= node->count) {
            /* If we are deleting more than the count of this node, we
             * can just delete the entire node without ziplist math. */
            delete_entire_node = 1;
            del = node->count;
        } else if (entry.offset >= 0 && extent + entry.offset >= node->count) {
            /* If deleting more nodes after this one, calculate delete based
             * on size of current node. */
            del = node->count - entry.offset;
        } else if (entry.offset < 0) {
            /* If offset is negative, we are in the first run of this loop
             * and we are deleting the entire range
             * from this start offset to end of list.  Since the Negative
             * offset is the number of elements until the tail of the list,
             * just use it directly as the deletion count. */
            del = -entry.offset;

            /* If the positive offset is greater than the remaining extent,
             * we only delete the remaining extent, not the entire offset.
             */
            if (del > extent)
                del = extent;
        } else {
            /* else, we are deleting less than the extent of this node, so
             * use extent directly. */
            del = extent;
        }

        D("[%ld]: asking to del: %ld because offset: %d; (ENTIRE NODE: %d), "
          "node count: %u",
          extent, del, entry.offset, delete_entire_node, node->count);

        if (delete_entire_node) {
            __quicklistDelNode(quicklist, node);
        } else {
            quicklistDecompressNodeForUse(node);
            node->zl = ziplistCreateInstance->ziplistDeleteRange(node->zl, entry.offset, del);
            quicklistNodeUpdateSz(node);
            node->count -= del;
            quicklist->count -= del;
            quicklistDeleteIfEmpty(quicklist, node);
            if (node)
                quicklistRecompressOnly(quicklist, node);
        }

        extent -= del;

        node = next;

        entry.offset = 0;
    }
    return 1;
}

/**
 * 获取 quicklist 的迭代器
 * 
 * @param quicklist 目标 quicklist
 * @param direction 迭代方向（AL_START_HEAD 或 AL_START_TAIL）
 * @return 返回新创建的迭代器指针
 */
quicklistIter *quicklistCreate::quicklistGetIterator(const quicklist *quicklist, int direction)
{
    quicklistIter *iter;

    iter =static_cast<quicklistIter *>(zmalloc(sizeof(*iter)));

    if (direction == AL_START_HEAD) {
        iter->current = quicklist->head;
        iter->offset = 0;
    } else if (direction == AL_START_TAIL) {
        iter->current = quicklist->tail;
        iter->offset = -1;
    }

    iter->direction = direction;
    iter->quicklistl = quicklist;

    iter->zi = NULL;

    return iter;
}

/**
 * 获取指向指定索引位置的迭代器
 * 
 * @param quicklist 目标 quicklist
 * @param direction 迭代方向
 * @param idx       起始索引
 * @return 返回新创建的迭代器指针
 */
quicklistIter *quicklistCreate::quicklistGetIteratorAtIdx(const quicklist *quicklist, int direction, const long long idx)
{
    quicklistEntry entry;

    if (quicklistIndex(quicklist, idx, &entry)) {
        quicklistIter *base = quicklistGetIterator(quicklist, direction);
        base->zi = NULL;
        base->current = entry.node;
        base->offset = entry.offset;
        return base;
    } else {
        return NULL;
    }
}

/**
 * 获取迭代器的下一个元素
 * 
 * @param iter  迭代器指针
 * @param node  存储元素的结构体
 * @return 成功返回 1，到达末尾返回 0
 */
int quicklistCreate::quicklistNext(quicklistIter *iter, quicklistEntry *entry)
{
    initEntry(entry);

    if (!iter) {
        D("Returning because no iter!");
        return 0;
    }

    entry->quicklistl = iter->quicklistl;
    entry->node = iter->current;

    if (!iter->current) {
        D("Returning because current node is NULL")
        return 0;
    }

    unsigned char *(*nextFn)(unsigned char *, unsigned char *) = NULL;
    int offset_update = 0;

    if (!iter->zi) {
        /* If !zi, use current index. */
        quicklistDecompressNodeForUse(iter->current);
        iter->zi = ziplistCreateInstance->ziplistIndex(iter->current->zl, iter->offset);
    } else {
        /* else, use existing iterator offset and get prev/next as necessary. */
        if (iter->direction == AL_START_HEAD) {
            nextFn = ziplistNext;
            offset_update = 1;
        } else if (iter->direction == AL_START_TAIL) {
            nextFn = ziplistPrev;
            offset_update = -1;
        }
        iter->zi = nextFn(iter->current->zl, iter->zi);
        iter->offset += offset_update;
    }

    entry->zi = iter->zi;
    entry->offset = iter->offset;

    if (iter->zi) {
        /* Populate value from existing ziplist position */
        ziplistCreateInstance->ziplistGet(entry->zi, &entry->value, &entry->sz, &entry->longval);
        return 1;
    } else {
        /* We ran out of ziplist entries.
         * Pick next node, update offset, then re-run retrieval. */
        quicklistCompress(iter->quicklistl, iter->current);
        if (iter->direction == AL_START_HEAD) {
            /* Forward traversal */
            D("Jumping to start of next node");
            iter->current = iter->current->next;
            iter->offset = 0;
        } else if (iter->direction == AL_START_TAIL) {
            /* Reverse traversal */
            D("Jumping to end of previous node");
            iter->current = iter->current->prev;
            iter->offset = -1;
        }
        iter->zi = NULL;
        return quicklistNext(iter, entry);
    }
}

/**
 * 释放迭代器占用的内存
 * 
 * @param iter 要释放的迭代器指针
 */
void quicklistCreate::quicklistReleaseIterator(quicklistIter *iter)
{
    if (iter->current)
        quicklistCompress(iter->quicklistl, iter->current);

    zfree(iter);
}

/**
 * 复制一个 quicklist
 * 
 * @param orig 源 quicklist 指针
 * @return 返回复制后的新 quicklist 指针
 */
quicklist *quicklistCreate::quicklistDup(quicklist *orig)
{
    quicklist *copy;

    copy = quicklistNew(orig->fill, orig->compress);

    for (quicklistNode *current = orig->head; current;
         current = current->next) {
        quicklistNode *node = quicklistCreateNode();

        if (current->encoding == QUICKLIST_NODE_ENCODING_LZF) {
            quicklistLZF *lzf = (quicklistLZF *)current->zl;
            size_t lzf_sz = sizeof(*lzf) + lzf->sz;
            node->zl = static_cast<unsigned char*>(zmalloc(lzf_sz));
            memcpy(node->zl, current->zl, lzf_sz);
        } else if (current->encoding == QUICKLIST_NODE_ENCODING_RAW) {
            node->zl = static_cast<unsigned char*>(zmalloc(current->sz));
            memcpy(node->zl, current->zl, current->sz);
        }

        node->count = current->count;
        copy->count += node->count;
        node->sz = current->sz;
        node->encoding = current->encoding;

        _quicklistInsertNodeAfter(copy, copy->tail, node);
    }

    /* copy->count must equal orig->count here */
    return copy;
}

/**
 * 获取指定索引位置的元素
 * 
 * @param quicklist 目标 quicklist
 * @param index     元素索引
 * @param entry     存储元素的结构体
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistIndex(const quicklist *quicklist, const long long idx, quicklistEntry *entry)
{
    quicklistNode *n;
    unsigned long long accum = 0;
    unsigned long long index;
    int forward = idx < 0 ? 0 : 1; /* < 0 -> reverse, 0+ -> forward */

    initEntry(entry);
    entry->quicklistl = quicklist;

    if (!forward) {
        index = (-idx) - 1;
        n = quicklist->tail;
    } else {
        index = idx;
        n = quicklist->head;
    }

    if (index >= quicklist->count)
        return 0;

    while (likely(n)) {
        if ((accum + n->count) > index) {
            break;
        } else {
            D("Skipping over (%p) %u at accum %lld", (void *)n, n->count,
              accum);
            accum += n->count;
            n = forward ? n->next : n->prev;
        }
    }

    if (!n)
        return 0;

    D("Found node: %p at accum %llu, idx %llu, sub+ %llu, sub- %llu", (void *)n,
      accum, index, index - accum, (-index) - 1 + accum);

    entry->node = n;
    if (forward) {
        /* forward = normal head-to-tail offset. */
        entry->offset = index - accum;
    } else {
        /* reverse = need negative offset for tail-to-head, so undo
         * the result of the original if (index < 0) above. */
        entry->offset = (-index) - 1 + accum;
    }

    quicklistDecompressNodeForUse(entry->node);
    entry->zi = ziplistCreateInstance->ziplistIndex(entry->node->zl, entry->offset);
    if (!ziplistCreateInstance->ziplistGet(entry->zi, &entry->value, &entry->sz, &entry->longval))
        assert(0); /* This can happen on corrupt ziplist with fake entry count. */
    /* The caller will use our result, so we don't re-compress here.
     * The caller can recompress or delete the node as needed. */
    return 1;
}

/**
 * 重置迭代器到头部
 * 
 * @param quicklist 目标 quicklist
 * @param li        迭代器指针
 */
void quicklistCreate::quicklistRewind(quicklist *quicklist, quicklistIter *li)
{

}

/**
 * 重置迭代器到尾部
 * 
 * @param quicklist 目标 quicklist
 * @param li        迭代器指针
 */
void quicklistCreate::quicklistRewindTail(quicklist *quicklist, quicklistIter *li)
{

}

/**
 * 将 quicklist 的尾部元素移到头部
 * 
 * @param quicklist 目标 quicklist
 */
void quicklistCreate::quicklistRotate(quicklist *quicklist)
{
    if (quicklist->count <= 1)
        return;

    /* First, get the tail entry */
    unsigned char *p = ziplistCreateInstance->ziplistIndex(quicklist->tail->zl, -1);
    unsigned char *value, *tmp;
    long long longval;
    unsigned int sz;
    char longstr[32] = {0};
    ziplistCreateInstance->ziplistGet(p, &tmp, &sz, &longval);

    /* If value found is NULL, then ziplistGet populated longval instead */
    if (!tmp) {
        /* Write the longval as a string so we can re-add it */
        sz = toolFuncInstance->ll2string(longstr, sizeof(longstr), longval);
        value = (unsigned char *)longstr;
    } else if (quicklist->len == 1) {
        /* Copy buffer since there could be a memory overlap when move
         * entity from tail to head in the same ziplist. */
        value =static_cast<unsigned char*> (zmalloc(sz));
        memcpy(value, tmp, sz);
    } else {
        value = tmp;
    }

    /* Add tail entry to head (must happen before tail is deleted). */
    quicklistPushHead(quicklist, value, sz);

    /* If quicklist has only one node, the head ziplist is also the
     * tail ziplist and PushHead() could have reallocated our single ziplist,
     * which would make our pre-existing 'p' unusable. */
    if (quicklist->len == 1) {
        p = ziplistCreateInstance->ziplistIndex(quicklist->tail->zl, -1);
    }

    /* Remove tail entry. */
    quicklistDelIndex(quicklist, quicklist->tail, &p);
    if (value != (unsigned char*)longstr && value != tmp)
        zfree(value);
}

/**
 * 弹出 quicklist 指定位置的元素（自定义保存函数）
 * 
 * @param quicklist 目标 quicklist
 * @param where     弹出位置（QL_HEAD 或 QL_TAIL）
 * @param data      存储弹出数据的指针
 * @param sz        存储数据大小的指针
 * @param sval      存储整型值的指针
 * @param saver     自定义保存函数
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistPopCustom(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *sval, void *(*saver)(unsigned char *data, unsigned int sz))
{
    unsigned char *p;
    unsigned char *vstr;
    unsigned int vlen;
    long long vlong;
    int pos = (where == QUICKLIST_HEAD) ? 0 : -1;

    if (quicklist->count == 0)
        return 0;

    if (data)
        *data = NULL;
    if (sz)
        *sz = 0;
    if (sval)
        *sval = -123456789;

    quicklistNode *node;
    if (where == QUICKLIST_HEAD && quicklist->head) {
        node = quicklist->head;
    } else if (where == QUICKLIST_TAIL && quicklist->tail) {
        node = quicklist->tail;
    } else {
        return 0;
    }

    p = ziplistCreateInstance->ziplistIndex(node->zl, pos);
    if (ziplistCreateInstance->ziplistGet(p, &vstr, &vlen, &vlong)) {
        if (vstr) {
            if (data)
                *data = static_cast<unsigned char*>(saver(vstr, vlen));
            if (sz)
                *sz = vlen;
        } else {
            if (data)
                *data = NULL;
            if (sval)
                *sval = vlong;
        }
        quicklistDelIndex(quicklist, node, &p);
        return 1;
    }
    return 0;
}

/**
 * 弹出 quicklist 指定位置的元素
 * 
 * @param quicklist 目标 quicklist
 * @param where     弹出位置（QL_HEAD 或 QL_TAIL）
 * @param data      存储弹出数据的指针
 * @param sz        存储数据大小的指针
 * @param slong     存储整型值的指针
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistPop(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *slong)
{
    unsigned char *vstr;
    unsigned int vlen;
    long long vlong;
    if (quicklist->count == 0)
        return 0;
    int ret = quicklistPopCustom(quicklist, where, &vstr, &vlen, &vlong,
                                 _quicklistSaver);
    if (data)
        *data = vstr;
    if (slong)
        *slong = vlong;
    if (sz)
        *sz = vlen;
    return ret;
}

/**
 * 获取 quicklist 中的元素总数
 * 
 * @param ql 目标 quicklist
 * @return quicklist 中的元素数量
 */
unsigned long quicklistCreate::quicklistCount(const quicklist *ql)
{
    return ql->count;
}

/**
 * 比较两个 quicklist 节点的数据
 * 
 * @param p1     第一个数据指针
 * @param p2     第二个数据指针
 * @param p2_len 第二个数据的长度
 * @return 相等返回 1，不等返回 0
 */
int quicklistCreate::quicklistCompare(unsigned char *p1, unsigned char *p2, int p2_len)
{
    return ziplistCreateInstance->ziplistCompare(p1, p2, p2_len);
}

/**
 * 获取 LZF 压缩节点的数据
 * 
 * @param node  目标节点
 * @param data  存储解压后数据的指针
 * @return 返回数据大小（字节）
 */
size_t quicklistCreate::quicklistGetLzf(const quicklistNode *node, void **data)
{
    quicklistLZF *lzf = (quicklistLZF *)node->zl;
    *data = lzf->compressed;
    return lzf->sz;
}

/* Bookmarks 相关函数 */

/**
 * 创建一个 quicklist 书签
 * 
 * @param ql_ref  quicklist 指针的引用
 * @param name    书签名称
 * @param node    关联的节点
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistBookmarkCreate(quicklist **ql_ref, const char *name, quicklistNode *node)
{
    quicklist *ql = *ql_ref;
    if (ql->bookmark_count >= QL_MAX_BM)
        return 0;
    quicklistBookmark *bm = _quicklistBookmarkFindByName(ql, name);
    if (bm) {
        bm->node = node;
        return 1;
    }
    ql =static_cast<quicklist *> (zrealloc(ql, sizeof(quicklist) + (ql->bookmark_count+1) * sizeof(quicklistBookmark)));
    *ql_ref = ql;
    ql->bookmarks[ql->bookmark_count].node = node;
    ql->bookmarks[ql->bookmark_count].name = zstrdup(name);
    ql->bookmark_count++;
    return 1;
}

/**
 * 删除指定名称的书签
 * 
 * @param ql    目标 quicklist
 * @param name  书签名称
 * @return 成功返回 1，失败返回 0
 */
int quicklistCreate::quicklistBookmarkDelete(quicklist *ql, const char *name)
{
    quicklistBookmark *bm = _quicklistBookmarkFindByName(ql, name);
    if (!bm)
        return 0;
    _quicklistBookmarkDelete(ql, bm);
    return 1;
}

/**
 * 查找指定名称的书签
 * 
 * @param ql    目标 quicklist
 * @param name  书签名称
 * @return 找到返回关联的节点指针，未找到返回 NULL
 */
quicklistNode *quicklistCreate::quicklistBookmarkFind(quicklist *ql, const char *name)
{
    quicklistBookmark *bm = _quicklistBookmarkFindByName(ql, name);
    if (!bm) return NULL;
    return bm->node;
}

/**
 * 清除 quicklist 中所有书签
 * 
 * @param ql 目标 quicklist
 */
void quicklistCreate::quicklistBookmarksClear(quicklist *ql)
{
    while (ql->bookmark_count)
        zfree(ql->bookmarks[--ql->bookmark_count].name);
}

/**
 * 更新指定quicklist节点的大小信息。
 * 
 * @param node 待更新的quicklist节点
 */
void quicklistCreate::quicklistNodeUpdateSz(quicklistNode *node)
{
    (node)->sz = ziplistCreateInstance->ziplistBlobLen((node)->zl);                               
}   
/**
 * 创建一个新的quicklist节点。
 * 
 * @return 新创建的quicklist节点，如果失败则返回NULL
 */
quicklistNode *quicklistCreate::quicklistCreateNode(void) 
{
    quicklistNode *node;
    node =static_cast<quicklistNode*>(zmalloc(sizeof(*node)));
    node->zl = NULL;
    node->count = 0;
    node->sz = 0;
    node->next = node->prev = NULL;
    node->encoding = QUICKLIST_NODE_ENCODING_RAW;
    node->container = QUICKLIST_NODE_CONTAINER_ZIPLIST;
    node->recompress = 0;
    return node;
}

/**
 * 在指定节点前插入一个新节点。
 * 
 * @param quicklist quicklist链表
 * @param old_node  参考节点
 * @param new_node  待插入的新节点
 */
void quicklistCreate::_quicklistInsertNodeBefore(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node) 
{
    __quicklistInsertNode(quicklist, old_node, new_node, 0);
}

/**
 * 在指定位置插入一个新节点（可选择前插或后插）。
 * 
 * @param quicklist quicklist链表
 * @param old_node  参考节点
 * @param new_node  待插入的新节点
 * @param after     插入位置标志，1表示在参考节点后插入，0表示在参考节点前插入
 */
void quicklistCreate::__quicklistInsertNode(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node, int after) 
{
    if (after) {
        new_node->prev = old_node;
        if (old_node) {
            new_node->next = old_node->next;
            if (old_node->next)
                old_node->next->prev = new_node;
            old_node->next = new_node;
        }
        if (quicklist->tail == old_node)
            quicklist->tail = new_node;
    } else {
        new_node->next = old_node;
        if (old_node) {
            new_node->prev = old_node->prev;
            if (old_node->prev)
                old_node->prev->next = new_node;
            old_node->prev = new_node;
        }
        if (quicklist->head == old_node)
            quicklist->head = new_node;
    }
    /* If this insert creates the only element so far, initialize head/tail. */
    if (quicklist->len == 0) {
        quicklist->head = quicklist->tail = new_node;
    }

    /* Update len first, so in __quicklistCompress we know exactly len */
    quicklist->len++;

    if (old_node)
        quicklistCompress(quicklist, old_node);
}



/* Force 'quicklist' to meet compression guidelines set by compress depth.
 * The only way to guarantee interior nodes get compressed is to iterate
 * to our "interior" compress depth then compress the next node we find.
 * If compress depth is larger than the entire list, we return immediately. */
/**
 * 对指定节点执行压缩操作（内部使用）。
 * 
 * @param quicklist quicklist链表
 * @param node      待压缩的节点
 */
void quicklistCreate::__quicklistCompress(const quicklist *quicklist,quicklistNode *node)
 {
    /* If length is less than our compress depth (from both sides),
     * we can't compress anything. */
    if (!quicklistAllowsCompression(quicklist) ||
        quicklist->len < (unsigned int)(quicklist->compress * 2))
        return;

    /* Iterate until we reach compress depth for both sides of the list.a
     * Note: because we do length checks at the *top* of this function,
     *       we can skip explicit null checks below. Everything exists. */
    quicklistNode *forward = quicklist->head;
    quicklistNode *reverse = quicklist->tail;
    int depth = 0;
    int in_depth = 0;
    while (depth++ < quicklist->compress) {
        quicklistDecompressNode(forward);
        quicklistDecompressNode(reverse);

        if (forward == node || reverse == node)
            in_depth = 1;

        /* We passed into compress depth of opposite side of the quicklist
         * so there's no need to compress anything and we can exit. */
        if (forward == reverse || forward->next == reverse)
            return;

        forward = forward->next;
        reverse = reverse->prev;
    }

    if (!in_depth)
        quicklistCompressNode(node);

    /* At this point, forward and reverse are one node beyond depth */
    quicklistCompressNode(forward);
    quicklistCompressNode(reverse);
}
/**
 * 尝试压缩指定节点并返回压缩结果。
 * 
 * @param node 待压缩的节点
 * @return 压缩成功返回1，失败返回0
 */
int quicklistCreate::__quicklistCompressNode(quicklistNode *node) 
{
#ifdef REDIS_TEST
    node->attempted_compress = 1;
#endif

    /* Don't bother compressing small values */
    if (node->sz < MIN_COMPRESS_BYTES)
        return 0;

    quicklistLZF *lzf =static_cast<quicklistLZF*>(zmalloc(sizeof(*lzf) + node->sz));

    /* Cancel if compression fails or doesn't compress small enough */
    if (((lzf->sz = toolFuncInstance->lzf_compress(node->zl, node->sz, lzf->compressed,
                                 node->sz)) == 0) ||
        lzf->sz + MIN_COMPRESS_IMPROVE >= node->sz) {
        /* lzf_compress aborts/rejects compression if value not compressable. */
        zfree(lzf);
        return 0;
    }
    lzf =static_cast<quicklistLZF*>(zrealloc(lzf, sizeof(*lzf) + lzf->sz));
    zfree(node->zl);
    node->zl = (unsigned char *)lzf;
    node->encoding = QUICKLIST_NODE_ENCODING_LZF;
    node->recompress = 0;
    return 1;
}
/**
 * 对指定节点执行压缩操作（外部接口）。
 * 
 * @param _ql    quicklist链表
 * @param _node  待压缩的节点
 */
void quicklistCreate::quicklistCompress(const quicklist *_ql,quicklistNode *_node)
{
    if ((_node)->recompress) 
        quicklistCompressNode((_node));
    else                                                                   
        __quicklistCompress((_ql), (_node)); 
}
/**
 * 尝试压缩指定节点并返回压缩结果。
 * 
 * @param node 待压缩的节点
 * @return 压缩成功返回1，失败返回0
 */
void quicklistCreate::quicklistCompressNode(quicklistNode *_node)
{
    if ((_node) && (_node)->encoding == QUICKLIST_NODE_ENCODING_RAW) 
    {     
        __quicklistCompressNode(_node);                                  
    }  
}
/**
 * 仅对指定节点重新执行压缩操作（不影响其他节点）。
 * 
 * @param _ql    quicklist链表
 * @param _node  待重新压缩的节点
 */
void quicklistCreate::quicklistRecompressOnly(const quicklist *_ql,quicklistNode *_node)
{
    if ((_node)->recompress)                                               
        quicklistCompressNode(_node);    
}
/**
 * 检查指定quicklist是否允许执行压缩操作。
 * 
 * @param _ql quicklist链表
 * @return 允许压缩返回true，否则返回false
 */
bool quicklistCreate::quicklistAllowsCompression(const quicklist *_ql)
{
   return (_ql)->compress != 0;
}
/**
 * 解压缩指定节点。
 * 
 * @param _node 待解压缩的节点
 */
void quicklistCreate::quicklistDecompressNode(quicklistNode *_node)
{
                                                                   
    if ((_node) && (_node)->encoding == QUICKLIST_NODE_ENCODING_LZF) 
    {     
        __quicklistDecompressNode((_node));                                
    }                                                                      
}                                         
/**
 * 尝试解压缩指定节点并返回解压缩结果。
 * 
 * @param node 待解压缩的节点
 * @return 解压缩成功返回1，失败返回0
 */
int quicklistCreate::__quicklistDecompressNode(quicklistNode *node) 
{
    void *decompressed = zmalloc(node->sz);
    quicklistLZF *lzf = (quicklistLZF *)node->zl;
    if (toolFuncInstance->lzf_decompress(lzf->compressed, lzf->sz, decompressed, node->sz) == 0) {
        /* Someone requested decompress, but we can't decompress.  Not good. */
        zfree(decompressed);
        return 0;
    }
    zfree(lzf);
    node->zl = static_cast<unsigned char*>(decompressed);
    node->encoding = QUICKLIST_NODE_ENCODING_RAW;
    return 1;
}

#define sizeMeetsSafetyLimit(sz) ((sz) <= SIZE_SAFETY_LIMIT)
/**
 * 检查节点是否允许插入新元素。
 * 
 * @param node 待检查的节点
 * @param fill 填充因子
 * @param sz   待插入元素的大小
 * @return 允许插入返回1，否则返回0
 */
int quicklistCreate::_quicklistNodeAllowInsert(const quicklistNode *node,const int fill, const size_t sz)
{
    if (unlikely(!node))
        return 0;

    int ziplist_overhead;
    /* size of previous offset */
    if (sz < 254)
        ziplist_overhead = 1;
    else
        ziplist_overhead = 5;

    /* size of forward offset */
    if (sz < 64)
        ziplist_overhead += 1;
    else if (likely(sz < 16384))
        ziplist_overhead += 2;
    else
        ziplist_overhead += 5;

    /* new_sz overestimates if 'sz' encodes to an integer type */
    unsigned int new_sz = node->sz + sz + ziplist_overhead;
    if ((_quicklistNodeSizeMeetsOptimizationRequirement(new_sz, fill)))
        return 1;
    /* when we return 1 above we know that the limit is a size limit (which is
     * safe, see comments next to optimization_level and SIZE_SAFETY_LIMIT) */
    else if (!sizeMeetsSafetyLimit(new_sz))
        return 0;
    else if ((int)node->count < fill)
        return 1;
    else
        return 0;
}
/**
 * 检查节点大小是否满足优化要求。
 * 
 * @param sz   节点大小
 * @param fill 填充因子
 * @return 满足要求返回1，否则返回0
 */
int quicklistCreate::_quicklistNodeSizeMeetsOptimizationRequirement(const size_t sz,const int fill)
{
    if (fill >= 0)
        return 0;

    size_t offset = (-fill) - 1;
    if (offset < (sizeof(optimization_level) / sizeof(*optimization_level))) {
        if (sz <= optimization_level[offset]) {
            return 1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
/**
 * 在指定节点后插入一个新节点。
 * 
 * @param quicklist quicklist链表
 * @param old_node  参考节点
 * @param new_node  待插入的新节点
 */
void quicklistCreate::_quicklistInsertNodeAfter(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node) 
{
    __quicklistInsertNode(quicklist, old_node, new_node, 1);
}
/**
 * 在指定位置插入一个新元素。
 * 
 * @param quicklist quicklist链表
 * @param entry     参考位置
 * @param value     待插入的值
 * @param sz        待插入值的大小
 * @param after     插入位置标志，1表示在参考位置后插入，0表示在参考位置前插入
 */
void quicklistCreate::_quicklistInsert(quicklist *quicklist, quicklistEntry *entry,void *value, const size_t sz, int after) 
{
    int full = 0, at_tail = 0, at_head = 0, full_next = 0, full_prev = 0;
    int fill = quicklist->fill;
    quicklistNode *node = entry->node;
    quicklistNode *new_node = NULL;
    assert(sz < UINT32_MAX); /* TODO: add support for quicklist nodes that are sds encoded (not zipped) */

    if (!node) {
        /* we have no reference node, so let's create only node in the list */
        D("No node given!");
        new_node = quicklistCreateNode();
        new_node->zl = ziplistCreateInstance->ziplistPush(ziplistCreateInstance->ziplistNew(),static_cast<unsigned char*>(value), sz, ZIPLIST_HEAD);
        __quicklistInsertNode(quicklist, NULL, new_node, after);
        new_node->count++;
        quicklist->count++;
        return;
    }

    /* Populate accounting flags for easier boolean checks later */
    if (!_quicklistNodeAllowInsert(node, fill, sz)) {
        D("Current node is full with count %d with requested fill %lu",
          node->count, fill);
        full = 1;
    }

    if (after && (entry->offset == node->count)) {
        D("At Tail of current ziplist");
        at_tail = 1;
        if (!_quicklistNodeAllowInsert(node->next, fill, sz)) {
            D("Next node is full too.");
            full_next = 1;
        }
    }

    if (!after && (entry->offset == 0)) {
        D("At Head");
        at_head = 1;
        if (!_quicklistNodeAllowInsert(node->prev, fill, sz)) {
            D("Prev node is full too.");
            full_prev = 1;
        }
    }

    /* Now determine where and how to insert the new element */
    if (!full && after) {
        D("Not full, inserting after current position.");
        quicklistDecompressNodeForUse(node);
        unsigned char *next = ziplistCreateInstance->ziplistNext(node->zl, entry->zi);
        if (next == NULL) {
            node->zl = ziplistCreateInstance->ziplistPush(node->zl, static_cast<unsigned char*>(value), sz, ZIPLIST_TAIL);
        } else {
            node->zl = ziplistCreateInstance->ziplistInsert(node->zl, next, static_cast<unsigned char*>(value), sz);
        }
        node->count++;
        quicklistNodeUpdateSz(node);
        quicklistRecompressOnly(quicklist, node);
    } else if (!full && !after) {
        D("Not full, inserting before current position.");
        quicklistDecompressNodeForUse(node);
        node->zl = ziplistCreateInstance->ziplistInsert(node->zl, entry->zi, static_cast<unsigned char*>(value), sz);
        node->count++;
        quicklistNodeUpdateSz(node);
        quicklistRecompressOnly(quicklist, node);
    } else if (full && at_tail && node->next && !full_next && after) {
        /* If we are: at tail, next has free space, and inserting after:
         *   - insert entry at head of next node. */
        D("Full and tail, but next isn't full; inserting next node head");
        new_node = node->next;
        quicklistDecompressNodeForUse(new_node);
        new_node->zl = ziplistCreateInstance->ziplistPush(new_node->zl, static_cast<unsigned char*>(value), sz, ZIPLIST_HEAD);
        new_node->count++;
        quicklistNodeUpdateSz(new_node);
        quicklistRecompressOnly(quicklist, new_node);
    } else if (full && at_head && node->prev && !full_prev && !after) {
        /* If we are: at head, previous has free space, and inserting before:
         *   - insert entry at tail of previous node. */
        D("Full and head, but prev isn't full, inserting prev node tail");
        new_node = node->prev;
        quicklistDecompressNodeForUse(new_node);
        new_node->zl = ziplistCreateInstance->ziplistPush(new_node->zl, static_cast<unsigned char*>(value), sz, ZIPLIST_TAIL);
        new_node->count++;
        quicklistNodeUpdateSz(new_node);
        quicklistRecompressOnly(quicklist, new_node);
    } else if (full && ((at_tail && node->next && full_next && after) ||
                        (at_head && node->prev && full_prev && !after))) {
        /* If we are: full, and our prev/next is full, then:
         *   - create new node and attach to quicklist */
        D("\tprovisioning new node...");
        new_node = quicklistCreateNode();
        new_node->zl = ziplistCreateInstance->ziplistPush(ziplistCreateInstance->ziplistNew(), static_cast<unsigned char*>(value), sz, ZIPLIST_HEAD);
        new_node->count++;
        quicklistNodeUpdateSz(new_node);
        __quicklistInsertNode(quicklist, node, new_node, after);
    } else if (full) {
        /* else, node is full we need to split it. */
        /* covers both after and !after cases */
        D("\tsplitting node...");
        quicklistDecompressNodeForUse(node);
        new_node = _quicklistSplitNode(node, entry->offset, after);
        new_node->zl = ziplistCreateInstance->ziplistPush(new_node->zl, static_cast<unsigned char*>(value), sz,
                                   after ? ZIPLIST_HEAD : ZIPLIST_TAIL);
        new_node->count++;
        quicklistNodeUpdateSz(new_node);
        __quicklistInsertNode(quicklist, node, new_node, after);
        _quicklistMergeNodes(quicklist, node);
    }

    quicklist->count++;
}
/**
 * 为使用而解压缩节点（使用后可能需要重新压缩）。
 * 
 * @param node 待解压缩的节点
 */
 void quicklistCreate::quicklistDecompressNodeForUse(quicklistNode *node)                                   
 {
    if ((node) && (node)->encoding == QUICKLIST_NODE_ENCODING_LZF) 
    {     
        __quicklistDecompressNode((node));                                
        (node)->recompress = 1;                                           
    }                                                                      
 }
/**
 * 分割指定节点为两个节点。
 * 
 * @param node   待分割的节点
 * @param offset 分割位置偏移量
 * @param after  分割方式标志，1表示在偏移量后分割，0表示在偏移量前分割
 * @return 分割后生成的新节点
 */
quicklistNode *quicklistCreate::_quicklistSplitNode(quicklistNode *node, int offset,int after) 
{
    size_t zl_sz = node->sz;

    quicklistNode *new_node = quicklistCreateNode();
    new_node->zl = static_cast<unsigned char*>(zmalloc(zl_sz));

    /* Copy original ziplist so we can split it */
    memcpy(new_node->zl, node->zl, zl_sz);

    /* Ranges to be trimmed: -1 here means "continue deleting until the list ends" */
    int orig_start = after ? offset + 1 : 0;
    int orig_extent = after ? -1 : offset;
    int new_start = after ? 0 : offset;
    int new_extent = after ? offset + 1 : -1;

    D("After %d (%d); ranges: [%d, %d], [%d, %d]", after, offset, orig_start,
      orig_extent, new_start, new_extent);

    node->zl = ziplistCreateInstance->ziplistDeleteRange(node->zl, orig_start, orig_extent);
    node->count = ziplistCreateInstance->ziplistLen(node->zl);
    quicklistNodeUpdateSz(node);

    new_node->zl = ziplistCreateInstance->ziplistDeleteRange(new_node->zl, new_start, new_extent);
    new_node->count = ziplistCreateInstance->ziplistLen(new_node->zl);
    quicklistNodeUpdateSz(new_node);

    D("After split lengths: orig (%d), new (%d)", node->count, new_node->count);
    return new_node;
}
/**
 * 合并相邻节点。
 * 
 * @param quicklist quicklist链表
 * @param center    中心节点（将合并其相邻节点）
 */
void quicklistCreate::_quicklistMergeNodes(quicklist *quicklist, quicklistNode *center) 
{
    int fill = quicklist->fill;
    quicklistNode *prev, *prev_prev, *next, *next_next, *target;
    prev = prev_prev = next = next_next = target = NULL;

    if (center->prev) {
        prev = center->prev;
        if (center->prev->prev)
            prev_prev = center->prev->prev;
    }

    if (center->next) {
        next = center->next;
        if (center->next->next)
            next_next = center->next->next;
    }

    /* Try to merge prev_prev and prev */
    if (_quicklistNodeAllowMerge(prev, prev_prev, fill)) {
        _quicklistZiplistMerge(quicklist, prev_prev, prev);
        prev_prev = prev = NULL; /* they could have moved, invalidate them. */
    }

    /* Try to merge next and next_next */
    if (_quicklistNodeAllowMerge(next, next_next, fill)) {
        _quicklistZiplistMerge(quicklist, next, next_next);
        next = next_next = NULL; /* they could have moved, invalidate them. */
    }

    /* Try to merge center node and previous node */
    if (_quicklistNodeAllowMerge(center, center->prev, fill)) {
        target = _quicklistZiplistMerge(quicklist, center->prev, center);
        center = NULL; /* center could have been deleted, invalidate it. */
    } else {
        /* else, we didn't merge here, but target needs to be valid below. */
        target = center;
    }

    /* Use result of center merge (or original) to merge with next node. */
    if (_quicklistNodeAllowMerge(target, target->next, fill)) {
        _quicklistZiplistMerge(quicklist, target, target->next);
    }
}
/**
 * 检查两个节点是否允许合并。
 * 
 * @param a    第一个节点
 * @param b    第二个节点
 * @param fill 填充因子
 * @return 允许合并返回1，否则返回0
 */
int quicklistCreate::_quicklistNodeAllowMerge(const quicklistNode *a,const quicklistNode *b,const int fill) 
{
    if (!a || !b)
        return 0;

    /* approximate merged ziplist size (- 11 to remove one ziplist
     * header/trailer) */
    unsigned int merge_sz = a->sz + b->sz - 11;
    if (likely(_quicklistNodeSizeMeetsOptimizationRequirement(merge_sz, fill)))
        return 1;
    /* when we return 1 above we know that the limit is a size limit (which is
     * safe, see comments next to optimization_level and SIZE_SAFETY_LIMIT) */
    else if (!sizeMeetsSafetyLimit(merge_sz))
        return 0;
    else if ((int)(a->count + b->count) <= fill)
        return 1;
    else
        return 0;
}
/**
 * 合并两个节点的ziplist数据。
 * 
 * @param quicklist quicklist链表
 * @param a         第一个节点
 * @param b         第二个节点
 * @return 合并后的节点
 */
quicklistNode *quicklistCreate::_quicklistZiplistMerge(quicklist *quicklist,quicklistNode *a,quicklistNode *b) 
{
    D("Requested merge (a,b) (%u, %u)", a->count, b->count);

    quicklistDecompressNode(a);
    quicklistDecompressNode(b);
    if ((ziplistCreateInstance->ziplistMerge(&a->zl, &b->zl))) {
        /* We merged ziplists! Now remove the unused quicklistNode. */
        quicklistNode *keep = NULL, *nokeep = NULL;
        if (!a->zl) {
            nokeep = a;
            keep = b;
        } else if (!b->zl) {
            nokeep = b;
            keep = a;
        }
        keep->count = ziplistCreateInstance->ziplistLen(keep->zl);
        quicklistNodeUpdateSz(keep);

        nokeep->count = 0;
        __quicklistDelNode(quicklist, nokeep);
        quicklistCompress(quicklist, keep);
        return keep;
    } else {
        /* else, the merge returned NULL and nothing changed. */
        return NULL;
    }
}
/**
 * 删除指定节点（内部使用）。
 * 
 * @param quicklist quicklist链表
 * @param node      待删除的节点
 */
void quicklistCreate::__quicklistDelNode(quicklist *quicklist,quicklistNode *node) 
 {
    /* Update the bookmark if any */
    quicklistBookmark *bm = _quicklistBookmarkFindByNode(quicklist, node);
    if (bm) {
        bm->node = node->next;
        /* if the bookmark was to the last node, delete it. */
        if (!bm->node)
            _quicklistBookmarkDelete(quicklist, bm);
    }

    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;

    if (node == quicklist->tail) {
        quicklist->tail = node->prev;
    }

    if (node == quicklist->head) {
        quicklist->head = node->next;
    }

    /* Update len first, so in __quicklistCompress we know exactly len */
    quicklist->len--;
    quicklist->count -= node->count;

    /* If we deleted a node within our compress depth, we
     * now have compressed nodes needing to be decompressed. */
    __quicklistCompress(quicklist, NULL);

    zfree(node->zl);
    zfree(node);
}
/**
 * 删除指定书签。
 * 
 * @param ql quicklist链表
 * @param bm 待删除的书签
 */
void quicklistCreate::_quicklistBookmarkDelete(quicklist *ql, quicklistBookmark *bm)
 {
    int index = bm - ql->bookmarks;
    zfree(bm->name);
    ql->bookmark_count--;
    memmove(bm, bm+1, (ql->bookmark_count - index)* sizeof(*bm));
    /* NOTE: We do not shrink (realloc) the quicklist yet (to avoid resonance,
     * it may be re-used later (a call to realloc may NOP). */
}
/**
 * 根据节点查找对应的书签。
 * 
 * @param ql   quicklist链表
 * @param node 目标节点
 * @return 找到的书签，如果未找到则返回NULL
 */
quicklistBookmark *quicklistCreate::_quicklistBookmarkFindByNode(quicklist *ql, quicklistNode *node)
{
    unsigned i;
    for (i=0; i<ql->bookmark_count; i++) {
        if (ql->bookmarks[i].node == node) {
            return &ql->bookmarks[i];
        }
    }
    return NULL;
}
/**
 * 根据名称查找对应的书签。
 * 
 * @param ql   quicklist链表
 * @param name 书签名称
 * @return 找到的书签，如果未找到则返回NULL
 */
quicklistBookmark *quicklistCreate::_quicklistBookmarkFindByName(quicklist *ql, const char *name) 
{
    unsigned i;
    for (i=0; i<ql->bookmark_count; i++) {
        if (!strcmp(ql->bookmarks[i].name, name)) {
            return &ql->bookmarks[i];
        }
    }
    return NULL;
}
/**
 * 删除指定索引位置的元素。
 * 
 * @param quicklist quicklist链表
 * @param node      元素所在的节点
 * @param p         指向元素的指针
 * @return 删除成功返回1，失败返回0
 */
int quicklistCreate::quicklistDelIndex(quicklist *quicklist, quicklistNode *node,unsigned char **p) 
{
    int gone = 0;

    node->zl = ziplistCreateInstance->ziplistDelete(node->zl, p);
    node->count--;
    if (node->count == 0) {
        gone = 1;
        __quicklistDelNode(quicklist, node);
    } else {
        quicklistNodeUpdateSz(node);
    }
    quicklist->count--;
    /* If we deleted the node, the original node is no longer valid */
    return gone ? 1 : 0;
}


/**
 * 如果节点为空则删除该节点。
 * 
 * @param quicklist quicklist链表
 * @param node      待检查的节点
 */
void quicklistCreate::quicklistDeleteIfEmpty(quicklist *quicklist, quicklistNode *node)
{
    if ((node)->count == 0) 
    {                                                 
        __quicklistDelNode((quicklist), (node));                                     
        (node) = NULL;                                                        
    }    
}
/**
 * 获取ziplist中下一个元素的指针。
 * 
 * @param zl 指向ziplist的指针
 * @param p  当前元素的指针
 * @return 下一个元素的指针，如果没有下一个元素则返回NULL
 */
unsigned char *quicklistCreate::ziplistNext(unsigned char *zl, unsigned char *p)
 {
    ((void) zl);
    size_t zlbytes = intrev32ifbe(ZIPLIST_BYTES(zl));

    /* "p" could be equal to ZIP_END, caused by ziplistDelete,
     * and we should return NULL. Otherwise, we should return NULL
     * when the *next* element is ZIP_END (there is no next entry). */
    if (p[0] == ZIP_END) {
        return NULL;
    }
    
    p += ziplistCreatel.zipRawEntryLength(p);
    if (p[0] == ZIP_END) {
        return NULL;
    }
    ziplistCreatel.zipAssertValidEntry(zl, zlbytes, p);
    return p;
}

/* Return pointer to previous entry in ziplist. */
/**
 * 获取ziplist中前一个元素的指针。
 * 
 * @param zl 指向ziplist的指针
 * @param p  当前元素的指针
 * @return 前一个元素的指针，如果没有前一个元素则返回NULL
 */
unsigned char *quicklistCreate::ziplistPrev(unsigned char *zl, unsigned char *p) 
{
    unsigned int prevlensize, prevlen = 0;
    /* Iterating backwards from ZIP_END should return the tail. When "p" is
     * equal to the first element of the list, we're already at the head,
     * and should return NULL. */
    if (p[0] == ZIP_END) {
        p = ZIPLIST_ENTRY_TAIL(zl);
        return (p[0] == ZIP_END) ? NULL : p;
    } else if (p == ZIPLIST_ENTRY_HEAD(zl)) {
        return NULL;
    } else {
        ziplistCreatel.zipDecodePrevlen(p, prevlensize, prevlen);
        assert(prevlen > 0);
        p-=prevlen;
        size_t zlbytes = intrev32ifbe(ZIPLIST_BYTES(zl));
        ziplistCreatel.zipAssertValidEntry(zl, zlbytes, p);
        return p;
    }
}
/**
 * quicklist数据保存器（用于序列化数据）。
 * 
 * @param data 待保存的数据
 * @param sz   数据大小
 * @return 保存后的数据指针
 */
void *quicklistCreate::_quicklistSaver(unsigned char *data, unsigned int sz)
{
    unsigned char *vstr;
    if (data) {
        vstr =static_cast< unsigned char *>(zmalloc(sz));
        memcpy(vstr, data, sz);
        return vstr;
    }
    return NULL;
}
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
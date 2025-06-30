/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/30
 * All rights reserved. No one may copy or transfer.
 * Description: quicklist implementation
 */

 #ifndef REDIS_BASE_QUICKLIST_H
 #define REDIS_BASE_QUICKLIST_H
 #include "define.h"
 #include <stdint.h>
 #include <stddef.h> 
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class ziplistCreate;
class toolFunc;
typedef struct quicklistNode {
    struct quicklistNode *prev;
    struct quicklistNode *next;
    unsigned char *zl;
    unsigned int sz;             /* ziplist size in bytes */
    unsigned int count : 16;     /* count of items in ziplist */
    unsigned int encoding : 2;   /* RAW==1 or LZF==2 */
    unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
    unsigned int recompress : 1; /* was this node previous compressed? */
    unsigned int attempted_compress : 1; /* node can't compress; too small */
    unsigned int extra : 10; /* more bits to steal for future usage */
} quicklistNode;

typedef struct quicklistLZF {
    unsigned int sz; /* LZF size in bytes*/
    char compressed[];
} quicklistLZF;

typedef struct quicklistBookmark {
    quicklistNode *node;
    char *name;
} quicklistBookmark;

#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#   define QL_FILL_BITS 14
#   define QL_COMP_BITS 14
#   define QL_BM_BITS 4
#elif UINTPTR_MAX == 0xffffffffffffffff
/* 64-bit */
#   define QL_FILL_BITS 16
#   define QL_COMP_BITS 16
#   define QL_BM_BITS 4 /* we can encode more, but we rather limit the user
                           since they cause performance degradation. */
#else
#   error unknown arch bits count
#endif

typedef struct quicklist {
    quicklistNode *head;
    quicklistNode *tail;
    unsigned long count;        /* total count of all entries in all ziplists */
    unsigned long len;          /* number of quicklistNodes */
    int fill : QL_FILL_BITS;              /* fill factor for individual nodes */
    unsigned int compress : QL_COMP_BITS; /* depth of end nodes not to compress;0=off */
    unsigned int bookmark_count: QL_BM_BITS;
    quicklistBookmark bookmarks[];
} quicklist;

typedef struct quicklistIter {
    const quicklist *quicklistl;
    quicklistNode *current;
    unsigned char *zi;
    long offset; /* offset in current ziplist */
    int direction;
} quicklistIter;

typedef struct quicklistEntry {
    const quicklist *quicklistl;
    quicklistNode *node;
    unsigned char *zi;
    unsigned char *value;
    long long longval;
    unsigned int sz;
    int offset;
} quicklistEntry;

class quicklistCreate
{
public:
    quicklistCreate();
    ~quicklistCreate();
public:
    /**
     * 创建一个新的 quicklist
     * 
     * @return 返回新创建的 quicklist 指针，失败时返回 NULL
     */
    quicklist *quicklistCrt(void);

    /**
     * 使用指定参数创建一个新的 quicklist
     * 
     * @param fill     每个节点的填充因子
     * @param compress 压缩深度
     * @return 返回新创建的 quicklist 指针，失败时返回 NULL
     */
    quicklist *quicklistNew(int fill, int compress);

    /**
     * 设置 quicklist 的压缩深度
     * 
     * @param quicklist 目标 quicklist
     * @param depth     压缩深度值
     */
    void quicklistSetCompressDepth(quicklist *quicklist, int depth);

    /**
     * 设置 quicklist 节点的填充因子
     * 
     * @param quicklist 目标 quicklist
     * @param fill      填充因子值
     */
    void quicklistSetFill(quicklist *quicklist, int fill);

    /**
     * 一次性设置 quicklist 的填充因子和压缩深度
     * 
     * @param quicklist 目标 quicklist
     * @param fill      填充因子值
     * @param depth     压缩深度值
     */
    void quicklistSetOptions(quicklist *quicklist, int fill, int depth);

    /**
     * 释放 quicklist 占用的内存
     * 
     * @param quicklist 要释放的 quicklist 指针
     */
    void quicklistRelease(quicklist *quicklist);

    /**
     * 在 quicklist 头部插入一个元素
     * 
     * @param quicklist 目标 quicklist
     * @param value     要插入的值指针
     * @param sz        值的大小（字节）
     * @return 成功返回 1，失败返回 0
     */
    int quicklistPushHead(quicklist *quicklist, void *value, const size_t sz);

    /**
     * 在 quicklist 尾部插入一个元素
     * 
     * @param quicklist 目标 quicklist
     * @param value     要插入的值指针
     * @param sz        值的大小（字节）
     * @return 成功返回 1，失败返回 0
     */
    int quicklistPushTail(quicklist *quicklist, void *value, const size_t sz);

    /**
     * 在 quicklist 指定位置插入一个元素
     * 
     * @param quicklist 目标 quicklist
     * @param value     要插入的值指针
     * @param sz        值的大小（字节）
     * @param where     插入位置（QL_HEAD 或 QL_TAIL）
     */
    void quicklistPush(quicklist *quicklist, void *value, const size_t sz, int where);

    /**
     * 将一个 ziplist 追加到 quicklist 尾部
     * 
     * @param quicklist 目标 quicklist
     * @param zl        要追加的 ziplist 指针
     */
    void quicklistAppendZiplist(quicklist *quicklist, unsigned char *zl);

    /**
     * 从 ziplist 中提取值并追加到 quicklist
     * 
     * @param quicklist 目标 quicklist
     * @param zl        源 ziplist 指针
     * @return 返回操作后的 quicklist 指针
     */
    quicklist *quicklistAppendValuesFromZiplist(quicklist *quicklist, unsigned char *zl);

    /**
     * 从 ziplist 创建一个新的 quicklist
     * 
     * @param fill     填充因子
     * @param compress 压缩深度
     * @param zl       源 ziplist 指针
     * @return 返回新创建的 quicklist 指针
     */
    quicklist *quicklistCreateFromZiplist(int fill, int compress, unsigned char *zl);

    /**
     * 在指定节点后插入一个元素
     * 
     * @param quicklist 目标 quicklist
     * @param node      参考节点
     * @param value     要插入的值指针
     * @param sz        值的大小（字节）
     */
    void quicklistInsertAfter(quicklist *quicklist, quicklistEntry *node, void *value, const size_t sz);

    /**
     * 在指定节点前插入一个元素
     * 
     * @param quicklist 目标 quicklist
     * @param node      参考节点
     * @param value     要插入的值指针
     * @param sz        值的大小（字节）
     */
    void quicklistInsertBefore(quicklist *quicklist, quicklistEntry *node, void *value, const size_t sz);

    /**
     * 删除迭代器当前指向的元素
     * 
     * @param iter  迭代器指针
     * @param entry 要删除的元素
     */
    void quicklistDelEntry(quicklistIter *iter, quicklistEntry *entry);

    /**
     * 替换指定索引位置的元素
     * 
     * @param quicklist 目标 quicklist
     * @param index     要替换的元素索引
     * @param data      新数据指针
     * @param sz        新数据大小（字节）
     * @return 成功返回 1，失败返回 0
     */
    int quicklistReplaceAtIndex(quicklist *quicklist, long index, void *data, int sz);

    /**
     * 删除指定范围内的元素
     * 
     * @param quicklist 目标 quicklist
     * @param start     起始索引
     * @param stop      结束索引
     * @return 删除的元素数量
     */
    int quicklistDelRange(quicklist *quicklist, const long start, const long stop);

    /**
     * 获取 quicklist 的迭代器
     * 
     * @param quicklist 目标 quicklist
     * @param direction 迭代方向（AL_START_HEAD 或 AL_START_TAIL）
     * @return 返回新创建的迭代器指针
     */
    quicklistIter *quicklistGetIterator(const quicklist *quicklist, int direction);

    /**
     * 获取指向指定索引位置的迭代器
     * 
     * @param quicklist 目标 quicklist
     * @param direction 迭代方向
     * @param idx       起始索引
     * @return 返回新创建的迭代器指针
     */
    quicklistIter *quicklistGetIteratorAtIdx(const quicklist *quicklist, int direction, const long long idx);

    /**
     * 获取迭代器的下一个元素
     * 
     * @param iter  迭代器指针
     * @param node  存储元素的结构体
     * @return 成功返回 1，到达末尾返回 0
     */
    int quicklistNext(quicklistIter *iter, quicklistEntry *entry);

    /**
     * 释放迭代器占用的内存
     * 
     * @param iter 要释放的迭代器指针
     */
    void quicklistReleaseIterator(quicklistIter *iter);

    /**
     * 复制一个 quicklist
     * 
     * @param orig 源 quicklist 指针
     * @return 返回复制后的新 quicklist 指针
     */
    quicklist *quicklistDup(quicklist *orig);

    /**
     * 获取指定索引位置的元素
     * 
     * @param quicklist 目标 quicklist
     * @param index     元素索引
     * @param entry     存储元素的结构体
     * @return 成功返回 1，失败返回 0
     */
    int quicklistIndex(const quicklist *quicklist, const long long index, quicklistEntry *entry);

    /**
     * 重置迭代器到头部
     * 
     * @param quicklist 目标 quicklist
     * @param li        迭代器指针
     */
    void quicklistRewind(quicklist *quicklist, quicklistIter *li);

    /**
     * 重置迭代器到尾部
     * 
     * @param quicklist 目标 quicklist
     * @param li        迭代器指针
     */
    void quicklistRewindTail(quicklist *quicklist, quicklistIter *li);

    /**
     * 将 quicklist 的尾部元素移到头部
     * 
     * @param quicklist 目标 quicklist
     */
    void quicklistRotate(quicklist *quicklist);

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
    int quicklistPopCustom(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *sval, void *(*saver)(unsigned char *data, unsigned int sz));

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
    int quicklistPop(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *slong);

    /**
     * 获取 quicklist 中的元素总数
     * 
     * @param ql 目标 quicklist
     * @return quicklist 中的元素数量
     */
    unsigned long quicklistCount(const quicklist *ql);

    /**
     * 比较两个 quicklist 节点的数据
     * 
     * @param p1     第一个数据指针
     * @param p2     第二个数据指针
     * @param p2_len 第二个数据的长度
     * @return 相等返回 1，不等返回 0
     */
    int quicklistCompare(unsigned char *p1, unsigned char *p2, int p2_len);

    /**
     * 获取 LZF 压缩节点的数据
     * 
     * @param node  目标节点
     * @param data  存储解压后数据的指针
     * @return 返回数据大小（字节）
     */
    size_t quicklistGetLzf(const quicklistNode *node, void **data);

    /* Bookmarks 相关函数 */

    /**
     * 创建一个 quicklist 书签
     * 
     * @param ql_ref  quicklist 指针的引用
     * @param name    书签名称
     * @param node    关联的节点
     * @return 成功返回 1，失败返回 0
     */
    int quicklistBookmarkCreate(quicklist **ql_ref, const char *name, quicklistNode *node);

    /**
     * 删除指定名称的书签
     * 
     * @param ql    目标 quicklist
     * @param name  书签名称
     * @return 成功返回 1，失败返回 0
     */
    int quicklistBookmarkDelete(quicklist *ql, const char *name);

    /**
     * 查找指定名称的书签
     * 
     * @param ql    目标 quicklist
     * @param name  书签名称
     * @return 找到返回关联的节点指针，未找到返回 NULL
     */
    quicklistNode *quicklistBookmarkFind(quicklist *ql, const char *name);

    /**
     * 清除 quicklist 中所有书签
     * 
     * @param ql 目标 quicklist
     */
    void quicklistBookmarksClear(quicklist *ql);


public:
    void quicklistNodeUpdateSz(quicklistNode *node);
    quicklistNode *quicklistCreateNode(void);
    void _quicklistInsertNodeBefore(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node);
    void __quicklistInsertNode(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node, int after); 
    void __quicklistCompress(const quicklist *quicklist,quicklistNode *node);
    int __quicklistCompressNode(quicklistNode *node);
    void quicklistCompress(const quicklist *_ql,quicklistNode *_node);
    void quicklistCompressNode(quicklistNode *_node);
    void quicklistRecompressOnly(const quicklist *_ql,quicklistNode *_node);
    bool quicklistAllowsCompression(const quicklist *_ql);
    void quicklistDecompressNode(quicklistNode *_node);
    int __quicklistDecompressNode(quicklistNode *node);
    int _quicklistNodeAllowInsert(const quicklistNode *node,const int fill, const size_t sz);
    int _quicklistNodeSizeMeetsOptimizationRequirement(const size_t sz,const int fill);
    void _quicklistInsertNodeAfter(quicklist *quicklist,quicklistNode *old_node,quicklistNode *new_node);
    void _quicklistInsert(quicklist *quicklist, quicklistEntry *entry,void *value, const size_t sz, int after);
    void quicklistDecompressNodeForUse(quicklistNode *node) ;
    quicklistNode *_quicklistSplitNode(quicklistNode *node, int offset,int after);
    void _quicklistMergeNodes(quicklist *quicklist, quicklistNode *center) ;
    int _quicklistNodeAllowMerge(const quicklistNode *a,const quicklistNode *b,const int fill);
    quicklistNode *_quicklistZiplistMerge(quicklist *quicklist,quicklistNode *a,quicklistNode *b) ; 
    void __quicklistDelNode(quicklist *quicklist,quicklistNode *node) ;
    void _quicklistBookmarkDelete(quicklist *ql, quicklistBookmark *bm);
    quicklistBookmark *_quicklistBookmarkFindByNode(quicklist *ql, quicklistNode *node);
    quicklistBookmark *_quicklistBookmarkFindByName(quicklist *ql, const char *name) ;
    int quicklistDelIndex(quicklist *quicklist, quicklistNode *node,unsigned char **p) ;
    void quicklistDeleteIfEmpty(quicklist *quicklist, quicklistNode *node);
    static unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);
    static unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);
private:
    ziplistCreate *ziplistCreateInstance;
    toolFunc *toolFuncInstance;
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
 #endif
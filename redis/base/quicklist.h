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
    int quicklistIndex(const quicklist *quicklist, const long long idx, quicklistEntry *entry);

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
    
    /**
     * 更新指定quicklist节点的大小信息。
     * 
     * @param node 待更新的quicklist节点
     */
    void quicklistNodeUpdateSz(quicklistNode *node);

    /**
     * 创建一个新的quicklist节点。
     * 
     * @return 新创建的quicklist节点，如果失败则返回NULL
     */
    quicklistNode *quicklistCreateNode(void);

    /**
     * 在指定节点前插入一个新节点。
     * 
     * @param quicklist quicklist链表
     * @param old_node  参考节点
     * @param new_node  待插入的新节点
     */
    void _quicklistInsertNodeBefore(quicklist *quicklist, quicklistNode *old_node, quicklistNode *new_node);

    /**
     * 在指定位置插入一个新节点（可选择前插或后插）。
     * 
     * @param quicklist quicklist链表
     * @param old_node  参考节点
     * @param new_node  待插入的新节点
     * @param after     插入位置标志，1表示在参考节点后插入，0表示在参考节点前插入
     */
    void __quicklistInsertNode(quicklist *quicklist, quicklistNode *old_node, quicklistNode *new_node, int after); 

    /**
     * 对指定节点执行压缩操作（内部使用）。
     * 
     * @param quicklist quicklist链表
     * @param node      待压缩的节点
     */
    void __quicklistCompress(const quicklist *quicklist, quicklistNode *node);

    /**
     * 尝试压缩指定节点并返回压缩结果。
     * 
     * @param node 待压缩的节点
     * @return 压缩成功返回1，失败返回0
     */
    int __quicklistCompressNode(quicklistNode *node);

    /**
     * 对指定节点执行压缩操作（外部接口）。
     * 
     * @param _ql    quicklist链表
     * @param _node  待压缩的节点
     */
    void quicklistCompress(const quicklist *_ql, quicklistNode *_node);

    /**
     * 对指定节点执行压缩操作（节点级别接口）。
     * 
     * @param _node 待压缩的节点
     */
    void quicklistCompressNode(quicklistNode *_node);

    /**
     * 仅对指定节点重新执行压缩操作（不影响其他节点）。
     * 
     * @param _ql    quicklist链表
     * @param _node  待重新压缩的节点
     */
    void quicklistRecompressOnly(const quicklist *_ql, quicklistNode *_node);

    /**
     * 检查指定quicklist是否允许执行压缩操作。
     * 
     * @param _ql quicklist链表
     * @return 允许压缩返回true，否则返回false
     */
    bool quicklistAllowsCompression(const quicklist *_ql);

    /**
     * 解压缩指定节点。
     * 
     * @param _node 待解压缩的节点
     */
    void quicklistDecompressNode(quicklistNode *_node);

    /**
     * 尝试解压缩指定节点并返回解压缩结果。
     * 
     * @param node 待解压缩的节点
     * @return 解压缩成功返回1，失败返回0
     */
    int __quicklistDecompressNode(quicklistNode *node);

    /**
     * 检查节点是否允许插入新元素。
     * 
     * @param node 待检查的节点
     * @param fill 填充因子
     * @param sz   待插入元素的大小
     * @return 允许插入返回1，否则返回0
     */
    int _quicklistNodeAllowInsert(const quicklistNode *node, const int fill, const size_t sz);

    /**
     * 检查节点大小是否满足优化要求。
     * 
     * @param sz   节点大小
     * @param fill 填充因子
     * @return 满足要求返回1，否则返回0
     */
    int _quicklistNodeSizeMeetsOptimizationRequirement(const size_t sz, const int fill);

    /**
     * 在指定节点后插入一个新节点。
     * 
     * @param quicklist quicklist链表
     * @param old_node  参考节点
     * @param new_node  待插入的新节点
     */
    void _quicklistInsertNodeAfter(quicklist *quicklist, quicklistNode *old_node, quicklistNode *new_node);

    /**
     * 在指定位置插入一个新元素。
     * 
     * @param quicklist quicklist链表
     * @param entry     参考位置
     * @param value     待插入的值
     * @param sz        待插入值的大小
     * @param after     插入位置标志，1表示在参考位置后插入，0表示在参考位置前插入
     */
    void _quicklistInsert(quicklist *quicklist, quicklistEntry *entry, void *value, const size_t sz, int after);

    /**
     * 为使用而解压缩节点（使用后可能需要重新压缩）。
     * 
     * @param node 待解压缩的节点
     */
    void quicklistDecompressNodeForUse(quicklistNode *node);

    /**
     * 分割指定节点为两个节点。
     * 
     * @param node   待分割的节点
     * @param offset 分割位置偏移量
     * @param after  分割方式标志，1表示在偏移量后分割，0表示在偏移量前分割
     * @return 分割后生成的新节点
     */
    quicklistNode *_quicklistSplitNode(quicklistNode *node, int offset, int after);

    /**
     * 合并相邻节点。
     * 
     * @param quicklist quicklist链表
     * @param center    中心节点（将合并其相邻节点）
     */
    void _quicklistMergeNodes(quicklist *quicklist, quicklistNode *center);

    /**
     * 检查两个节点是否允许合并。
     * 
     * @param a    第一个节点
     * @param b    第二个节点
     * @param fill 填充因子
     * @return 允许合并返回1，否则返回0
     */
    int _quicklistNodeAllowMerge(const quicklistNode *a, const quicklistNode *b, const int fill);

    /**
     * 合并两个节点的ziplist数据。
     * 
     * @param quicklist quicklist链表
     * @param a         第一个节点
     * @param b         第二个节点
     * @return 合并后的节点
     */
    quicklistNode *_quicklistZiplistMerge(quicklist *quicklist, quicklistNode *a, quicklistNode *b); 

    /**
     * 删除指定节点（内部使用）。
     * 
     * @param quicklist quicklist链表
     * @param node      待删除的节点
     */
    void __quicklistDelNode(quicklist *quicklist, quicklistNode *node);

    /**
     * 删除指定书签。
     * 
     * @param ql quicklist链表
     * @param bm 待删除的书签
     */
    void _quicklistBookmarkDelete(quicklist *ql, quicklistBookmark *bm);

    /**
     * 根据节点查找对应的书签。
     * 
     * @param ql   quicklist链表
     * @param node 目标节点
     * @return 找到的书签，如果未找到则返回NULL
     */
    quicklistBookmark *_quicklistBookmarkFindByNode(quicklist *ql, quicklistNode *node);

    /**
     * 根据名称查找对应的书签。
     * 
     * @param ql   quicklist链表
     * @param name 书签名称
     * @return 找到的书签，如果未找到则返回NULL
     */
    quicklistBookmark *_quicklistBookmarkFindByName(quicklist *ql, const char *name);

    /**
     * 删除指定索引位置的元素。
     * 
     * @param quicklist quicklist链表
     * @param node      元素所在的节点
     * @param p         指向元素的指针
     * @return 删除成功返回1，失败返回0
     */
    int quicklistDelIndex(quicklist *quicklist, quicklistNode *node, unsigned char **p);

    /**
     * 如果节点为空则删除该节点。
     * 
     * @param quicklist quicklist链表
     * @param node      待检查的节点
     */
    void quicklistDeleteIfEmpty(quicklist *quicklist, quicklistNode *node);

    /**
     * 获取ziplist中下一个元素的指针。
     * 
     * @param zl 指向ziplist的指针
     * @param p  当前元素的指针
     * @return 下一个元素的指针，如果没有下一个元素则返回NULL
     */
    static unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);

    /**
     * 获取ziplist中前一个元素的指针。
     * 
     * @param zl 指向ziplist的指针
     * @param p  当前元素的指针
     * @return 前一个元素的指针，如果没有前一个元素则返回NULL
     */
    static unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);

    /**
     * quicklist数据保存器（用于序列化数据）。
     * 
     * @param data 待保存的数据
     * @param sz   数据大小
     * @return 保存后的数据指针
     */
    static void *_quicklistSaver(unsigned char *data, unsigned int sz);
private:
    ziplistCreate *ziplistCreateInstance;
    toolFunc *toolFuncInstance;
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
 #endif
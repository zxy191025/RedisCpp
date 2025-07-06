/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/05
 * All rights reserved. No one may copy or transfer.
 * Description: 基数树也称为 压缩前缀树（Compact Prefix Tree），是一种空间高效的前缀树变体，用于快速存储和检索字符串键。
 */
#ifndef REDIS_BASE_RAX_H
#define REDIS_BASE_RAX_H
#include "define.h"
#include <stdint.h>
#include <stddef.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//

#define RAX_NODE_MAX_SIZE ((1<<29)-1)
#define RAX_STACK_STATIC_ITEMS 32

/* 基数树迭代器的状态被封装在这个数据结构中。 */
#define RAX_ITER_STATIC_LEN 128  /* 迭代器静态键缓冲区的默认长度 */
#define RAX_ITER_JUST_SEEKED (1<<0) /* 迭代器刚完成定位操作。首次迭代返回当前元素并清除此标志 */
#define RAX_ITER_EOF (1<<1)    /* 迭代已到达末尾 */
#define RAX_ITER_SAFE (1<<2)   /* 安全迭代器，允许在迭代过程中进行修改操作，但性能较低 */

/* raxNode 结构体：基数树的基本节点结构 */
typedef struct raxNode {
    uint32_t iskey:1;     /* 该节点是否包含键标识（1表示是键节点） */
    uint32_t isnull:1;    /* 关联的值为NULL（不存储值） */
    uint32_t iscompr:1;   /* 节点是否为压缩状态 */
    uint32_t size:29;     /* 子节点数量（非压缩时）或压缩字符串长度（压缩时） */
    unsigned char data[];  /* 柔性数组成员，存储节点数据（字符序列或子节点指针） */
} raxNode;

/* rax 结构体：基数树的整体结构 */
typedef struct rax {
    raxNode *head;    /* 基数树的根节点指针 */
    uint64_t numele;  /* 树中存储的键值对数量 */
    uint64_t numnodes; /* 树中的节点总数 */
} rax;

/* raxStack 结构体：基数树遍历辅助栈（用于保存父节点）
 * 设计原因：节点未包含父节点字段（节省空间），需要遍历时用辅助栈记录路径
 */
typedef struct raxStack {
    void **stack;      /* 栈指针数组（指向静态数组或堆分配数组） */
    size_t items, maxitems; /* 已存储项数和总容量 */
    /* 当项数不超过 RAXSTACK_STACK_ITEMS 时，使用静态数组避免堆分配 */
    void *static_items[RAX_STACK_STATIC_ITEMS];
    int oom;           /* 标记是否因内存不足导致入栈失败 */
} raxStack;

/* raxNodeCallback 函数指针类型：基数树节点回调函数
 * 用途：迭代器遍历节点时的通知回调（包括非键节点）
 * 返回值：若返回true表示回调修改了节点指针，迭代器需更新树内部指针
 * 应用场景：底层树结构分析、节点内存碎片整理（如Redis的应用）
 * 限制：当前仅支持正向迭代（raxNext）
 */
typedef int (*raxNodeCallback)(raxNode **noderef);

/* raxIterator 结构体：基数树迭代器 */
typedef struct raxIterator {
    int flags;         /* 迭代器标志位 */
    rax *rt;           /* 迭代的目标基数树 */
    unsigned char *key; /* 当前迭代的键字符串 */
    void *data;        /* 当前键关联的值 */
    size_t key_len;    /* 当前键的长度 */
    size_t key_max;    /* 键缓冲区的最大长度 */
    /* 静态键缓冲区：避免短键的动态内存分配 */
    unsigned char key_static_string[RAX_ITER_STATIC_LEN];
    raxNode *node;     /* 当前节点（仅用于不安全迭代） */
    raxStack stack;    /* 迭代路径栈（用于保存遍历路径上的节点） */
    raxNodeCallback node_cb; /* 可选节点回调函数（默认NULL） */
} raxIterator;

extern void *raxNotFound;

class raxCreate
{
public:
    raxCreate();
    ~raxCreate();
public:
    /**
     * 创建一个新的空基数树实例。
     * @return 返回初始化后的基数树根节点指针，失败时返回 NULL
     */
    rax *raxNew(void);

    /**
     * 向基数树中插入一个键值对。
     * @param rax 目标基数树实例
     * @param s 待插入的键（字符串）
     * @param len 键的长度（字节）
     * @param data 关联的值（可存储任意类型指针）
     * @param old 若不为 NULL，则存储旧值的指针（若键已存在）
     * @return 成功时返回 1，键已存在时返回 0，内存分配失败时返回 -1
     */
    int raxInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);

    /**
     * 尝试插入键值对（仅在键不存在时插入）。
     * @param rax 目标基数树实例
     * @param s 待插入的键
     * @param len 键的长度
     * @param data 关联的值
     * @param old 若不为 NULL，存储旧值指针（若键已存在）
     * @return 成功插入返回 1，键已存在返回 0，错误返回 -1
     */
    int raxTryInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old);

    /**
     * 从基数树中删除指定键。
     * @param rax 目标基数树实例
     * @param s 待删除的键
     * @param len 键的长度
     * @param old 若不为 NULL，存储被删除的值的指针
     * @return 成功删除返回 1，键不存在返回 0
     */
    int raxRemove(rax *rax, unsigned char *s, size_t len, void **old);

    /**
     * 在基数树中查找指定键对应的值。
     * @param rax 目标基数树实例
     * @param s 待查找的键
     * @param len 键的长度
     * @return 找到时返回关联的值指针，未找到返回 NULL
     */
    void *raxFind(rax *rax, unsigned char *s, size_t len);

    /**
     * 释放基数树占用的所有内存（包括节点和值）。
     * @param rax 待释放的基数树实例
     */
    void raxFree(rax *rax);

    /**
     * 使用自定义回调函数释放基数树。
     * @param rax 待释放的基数树实例
     * @param free_callback 释放值的回调函数（可为 NULL）
     */
    void raxFreeWithCallback(rax *rax, void (*free_callback)(void*));

    /**
     * 初始化基数树迭代器。
     * @param it 迭代器实例
     * @param rt 目标基数树
     */
    void raxStart(raxIterator *it, rax *rt);

    /**
     * 将迭代器定位到满足条件的元素。
     * @param it 迭代器实例
     * @param op 操作符（如 ">"、"<="、"=" 等）
     * @param ele 目标元素
     * @param len 元素长度
     * @return 成功定位返回 1，未找到匹配元素返回 0
     */
    int raxSeek(raxIterator *it, const char *op, unsigned char *ele, size_t len);

    /**
     * 将迭代器移动到下一个元素（按字典序）。
     * @param it 迭代器实例
     * @return 存在下一个元素返回 1，到达末尾返回 0
     */
    int raxNext(raxIterator *it);

    /**
     * 将迭代器移动到上一个元素（按字典序）。
     * @param it 迭代器实例
     * @return 存在上一个元素返回 1，到达起始位置返回 0
     */
    int raxPrev(raxIterator *it);

    /**
     * 随机游走迭代器指定步数。
     * @param it 迭代器实例
     * @param steps 游走步数
     * @return 实际游走的步数
     */
    int raxRandomWalk(raxIterator *it, size_t steps);

    /**
     * 比较迭代器当前元素与指定键的关系。
     * @param iter 迭代器实例
     * @param op 比较操作符
     * @param key 比较键
     * @param key_len 键长度
     * @return 满足关系返回 1，不满足返回 0
     */
    int raxCompare(raxIterator *iter, const char *op, unsigned char *key, size_t key_len);

    /**
     * 停止迭代并释放迭代器资源。
     * @param it 迭代器实例
     */
    void raxStop(raxIterator *it);

    /**
     * 检查迭代器是否已到达末尾。
     * @param it 迭代器实例
     * @return 已到达末尾返回 1，否则返回 0
     */
    int raxEOF(raxIterator *it);

    /**
     * 打印基数树的调试信息（结构和内容）。
     * @param rax 目标基数树实例
     */
    void raxShow(rax *rax);

    /**
     * 获取基数树中存储的键值对数量。
     * @param rax 目标基数树实例
     * @return 键值对数量
     */
    uint64_t raxSize(rax *rax);

    /**
     * 触碰指定节点（更新访问统计信息）。
     * @param n 目标节点
     * @return 节点被触碰的次数
     */
    unsigned long raxTouch(raxNode *n);

    /**
     * 启用或禁用调试消息输出。
     * @param onoff 1 启用，0 禁用
     */
    void raxSetDebugMsg(int onoff);

    /**
     * 直接设置节点关联的数据。
     * @param n 目标节点
     * @param data 待设置的数据指针
     */
    void raxSetData(raxNode *n, void *data);

    /**
     * 创建一个新的前缀树节点
     * @param children 子节点数量
     * @param datafield 是否包含数据字段
     * @return 返回新创建的节点指针，失败时返回NULL
     */
    raxNode *raxNewNode(size_t children, int datafield);

    /**
     * 向Rax树中插入键值对
     * @param rax 目标Rax树
     * @param s 待插入的键(字符串)
     * @param len 键的长度
     * @param data 待插入的值
     * @param old 如果不为NULL，用于存储旧值的指针
     * @param overwrite 是否覆盖已存在的键
     * @return 成功返回1，键已存在且overwrite为0返回0，失败返回-1
     */
    int raxGenericInsert(rax *rax, unsigned char *s, size_t len, void *data, void **old, int overwrite);

    /**
     * 执行Rax树的低层遍历操作
     * @param rax 目标Rax树
     * @param s 搜索键
     * @param len 键长度
     * @param stopnode 存储搜索停止的节点
     * @param plink 存储父节点链接
     * @param splitpos 存储分割位置
     * @param ts 遍历栈
     * @return 返回匹配的长度
     */
    size_t raxLowWalk(rax *rax, unsigned char *s, size_t len, raxNode **stopnode, raxNode ***plink, int *splitpos, raxStack *ts);

    /**
     * 为节点重新分配内存以存储数据
     * @param n 目标节点
     * @param data 待存储的数据
     * @return 返回重新分配后的节点指针
     */
    raxNode *raxReallocForData(raxNode *n, void *data);

    /**
     * 获取节点存储的数据
     * @param n 目标节点
     * @return 返回节点存储的数据指针
     */
    void *raxGetData(raxNode *n);

    /**
     * 压缩前缀树节点
     * @param n 目标节点
     * @param s 键字符串
     * @param len 键长度
     * @param child 子节点指针
     * @return 返回压缩后的节点指针
     */
    raxNode *raxCompressNode(raxNode *n, unsigned char *s, size_t len, raxNode **child);

    /**
     * 向节点添加子节点
     * @param n 父节点
     * @param c 子节点对应的字符
     * @param childptr 子节点指针
     * @param parentlink 父节点链接
     * @return 返回添加子节点后的父节点指针
     */
    raxNode *raxAddChild(raxNode *n, unsigned char c, raxNode **childptr, raxNode ***parentlink);

    /**
     * 向栈中压入元素
     * @param ts 目标栈
     * @param ptr 待压入的指针
     * @return 成功返回1，失败返回0
     */
    int raxStackPush(raxStack *ts, void *ptr);

    /**
     * 初始化栈
     * @param ts 目标栈
     */
    void raxStackInit(raxStack *ts);

    /**
     * 从栈中弹出元素
     * @param ts 目标栈
     * @return 返回弹出的元素指针，栈为空时返回NULL
     */
    void *raxStackPop(raxStack *ts);

    /**
     * 移除父节点中的子节点
     * @param parent 父节点
     * @param child 待移除的子节点
     * @return 返回移除子节点后的父节点指针
     */
    raxNode *raxRemoveChild(raxNode *parent, raxNode *child);

    /**
     * 查找子节点在父节点中的链接位置
     * @param parent 父节点
     * @param child 子节点
     * @return 返回指向子节点链接的指针
     */
    raxNode **raxFindParentLink(raxNode *parent, raxNode *child);

    /**
     * 查找迭代器当前位置的最大键
     * @param it 迭代器
     * @return 成功返回1，失败返回0
     */
    int raxSeekGreatest(raxIterator *it);

    /**
     * 向迭代器添加字符
     * @param it 迭代器
     * @param s 字符数组
     * @param len 字符长度
     * @return 成功返回1，失败返回0
     */
    int raxIteratorAddChars(raxIterator *it, unsigned char *s, size_t len);

    /**
     * 迭代器向前移动一步
     * @param it 迭代器
     * @param noup 是否禁止向上移动
     * @return 成功返回1，无更多元素返回0，失败返回-1
     */
    int raxIteratorPrevStep(raxIterator *it, int noup);

    /**
     * 从迭代器中删除指定数量的字符
     * @param it 迭代器
     * @param count 删除的字符数量
     */
    void raxIteratorDelChars(raxIterator *it, size_t count);

    /**
     * 迭代器向后移动一步
     * @param it 迭代器
     * @param noup 是否禁止向上移动
     * @return 成功返回1，无更多元素返回0，失败返回-1
     */
    int raxIteratorNextStep(raxIterator *it, int noup);

    /**
     * 递归释放Rax树节点
     * @param rax Rax树
     * @param n 当前节点
     * @param free_callback 释放数据的回调函数
     */
    void raxRecursiveFree(rax *rax, raxNode *n, void (*free_callback)(void*));

    /**
     * 递归显示Rax树结构
     * @param level 当前层级
     * @param lpad 左填充空格数
     * @param n 当前节点
     */
    void raxRecursiveShow(int level, int lpad, raxNode *n);
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif


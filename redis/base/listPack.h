/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/06
 * All rights reserved. No one may copy or transfer.
 * Description: listPack header
 */
#ifndef REDIS_BASE_LISTPACK
#define REDIS_BASE_LISTPACK
#include "define.h"
#include <stdlib.h>
#include <stdint.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class listPackCreate
{
public:
    listPackCreate();
    ~listPackCreate();
public:
    /**
     * 分配一个新的链表内存块，初始容量为capacity字节。
     * @param capacity 所需的初始容量（字节）
     * @return 指向新分配内存块的指针，失败时返回NULL
     */
    unsigned char *lpNew(size_t capacity);

    /**
     * 释放链表内存块占用的内存。
     * @param lp 指向链表内存块的指针
     */
    void lpFree(unsigned char *lp);

    /**
     * 收缩链表内存块以匹配其实际使用大小，优化内存占用。
     * @param lp 指向链表内存块的指针
     * @return 指向收缩后内存块的指针，可能与原指针不同
     */
    unsigned char* lpShrinkToFit(unsigned char *lp);

    /**
     * 在指定位置插入一个元素到链表中。
     * @param lp 指向链表内存块的指针
     * @param ele 要插入的元素数据
     * @param size 元素大小（字节）
     * @param p 插入位置的参考指针
     * @param where 插入位置标识：0=之前，1=之后
     * @param newp 用于返回新插入元素的指针
     * @return 指向更新后链表的指针
     */
    unsigned char *lpInsert(unsigned char *lp, unsigned char *ele, uint32_t size, unsigned char *p, int where, unsigned char **newp);

    /**
     * 在链表末尾追加一个元素。
     * @param lp 指向链表内存块的指针
     * @param ele 要追加的元素数据
     * @param size 元素大小（字节）
     * @return 指向更新后链表的指针
     */
    unsigned char *lpAppend(unsigned char *lp, unsigned char *ele, uint32_t size);

    /**
     * 从链表中删除指定元素。
     * @param lp 指向链表内存块的指针
     * @param p 指向要删除元素的指针
     * @param newp 用于返回删除元素后的下一个元素指针
     * @return 指向更新后链表的指针
     */
    unsigned char *lpDelete(unsigned char *lp, unsigned char *p, unsigned char **newp);

    /**
     * 获取链表中的元素数量。
     * @param lp 指向链表内存块的指针
     * @return 链表中的元素数量
     */
    uint32_t lpLength(unsigned char *lp);

    /**
     * 获取当前元素的数据部分，并更新计数器。
     * @param p 指向当前元素的指针
     * @param count 计数器指针，用于跟踪元素位置
     * @param intbuf 用于存储整数值的缓冲区
     * @return 指向下一个元素的指针
     */
    unsigned char *lpGet(unsigned char *p, int64_t *count, unsigned char *intbuf);

    /**
     * 获取链表的第一个元素。
     * @param lp 指向链表内存块的指针
     * @return 指向第一个元素的指针
     */
    unsigned char *lpFirst(unsigned char *lp);

    /**
     * 获取链表的最后一个元素。
     * @param lp 指向链表内存块的指针
     * @return 指向最后一个元素的指针
     */
    unsigned char *lpLast(unsigned char *lp);

    /**
     * 获取指定元素的下一个元素。
     * @param lp 指向链表内存块的指针
     * @param p 指向当前元素的指针
     * @return 指向下一个元素的指针，若当前元素是最后一个则返回NULL
     */
    unsigned char *lpNext(unsigned char *lp, unsigned char *p);

    /**
     * 获取指定元素的前一个元素。
     * @param lp 指向链表内存块的指针
     * @param p 指向当前元素的指针
     * @return 指向前一个元素的指针，若当前元素是第一个则返回NULL
     */
    unsigned char *lpPrev(unsigned char *lp, unsigned char *p);

    /**
     * 获取链表占用的总字节数。
     * @param lp 指向链表内存块的指针
     * @return 链表占用的总字节数
     */
    uint32_t lpBytes(unsigned char *lp);

    /**
     * 定位到链表中指定索引位置的元素。
     * @param lp 指向链表内存块的指针
     * @param index 要定位的元素索引（从0开始）
     * @return 指向指定索引元素的指针，若索引超出范围则返回NULL
     */
    unsigned char *lpSeek(unsigned char *lp, long index);

    /**
     * 验证链表的完整性。
     * @param lp 指向链表内存块的指针
     * @param size 链表预期大小（字节）
     * @param deep 是否进行深度验证（检查每个元素）
     * @return 验证结果：0=完整，非0=不完整
     */
    int lpValidateIntegrity(unsigned char *lp, size_t size, int deep);

    /**
     * 获取链表中第一个有效元素。
     * @param lp 指向链表内存块的指针
     * @return 指向第一个有效元素的指针，若没有有效元素则返回NULL
     */
    unsigned char *lpValidateFirst(unsigned char *lp);

    /**
     * 获取链表中下一个有效元素，并验证其有效性。
     * @param lp 指向链表内存块的指针
     * @param pp 指向当前元素指针的指针，函数会更新其指向下一个有效元素
     * @param lpbytes 链表总字节数
     * @return 验证结果：0=有效，非0=无效
     */
    int lpValidateNext(unsigned char *lp, unsigned char **pp, size_t lpbytes);

    /**
     * 跳过当前元素，指向下一个元素的起始位置。
     * @param p 指向当前元素的指针
     * @return 指向下一个元素的指针
     */
    unsigned char *lpSkip(unsigned char *p);

    /**
     * 确定元素的编码类型并计算编码后的长度。
     * @param ele 要编码的元素数据
     * @param size 元素大小（字节）
     * @param intenc 用于存储整数编码类型的缓冲区
     * @param enclen 用于返回编码后的长度
     * @return 编码类型
     */
    int lpEncodeGetType(unsigned char *ele, uint32_t size, unsigned char *intenc, uint64_t *enclen);

    /**
     * 计算编码数据的反向长度（用于解码）。
     * @param buf 指向编码数据的指针
     * @param l 编码数据的长度
     * @return 反向长度值
     */
    unsigned long lpEncodeBacklen(unsigned char *buf, uint64_t l);

    /**
     * 不安全地获取当前元素编码后的大小（不验证指针有效性）。
     * @param p 指向当前元素的指针
     * @return 编码后的大小（字节）
     */
    uint32_t lpCurrentEncodedSizeUnsafe(unsigned char *p);

    /**
     * 将字符串编码并存储到缓冲区中。
     * @param buf 目标缓冲区
     * @param s 源字符串
     * @param len 字符串长度
     */
    void lpEncodeString(unsigned char *buf, unsigned char *s, uint32_t len);

    /**
     * 将字符串转换为64位整数值。
     * @param s 源字符串
     * @param slen 字符串长度
     * @param value 用于存储转换结果的指针
     * @return 转换结果：0=成功，非0=失败
     */
    int lpStringToInt64(const char *s, unsigned long slen, int64_t *value);

    /**
     * 断言指定位置的元素是有效的链表条目。
     * @param lp 指向链表内存块的指针
     * @param lpbytes 链表总字节数
     * @param p 指向要验证元素的指针
     */
    void lpAssertValidEntry(unsigned char* lp, size_t lpbytes, unsigned char *p);

    /**
     * 解码反向长度值。
     * @param p 指向编码数据的指针
     * @return 解码后的反向长度值
     */
    uint64_t lpDecodeBacklen(unsigned char *p);

    /**
     * 获取当前元素编码后的实际字节大小。
     * @param p 指向当前元素的指针
     * @return 编码后的实际字节大小
     */
    uint32_t lpCurrentEncodedSizeBytes(unsigned char *p);
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
 #endif
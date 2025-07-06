/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/06
 * All rights reserved. No one may copy or transfer.
 * Description: 实现 List Pack（紧凑列表） 的核心头文件，用于高效存储和操作列表数据。
 * List Pack 是一种紧凑、无冗余、双向可遍历的数据结构，旨在替代早期的 ziplist，解决其在连锁更新（Cascade Update）时的性能问题。
 */
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "listPack.h"
#include "zmallocDf.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
#define LP_ENCODING_7BIT_UINT 0
#define LP_ENCODING_7BIT_UINT_MASK 0x80
#define LP_ENCODING_IS_7BIT_UINT(byte) (((byte)&LP_ENCODING_7BIT_UINT_MASK)==LP_ENCODING_7BIT_UINT)

#define LP_ENCODING_6BIT_STR 0x80
#define LP_ENCODING_6BIT_STR_MASK 0xC0
#define LP_ENCODING_IS_6BIT_STR(byte) (((byte)&LP_ENCODING_6BIT_STR_MASK)==LP_ENCODING_6BIT_STR)

#define LP_ENCODING_13BIT_INT 0xC0
#define LP_ENCODING_13BIT_INT_MASK 0xE0
#define LP_ENCODING_IS_13BIT_INT(byte) (((byte)&LP_ENCODING_13BIT_INT_MASK)==LP_ENCODING_13BIT_INT)

#define LP_ENCODING_12BIT_STR 0xE0
#define LP_ENCODING_12BIT_STR_MASK 0xF0
#define LP_ENCODING_IS_12BIT_STR(byte) (((byte)&LP_ENCODING_12BIT_STR_MASK)==LP_ENCODING_12BIT_STR)

#define LP_ENCODING_16BIT_INT 0xF1
#define LP_ENCODING_16BIT_INT_MASK 0xFF
#define LP_ENCODING_IS_16BIT_INT(byte) (((byte)&LP_ENCODING_16BIT_INT_MASK)==LP_ENCODING_16BIT_INT)

#define LP_ENCODING_24BIT_INT 0xF2
#define LP_ENCODING_24BIT_INT_MASK 0xFF
#define LP_ENCODING_IS_24BIT_INT(byte) (((byte)&LP_ENCODING_24BIT_INT_MASK)==LP_ENCODING_24BIT_INT)

#define LP_ENCODING_32BIT_INT 0xF3
#define LP_ENCODING_32BIT_INT_MASK 0xFF
#define LP_ENCODING_IS_32BIT_INT(byte) (((byte)&LP_ENCODING_32BIT_INT_MASK)==LP_ENCODING_32BIT_INT)

#define LP_ENCODING_64BIT_INT 0xF4
#define LP_ENCODING_64BIT_INT_MASK 0xFF
#define LP_ENCODING_IS_64BIT_INT(byte) (((byte)&LP_ENCODING_64BIT_INT_MASK)==LP_ENCODING_64BIT_INT)

#define LP_ENCODING_32BIT_STR 0xF0
#define LP_ENCODING_32BIT_STR_MASK 0xFF
#define LP_ENCODING_IS_32BIT_STR(byte) (((byte)&LP_ENCODING_32BIT_STR_MASK)==LP_ENCODING_32BIT_STR)

#define LP_EOF 0xFF

#define LP_ENCODING_6BIT_STR_LEN(p) ((p)[0] & 0x3F)
#define LP_ENCODING_12BIT_STR_LEN(p) ((((p)[0] & 0xF) << 8) | (p)[1])
#define LP_ENCODING_32BIT_STR_LEN(p) (((uint32_t)(p)[1]<<0) | \
                                      ((uint32_t)(p)[2]<<8) | \
                                      ((uint32_t)(p)[3]<<16) | \
                                      ((uint32_t)(p)[4]<<24))

#define lpGetTotalBytes(p)           (((uint32_t)(p)[0]<<0) | \
                                      ((uint32_t)(p)[1]<<8) | \
                                      ((uint32_t)(p)[2]<<16) | \
                                      ((uint32_t)(p)[3]<<24))

#define lpGetNumElements(p)          (((uint32_t)(p)[4]<<0) | \
                                      ((uint32_t)(p)[5]<<8))
#define lpSetTotalBytes(p,v) do { \
    (p)[0] = (v)&0xff; \
    (p)[1] = ((v)>>8)&0xff; \
    (p)[2] = ((v)>>16)&0xff; \
    (p)[3] = ((v)>>24)&0xff; \
} while(0)

#define lpSetNumElements(p,v) do { \
    (p)[4] = (v)&0xff; \
    (p)[5] = ((v)>>8)&0xff; \
} while(0)

/* Validates that 'p' is not ouside the listpack.
 * All function that return a pointer to an element in the listpack will assert
 * that this element is valid, so it can be freely used.
 * Generally functions such lpNext and lpDelete assume the input pointer is
 * already validated (since it's the return value of another function). */
#define ASSERT_INTEGRITY(lp, p) do { \
    assert((p) >= (lp)+LP_HDR_SIZE && (p) < (lp)+lpGetTotalBytes((lp))); \
} while (0)

/* Similar to the above, but validates the entire element lenth rather than just
 * it's pointer. */
#define ASSERT_INTEGRITY_LEN(lp, p, len) do { \
    assert((p) >= (lp)+LP_HDR_SIZE && (p)+(len) < (lp)+lpGetTotalBytes((lp))); \
} while (0)

listPackCreate::listPackCreate()
{

}
listPackCreate::~listPackCreate()
{
    
}
/**
 * 分配一个新的链表内存块，初始容量为capacity字节。
 * @param capacity 所需的初始容量（字节）
 * @return 指向新分配内存块的指针，失败时返回NULL
 */
unsigned char *listPackCreate::lpNew(size_t capacity)
{
    unsigned char *lp =static_cast<unsigned char*>(zmalloc(capacity > LP_HDR_SIZE+1 ? capacity : LP_HDR_SIZE+1));
    if (lp == NULL) return NULL;
    lpSetTotalBytes(lp,LP_HDR_SIZE+1);
    lpSetNumElements(lp,0);
    lp[LP_HDR_SIZE] = LP_EOF;
    return lp;
}

/**
 * 释放链表内存块占用的内存。
 * @param lp 指向链表内存块的指针
 */
void listPackCreate::lpFree(unsigned char *lp)
{
    zfree(lp);
}

/**
 * 收缩链表内存块以匹配其实际使用大小，优化内存占用。
 * @param lp 指向链表内存块的指针
 * @return 指向收缩后内存块的指针，可能与原指针不同
 */
unsigned char* listPackCreate::lpShrinkToFit(unsigned char *lp)
{
    size_t size = lpGetTotalBytes(lp);
    if (size < zmalloc_size(lp)) {
        return static_cast<unsigned char*>(zrealloc(lp, size));
    } else {
        return lp;
    }
}

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
unsigned char *listPackCreate::lpInsert(unsigned char *lp, unsigned char *ele, uint32_t size, unsigned char *p, int where, unsigned char **newp)
{
    unsigned char intenc[LP_MAX_INT_ENCODING_LEN];
    unsigned char backlen[LP_MAX_BACKLEN_SIZE];

    uint64_t enclen; /* The length of the encoded element. */

    /* An element pointer set to NULL means deletion, which is conceptually
     * replacing the element with a zero-length element. So whatever we
     * get passed as 'where', set it to LP_REPLACE. */
    if (ele == NULL) where = LP_REPLACE;

    /* If we need to insert after the current element, we just jump to the
     * next element (that could be the EOF one) and handle the case of
     * inserting before. So the function will actually deal with just two
     * cases: LP_BEFORE and LP_REPLACE. */
    if (where == LP_AFTER) {
        p = lpSkip(p);
        where = LP_BEFORE;
        ASSERT_INTEGRITY(lp, p);
    }

    /* Store the offset of the element 'p', so that we can obtain its
     * address again after a reallocation. */
    unsigned long poff = p-lp;

    /* Calling lpEncodeGetType() results into the encoded version of the
     * element to be stored into 'intenc' in case it is representable as
     * an integer: in that case, the function returns LP_ENCODING_INT.
     * Otherwise if LP_ENCODING_STR is returned, we'll have to call
     * lpEncodeString() to actually write the encoded string on place later.
     *
     * Whatever the returned encoding is, 'enclen' is populated with the
     * length of the encoded element. */
    int enctype;
    if (ele) {
        enctype = lpEncodeGetType(ele,size,intenc,&enclen);
    } else {
        enctype = -1;
        enclen = 0;
    }

    /* We need to also encode the backward-parsable length of the element
     * and append it to the end: this allows to traverse the listpack from
     * the end to the start. */
    unsigned long backlen_size = ele ? lpEncodeBacklen(backlen,enclen) : 0;
    uint64_t old_listpack_bytes = lpGetTotalBytes(lp);
    uint32_t replaced_len  = 0;
    if (where == LP_REPLACE) {
        replaced_len = lpCurrentEncodedSizeUnsafe(p);
        replaced_len += lpEncodeBacklen(NULL,replaced_len);
        ASSERT_INTEGRITY_LEN(lp, p, replaced_len);
    }

    uint64_t new_listpack_bytes = old_listpack_bytes + enclen + backlen_size
                                  - replaced_len;
    if (new_listpack_bytes > UINT32_MAX) return NULL;

    /* We now need to reallocate in order to make space or shrink the
     * allocation (in case 'when' value is LP_REPLACE and the new element is
     * smaller). However we do that before memmoving the memory to
     * make room for the new element if the final allocation will get
     * larger, or we do it after if the final allocation will get smaller. */

    unsigned char *dst = lp + poff; /* May be updated after reallocation. */

    /* Realloc before: we need more room. */
    if (new_listpack_bytes > old_listpack_bytes &&
        new_listpack_bytes > zmalloc_size(lp)) {
        if ((lp =static_cast<unsigned char*>(zrealloc(lp,new_listpack_bytes))) == NULL) return NULL;
        dst = lp + poff;
    }

    /* Setup the listpack relocating the elements to make the exact room
     * we need to store the new one. */
    if (where == LP_BEFORE) {
        memmove(dst+enclen+backlen_size,dst,old_listpack_bytes-poff);
    } else { /* LP_REPLACE. */
        long lendiff = (enclen+backlen_size)-replaced_len;
        memmove(dst+replaced_len+lendiff,
                dst+replaced_len,
                old_listpack_bytes-poff-replaced_len);
    }

    /* Realloc after: we need to free space. */
    if (new_listpack_bytes < old_listpack_bytes) {
        if ((lp = static_cast<unsigned char *>(zrealloc(lp,new_listpack_bytes))) == NULL) return NULL;
        dst = lp + poff;
    }

    /* Store the entry. */
    if (newp) {
        *newp = dst;
        /* In case of deletion, set 'newp' to NULL if the next element is
         * the EOF element. */
        if (!ele && dst[0] == LP_EOF) *newp = NULL;
    }
    if (ele) {
        if (enctype == LP_ENCODING_INT) {
            memcpy(dst,intenc,enclen);
        } else {
            lpEncodeString(dst,ele,size);
        }
        dst += enclen;
        memcpy(dst,backlen,backlen_size);
        dst += backlen_size;
    }

    /* Update header. */
    if (where != LP_REPLACE || ele == NULL) {
        uint32_t num_elements = lpGetNumElements(lp);
        if (num_elements != LP_HDR_NUMELE_UNKNOWN) {
            if (ele)
                lpSetNumElements(lp,num_elements+1);
            else
                lpSetNumElements(lp,num_elements-1);
        }
    }
    lpSetTotalBytes(lp,new_listpack_bytes);

#if 0
    /* This code path is normally disabled: what it does is to force listpack
     * to return *always* a new pointer after performing some modification to
     * the listpack, even if the previous allocation was enough. This is useful
     * in order to spot bugs in code using listpacks: by doing so we can find
     * if the caller forgets to set the new pointer where the listpack reference
     * is stored, after an update. */
    unsigned char *oldlp = lp;
    lp = lp_malloc(new_listpack_bytes);
    memcpy(lp,oldlp,new_listpack_bytes);
    if (newp) {
        unsigned long offset = (*newp)-oldlp;
        *newp = lp + offset;
    }
    /* Make sure the old allocation contains garbage. */
    memset(oldlp,'A',new_listpack_bytes);
    lp_free(oldlp);
#endif

    return lp;
}

/**
 * 在链表末尾追加一个元素。
 * @param lp 指向链表内存块的指针
 * @param ele 要追加的元素数据
 * @param size 元素大小（字节）
 * @return 指向更新后链表的指针
 */
unsigned char *listPackCreate::lpAppend(unsigned char *lp, unsigned char *ele, uint32_t size)
{
    uint64_t listpack_bytes = lpGetTotalBytes(lp);
    unsigned char *eofptr = lp + listpack_bytes - 1;
    return lpInsert(lp,ele,size,eofptr,LP_BEFORE,NULL);
}

/**
 * 从链表中删除指定元素。
 * @param lp 指向链表内存块的指针
 * @param p 指向要删除元素的指针
 * @param newp 用于返回删除元素后的下一个元素指针
 * @return 指向更新后链表的指针
 */
unsigned char *listPackCreate::lpDelete(unsigned char *lp, unsigned char *p, unsigned char **newp)
{
    return lpInsert(lp,NULL,0,p,LP_REPLACE,newp);
}

/**
 * 获取链表中的元素数量。
 * @param lp 指向链表内存块的指针
 * @return 链表中的元素数量
 */
uint32_t listPackCreate::lpLength(unsigned char *lp)
{
    uint32_t numele = lpGetNumElements(lp);
    if (numele != LP_HDR_NUMELE_UNKNOWN) return numele;

    /* Too many elements inside the listpack. We need to scan in order
     * to get the total number. */
    uint32_t count = 0;
    unsigned char *p = lpFirst(lp);
    while(p) {
        count++;
        p = lpNext(lp,p);
    }

    /* If the count is again within range of the header numele field,
     * set it. */
    if (count < LP_HDR_NUMELE_UNKNOWN) lpSetNumElements(lp,count);
    return count;
}

/**
 * 获取当前元素的数据部分，并更新计数器。
 * @param p 指向当前元素的指针
 * @param count 计数器指针，用于跟踪元素位置
 * @param intbuf 用于存储整数值的缓冲区
 * @return 指向下一个元素的指针
 */
unsigned char *listPackCreate::lpGet(unsigned char *p, int64_t *count, unsigned char *intbuf)
{
    int64_t val;
    uint64_t uval, negstart, negmax;

    assert(p); /* assertion for valgrind (avoid NPD) */
    if (LP_ENCODING_IS_7BIT_UINT(p[0])) {
        negstart = UINT64_MAX; /* 7 bit ints are always positive. */
        negmax = 0;
        uval = p[0] & 0x7f;
    } else if (LP_ENCODING_IS_6BIT_STR(p[0])) {
        *count = LP_ENCODING_6BIT_STR_LEN(p);
        return p+1;
    } else if (LP_ENCODING_IS_13BIT_INT(p[0])) {
        uval = ((p[0]&0x1f)<<8) | p[1];
        negstart = (uint64_t)1<<12;
        negmax = 8191;
    } else if (LP_ENCODING_IS_16BIT_INT(p[0])) {
        uval = (uint64_t)p[1] |
               (uint64_t)p[2]<<8;
        negstart = (uint64_t)1<<15;
        negmax = UINT16_MAX;
    } else if (LP_ENCODING_IS_24BIT_INT(p[0])) {
        uval = (uint64_t)p[1] |
               (uint64_t)p[2]<<8 |
               (uint64_t)p[3]<<16;
        negstart = (uint64_t)1<<23;
        negmax = UINT32_MAX>>8;
    } else if (LP_ENCODING_IS_32BIT_INT(p[0])) {
        uval = (uint64_t)p[1] |
               (uint64_t)p[2]<<8 |
               (uint64_t)p[3]<<16 |
               (uint64_t)p[4]<<24;
        negstart = (uint64_t)1<<31;
        negmax = UINT32_MAX;
    } else if (LP_ENCODING_IS_64BIT_INT(p[0])) {
        uval = (uint64_t)p[1] |
               (uint64_t)p[2]<<8 |
               (uint64_t)p[3]<<16 |
               (uint64_t)p[4]<<24 |
               (uint64_t)p[5]<<32 |
               (uint64_t)p[6]<<40 |
               (uint64_t)p[7]<<48 |
               (uint64_t)p[8]<<56;
        negstart = (uint64_t)1<<63;
        negmax = UINT64_MAX;
    } else if (LP_ENCODING_IS_12BIT_STR(p[0])) {
        *count = LP_ENCODING_12BIT_STR_LEN(p);
        return p+2;
    } else if (LP_ENCODING_IS_32BIT_STR(p[0])) {
        *count = LP_ENCODING_32BIT_STR_LEN(p);
        return p+5;
    } else {
        uval = 12345678900000000ULL + p[0];
        negstart = UINT64_MAX;
        negmax = 0;
    }

    /* We reach this code path only for integer encodings.
     * Convert the unsigned value to the signed one using two's complement
     * rule. */
    if (uval >= negstart) {
        /* This three steps conversion should avoid undefined behaviors
         * in the unsigned -> signed conversion. */
        uval = negmax-uval;
        val = uval;
        val = -val-1;
    } else {
        val = uval;
    }

    /* Return the string representation of the integer or the value itself
     * depending on intbuf being NULL or not. */
    if (intbuf) {
        *count = snprintf((char*)intbuf,LP_INTBUF_SIZE,"%lld",(long long)val);
        return intbuf;
    } else {
        *count = val;
        return NULL;
    }
}

/**
 * 获取链表的第一个元素。
 * @param lp 指向链表内存块的指针
 * @return 指向第一个元素的指针
 */
unsigned char *listPackCreate::lpFirst(unsigned char *lp)
{
    unsigned char *p = lp + LP_HDR_SIZE; /* Skip the header. */
    if (p[0] == LP_EOF) return NULL;
    lpAssertValidEntry(lp, lpBytes(lp), p);
    return p;
}

/**
 * 获取链表的最后一个元素。
 * @param lp 指向链表内存块的指针
 * @return 指向最后一个元素的指针
 */
unsigned char *listPackCreate::lpLast(unsigned char *lp)
{
    unsigned char *p = lp+lpGetTotalBytes(lp)-1; /* Seek EOF element. */
    return lpPrev(lp,p); /* Will return NULL if EOF is the only element. */
}

/**
 * 获取指定元素的下一个元素。
 * @param lp 指向链表内存块的指针
 * @param p 指向当前元素的指针
 * @return 指向下一个元素的指针，若当前元素是最后一个则返回NULL
 */
unsigned char *listPackCreate::lpNext(unsigned char *lp, unsigned char *p)
{
    assert(p);
    p = lpSkip(p);
    if (p[0] == LP_EOF) return NULL;
    lpAssertValidEntry(lp, lpBytes(lp), p);
    return p;
}

/**
 * 获取指定元素的前一个元素。
 * @param lp 指向链表内存块的指针
 * @param p 指向当前元素的指针
 * @return 指向前一个元素的指针，若当前元素是第一个则返回NULL
 */
unsigned char *listPackCreate::lpPrev(unsigned char *lp, unsigned char *p)
{
    assert(p);
    if (p-lp == LP_HDR_SIZE) return NULL;
    p--; /* Seek the first backlen byte of the last element. */
    uint64_t prevlen = lpDecodeBacklen(p);
    prevlen += lpEncodeBacklen(NULL,prevlen);
    p -= prevlen-1; /* Seek the first byte of the previous entry. */
    lpAssertValidEntry(lp, lpBytes(lp), p);
    return p;
}

/**
 * 获取链表占用的总字节数。
 * @param lp 指向链表内存块的指针
 * @return 链表占用的总字节数
 */
uint32_t listPackCreate::lpBytes(unsigned char *lp)
{
    return lpGetTotalBytes(lp);
}

/**
 * 定位到链表中指定索引位置的元素。
 * @param lp 指向链表内存块的指针
 * @param index 要定位的元素索引（从0开始）
 * @return 指向指定索引元素的指针，若索引超出范围则返回NULL
 */
unsigned char *listPackCreate::lpSeek(unsigned char *lp, long index)
{
    int forward = 1; /* Seek forward by default. */

    /* We want to seek from left to right or the other way around
     * depending on the listpack length and the element position.
     * However if the listpack length cannot be obtained in constant time,
     * we always seek from left to right. */
    uint32_t numele = lpGetNumElements(lp);
    if (numele != LP_HDR_NUMELE_UNKNOWN) {
        if (index < 0) index = (long)numele+index;
        if (index < 0) return NULL; /* Index still < 0 means out of range. */
        if (index >= (long)numele) return NULL; /* Out of range the other side. */
        /* We want to scan right-to-left if the element we are looking for
         * is past the half of the listpack. */
        if (index > (long)numele/2) {
            forward = 0;
            /* Right to left scanning always expects a negative index. Convert
             * our index to negative form. */
            index -= numele;
        }
    } else {
        /* If the listpack length is unspecified, for negative indexes we
         * want to always scan right-to-left. */
        if (index < 0) forward = 0;
    }

    /* Forward and backward scanning is trivially based on lpNext()/lpPrev(). */
    if (forward) {
        unsigned char *ele = lpFirst(lp);
        while (index > 0 && ele) {
            ele = lpNext(lp,ele);
            index--;
        }
        return ele;
    } else {
        unsigned char *ele = lpLast(lp);
        while (index < -1 && ele) {
            ele = lpPrev(lp,ele);
            index++;
        }
        return ele;
    }
}

/**
 * 验证链表的完整性。
 * @param lp 指向链表内存块的指针
 * @param size 链表预期大小（字节）
 * @param deep 是否进行深度验证（检查每个元素）
 * @return 验证结果：0=完整，非0=不完整
 */
int listPackCreate::lpValidateIntegrity(unsigned char *lp, size_t size, int deep)
{
    /* Check that we can actually read the header. (and EOF) */
    if (size < LP_HDR_SIZE + 1)
        return 0;

    /* Check that the encoded size in the header must match the allocated size. */
    size_t bytes = lpGetTotalBytes(lp);
    if (bytes != size)
        return 0;

    /* The last byte must be the terminator. */
    if (lp[size-1] != LP_EOF)
        return 0;

    if (!deep)
        return 1;

    /* Validate the invividual entries. */
    uint32_t count = 0;
    unsigned char *p = lp + LP_HDR_SIZE;
    while(p && p[0] != LP_EOF) {
        if (!lpValidateNext(lp, &p, bytes))
            return 0;
        count++;
    }

    /* Check that the count in the header is correct */
    uint32_t numele = lpGetNumElements(lp);
    if (numele != LP_HDR_NUMELE_UNKNOWN && numele != count)
        return 0;

    return 1;
}

/**
 * 获取链表中第一个有效元素。
 * @param lp 指向链表内存块的指针
 * @return 指向第一个有效元素的指针，若没有有效元素则返回NULL
 */
unsigned char *listPackCreate::lpValidateFirst(unsigned char *lp)
{
    unsigned char *p = lp + LP_HDR_SIZE; /* Skip the header. */
    if (p[0] == LP_EOF) return NULL;
    return p;
}

/**
 * 获取链表中下一个有效元素，并验证其有效性。
 * @param lp 指向链表内存块的指针
 * @param pp 指向当前元素指针的指针，函数会更新其指向下一个有效元素
 * @param lpbytes 链表总字节数
 * @return 验证结果：0=有效，非0=无效
 */
int listPackCreate::lpValidateNext(unsigned char *lp, unsigned char **pp, size_t lpbytes)
{
#define OUT_OF_RANGE(p) ( \
        (p) < lp + LP_HDR_SIZE || \
        (p) > lp + lpbytes - 1)
    unsigned char *p = *pp;
    if (!p)
        return 0;

    /* Before accessing p, make sure it's valid. */
    if (OUT_OF_RANGE(p))
        return 0;

    if (*p == LP_EOF) {
        *pp = NULL;
        return 1;
    }

    /* check that we can read the encoded size */
    uint32_t lenbytes = lpCurrentEncodedSizeBytes(p);
    if (!lenbytes)
        return 0;

    /* make sure the encoded entry length doesn't rech outside the edge of the listpack */
    if (OUT_OF_RANGE(p + lenbytes))
        return 0;

    /* get the entry length and encoded backlen. */
    unsigned long entrylen = lpCurrentEncodedSizeUnsafe(p);
    unsigned long encodedBacklen = lpEncodeBacklen(NULL,entrylen);
    entrylen += encodedBacklen;

    /* make sure the entry doesn't rech outside the edge of the listpack */
    if (OUT_OF_RANGE(p + entrylen))
        return 0;

    /* move to the next entry */
    p += entrylen;

    /* make sure the encoded length at the end patches the one at the beginning. */
    uint64_t prevlen = lpDecodeBacklen(p-1);
    if (prevlen + encodedBacklen != entrylen)
        return 0;

    *pp = p;
    return 1;
#undef OUT_OF_RANGE
}

/**
 * 跳过当前元素，指向下一个元素的起始位置。
 * @param p 指向当前元素的指针
 * @return 指向下一个元素的指针
 */
unsigned char *listPackCreate::lpSkip(unsigned char *p) 
{
    unsigned long entrylen = lpCurrentEncodedSizeUnsafe(p);
    entrylen += lpEncodeBacklen(NULL,entrylen);
    p += entrylen;
    return p;
}

/* Given an element 'ele' of size 'size', determine if the element can be
 * represented inside the listpack encoded as integer, and returns
 * LP_ENCODING_INT if so. Otherwise returns LP_ENCODING_STR if no integer
 * encoding is possible.
 *
 * If the LP_ENCODING_INT is returned, the function stores the integer encoded
 * representation of the element in the 'intenc' buffer.
 *
 * Regardless of the returned encoding, 'enclen' is populated by reference to
 * the number of bytes that the string or integer encoded element will require
 * in order to be represented. */
/**
 * 确定元素的编码类型并计算编码后的长度。
 * @param ele 要编码的元素数据
 * @param size 元素大小（字节）
 * @param intenc 用于存储整数编码类型的缓冲区
 * @param enclen 用于返回编码后的长度
 * @return 编码类型
 */
int listPackCreate::lpEncodeGetType(unsigned char *ele, uint32_t size, unsigned char *intenc, uint64_t *enclen) 
{
    int64_t v;
    if (lpStringToInt64((const char*)ele, size, &v)) {
        if (v >= 0 && v <= 127) {
            /* Single byte 0-127 integer. */
            intenc[0] = v;
            *enclen = 1;
        } else if (v >= -4096 && v <= 4095) {
            /* 13 bit integer. */
            if (v < 0) v = ((int64_t)1<<13)+v;
            intenc[0] = (v>>8)|LP_ENCODING_13BIT_INT;
            intenc[1] = v&0xff;
            *enclen = 2;
        } else if (v >= -32768 && v <= 32767) {
            /* 16 bit integer. */
            if (v < 0) v = ((int64_t)1<<16)+v;
            intenc[0] = LP_ENCODING_16BIT_INT;
            intenc[1] = v&0xff;
            intenc[2] = v>>8;
            *enclen = 3;
        } else if (v >= -8388608 && v <= 8388607) {
            /* 24 bit integer. */
            if (v < 0) v = ((int64_t)1<<24)+v;
            intenc[0] = LP_ENCODING_24BIT_INT;
            intenc[1] = v&0xff;
            intenc[2] = (v>>8)&0xff;
            intenc[3] = v>>16;
            *enclen = 4;
        } else if (v >= -2147483648 && v <= 2147483647) {
            /* 32 bit integer. */
            if (v < 0) v = ((int64_t)1<<32)+v;
            intenc[0] = LP_ENCODING_32BIT_INT;
            intenc[1] = v&0xff;
            intenc[2] = (v>>8)&0xff;
            intenc[3] = (v>>16)&0xff;
            intenc[4] = v>>24;
            *enclen = 5;
        } else {
            /* 64 bit integer. */
            uint64_t uv = v;
            intenc[0] = LP_ENCODING_64BIT_INT;
            intenc[1] = uv&0xff;
            intenc[2] = (uv>>8)&0xff;
            intenc[3] = (uv>>16)&0xff;
            intenc[4] = (uv>>24)&0xff;
            intenc[5] = (uv>>32)&0xff;
            intenc[6] = (uv>>40)&0xff;
            intenc[7] = (uv>>48)&0xff;
            intenc[8] = uv>>56;
            *enclen = 9;
        }
        return LP_ENCODING_INT;
    } else {
        if (size < 64) *enclen = 1+size;
        else if (size < 4096) *enclen = 2+size;
        else *enclen = 5+(uint64_t)size;
        return LP_ENCODING_STRING;
    }
}

/* Store a reverse-encoded variable length field, representing the length
 * of the previous element of size 'l', in the target buffer 'buf'.
 * The function returns the number of bytes used to encode it, from
 * 1 to 5. If 'buf' is NULL the function just returns the number of bytes
 * needed in order to encode the backlen. */
/**
 * 计算编码数据的反向长度（用于解码）。
 * @param buf 指向编码数据的指针
 * @param l 编码数据的长度
 * @return 反向长度值
 */
unsigned long listPackCreate::lpEncodeBacklen(unsigned char *buf, uint64_t l) 
{
    if (l <= 127) {
        if (buf) buf[0] = l;
        return 1;
    } else if (l < 16383) {
        if (buf) {
            buf[0] = l>>7;
            buf[1] = (l&127)|128;
        }
        return 2;
    } else if (l < 2097151) {
        if (buf) {
            buf[0] = l>>14;
            buf[1] = ((l>>7)&127)|128;
            buf[2] = (l&127)|128;
        }
        return 3;
    } else if (l < 268435455) {
        if (buf) {
            buf[0] = l>>21;
            buf[1] = ((l>>14)&127)|128;
            buf[2] = ((l>>7)&127)|128;
            buf[3] = (l&127)|128;
        }
        return 4;
    } else {
        if (buf) {
            buf[0] = l>>28;
            buf[1] = ((l>>21)&127)|128;
            buf[2] = ((l>>14)&127)|128;
            buf[3] = ((l>>7)&127)|128;
            buf[4] = (l&127)|128;
        }
        return 5;
    }
}


/* Return the encoded length of the listpack element pointed by 'p'.
 * This includes the encoding byte, length bytes, and the element data itself.
 * If the element encoding is wrong then 0 is returned.
 * Note that this method may access additional bytes (in case of 12 and 32 bit
 * str), so should only be called when we know 'p' was already validated by
 * lpCurrentEncodedSizeBytes or ASSERT_INTEGRITY_LEN (possibly since 'p' is
 * a return value of another function that validated its return. */
/**
 * 不安全地获取当前元素编码后的大小（不验证指针有效性）。
 * @param p 指向当前元素的指针
 * @return 编码后的大小（字节）
 */
uint32_t listPackCreate::lpCurrentEncodedSizeUnsafe(unsigned char *p) 
{
    if (LP_ENCODING_IS_7BIT_UINT(p[0])) return 1;
    if (LP_ENCODING_IS_6BIT_STR(p[0])) return 1+LP_ENCODING_6BIT_STR_LEN(p);
    if (LP_ENCODING_IS_13BIT_INT(p[0])) return 2;
    if (LP_ENCODING_IS_16BIT_INT(p[0])) return 3;
    if (LP_ENCODING_IS_24BIT_INT(p[0])) return 4;
    if (LP_ENCODING_IS_32BIT_INT(p[0])) return 5;
    if (LP_ENCODING_IS_64BIT_INT(p[0])) return 9;
    if (LP_ENCODING_IS_12BIT_STR(p[0])) return 2+LP_ENCODING_12BIT_STR_LEN(p);
    if (LP_ENCODING_IS_32BIT_STR(p[0])) return 5+LP_ENCODING_32BIT_STR_LEN(p);
    if (p[0] == LP_EOF) return 1;
    return 0;
}

/* Encode the string element pointed by 's' of size 'len' in the target
 * buffer 's'. The function should be called with 'buf' having always enough
 * space for encoding the string. This is done by calling lpEncodeGetType()
 * before calling this function. */
/**
 * 将字符串编码并存储到缓冲区中。
 * @param buf 目标缓冲区
 * @param s 源字符串
 * @param len 字符串长度
 */
void listPackCreate::lpEncodeString(unsigned char *buf, unsigned char *s, uint32_t len) 
{
    if (len < 64) {
        buf[0] = len | LP_ENCODING_6BIT_STR;
        memcpy(buf+1,s,len);
    } else if (len < 4096) {
        buf[0] = (len >> 8) | LP_ENCODING_12BIT_STR;
        buf[1] = len & 0xff;
        memcpy(buf+2,s,len);
    } else {
        buf[0] = LP_ENCODING_32BIT_STR;
        buf[1] = len & 0xff;
        buf[2] = (len >> 8) & 0xff;
        buf[3] = (len >> 16) & 0xff;
        buf[4] = (len >> 24) & 0xff;
        memcpy(buf+5,s,len);
    }
}

/**
 * 将字符串转换为64位整数值。
 * @param s 源字符串
 * @param slen 字符串长度
 * @param value 用于存储转换结果的指针
 * @return 转换结果：0=成功，非0=失败
 */
int listPackCreate::lpStringToInt64(const char *s, unsigned long slen, int64_t *value) 
{
    const char *p = s;
    unsigned long plen = 0;
    int negative = 0;
    uint64_t v;

    if (plen == slen)
        return 0;

    /* Special case: first and only digit is 0. */
    if (slen == 1 && p[0] == '0') {
        if (value != NULL) *value = 0;
        return 1;
    }

    if (p[0] == '-') {
        negative = 1;
        p++; plen++;

        /* Abort on only a negative sign. */
        if (plen == slen)
            return 0;
    }

    /* First digit should be 1-9, otherwise the string should just be 0. */
    if (p[0] >= '1' && p[0] <= '9') {
        v = p[0]-'0';
        p++; plen++;
    } else if (p[0] == '0' && slen == 1) {
        *value = 0;
        return 1;
    } else {
        return 0;
    }

    while (plen < slen && p[0] >= '0' && p[0] <= '9') {
        if (v > (UINT64_MAX / 10)) /* Overflow. */
            return 0;
        v *= 10;

        if (v > (UINT64_MAX - (p[0]-'0'))) /* Overflow. */
            return 0;
        v += p[0]-'0';

        p++; plen++;
    }

    /* Return if not all bytes were used. */
    if (plen < slen)
        return 0;

    if (negative) {
        if (v > ((uint64_t)(-(INT64_MIN+1))+1)) /* Overflow. */
            return 0;
        if (value != NULL) *value = -v;
    } else {
        if (v > INT64_MAX) /* Overflow. */
            return 0;
        if (value != NULL) *value = v;
    }
    return 1;
}

/* Validate that the entry doesn't reach outside the listpack allocation. */
/**
 * 断言指定位置的元素是有效的链表条目。
 * @param lp 指向链表内存块的指针
 * @param lpbytes 链表总字节数
 * @param p 指向要验证元素的指针
 */
void listPackCreate::lpAssertValidEntry(unsigned char* lp, size_t lpbytes, unsigned char *p) 
{
    assert(lpValidateNext(lp, &p, lpbytes));
}

/**
 * 解码反向长度值。
 * @param p 指向编码数据的指针
 * @return 解码后的反向长度值
 */
uint64_t listPackCreate::lpDecodeBacklen(unsigned char *p) 
{
    uint64_t val = 0;
    uint64_t shift = 0;
    do {
        val |= (uint64_t)(p[0] & 127) << shift;
        if (!(p[0] & 128)) break;
        shift += 7;
        p--;
        if (shift > 28) return UINT64_MAX;
    } while(1);
    return val;
}

/**
 * 获取当前元素编码后的实际字节大小。
 * @param p 指向当前元素的指针
 * @return 编码后的实际字节大小
 */
uint32_t listPackCreate::lpCurrentEncodedSizeBytes(unsigned char *p) 
{
    if (LP_ENCODING_IS_7BIT_UINT(p[0])) return 1;
    if (LP_ENCODING_IS_6BIT_STR(p[0])) return 1;
    if (LP_ENCODING_IS_13BIT_INT(p[0])) return 1;
    if (LP_ENCODING_IS_16BIT_INT(p[0])) return 1;
    if (LP_ENCODING_IS_24BIT_INT(p[0])) return 1;
    if (LP_ENCODING_IS_32BIT_INT(p[0])) return 1;
    if (LP_ENCODING_IS_64BIT_INT(p[0])) return 1;
    if (LP_ENCODING_IS_12BIT_STR(p[0])) return 2;
    if (LP_ENCODING_IS_32BIT_STR(p[0])) return 5;
    if (p[0] == LP_EOF) return 1;
    return 0;
}
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//

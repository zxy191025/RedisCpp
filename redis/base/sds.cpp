/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/12
 * Description: A C dynamic strings library
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include "sds.h"
#include "zmallocDf.h"
const char *SDS_NOINIT = "SDS_NOINIT";
typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
#define SDS_LLSTR_SIZE 21

/**
 * 判断字符是否为十六进制数字
 * @param c 字符
 * @return 是十六进制数字返回1，否则返回0
 */
 /* Helper function for sdssplitargs() that returns non zero if 'c'
 * is a valid hex digit. */
int sdsCreate::is_hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}
/**
 * 将十六进制字符转换为对应的整数值
 * @param c 十六进制字符
 * @return 对应的整数值（0-15），无效字符返回-1
 */
/* Helper function for sdssplitargs() that converts a hex digit into an
 * integer from 0 to 15 */
int sdsCreate::hex_digit_to_int(char c) {
    switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
    }
}
//获取 SDS 字符串的长度，不包括\0结尾符
size_t sdsCreate::sdslen(const char* s) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;//SDS_HDR(8,s)回退到头部地址，再强制转化为 sdshdr8*
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->len;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}
//查询当前 SDS 字符串的空闲空间
//获取空闲空间
//返回当前 SDS 对象中尚未使用的字节数（即预留的可追加空间）。
//例如：若 SDS 总容量为 100 字节，已使用 60 字节，则 sdsavail(s) 返回 40。
size_t sdsCreate::sdsavail(const char* s) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            return 0;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            return sh->alloc - sh->len;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            return sh->alloc - sh->len;
        }
    }
    return 0;
}
/**
 * 设置sds字符串的长度
 * @param s sds字符串
 * @param newlen 新的长度
 */
void sdsCreate::sdssetlen(char* s, size_t newlen) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len = newlen;
            break;
    }
}
/**
 * 增加sds字符串的长度
 * @param s sds字符串
 * @param inc 增加的长度
 */
void sdsCreate::sdsinclen(char* s, size_t inc) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            {
                unsigned char *fp = ((unsigned char*)s)-1;
                unsigned char newlen = SDS_TYPE_5_LEN(flags)+inc;
                *fp = SDS_TYPE_5 | (newlen << SDS_TYPE_BITS);
            }
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->len += inc;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->len += inc;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->len += inc;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->len += inc;
            break;
    }
}
/**
 * 获取sds字符串的分配空间大小
 * @param s sds字符串
 * @return 分配的空间大小
 */
size_t sdsCreate::sdsalloc(const char* s) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return SDS_TYPE_5_LEN(flags);
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->alloc;
        case SDS_TYPE_16:
            return SDS_HDR(16,s)->alloc;
        case SDS_TYPE_32:
            return SDS_HDR(32,s)->alloc;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->alloc;
    }
    return 0;
}
/**
 * 设置sds字符串的分配空间大小
 * @param s sds字符串
 * @param newlen 新的分配空间大小
 */
void sdsCreate::sdssetalloc(char* s, size_t newlen) 
{
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            /* Nothing to do, this type has no total allocation info. */
            break;
        case SDS_TYPE_8:
            SDS_HDR(8,s)->alloc = newlen;
            break;
        case SDS_TYPE_16:
            SDS_HDR(16,s)->alloc = newlen;
            break;
        case SDS_TYPE_32:
            SDS_HDR(32,s)->alloc = newlen;
            break;
        case SDS_TYPE_64:
            SDS_HDR(64,s)->alloc = newlen;
            break;
    }
}
/**
 * 获取指定类型的sds头部大小
 * @param type sds类型
 * @return 头部大小
 */
int sdsCreate::sdsHdrSize(char type) 
{
    switch(type&SDS_TYPE_MASK) {
        case SDS_TYPE_5:
            return sizeof(struct sdshdr5);
        case SDS_TYPE_8:
            return sizeof(struct sdshdr8);
        case SDS_TYPE_16:
            return sizeof(struct sdshdr16);
        case SDS_TYPE_32:
            return sizeof(struct sdshdr32);
        case SDS_TYPE_64:
            return sizeof(struct sdshdr64);
    }
    return 0;
}
/**
 * 根据字符串大小确定所需的sds类型
 * @param string_size 字符串大小
 * @return sds类型
 */
char sdsCreate::sdsReqType(size_t string_size) 
{
    if (string_size < 1<<5)
        return SDS_TYPE_5;
    if (string_size < 1<<8)
        return SDS_TYPE_8;
    if (string_size < 1<<16)
        return SDS_TYPE_16;
#if (LONG_MAX == LLONG_MAX)
    if (string_size < 1ll<<32)
        return SDS_TYPE_32;
    return SDS_TYPE_64;
#else
    return SDS_TYPE_32;
#endif
}

//用于返回不同 SDS 类型所能表示的最大字符串长度。
//SDS 通过多种类型（如 SDS_TYPE_5、SDS_TYPE_8 等）优化内存使用，
//每种类型的最大长度由其存储长度字段的位数决定。
/**
 * 获取指定sds类型允许的最大字符串长度
 * @param type sds类型
 * @return 最大长度
 */
size_t sdsCreate::sdsTypeMaxSize(char type) 
{
    if (type == SDS_TYPE_5)
        return (1<<5) - 1;
    if (type == SDS_TYPE_8)
        return (1<<8) - 1;
    if (type == SDS_TYPE_16)
        return (1<<16) - 1;
#if (LONG_MAX == LLONG_MAX)
    if (type == SDS_TYPE_32)
        return (1ll<<32) - 1;
#endif
    return -1; /* this is equivalent to the max SDS_TYPE_64 or SDS_TYPE_32 */
}

int sdsll2str(char *s, long long value) {
    char *p, aux;
    unsigned long long v;
    size_t l;

    /* Generate the string representation, this method produces
     * a reversed string. */
    v = (value < 0) ? -value : value;
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);
    if (value < 0) *p++ = '-';

    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';

    /* Reverse the string. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

/* Identical sdsll2str(), but for unsigned long long type. */
int sdsull2str(char *s, unsigned long long v) {
    char *p, aux;
    size_t l;

    /* Generate the string representation, this method produces
     * a reversed string. */
    p = s;
    do {
        *p++ = '0'+(v%10);
        v /= 10;
    } while(v);

    /* Compute length and add null term. */
    l = p-s;
    *p = '\0';

    /* Reverse the string. */
    p--;
    while(s < p) {
        aux = *s;
        *s = *p;
        *p = aux;
        s++;
        p--;
    }
    return l;
}

/* Create a new sds string with the content specified by the 'init' pointer
 * and 'initlen'.
 * If NULL is used for 'init' the string is initialized with zero bytes.
 * If SDS_NOINIT is used, the buffer is left uninitialized;
 *
 * The string is always null-termined (all the sds strings are, always) so
 * even if you create an sds string with:
 *
 * mystring = sdsnewlen("abc",3);
 *
 * You can print the string with printf() as there is an implicit \0 at the
 * end of the string. However the string is binary safe and can contain
 * \0 characters in the middle, as the length is stored in the sds header. */

// init：初始数据指针（可为 NULL 或 SDS_NOINIT）。
// initlen：字符串初始长度。
// trymalloc：是否使用尝试分配（失败返回 NULL 而非终止）。
/**
 * 创建一个新的sds字符串
 * @param init 初始数据，如果为NULL则初始化为空
 * @param initlen 初始数据长度
 * @param trymalloc 如果为1则使用try-malloc变体
 * @return 新创建的sds字符串
 */
sds sdsCreate::_sdsnewlen(const void *init, size_t initlen, int trymalloc) {
    void *sh;
    sds s;
    char type = sdsReqType(initlen);
    /* Empty strings are usually created in order to append. Use type 8
     * since type 5 is not good at this. */
    if (type == SDS_TYPE_5 && initlen == 0) type = SDS_TYPE_8;
    int hdrlen = sdsHdrSize(type);
    unsigned char *fp; /* flags pointer. */
    size_t usable;

    assert(initlen + hdrlen + 1 > initlen); /* Catch size_t overflow */
    sh = trymalloc?
        ztrymalloc_usable(hdrlen+initlen+1, &usable) :
        zmalloc_usable(hdrlen+initlen+1, &usable);//分配内存 = 头部大小 + 数据长度 + 1 字节终止符。
    if (sh == NULL) return NULL;
    if (init==SDS_NOINIT)
        init = NULL;
    else if (!init)
        memset(sh, 0, hdrlen+initlen+1);
    s = (char*)sh+hdrlen;
    fp = ((unsigned char*)s)-1;
    usable = usable-hdrlen-1;
    if (usable > sdsTypeMaxSize(type))
        usable = sdsTypeMaxSize(type);
    switch(type) {
        case SDS_TYPE_5: {
            *fp = type | (initlen << SDS_TYPE_BITS);
            break;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            sh->len = initlen;
            sh->alloc = usable;
            *fp = type;
            break;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            sh->len = initlen;
            sh->alloc = usable;
            *fp = type;
            break;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            sh->len = initlen;
            sh->alloc = usable;
            *fp = type;
            break;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            sh->len = initlen;
            sh->alloc = usable;
            *fp = type;
            break;
        }
    }
    if (initlen && init)
        memcpy(s, init, initlen);
    s[initlen] = '\0';
    return s;
}
// 创建指定长度的 SDS 字符串（标准版本）
// - init: 初始数据指针（可为 NULL，表示初始化为空）
// - initlen: 初始数据长度
// - 内存分配失败时触发 Redis 错误处理（通常终止程序）
sds sdsCreate::sdsnewlen(const void *init, size_t initlen)
{
     return _sdsnewlen(init, initlen, 0);
}
// 创建指定长度的 SDS 字符串（尝试版本）
// - 参数与 sdsnewlen 相同
// - 内存分配失败时返回 NULL，不触发错误处理
sds sdsCreate::sdstrynewlen(const void *init, size_t initlen)
{
    return _sdsnewlen(init, initlen, 1);
}
// - init: C 风格字符串指针（以 '\0' 结尾）
// - 自动计算字符串长度（使用 strlen）
// - 若 init 为 NULL，等价于创建空字符串
sds sdsCreate::sdsnew(const char *init)
{
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}
//创建一个空的 SDS 字符串（长度为 0）
sds sdsCreate::sdsempty(void)
{
    return sdsnewlen("",0);
}
//复制一个 SDS 字符串
sds sdsCreate::sdsdup(const sds s)
{
    return sdsnewlen(s, sdslen(s));
}
/**
 * 释放 SDS 对象占用的内存
 * 
 * @param s 需要释放的 SDS 对象指针。若为 NULL 则直接返回
 * 
 * @details
 * 1. 功能：
 *    - 释放 SDS 对象的内存空间
 *    - 同时释放 SDS 头部和字符串数据部分
 * 
 */
void sdsCreate::sdsfree(sds s)
{
    if (s == NULL) return;
    void* ptr = (char*)s-sdsHdrSize(s[-1]);//在 C 语言中，s[-1] 等价于 *(s - 1)
    zfree(ptr);  // 安全，ptr是可修改的左值
}
//将 SDS 字符串扩展到指定长度，并用 '\0' 填充新增区域
sds sdsCreate::sdsgrowzero(sds s, size_t len)
{
    size_t curlen = sdslen(s);

    if (len <= curlen) return s;
    s = sdsMakeRoomFor(s,len-curlen);
    if (s == NULL) return NULL;

    /* Make sure added region doesn't contain garbage */
    memset(s+curlen,0,(len-curlen+1)); /* also set trailing \0 byte */
    sdssetlen(s, len);
    return s;
}
//将任意二进制数据追加到 SDS 字符串末尾
/* s: 目标 SDS 字符串（会被修改）
*  t: 源数据指针（可以是字符串、二进制数据等）
*  len: 要追加的数据长度（字节）*/
sds sdsCreate::sdscatlen(sds s, const void *t, size_t len)
{
    size_t curlen = sdslen(s);

    s = sdsMakeRoomFor(s,len);
    if (s == NULL) return NULL;
    memcpy(s+curlen, t, len);
    sdssetlen(s, curlen+len);
    s[curlen+len] = '\0';
    return s;
}
/*
* 将C风格字符串t追加到SDS字符串s的末尾
* 
* 参数:
*   s: 目标SDS字符串，函数会将t追加到这个字符串后
*   t: 要追加的C风格字符串(以'\0'结尾)
* 
* 返回值:
*   返回追加后的SDS字符串指针
*   注意：返回的指针可能与输入的s相同，也可能不同
*         如果发生了内存重新分配，返回的会是新指针
*         因此调用者应始终使用返回值更新SDS引用
*/
sds sdsCreate::sdscat(sds s, const char *t)
{
    return sdscatlen(s, t, strlen(t));
}
/**
 * 将一个sds字符串追加到另一个sds字符串末尾
 * @param s 目标sds字符串
 * @param t 源sds字符串
 * @return 追加后的目标sds字符串
 */
sds sdsCreate::sdscatsds(sds s, const sds t)
{
    return sdscatlen(s, t, sdslen(t));
}
/**
 * 将指定长度的字符串复制到sds中，覆盖原内容
 * @param s 目标sds字符串
 * @param t 源字符串
 * @param len 要复制的长度
 * @return 目标sds字符串
 */
sds sdsCreate::sdscpylen(sds s, const char *t, size_t len)
{
    if (sdsalloc(s) < len) 
    {
        s = sdsMakeRoomFor(s,len-sdslen(s));
        if (s == NULL) return NULL;
    }
    memcpy(s, t, len);
    s[len] = '\0';
    sdssetlen(s, len);
    return s;
}
/*
    * 将 C 风格字符串复制到 SDS 对象中，覆盖原有内容
    * 
    * @param s 目标 SDS 对象，将被新内容覆盖
    * @param t 源 C 风格字符串（以 '\0' 结尾）
    * 
    * @return 复制后的 SDS 对象指针
    *         - 若内存分配失败，返回 NULL
    *         - 若成功，返回更新后的 SDS 对象（可能与输入指针相同或不同）
    * 
    */
sds sdsCreate::sdscpy(sds s, const char *t)
{
    return sdscpylen(s, t, strlen(t));
}

sds sdsCreate::sdscatvprintf(sds s, const char *fmt, va_list ap)
{
    va_list cpy;
    char staticbuf[1024], *buf = staticbuf, *t;
    size_t buflen = strlen(fmt)*2;
    int bufstrlen;

    /* We try to start using a static buffer for speed.
     * If not possible we revert to heap allocation. */
    if (buflen > sizeof(staticbuf)) {
        buf = static_cast<char*>(zmalloc(buflen));
        if (buf == NULL) return NULL;
    } else {
        buflen = sizeof(staticbuf);
    }

    /* Alloc enough space for buffer and \0 after failing to
     * fit the string in the current buffer size. */
    while(1) {
        va_copy(cpy,ap);
        bufstrlen = vsnprintf(buf, buflen, fmt, cpy);
        va_end(cpy);
        if (bufstrlen < 0) {
            if (buf != staticbuf) zfree(buf);
            return NULL;
        }
        if (((size_t)bufstrlen) >= buflen) {
            if (buf != staticbuf) zfree(buf);
            buflen = ((size_t)bufstrlen) + 1;
            buf = static_cast<char*>(zmalloc(buflen));
            if (buf == NULL) return NULL;
            continue;
        }
        break;
    }

    /* Finally concat the obtained string to the SDS string and return it. */
    t = sdscatlen(s, buf, bufstrlen);
    if (buf != staticbuf) zfree(buf);
    return t;
}
/**
 * 按格式化字符串将数据追加到 SDS 对象末尾
 * 
 * @param s 目标 SDS 对象，追加操作将在此对象上进行
 * @param fmt 格式化字符串，同标准库的 printf 格式
 * @param ... 可变参数列表，根据 fmt 指定的格式提供数据
 * 
 * @return 追加后的 SDS 对象指针
 *         - 若内存分配失败，返回 NULL
 *         - 若成功，返回更新后的 SDS 对象（可能与输入指针相同或不同）
 * 
 */
sds sdsCreate::sdscatprintf(sds s, const char *fmt, ...)
{
    va_list ap;
    char *t;
    va_start(ap, fmt);
    t = sdscatvprintf(s,fmt,ap);
    va_end(ap);
    return t;
}
/*
* 按格式化字符串将数据追加到 SDS 对象末尾（优化版）
* 
* @param s 目标 SDS 对象，追加操作将在此对象上进行
* @param fmt 格式化字符串，支持简化的格式说明符
* @param ... 可变参数列表，根据 fmt 指定的格式提供数据
* */
sds sdsCreate::sdscatfmt(sds s, char const *fmt, ...)
{
    size_t initlen = sdslen(s);
    const char *f = fmt;
    long i;
    va_list ap;

    /* To avoid continuous reallocations, let's start with a buffer that
     * can hold at least two times the format string itself. It's not the
     * best heuristic but seems to work in practice. */
    s = sdsMakeRoomFor(s, strlen(fmt)*2);
    va_start(ap,fmt);
    f = fmt;    /* Next format specifier byte to process. */
    i = initlen; /* Position of the next byte to write to dest str. */
    while(*f) {
        char next, *str;
        size_t l;
        long long num;
        unsigned long long unum;

        /* Make sure there is always space for at least 1 char. */
        if (sdsavail(s)==0) {
            s = sdsMakeRoomFor(s,1);
        }

        switch(*f) {
        case '%':
            next = *(f+1);
            f++;
            switch(next) {
            case 's':
            case 'S':
                str = va_arg(ap,char*);
                l = (next == 's') ? strlen(str) : sdslen(str);
                if (sdsavail(s) < l) {
                    s = sdsMakeRoomFor(s,l);
                }
                memcpy(s+i,str,l);
                sdsinclen(s,l);
                i += l;
                break;
            case 'i':
            case 'I':
                if (next == 'i')
                    num = va_arg(ap,int);
                else
                    num = va_arg(ap,long long);
                {
                    char buf[SDS_LLSTR_SIZE];
                    l = sdsll2str(buf,num);
                    if (sdsavail(s) < l) {
                        s = sdsMakeRoomFor(s,l);
                    }
                    memcpy(s+i,buf,l);
                    sdsinclen(s,l);
                    i += l;
                }
                break;
            case 'u':
            case 'U':
                if (next == 'u')
                    unum = va_arg(ap,unsigned int);
                else
                    unum = va_arg(ap,unsigned long long);
                {
                    char buf[SDS_LLSTR_SIZE];
                    l = sdsull2str(buf,unum);
                    if (sdsavail(s) < l) {
                        s = sdsMakeRoomFor(s,l);
                    }
                    memcpy(s+i,buf,l);
                    sdsinclen(s,l);
                    i += l;
                }
                break;
            default: /* Handle %% and generally %<unknown>. */
                s[i++] = next;
                sdsinclen(s,1);
                break;
            }
            break;
        default:
            s[i++] = *f;
            sdsinclen(s,1);
            break;
        }
        f++;
    }
    va_end(ap);

    /* Add null-term */
    s[i] = '\0';
    return s;
}
/**
 * 移除 SDS 字符串首尾的指定字符集合
 * 
 * @param s 目标 SDS 对象，将被修改
 * @param cset 需要移除的字符集合（以 '\0' 结尾的字符串）
 * 
 * @return 修改后的 SDS 对象指针
 *         - 若 s 为 NULL，直接返回 NULL
 *         - 返回值与输入 s 相同（不重新分配内存，仅调整内容）
 * 
 */
sds sdsCreate::sdstrim(sds s, const char *cset)
{
    char *start, *end, *sp, *ep;
    size_t len;

    sp = start = s;
    ep = end = s+sdslen(s)-1;
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > sp && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1);
    if (s != sp) memmove(s, sp, len);
    s[len] = '\0';
    sdssetlen(s,len);
    return s;
}
/**
 * 截取sds字符串的子串，修改原sds，截取从start开始长度为len的内容
 * @param s 源sds字符串
 * @param start 起始位置
 * @param len 子串长度
 */
void sdsCreate::sdssubstr(sds s, size_t start, size_t len)
{
    /* Clamp out of range input */
    size_t oldlen = sdslen(s);
    if (start >= oldlen) start = len = 0;
    if (len > oldlen-start) len = oldlen-start;

    /* Move the data */
    if (len) memmove(s, s+start, len);
    s[len] = 0;
    sdssetlen(s,len);
}
//截取并保留 SDS 字符串的指定区间，将原字符串修改为指定范围内的子串
//end = -1代表最后一个字符 -2 表示倒数第二个字符，依此类推。
void sdsCreate::sdsrange(sds s, ssize_t start, ssize_t end)
{
    size_t newlen, len = sdslen(s);
    if (len == 0) return;
    if (start < 0)
        start = len + start;
    if (end < 0)
        end = len + end;
    newlen = (start > end) ? 0 : (end-start)+1;
    sdssubstr(s, start, newlen);
}
/* Set the sds string length to the length as obtained with strlen(), so
 * considering as content only up to the first null term character.
 *
 * This function is useful when the sds string is hacked manually in some
 * way, like in the following example:
 *
 * s = sdsnew("foobar");
 * s[2] = '\0';
 * sdsupdatelen(s);
 * printf("%d\n", sdslen(s));
 *
 * The output will be "2", but if we comment out the call to sdsupdatelen()
 * the output will be "6" as the string was modified but the logical length
 * remains 6 bytes. */
/**
 * 更新sds字符串的长度信息（当字符串内容被直接修改时使用）
 * @param s sds字符串
 */
void sdsCreate::sdsupdatelen(sds s)
{
    size_t reallen = strlen(s);
    sdssetlen(s, reallen);
}
/* Modify an sds string in-place to make it empty (zero length).
 * However all the existing buffer is not discarded but set as free space
 * so that next append operations will not require allocations up to the
 * number of bytes previously available. */
/**
 * 清空sds字符串内容（将长度设为0，保留分配空间）
 * @param s sds字符串
 */
void sdsCreate::sdsclear(sds s)
{
    sdssetlen(s, 0);
    s[0] = '\0';
}
//sdscmp 函数用于比较两个 SDS 字符串的内容
// 返回值规则：
// 负数：s1 小于 s2（如 s1="abc"，s2="abd"）。
// 零：s1 等于 s2。
// 正数：s1 大于 s2（如 s1="abd"，s2="abc"）。
/* Compare two sds strings s1 and s2 with memcmp().
 *
 * Return value:
 *
 *     positive if s1 > s2.
 *     negative if s1 < s2.
 *     0 if s1 and s2 are exactly the same binary string.
 *
 * If two strings share exactly the same prefix, but one of the two has
 * additional characters, the longer string is considered to be greater than
 * the smaller one. */
int sdsCreate::sdscmp(const sds s1, const sds s2)
{
    size_t l1, l2, minlen;
    int cmp;

    l1 = sdslen(s1);
    l2 = sdslen(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1,s2,minlen);
    if (cmp == 0) return l1>l2? 1: (l1<l2? -1: 0);
    return cmp;
}
/* Split 's' with separator in 'sep'. An array
 * of sds strings is returned. *count will be set
 * by reference to the number of tokens returned.
 *
 * On out of memory, zero length string, zero length
 * separator, NULL is returned.
 *
 * Note that 'sep' is able to split a string using
 * a multi-character separator. For example
 * sdssplit("foo_-_bar","_-_"); will return two
 * elements "foo" and "bar".
 *
 * This version of the function is binary-safe but
 * requires length arguments. sdssplit() is just the
 * same function but for zero-terminated strings.
 */
/**
 * 将字符串按指定分隔符分割成多个sds字符串
 * @param s 源字符串
 * @param len 源字符串长度
 * @param sep 分隔符
 * @param seplen 分隔符长度
 * @param count 输出参数，返回分割后的字符串数量
 * @return 返回sds字符串数组，使用完需调用sdsfreesplitres释放内存
 */
sds *sdsCreate::sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count)
{
    int elements = 0, slots = 5;
    long start = 0, j;
    sds *tokens;

    if (seplen < 1 || len < 0) return NULL;

    tokens = static_cast<sds*>(zmalloc(sizeof(sds)*slots));
    if (tokens == NULL) return NULL;

    if (len == 0) {
        *count = 0;
        return tokens;
    }
    for (j = 0; j < (len-(seplen-1)); j++) {
        /* make sure there is room for the next element and the final one */
        if (slots < elements+2) {
            sds *newtokens;

            slots *= 2;
            newtokens = static_cast<sds *>(zrealloc(tokens,sizeof(sds)*slots));
            if (newtokens == NULL) goto cleanup;
            tokens = newtokens;
        }
        /* search the separator */
        if ((seplen == 1 && *(s+j) == sep[0]) || (memcmp(s+j,sep,seplen) == 0)) {
            tokens[elements] = sdsnewlen(s+start,j-start);
            if (tokens[elements] == NULL) goto cleanup;
            elements++;
            start = j+seplen;
            j = j+seplen-1; /* skip the separator */
        }
    }
    /* Add the final element. We are sure there is room in the tokens array. */
    tokens[elements] = sdsnewlen(s+start,len-start);
    if (tokens[elements] == NULL) goto cleanup;
    elements++;
    *count = elements;
    return tokens;

cleanup:
    {
        int i;
        for (i = 0; i < elements; i++) sdsfree(tokens[i]);
        zfree(tokens);
        *count = 0;
        return NULL;
    }
}
/**
 * 释放sdssplitlen分配的内存
 * @param tokens sdssplitlen返回的字符串数组
 * @param count 字符串数量
 */
void sdsCreate::sdsfreesplitres(sds *tokens, int count)
{
    if (!tokens) return;
    while(count--)
        sdsfree(tokens[count]);
    zfree(tokens);
}
/**
 * 将sds字符串转换为小写
 * @param s 源sds字符串
 */
/* Apply tolower() to every character of the sds string 's'. */
void sdsCreate::sdstolower(sds s)
{
    size_t len = sdslen(s), j;
    for (j = 0; j < len; j++) s[j] = tolower(s[j]);
}
/* Apply toupper() to every character of the sds string 's'. */
/**
 * 将sds字符串转换为大写
 * @param s 源sds字符串
 */
void sdsCreate::sdstoupper(sds s)
{
    size_t len = sdslen(s), j;

    for (j = 0; j < len; j++) s[j] = toupper(s[j]);
}
/* Create an sds string from a long long value. It is much faster than:
 *
 * sdscatprintf(sdsempty(),"%lld\n", value);
 */
/**
 * 将long long类型整数转换为sds字符串
 * @param value 整数值
 * @return 转换后的sds字符串
 */
sds sdsCreate::sdsfromlonglong(long long value)
{
    char buf[SDS_LLSTR_SIZE];
    int len = sdsll2str(buf,value);

    return sdsnewlen(buf,len);
}
// 该函数将原始字符串转换为 C 语言可识别的转义格式，
// 例如将二进制数据中的\0转为\\0，确保字符串在日志或序列化场景中能正确展示。转义后的字符串始终以双引号包裹，
// 符合 C 语言字符串字面量的格式。

// 应用场景
// 日志记录：将二进制数据或特殊字符转义后存入日志，避免乱码或解析错误。
// 调试输出：在调试时展示字符串的原始内容（包括转义字符）。
// 协议序列化：生成符合特定格式要求的转义字符串（如 JSON、CSV 中的字符串字段）。
/* Append to the sds string "s" an escaped string representation where
 * all the non-printable characters (tested with isprint()) are turned into
 * escapes in the form "\n\r\a...." or "\x<hex-number>".
 *
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
sds sdsCreate::sdscatrepr(sds s, const char *p, size_t len)
{
    s = sdscatlen(s,"\"",1);
    while(len--) {
        switch(*p) {
        case '\\':
        case '"':
            s = sdscatprintf(s,"\\%c",*p);
            break;
        case '\n': s = sdscatlen(s,"\\n",2); break;
        case '\r': s = sdscatlen(s,"\\r",2); break;
        case '\t': s = sdscatlen(s,"\\t",2); break;
        case '\a': s = sdscatlen(s,"\\a",2); break;
        case '\b': s = sdscatlen(s,"\\b",2); break;
        default:
            if (isprint(*p))
                s = sdscatprintf(s,"%c",*p);
            else
                s = sdscatprintf(s,"\\x%02x",(unsigned char)*p);
            break;
        }
        p++;
    }
    return sdscatlen(s,"\"",1);
}
/* Split a line into arguments, where every argument can be in the
 * following programming-language REPL-alike form:
 *
 * foo bar "newline are supported\n" and "\xff\x00otherstuff"
 *
 * The number of arguments is stored into *argc, and an array
 * of sds is returned.
 *
 * The caller should free the resulting array of sds strings with
 * sdsfreesplitres().
 *
 * Note that sdscatrepr() is able to convert back a string into
 * a quoted string in the same format sdssplitargs() is able to parse.
 *
 * The function returns the allocated tokens on success, even when the
 * input string is empty, or NULL if the input contains unbalanced
 * quotes or closed quotes followed by non space characters
 * as in: "foo"bar or "foo'
 */
/**
 * 将命令行参数解析为sds字符串数组
 * @param line 命令行字符串
 * @param argc 输出参数，返回解析后的参数数量
 * @return sds字符串数组，使用完需调用sdsfreesplitres释放
 */
sds *sdsCreate::sdssplitargs(const char *line, int *argc)
{
    const char *p = line;
    char *current = NULL;
    char **vector = NULL;

    *argc = 0;
    while(1) {
        /* skip blanks */
        while(*p && isspace(*p)) p++;
        if (*p) {
            /* get a token */
            int inq=0;  /* set to 1 if we are in "quotes" */
            int insq=0; /* set to 1 if we are in 'single quotes' */
            int done=0;

            if (current == NULL) current = sdsempty();
            while(!done) {
                if (inq) {
                    if (*p == '\\' && *(p+1) == 'x' &&
                                             is_hex_digit(*(p+2)) &&
                                             is_hex_digit(*(p+3)))
                    {
                        unsigned char byte;

                        byte = (hex_digit_to_int(*(p+2))*16)+
                                hex_digit_to_int(*(p+3));
                        current = sdscatlen(current,(char*)&byte,1);
                        p += 3;
                    } else if (*p == '\\' && *(p+1)) {
                        char c;

                        p++;
                        switch(*p) {
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'b': c = '\b'; break;
                        case 'a': c = '\a'; break;
                        default: c = *p; break;
                        }
                        current = sdscatlen(current,&c,1);
                    } else if (*p == '"') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto err;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto err;
                    } else {
                        current = sdscatlen(current,p,1);
                    }
                } else if (insq) {
                    if (*p == '\\' && *(p+1) == '\'') {
                        p++;
                        current = sdscatlen(current,"'",1);
                    } else if (*p == '\'') {
                        /* closing quote must be followed by a space or
                         * nothing at all. */
                        if (*(p+1) && !isspace(*(p+1))) goto err;
                        done=1;
                    } else if (!*p) {
                        /* unterminated quotes */
                        goto err;
                    } else {
                        current = sdscatlen(current,p,1);
                    }
                } else {
                    switch(*p) {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\0':
                        done=1;
                        break;
                    case '"':
                        inq=1;
                        break;
                    case '\'':
                        insq=1;
                        break;
                    default:
                        current = sdscatlen(current,p,1);
                        break;
                    }
                }
                if (*p) p++;
            }
            /* add the token to the vector */
            vector = static_cast<char**>(zrealloc(vector,((*argc)+1)*sizeof(char*)));
            vector[*argc] = current;
            (*argc)++;
            current = NULL;
        } else {
            /* Even on empty input string return something not NULL. */
            if (vector == NULL) vector =  static_cast<char**>(zmalloc(sizeof(void*)));
            return vector;
        }
    }

err:
    while((*argc)--)
        sdsfree(vector[*argc]);
    zfree(vector);
    if (current) sdsfree(current);
    *argc = 0;
    return NULL;
}
/**
 * 替换sds字符串中的字符
 * @param s 源sds字符串
 * @param from 要替换的字符集
 * @param to 替换后的字符集
 * @param setlen 字符集长度
 * @return 替换后的sds字符串
 */
/* Modify the string substituting all the occurrences of the set of
 * characters specified in the 'from' string to the corresponding character
 * in the 'to' array.
 *
 * For instance: sdsmapchars(mystring, "ho", "01", 2)
 * will have the effect of turning the string "hello" into "0ell1".
 *
 * The function returns the sds string pointer, that is always the same
 * as the input pointer since no resize is needed. */
sds sdsCreate::sdsmapchars(sds s, const char *from, const char *to, size_t setlen)
{
    size_t j, i, l = sdslen(s);

    for (j = 0; j < l; j++) {
        for (i = 0; i < setlen; i++) {
            if (s[j] == from[i]) {
                s[j] = to[i];
                break;
            }
        }
    }
    return s;
}
/**
 * 使用分隔符连接字符串数组为一个sds字符串
 * @param argv 字符串数组
 * @param argc 字符串数量
 * @param sep 分隔符
 * @return 连接后的sds字符串
 */
/* Join an array of C strings using the specified separator (also a C string).
 * Returns the result as an sds string. */
sds sdsCreate::sdsjoin(char **argv, int argc, char *sep)
{
    sds join = sdsempty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sdscat(join, argv[j]);
        if (j != argc-1) join = sdscat(join,sep);
    }
    return join;
}
/**
 * 使用分隔符连接sds字符串数组为一个sds字符串
 * @param argv sds字符串数组
 * @param argc 字符串数量
 * @param sep 分隔符
 * @param seplen 分隔符长度
 * @return 连接后的sds字符串
 */
/* Like sdsjoin, but joins an array of SDS strings. */
sds sdsCreate::sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen)
{
    sds join = sdsempty();
    int j;

    for (j = 0; j < argc; j++) {
        join = sdscatsds(join, argv[j]);
        if (j != argc-1) join = sdscatlen(join,sep,seplen);
    }
    return join;
}
// /*
//  * 根据模板字符串生成动态内容的 SDS 字符串
//  * 
//  * @param temp 模板字符串，包含格式占位符（如 {{name}}）
//  * @param cb_func 回调函数指针，用于处理占位符替换逻辑
//  * @param cb_arg 传递给回调函数的用户参数（可自定义上下文）
//  * /     
/* Perform expansion of a template string and return the result as a newly
 * allocated sds.
 *
 * Template variables are specified using curly brackets, e.g. {variable}.
 * An opening bracket can be quoted by repeating it twice.
 */
sds sdsCreate::sdstemplate(const char *temp, sdstemplate_callback_t cb_func, void *cb_arg)
{
    sds res = sdsempty();
    const char *p = temp;

    while (*p) {
        /* Find next variable, copy everything until there */
        const char *sv = strchr(p, '{');
        if (!sv) {
            /* Not found: copy till rest of template and stop */
            res = sdscat(res, p);
            break;
        } else if (sv > p) {
            /* Found: copy anything up to the begining of the variable */
            res = sdscatlen(res, p, sv - p);
        }

        /* Skip into variable name, handle premature end or quoting */
        sv++;
        if (!*sv) goto error;       /* Premature end of template */
        if (*sv == '{') {
            /* Quoted '{' */
            p = sv + 1;
            res = sdscat(res, "{");
            continue;
        }

        /* Find end of variable name, handle premature end of template */
        const char *ev = strchr(sv, '}');
        if (!ev) goto error;

        /* Pass variable name to callback and obtain value. If callback failed,
         * abort. */
        sds varname = sdsnewlen(sv, ev - sv);
        sds value = cb_func(varname, cb_arg);
        sdsfree(varname);
        if (!value) goto error;

        /* Append value to result and continue */
        res = sdscat(res, value);
        sdsfree(value);
        p = ev + 1;
    }

    return res;

error:
    sdsfree(res);
    return NULL;
}
/* Enlarge the free space at the end of the sds string so that the caller
 * is sure that after calling this function can overwrite up to addlen
 * bytes after the end of the string, plus one more byte for nul term.
 *
 * Note: this does not change the *length* of the sds string as returned
 * by sdslen(), but only the free buffer space we have. */
/*
 * 扩展 SDS 的内存空间，确保有足够空间容纳额外的 addlen 字节数据
 * 
 * 参数:
 *   s: 待扩展的 SDS 对象
 *   addlen: 需要额外分配的字节数
 * 
 * 返回值:
 *   返回扩展后的 SDS 对象指针
 *   如果分配失败返回 NULL
 * 
 * 内存策略:
 *   - 如果需要的空间小于 1MB，则额外预分配相同大小的空间
 *   - 如果需要的空间大于 1MB，则额外预分配 1MB 空间
 *   - 这种策略使 SDS 具有惰性释放特性，减少频繁内存分配
 */
sds sdsCreate::sdsMakeRoomFor(sds s, size_t addlen)
{
    void *sh, *newsh;
    size_t avail = sdsavail(s);
    size_t len, newlen, reqlen;
    char type, oldtype = s[-1] & SDS_TYPE_MASK;
    int hdrlen;
    size_t usable;

    /* Return ASAP if there is enough space left. */
    if (avail >= addlen) return s;

    len = sdslen(s);
    sh = (char*)s-sdsHdrSize(oldtype);
    reqlen = newlen = (len+addlen);
    assert(newlen > len);   /* Catch size_t overflow */
    if (newlen < SDS_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += SDS_MAX_PREALLOC;

    type = sdsReqType(newlen);

    /* Don't use type 5: the user is appending to the string and type 5 is
     * not able to remember empty space, so sdsMakeRoomFor() must be called
     * at every appending operation. */
    if (type == SDS_TYPE_5) type = SDS_TYPE_8;

    hdrlen = sdsHdrSize(type);
    assert(hdrlen + newlen + 1 > reqlen);  /* Catch size_t overflow */
    if (oldtype==type) {
        newsh = zrealloc_usable(sh, hdrlen+newlen+1, &usable);
        if (newsh == NULL) return NULL;
        s = (char*)newsh+hdrlen;
    } else {
        /* Since the header size changes, need to move the string forward,
         * and can't use realloc */
        newsh = zmalloc_usable(hdrlen+newlen+1, &usable);
        if (newsh == NULL) return NULL;
        memcpy((char*)newsh+hdrlen, s, len+1);
        zfree(sh);
        s = (char*)newsh+hdrlen;
        s[-1] = type;
        sdssetlen(s, len);
    }
    usable = usable-hdrlen-1;
    if (usable > sdsTypeMaxSize(type))
        usable = sdsTypeMaxSize(type);
    sdssetalloc(s, usable);
    return s;
}
/* Increment the sds length and decrements the left free space at the
 * end of the string according to 'incr'. Also set the null term
 * in the new end of the string.
 *
 * This function is used in order to fix the string length after the
 * user calls sdsMakeRoomFor(), writes something after the end of
 * the current string, and finally needs to set the new length.
 *
 * Note: it is possible to use a negative increment in order to
 * right-trim the string.
 *
 * Usage example:
 *
 * Using sdsIncrLen() and sdsMakeRoomFor() it is possible to mount the
 * following schema, to cat bytes coming from the kernel to the end of an
 * sds string without copying into an intermediate buffer:
 *
 * oldlen = sdslen(s);
 * s = sdsMakeRoomFor(s, BUFFER_SIZE);
 * nread = read(fd, s+oldlen, BUFFER_SIZE);
 * ... check for nread <= 0 and handle it ...
 * sdsIncrLen(s, nread);
 */
/**
 * 直接调整 SDS 字符串的长度（高级底层操作）
 * 
 * @param s 目标 SDS 对象，必须为有效指针
 * @param incr 长度增量（可正可负）
 */
void sdsCreate::sdsIncrLen(sds s, ssize_t incr)
{
    unsigned char flags = s[-1];
    size_t len;
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_5: {
            unsigned char *fp = ((unsigned char*)s)-1;
            unsigned char oldlen = SDS_TYPE_5_LEN(flags);
            assert((incr > 0 && oldlen+incr < 32) || (incr < 0 && oldlen >= (unsigned int)(-incr)));
            *fp = SDS_TYPE_5 | ((oldlen+incr) << SDS_TYPE_BITS);
            len = oldlen+incr;
            break;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDS_TYPE_16: {
            SDS_HDR_VAR(16,s);
            assert((incr >= 0 && sh->alloc-sh->len >= incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDS_TYPE_32: {
            SDS_HDR_VAR(32,s);
            assert((incr >= 0 && sh->alloc-sh->len >= (unsigned int)incr) || (incr < 0 && sh->len >= (unsigned int)(-incr)));
            len = (sh->len += incr);
            break;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            assert((incr >= 0 && sh->alloc-sh->len >= (uint64_t)incr) || (incr < 0 && sh->len >= (uint64_t)(-incr)));
            len = (sh->len += incr);
            break;
        }
        default: len = 0; /* Just to avoid compilation warnings. */
    }
    s[len] = '\0';
}
/* Reallocate the sds string so that it has no free space at the end. The
 * contained string remains not altered, but next concatenation operations
 * will require a reallocation.
 *
 * After the call, the passed sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
/**
 * 移除sds字符串的空闲空间，压缩内存
 * @param s 源sds字符串
 * @return 压缩后的sds字符串
 */
sds sdsCreate::sdsRemoveFreeSpace(sds s)
{
    void *sh, *newsh;
    char type, oldtype = s[-1] & SDS_TYPE_MASK;
    int hdrlen, oldhdrlen = sdsHdrSize(oldtype);
    size_t len = sdslen(s);
    size_t avail = sdsavail(s);
    sh = (char*)s-oldhdrlen;

    /* Return ASAP if there is no space left. */
    if (avail == 0) return s;

    /* Check what would be the minimum SDS header that is just good enough to
     * fit this string. */
    type = sdsReqType(len);
    hdrlen = sdsHdrSize(type);

    /* If the type is the same, or at least a large enough type is still
     * required, we just realloc(), letting the allocator to do the copy
     * only if really needed. Otherwise if the change is huge, we manually
     * reallocate the string to use the different header type. */
    if (oldtype==type || type > SDS_TYPE_8) {
        newsh = zrealloc(sh, oldhdrlen+len+1);
        if (newsh == NULL) return NULL;
        s = (char*)newsh+oldhdrlen;
    } else {
        newsh = zmalloc(hdrlen+len+1);
        if (newsh == NULL) return NULL;
        memcpy((char*)newsh+hdrlen, s, len+1);
        zfree(sh);
        s = (char*)newsh+hdrlen;
        s[-1] = type;
        sdssetlen(s, len);
    }
    sdssetalloc(s, len);
    return s;
}
/**
 * 获取sds字符串的总分配大小（包括头部和字符串数据）
 * @param s sds字符串
 * @return 总分配大小
 */
/* Return the total size of the allocation of the specified sds string,
 * including:
 * 1) The sds header before the pointer.
 * 2) The string.
 * 3) The free buffer at the end if any.
 * 4) The implicit null term.
 */
size_t sdsCreate::sdsAllocSize(sds s)
{
    size_t alloc = sdsalloc(s);
    return sdsHdrSize(s[-1])+alloc+1;
}
/**
 * 获取sds字符串的数据指针（跳过头部）
 * @param s sds字符串
 * @return 数据指针
 */
/* Return the pointer of the actual SDS allocation (normally SDS strings
 * are referenced by the start of the string buffer). */
void *sdsCreate::sdsAllocPtr(sds s)
{
    return (void*) (s-sdsHdrSize(s[-1]));
}



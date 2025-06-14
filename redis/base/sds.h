/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/12
 * Description: A C dynamic strings library
 */
#ifndef __SDS_H
#define __SDS_H
#define SDS_MAX_PREALLOC (1024*1024)
extern const char *SDS_NOINIT;
#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
typedef char *sds;
//SDS_NOINIT 是一个空指针常量，用于告诉 SDS 创建函数不要初始化新分配的内存
/*
struct Example {
    char a;     // 1字节
    int b;      // 4字节
    char c;     // 1字节
};
// 总大小：1（a） + 3（填充） + 4（b） + 1（c） + 3（填充） = 12字节
struct __attribute__ ((__packed__)) Example {
    char a;     // 1字节
    int b;      // 4字节
    char c;     // 1字节
};
// 总大小：1（a） + 4（b） + 1（c） = 6字节
*/
/* Note: sdshdr5 is never used, we just access the flags byte directly.
 * However is here to document the layout of type 5 SDS strings. */
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr8 {
    uint8_t len; /* used */
    uint8_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr32 {
    uint32_t len; /* used */
    uint32_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));//用于从 SDS 字符串的数据区指针反推其头部结构体的地址,就是buf地址
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)//用于从 SDS（Simple Dynamic String）类型 5 的标志字节中提取字符串长度。



class sds
{
public:
        sds(){}
        sds(const sds&) = delete;
        sds& operator=(const sds&) = delete;

        sds sdsnewlen(const void *init, size_t initlen);
        sds sdstrynewlen(const void *init, size_t initlen);
        sds sdsnew(const char *init);
        sds sdsempty(void);
        sds sdsdup(const sds s);
        void sdsfree(sds s);
        sds sdsgrowzero(sds s, size_t len);
        sds sdscatlen(sds s, const void *t, size_t len);
        sds sdscat(sds s, const char *t);
        sds sdscatsds(sds s, const sds t);
        sds sdscpylen(sds s, const char *t, size_t len);
        sds sdscpy(sds s, const char *t);

        sds sdscatvprintf(sds s, const char *fmt, va_list ap);
        #ifdef __GNUC__
        sds sdscatprintf(sds s, const char *fmt, ...)
            __attribute__((format(printf, 2, 3)));
        #else
        sds sdscatprintf(sds s, const char *fmt, ...);
        #endif

        sds sdscatfmt(sds s, char const *fmt, ...);
        sds sdstrim(sds s, const char *cset);
        void sdssubstr(sds s, size_t start, size_t len);
        void sdsrange(sds s, ssize_t start, ssize_t end);
        void sdsupdatelen(sds s);
        void sdsclear(sds s);
        int sdscmp(const sds s1, const sds s2);
        sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
        void sdsfreesplitres(sds *tokens, int count);
        void sdstolower(sds s);
        void sdstoupper(sds s);
        sds sdsfromlonglong(long long value);
        sds sdscatrepr(sds s, const char *p, size_t len);
        sds *sdssplitargs(const char *line, int *argc);
        sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
        sds sdsjoin(char **argv, int argc, char *sep);
        sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);

        /* Callback for sdstemplate. The function gets called by sdstemplate
        * every time a variable needs to be expanded. The variable name is
        * provided as variable, and the callback is expected to return a
        * substitution value. Returning a NULL indicates an error.
        */
        typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
        sds sdstemplate(const char *template, sdstemplate_callback_t cb_func, void *cb_arg);

        /* Low level functions exposed to the user API */
        sds sdsMakeRoomFor(sds s, size_t addlen);
        void sdsIncrLen(sds s, ssize_t incr);
        sds sdsRemoveFreeSpace(sds s);
        size_t sdsAllocSize(sds s);
        void *sdsAllocPtr(sds s);

        /* Export the allocator used by SDS to the program using SDS.
        * Sometimes the program SDS is linked to, may use a different set of
        * allocators, but may want to allocate or free things that SDS will
        * respectively free or allocate. */
        void *sds_malloc(size_t size);
        void *sds_realloc(void *ptr, size_t size);
        void sds_free(void *ptr);


public:       
        inline size_t sdslen(const sds s) {
            unsigned char flags = s[-1];
            switch(flags&SDS_TYPE_MASK) {
                case SDS_TYPE_5:
                    return SDS_TYPE_5_LEN(flags);
                case SDS_TYPE_8:
                    return SDS_HDR(8,s)->len;
                case SDS_TYPE_16:
                    return SDS_HDR(16,s)->len;
                case SDS_TYPE_32:
                    return SDS_HDR(32,s)->len;
                case SDS_TYPE_64:
                    return SDS_HDR(64,s)->len;
            }
            return 0;
        }

        inline size_t sdsavail(const sds s) {
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

        inline void sdssetlen(sds s, size_t newlen) {
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

        inline void sdsinclen(sds s, size_t inc) {
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

        /* sdsalloc() = sdsavail() + sdslen() */
        inline size_t sdsalloc(const sds s) {
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

        inline void sdssetalloc(sds s, size_t newlen) {
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
};
#endif
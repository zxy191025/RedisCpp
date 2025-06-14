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
// #define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));//用于从 SDS 字符串的数据区指针反推其头部结构体的地址,就是buf地址
// #define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
// #define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)//用于从 SDS（Simple Dynamic String）类型 5 的标志字节中提取字符串长度。


#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = static_cast<sdshdr##T*>((void*)((s)-(sizeof(struct sdshdr##T))));
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)
class sds
{
public:
        sds(){}
        sds(const char *init);
        sds(const sds&) = delete;
        sds& operator=(const sds&) = delete;

        char* sdsnewlen(const void *init, size_t initlen);
        char* sdstrynewlen(const void *init, size_t initlen);
        char* sdsnew(const char *init);
        char* sdsempty(void);
        char* sdsdup(const char* s);
        void sdsfree(char* s);
        char* sdsgrowzero(char* s, size_t len);
        char* sdscatlen(char* s, const void *t, size_t len);
        char* sdscat(char* s, const char *t);
        char* sdscatsds(char* s, const char* t);
        char* sdscpylen(char* s, const char *t, size_t len);
        char* sdscpy(char* s, const char *t);

        char* sdscatvprintf(char* s, const char *fmt, va_list ap);
        
        #ifdef __GNUC__
        char* sdscatprintf(char* s, const char *fmt, ...);
        //  __attribute__((format(printf, 2, 3)));
        #else
        char* sdscatprintf(char* s, const char *fmt, ...);
        #endif

        char* sdscatfmt(char* s, char const *fmt, ...);
        char* sdstrim(char* s, const char *cset);
        void sdssubstr(char* s, size_t start, size_t len);
        void sdsrange(char* s, ssize_t start, ssize_t end);
        void sdsupdatelen(char* s);
        void sdsclear(char* s);
        int sdscmp(const char* s1, const char* s2);
        char* *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
        void sdsfreesplitres(char* *tokens, int count);
        void sdstolower(char* s);
        void sdstoupper(char* s);
        char* sdsfromlonglong(long long value);
        char* sdscatrepr(char* s, const char *p, size_t len);
        char* *sdssplitargs(const char *line, int *argc);
        char* sdsmapchars(char* s, const char *from, const char *to, size_t setlen);
        char* sdsjoin(char **argv, int argc, char *sep);
        char* sdsjoinsds(char* *argv, int argc, const char *sep, size_t seplen);

        /* Callback for sdstemplate. The function gets called by sdstemplate
        * every time a variable needs to be expanded. The variable name is
        * provided as variable, and the callback is expected to return a
        * substitution value. Returning a NULL indicates an error.
        */
        typedef char* (*sdstemplate_callback_t)(const char* variable, void *arg);
        char* sdstemplate(const char *temp, sdstemplate_callback_t cb_func, void *cb_arg);

        /* Low level functions exposed to the user API */
        char* sdsMakeRoomFor(char* s, size_t addlen);
        void sdsIncrLen(char* s, ssize_t incr);
        char* sdsRemoveFreeSpace(char* s);
        size_t sdsAllocSize(char* s);
        void *sdsAllocPtr(char* s);

        /* Export the allocator used by SDS to the program using SDS.
        * Sometimes the program SDS is linked to, may use a different set of
        * allocators, but may want to allocate or free things that SDS will
        * respectively free or allocate. */
        void *sds_malloc(size_t size);
        void *sds_realloc(void *ptr, size_t size);
        void sds_free(void *ptr);
public:       
        size_t sdslen(const char* s);
         size_t sdsavail(const char* s);
         void sdssetlen(char* s, size_t newlen);
         void sdsinclen(char* s, size_t inc);
         size_t sdsalloc(const char* s);
         void sdssetalloc(char* s, size_t newlen);
         int sdsHdrSize(char type);
         char sdsReqType(size_t string_size);
         size_t sdsTypeMaxSize(char type);
};
#endif
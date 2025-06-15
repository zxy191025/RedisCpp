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
//  高5位         低3位
//+-------+-------+
//| 00101 | 000   |
//+-------+-------+
// 长度=5   类型=0 (SDS_TYPE_5)
struct __attribute__ ((__packed__)) sdshdr5 {
    unsigned char flags; /* 3 lsb of type, and 5 msb of string length */
    char buf[];
};
//内存地址       | 内容
//---------------------------------
//ptr            | [type=8][len=5][alloc=10]  <-- struct sdshdr8
//ptr + 3        | 'h' 'e' 'l' 'l' 'o' '\0'    <-- s 指向此处
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
// flags 的低 3 位（Least Significant Bits, LSB）存储 SDS 的类型（如 SDS_TYPE_8、SDS_TYPE_16 等）。
// 支持 8 种类型（0-7），实际使用 5 种（SDS_TYPE_5 到 SDS_TYPE_64）。
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
//用于从 SDS 字符串的数据区指针反推其头部结构体的地址,就是buf地址
#define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = static_cast<sdshdr##T*>((void*)((s)-(sizeof(struct sdshdr##T))));
//调用SDS_HDR(8, s)
//SDS_HDR(8, s) → ((struct sdshdr8 *)((s)-(sizeof(struct sdshdr8))))
//内存地址       | 内容
//---------------------------------
//ptr            | [type=8][len=5][alloc=10]  <-- struct sdshdr8
//ptr + 3        | 'h' 'e' 'l' 'l' 'o' '\0'    <-- s 指向此处
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
#define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)
typedef char *sds;

class sdsCreate
{
public:
    sdsCreate(){}
    sdsCreate(const char *init);
    sdsCreate(const sdsCreate&) = delete;
    sdsCreate& operator=(const sdsCreate&) = delete;
public:
    // 创建指定长度的 SDS 字符串（标准版本）
    // - init: 初始数据指针（可为 NULL，表示初始化为空）
    // - initlen: 初始数据长度
    // - 内存分配失败时触发 Redis 错误处理（通常终止程序）
    sds sdsnewlen(const void *init, size_t initlen);
    // 创建指定长度的 SDS 字符串（尝试版本）
    // - 参数与 sdsnewlen 相同
    // - 内存分配失败时返回 NULL，不触发错误处理
    sds sdstrynewlen(const void *init, size_t initlen);
    // - init: C 风格字符串指针（以 '\0' 结尾）
    // - 自动计算字符串长度（使用 strlen）
    // - 若 init 为 NULL，等价于创建空字符串
    sds sdsnew(const char *init);
    //释放sds内存
    void sdsfree(sds s);
    // 将 C 字符串追加到 SDS 字符串的末尾
    sds sdscat(sds s, const char *t);
    //创建一个空的 SDS 字符串（长度为 0）
    sds sdsempty(void);
    //复制一个 SDS 字符串
    sds sdsdup(const sds s);
    //将 SDS 字符串扩展到指定长度，并用 '\0' 填充新增区域
    sds sdsgrowzero(sds s, size_t len);
    //将任意二进制数据追加到 SDS 字符串末尾
    /* s: 目标 SDS 字符串（会被修改）
    *  t: 源数据指针（可以是字符串、二进制数据等）
    *  len: 要追加的数据长度（字节）*/
    sds sdscatlen(sds s, const void *t, size_t len);

   
    sds sdscatsds(sds s, const sds t);
    sds sdscpylen(sds s, const char *t, size_t len);
    sds sdscpy(sds s, const char *t);
    sds sdscatvprintf(sds s, const char *fmt, va_list ap);
    sds sdscatprintf(sds s, const char *fmt, ...);
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
    typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
    sds sdstemplate(const char *temp, sdstemplate_callback_t cb_func, void *cb_arg);
    sds sdsMakeRoomFor(sds s, size_t addlen);
    void sdsIncrLen(sds s, ssize_t incr);
    sds sdsRemoveFreeSpace(sds s);
    size_t sdsAllocSize(sds s);
    void *sdsAllocPtr(sds s);
    sds _sdsnewlen(const void *init, size_t initlen, int trymalloc);
    int is_hex_digit(char c) ;
    int hex_digit_to_int(char c) ;
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
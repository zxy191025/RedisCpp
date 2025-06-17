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
    void sdsfree(sds s);
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
    sds sdscpy(sds s, const char *t);
    sds sdscatvprintf(sds s, const char *fmt, va_list ap);
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
    sds sdscatprintf(sds s, const char *fmt, ...);
    /*
    * 按格式化字符串将数据追加到 SDS 对象末尾（优化版）
    * 
    * @param s 目标 SDS 对象，追加操作将在此对象上进行
    * @param fmt 格式化字符串，支持简化的格式说明符
    * @param ... 可变参数列表，根据 fmt 指定的格式提供数据
    * */
    sds sdscatfmt(sds s, char const *fmt, ...);
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
    sds sdstrim(sds s, const char *cset);
    //截取并保留 SDS 字符串的指定区间，将原字符串修改为指定范围内的子串
    //end = -1代表最后一个字符 -2 表示倒数第二个字符，依此类推。
    void sdsrange(sds s, ssize_t start, ssize_t end);

    //sdscmp 函数用于比较两个 SDS 字符串的内容
    // 返回值规则：
    // 负数：s1 小于 s2（如 s1="abc"，s2="abd"）。
    // 零：s1 等于 s2。
    // 正数：s1 大于 s2（如 s1="abd"，s2="abc"）。
    int sdscmp(const sds s1, const sds s2);
    // 该函数将原始字符串转换为 C 语言可识别的转义格式，
    // 例如将二进制数据中的\0转为\\0，确保字符串在日志或序列化场景中能正确展示。转义后的字符串始终以双引号包裹，
    // 符合 C 语言字符串字面量的格式。

    // 应用场景
    // 日志记录：将二进制数据或特殊字符转义后存入日志，避免乱码或解析错误。
    // 调试输出：在调试时展示字符串的原始内容（包括转义字符）。
    // 协议序列化：生成符合特定格式要求的转义字符串（如 JSON、CSV 中的字符串字段）。
    sds sdscatrepr(sds s, const char *p, size_t len);
    typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
    // /*
    //  * 根据模板字符串生成动态内容的 SDS 字符串
    //  * 
    //  * @param temp 模板字符串，包含格式占位符（如 {{name}}）
    //  * @param cb_func 回调函数指针，用于处理占位符替换逻辑
    //  * @param cb_arg 传递给回调函数的用户参数（可自定义上下文）
    //  * /     
    sds sdstemplate(const char *temp, sdstemplate_callback_t cb_func, void *cb_arg);
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
    */
    // 执行扩容策略：
    // 小字符串按 2 倍扩容
    // 大字符串按固定比例扩容
    // 必要时升级 SDS 类型
    sds sdsMakeRoomFor(sds s, size_t addlen);
    /**
     * 直接调整 SDS 字符串的长度（高级底层操作）
     * 
     * @param s 目标 SDS 对象，必须为有效指针
     * @param incr 长度增量（可正可负）
     */
    void sdsIncrLen(sds s, ssize_t incr);
    //查询当前 SDS 字符串的空闲空间
    //获取空闲空间
    //返回当前 SDS 对象中尚未使用的字节数（即预留的可追加空间）。
    //例如：若 SDS 总容量为 100 字节，已使用 60 字节，则 sdsavail(s) 返回 40。
    size_t sdsavail(const char* s);
    
    //获取 SDS 字符串的长度，不包括\0结尾符
    size_t sdslen(const char* s);
public:
    void sdssubstr(sds s, size_t start, size_t len);
    sds *sdssplitlen(const char *s, ssize_t len, const char *sep, int seplen, int *count);
    void sdsfreesplitres(sds *tokens, int count);
    void sdstolower(sds s);
    void sdstoupper(sds s);
    sds sdsfromlonglong(long long value);
    sds sdscatsds(sds s, const sds t);
    sds sdscpylen(sds s, const char *t, size_t len);
    void sdssetlen(char* s, size_t newlen);
    void sdsinclen(char* s, size_t inc);
    size_t sdsalloc(const char* s);
    void sdssetalloc(char* s, size_t newlen);
    int sdsHdrSize(char type);
    char sdsReqType(size_t string_size);
    size_t sdsTypeMaxSize(char type);
    sds sdsRemoveFreeSpace(sds s);
    size_t sdsAllocSize(sds s);
    void *sdsAllocPtr(sds s);
    sds _sdsnewlen(const void *init, size_t initlen, int trymalloc);
    int is_hex_digit(char c) ;
    int hex_digit_to_int(char c) ;
    sds *sdssplitargs(const char *line, int *argc);
    sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen);
    sds sdsjoin(char **argv, int argc, char *sep);
    sds sdsjoinsds(sds *argv, int argc, const char *sep, size_t seplen);
    void sdsupdatelen(sds s);
    void sdsclear(sds s);

};
#endif
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: tool function implementation
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <float.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include "toolFunc.h"
#include "fmacros.h"
#include "sds.h"
/****************************** MACROS ******************************/
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

/**************************** VARIABLES *****************************/
typedef uint32_t WORD;  // 32-bit word
static const WORD k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};


/**
 * 循环左移操作 - 将32位值循环左移指定位数
 * 例如: rol(0x12345678, 8) = 0x34567812
 * 
 * @param value 待移位的32位值
 * @param bits  移位位数(0-31)
 * @return 循环左移后的结果
 */
#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

/* 
 * blk0() 和 blk() 用于消息块扩展
 * 在处理前将64字节输入块扩展为80个32位字
 */
/* 根据系统字节序处理输入数据（大端/小端转换） */
#if BYTE_ORDER == LITTLE_ENDIAN
/**
 * 小端系统下的块初始化 - 进行字节序转换
 * 将每个32位字从主机字节序转换为网络字节序(大端)
 */
#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    |(rol(block->l[i],8)&0x00FF00FF))
#elif BYTE_ORDER == BIG_ENDIAN
/**
 * 大端系统下的块初始化 - 直接使用原始数据
 */
#define blk0(i) block->l[i]
#else
#error "Endianness not defined!"
#endif

/**
 * 块扩展函数 - 生成后续64个消息字
 * 使用先前生成的字通过异或和循环移位生成新字
 * 
 * @param i 消息字索引(0-79)
 */
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

/* 
 * SHA-1算法四轮操作的宏定义
 * 每轮使用不同的逻辑函数和常量，处理80个消息字
 */
/**
 * 第1轮操作(0-19步) - 使用函数 f(t) = (B ∧ C) ∨ (¬B ∧ D)
 * @param v,w,x,y,z  当前5个状态寄存器
 * @param i         当前处理的消息字索引
 */
#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);

/**
 * 第1轮操作(20-39步) - 与R0类似但使用扩展后的消息字
 * @param v,w,x,y,z  当前5个状态寄存器
 * @param i         当前处理的消息字索引
 */
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);

/**
 * 第2轮操作(40-59步) - 使用函数 f(t) = B ⊕ C ⊕ D
 * @param v,w,x,y,z  当前5个状态寄存器
 * @param i         当前处理的消息字索引
 */
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);

/**
 * 第3轮操作(60-79步) - 使用函数 f(t) = (B ∧ C) ∨ (B ∧ D) ∨ (C ∧ D)
 * @param v,w,x,y,z  当前5个状态寄存器
 * @param i         当前处理的消息字索引
 */
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);

/**
 * 第4轮操作(80-79步) - 使用函数 f(t) = B ⊕ C ⊕ D
 * @param v,w,x,y,z  当前5个状态寄存器
 * @param i         当前处理的消息字索引
 */
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//

/**
 * 字符串模式匹配（带长度限制）
 * @param p 模式字符串
 * @param plen 模式长度
 * @param s 待匹配字符串
 * @param slen 待匹配字符串长度
 * @param nocase 是否忽略大小写(非零为忽略)
 * @return 匹配成功返回1，失败返回0
 */
/* Glob-style pattern matching. */
int toolFunc::stringmatchlen(const char *pattern, int patternLen,
        const char *string, int stringLen, int nocase)
{
    while(patternLen && stringLen) {
        switch(pattern[0]) {
        case '*':
            while (patternLen && pattern[1] == '*') {
                pattern++;
                patternLen--;
            }
            if (patternLen == 1)
                return 1; /* match */
            while(stringLen) {
                if (stringmatchlen(pattern+1, patternLen-1,
                            string, stringLen, nocase))
                    return 1; /* match */
                string++;
                stringLen--;
            }
            return 0; /* no match */
            break;
        case '?':
            string++;
            stringLen--;
            break;
        case '[':
        {
            int not1, match;

            pattern++;
            patternLen--;
            not1 = pattern[0] == '^';
            if (not1) {
                pattern++;
                patternLen--;
            }
            match = 0;
            while(1) {
                if (pattern[0] == '\\' && patternLen >= 2) {
                    pattern++;
                    patternLen--;
                    if (pattern[0] == string[0])
                        match = 1;
                } else if (pattern[0] == ']') {
                    break;
                } else if (patternLen == 0) {
                    pattern--;
                    patternLen++;
                    break;
                } else if (patternLen >= 3 && pattern[1] == '-') {
                    int start = pattern[0];
                    int end = pattern[2];
                    int c = string[0];
                    if (start > end) {
                        int t = start;
                        start = end;
                        end = t;
                    }
                    if (nocase) {
                        start = tolower(start);
                        end = tolower(end);
                        c = tolower(c);
                    }
                    pattern += 2;
                    patternLen -= 2;
                    if (c >= start && c <= end)
                        match = 1;
                } else {
                    if (!nocase) {
                        if (pattern[0] == string[0])
                            match = 1;
                    } else {
                        if (tolower((int)pattern[0]) == tolower((int)string[0]))
                            match = 1;
                    }
                }
                pattern++;
                patternLen--;
            }
            if (not1)
                match = !match;
            if (!match)
                return 0; /* no match */
            string++;
            stringLen--;
            break;
        }
        case '\\':
            if (patternLen >= 2) {
                pattern++;
                patternLen--;
            }
            /* fall through */
        default:
            if (!nocase) {
                if (pattern[0] != string[0])
                    return 0; /* no match */
            } else {
                if (tolower((int)pattern[0]) != tolower((int)string[0]))
                    return 0; /* no match */
            }
            string++;
            stringLen--;
            break;
        }
        pattern++;
        patternLen--;
        if (stringLen == 0) {
            while(*pattern == '*') {
                pattern++;
                patternLen--;
            }
            break;
        }
    }
    if (patternLen == 0 && stringLen == 0)
        return 1;
    return 0;
}
/**
 * 字符串模式匹配（完整长度）
 * @param p 模式字符串(以'\0'结尾)
 * @param s 待匹配字符串(以'\0'结尾)
 * @param nocase 是否忽略大小写(非零为忽略)
 * @return 匹配成功返回1，失败返回0
 */
int toolFunc::stringmatch(const char *pattern, const char *string, int nocase) {
    return stringmatchlen(pattern,strlen(pattern),string,strlen(string),nocase);
}

/* Convert a string representing an amount of memory into the number of
 * bytes, so for instance memtoll("1Gb") will return 1073741824 that is
 * (1024*1024*1024).
 *
 * On parsing error, if *err is not NULL, it's set to 1, otherwise it's
 * set to 0. On error the function return value is 0, regardless of the
 * fact 'err' is NULL or not. */
/**
 * 将内存表示的数值字符串转换为long long类型
 * 支持后缀：k/K(千), m/M(兆), g/G(吉), t/T(太)
 * @param p 输入字符串
 * @param err 错误指示器(成功返回0，失败返回非零)
 * @return 转换后的数值
 */
long long toolFunc::memtoll(const char *p, int *err) {
    const char *u;
    char buf[128];
    long mul; /* unit multiplier */
    long long val;
    unsigned int digits;

    if (err) *err = 0;

    /* Search the first non digit character. */
    u = p;
    if (*u == '-') u++;
    while(*u && isdigit(*u)) u++;
    if (*u == '\0' || !strcasecmp(u,"b")) {
        mul = 1;
    } else if (!strcasecmp(u,"k")) {
        mul = 1000;
    } else if (!strcasecmp(u,"kb")) {
        mul = 1024;
    } else if (!strcasecmp(u,"m")) {
        mul = 1000*1000;
    } else if (!strcasecmp(u,"mb")) {
        mul = 1024*1024;
    } else if (!strcasecmp(u,"g")) {
        mul = 1000L*1000*1000;
    } else if (!strcasecmp(u,"gb")) {
        mul = 1024L*1024*1024;
    } else {
        if (err) *err = 1;
        return 0;
    }

    /* Copy the digits into a buffer, we'll use strtoll() to convert
     * the digit (without the unit) into a number. */
    digits = u-p;
    if (digits >= sizeof(buf)) {
        if (err) *err = 1;
        return 0;
    }
    memcpy(buf,p,digits);
    buf[digits] = '\0';

    char *endptr;
    errno = 0;
    val = strtoll(buf,&endptr,10);
    if ((val == 0 && errno == EINVAL) || *endptr != '\0') {
        if (err) *err = 1;
        return 0;
    }
    return val*mul;
}

/* Search a memory buffer for any set of bytes, like strpbrk().
 * Returns pointer to first found char or NULL.
 */
/**
 * 在指定内存区域查找任意字符
 * @param s 源内存区域
 * @param len 源内存长度
 * @param chars 待查找字符集
 * @param charslen 字符集长度
 * @return 找到的第一个匹配字符的指针，未找到返回NULL
 */
const char *toolFunc::mempbrk(const char *s, size_t len, const char *chars, size_t charslen) {
    for (size_t j = 0; j < len; j++) {
        for (size_t n = 0; n < charslen; n++)
            if (s[j] == chars[n]) return &s[j];
    }

    return NULL;
}

/* Modify the buffer replacing all occurrences of chars from the 'from'
 * set with the corresponding char in the 'to' set. Always returns s.
 */
/**
 * 字符映射替换
 * @param s 源字符串
 * @param len 字符串长度
 * @param from 源字符集
 * @param to 目标字符集
 * @param setlen 字符集长度
 * @return 处理后的字符串指针
 */
char *toolFunc::memmapchars(char *s, size_t len, const char *from, const char *to, size_t setlen) {
    for (size_t j = 0; j < len; j++) {
        for (size_t i = 0; i < setlen; i++) {
            if (s[j] == from[i]) {
                s[j] = to[i];
                break;
            }
        }
    }
    return s;
}

/* Return the number of digits of 'v' when converted to string in radix 10.
 * See ll2string() for more information. */
/**
 * 计算无符号64位整数的十进制位数
 * @param v 输入值
 * @return 十进制位数
 */
uint32_t toolFunc::digits10(uint64_t v) {
    if (v < 10) return 1;
    if (v < 100) return 2;
    if (v < 1000) return 3;
    if (v < 1000000000000UL) {
        if (v < 100000000UL) {
            if (v < 1000000) {
                if (v < 10000) return 4;
                return 5 + (v >= 100000);
            }
            return 7 + (v >= 10000000UL);
        }
        if (v < 10000000000UL) {
            return 9 + (v >= 1000000000UL);
        }
        return 11 + (v >= 100000000000UL);
    }
    return 12 + digits10(v / 1000000000000UL);
}

/* Like digits10() but for signed values. */
/**
 * 计算有符号64位整数的十进制位数
 * @param v 输入值
 * @return 十进制位数(包括符号位)
 */
uint32_t toolFunc::sdigits10(int64_t v) {
    if (v < 0) {
        /* Abs value of LLONG_MIN requires special handling. */
        uint64_t uv = (v != LLONG_MIN) ?
                      (uint64_t)-v : ((uint64_t) LLONG_MAX)+1;
        return digits10(uv)+1; /* +1 for the minus. */
    } else {
        return digits10(v);
    }
}

/* Convert a long long into a string. Returns the number of
 * characters needed to represent the number.
 * If the buffer is not big enough to store the string, 0 is returned.
 *
 * Based on the following article (that apparently does not provide a
 * novel approach but only publicizes an already used technique):
 *
 * https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 *
 * Modified in order to handle signed integers since the original code was
 * designed for unsigned integers. */
/**
 * 将long long类型整数转换为字符串
 * @param s 输出缓冲区
 * @param len 缓冲区长度
 * @param value 输入值
 * @return 写入的字符数，失败返回-1
 */
int toolFunc::ll2string(char *dst, size_t dstlen, long long svalue) {
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int negative;
    unsigned long long value;

    /* The main loop works with 64bit unsigned integers for simplicity, so
     * we convert the number here and remember if it is negative. */
    if (svalue < 0) {
        if (svalue != LLONG_MIN) {
            value = -svalue;
        } else {
            value = ((unsigned long long) LLONG_MAX)+1;
        }
        negative = 1;
    } else {
        value = svalue;
        negative = 0;
    }

    /* Check length. */
    uint32_t const length = digits10(value)+negative;
    if (length >= dstlen) return 0;

    /* Null term. */
    uint32_t next = length;
    dst[next] = '\0';
    next--;
    while (value >= 100) {
        int const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }

    /* Handle last 1-2 digits. */
    if (value < 10) {
        dst[next] = '0' + (uint32_t) value;
    } else {
        int i = (uint32_t) value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    /* Add sign. */
    if (negative) dst[0] = '-';
    return length;
}

/* Convert a string into a long long. Returns 1 if the string could be parsed
 * into a (non-overflowing) long long, 0 otherwise. The value will be set to
 * the parsed value when appropriate.
 *
 * Note that this function demands that the string strictly represents
 * a long long: no spaces or other characters before or after the string
 * representing the number are accepted, nor zeroes at the start if not
 * for the string "0" representing the zero number.
 *
 * Because of its strictness, it is safe to use this function to check if
 * you can convert a string into a long long, and obtain back the string
 * from the number without any loss in the string representation. */
/**
 * 将字符串转换为long long类型整数
 * @param s 输入字符串
 * @param slen 字符串长度
 * @param value 输出值
 * @return 成功返回1，失败返回0
 */
int toolFunc::string2ll(const char *s, size_t slen, long long *value) {
    const char *p = s;
    size_t plen = 0;
    int negative = 0;
    unsigned long long v;

    /* A zero length string is not a valid number. */
    if (plen == slen)
        return 0;

    /* Special case: first and only digit is 0. */
    if (slen == 1 && p[0] == '0') {
        if (value != NULL) *value = 0;
        return 1;
    }

    /* Handle negative numbers: just set a flag and continue like if it
     * was a positive number. Later convert into negative. */
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
    } else {
        return 0;
    }

    /* Parse all the other digits, checking for overflow at every step. */
    while (plen < slen && p[0] >= '0' && p[0] <= '9') {
        if (v > (ULLONG_MAX / 10)) /* Overflow. */
            return 0;
        v *= 10;

        if (v > (ULLONG_MAX - (p[0]-'0'))) /* Overflow. */
            return 0;
        v += p[0]-'0';

        p++; plen++;
    }

    /* Return if not all bytes were used. */
    if (plen < slen)
        return 0;

    /* Convert to negative if needed, and do the final overflow check when
     * converting from unsigned long long to long long. */
    if (negative) {
        if (v > ((unsigned long long)(-(LLONG_MIN+1))+1)) /* Overflow. */
            return 0;
        if (value != NULL) *value = -v;
    } else {
        if (v > LLONG_MAX) /* Overflow. */
            return 0;
        if (value != NULL) *value = v;
    }
    return 1;
}

/* Helper function to convert a string to an unsigned long long value.
 * The function attempts to use the faster string2ll() function inside
 * Redis: if it fails, strtoull() is used instead. The function returns
 * 1 if the conversion happened successfully or 0 if the number is
 * invalid or out of range. */
/**
 * 将字符串转换为unsigned long long类型整数
 * @param s 输入字符串
 * @param value 输出值
 * @return 成功返回1，失败返回0
 */
int toolFunc::string2ull(const char *s, unsigned long long *value) {
    long long ll;
    if (string2ll(s,strlen(s),&ll)) {
        if (ll < 0) return 0; /* Negative values are out of range. */
        *value = ll;
        return 1;
    }
    errno = 0;
    char *endptr = NULL;
    *value = strtoull(s,&endptr,10);
    if (errno == EINVAL || errno == ERANGE || !(*s != '\0' && *endptr == '\0'))
        return 0; /* strtoull() failed. */
    return 1; /* Conversion done! */
}

/* Convert a string into a long. Returns 1 if the string could be parsed into a
 * (non-overflowing) long, 0 otherwise. The value will be set to the parsed
 * value when appropriate. */
/**
 * 将字符串转换为long类型整数
 * @param s 输入字符串
 * @param slen 字符串长度
 * @param value 输出值
 * @return 成功返回1，失败返回0
 */
int toolFunc::string2l(const char *s, size_t slen, long *lval) {
    long long llval;

    if (!string2ll(s,slen,&llval))
        return 0;

    if (llval < LONG_MIN || llval > LONG_MAX)
        return 0;

    *lval = (long)llval;
    return 1;
}

/* Convert a string into a double. Returns 1 if the string could be parsed
 * into a (non-overflowing) double, 0 otherwise. The value will be set to
 * the parsed value when appropriate.
 *
 * Note that this function demands that the string strictly represents
 * a double: no spaces or other characters before or after the string
 * representing the number are accepted. */
/**
 * 将字符串转换为long double类型浮点数
 * @param s 输入字符串
 * @param slen 字符串长度
 * @param dp 输出值
 * @return 成功返回1，失败返回0
 */
int toolFunc::string2ld(const char *s, size_t slen, long double *dp) {
    char buf[MAX_LONG_DOUBLE_CHARS];
    long double value;
    char *eptr;

    if (slen == 0 || slen >= sizeof(buf)) return 0;
    memcpy(buf,s,slen);
    buf[slen] = '\0';

    errno = 0;
    value = strtold(buf, &eptr);
    if (isspace(buf[0]) || eptr[0] != '\0' ||
        (size_t)(eptr-buf) != slen ||
        (errno == ERANGE &&
            (value == HUGE_VAL || value == -HUGE_VAL || value == 0)) ||
        errno == EINVAL ||
        isnan(value))
        return 0;

    if (dp) *dp = value;
    return 1;
}

/* Convert a string into a double. Returns 1 if the string could be parsed
 * into a (non-overflowing) double, 0 otherwise. The value will be set to
 * the parsed value when appropriate.
 *
 * Note that this function demands that the string strictly represents
 * a double: no spaces or other characters before or after the string
 * representing the number are accepted. */
/**
 * 将字符串转换为double类型浮点数
 * @param s 输入字符串
 * @param slen 字符串长度
 * @param dp 输出值
 * @return 成功返回1，失败返回0
 */
int toolFunc::string2d(const char *s, size_t slen, double *dp) {
    errno = 0;
    char *eptr;
    *dp = strtod(s, &eptr);
    if (slen == 0 ||
        isspace(((const char*)s)[0]) ||
        (size_t)(eptr-(char*)s) != slen ||
        (errno == ERANGE &&
            (*dp == HUGE_VAL || *dp == -HUGE_VAL || *dp == 0)) ||
        isnan(*dp))
        return 0;
    return 1;
}

/* Convert a double to a string representation. Returns the number of bytes
 * required. The representation should always be parsable by strtod(3).
 * This function does not support human-friendly formatting like ld2string
 * does. It is intended mainly to be used inside t_zset.c when writing scores
 * into a ziplist representing a sorted set. */
/**
 * 将double类型浮点数转换为字符串
 * @param buf 输出缓冲区
 * @param len 缓冲区长度
 * @param value 输入值
 * @return 写入的字符数，失败返回-1
 */
int toolFunc::d2string(char *buf, size_t len, double value) {
    if (isnan(value)) {
        len = snprintf(buf,len,"nan");
    } else if (isinf(value)) {
        if (value < 0)
            len = snprintf(buf,len,"-inf");
        else
            len = snprintf(buf,len,"inf");
    } else if (value == 0) {
        /* See: http://en.wikipedia.org/wiki/Signed_zero, "Comparisons". */
        if (1.0/value < 0)
            len = snprintf(buf,len,"-0");
        else
            len = snprintf(buf,len,"0");
    } else {
#if (DBL_MANT_DIG >= 52) && (LLONG_MAX == 0x7fffffffffffffffLL)
        /* Check if the float is in a safe range to be casted into a
         * long long. We are assuming that long long is 64 bit here.
         * Also we are assuming that there are no implementations around where
         * double has precision < 52 bit.
         *
         * Under this assumptions we test if a double is inside an interval
         * where casting to long long is safe. Then using two castings we
         * make sure the decimal part is zero. If all this is true we use
         * integer printing function that is much faster. */
        double min = -4503599627370495; /* (2^52)-1 */
        double max = 4503599627370496; /* -(2^52) */
        if (value > min && value < max && value == ((double)((long long)value)))
            len = ll2string(buf,len,(long long)value);
        else
#endif
            len = snprintf(buf,len,"%.17g",value);
    }

    return len;
}

/* Create a string object from a long double.
 * If mode is humanfriendly it does not use exponential format and trims trailing
 * zeroes at the end (may result in loss of precision).
 * If mode is default exp format is used and the output of snprintf()
 * is not modified (may result in loss of precision).
 * If mode is hex hexadecimal format is used (no loss of precision)
 *
 * The function returns the length of the string or zero if there was not
 * enough buffer room to store it. */
/**
 * 将long double类型浮点数转换为字符串
 * @param buf 输出缓冲区
 * @param len 缓冲区长度
 * @param value 输入值
 * @param mode 转换模式(见ld2string_mode枚举)
 * @return 写入的字符数，失败返回-1
 */
int toolFunc::ld2string(char *buf, size_t len, long double value, ld2string_mode mode) {
    size_t l = 0;

    if (isinf(value)) {
        /* Libc in odd systems (Hi Solaris!) will format infinite in a
         * different way, so better to handle it in an explicit way. */
        if (len < 5) return 0; /* No room. 5 is "-inf\0" */
        if (value > 0) {
            memcpy(buf,"inf",3);
            l = 3;
        } else {
            memcpy(buf,"-inf",4);
            l = 4;
        }
    } else {
        switch (mode) {
        case LD_STR_AUTO:
            l = snprintf(buf,len,"%.17Lg",value);
            if (l+1 > len) return 0; /* No room. */
            break;
        case LD_STR_HEX:
            l = snprintf(buf,len,"%La",value);
            if (l+1 > len) return 0; /* No room. */
            break;
        case LD_STR_HUMAN:
            /* We use 17 digits precision since with 128 bit floats that precision
             * after rounding is able to represent most small decimal numbers in a
             * way that is "non surprising" for the user (that is, most small
             * decimal numbers will be represented in a way that when converted
             * back into a string are exactly the same as what the user typed.) */
            l = snprintf(buf,len,"%.17Lf",value);
            if (l+1 > len) return 0; /* No room. */
            /* Now remove trailing zeroes after the '.' */
            if (strchr(buf,'.') != NULL) {
                char *p = buf+l-1;
                while(*p == '0') {
                    p--;
                    l--;
                }
                if (*p == '.') l--;
            }
            if (l == 2 && buf[0] == '-' && buf[1] == '0') {
                buf[0] = '0';
                l = 1;
            }
            break;
        default: return 0; /* Invalid mode. */
        }
    }
    buf[l] = '\0';
    return l;
}

/* Get random bytes, attempts to get an initial seed from /dev/urandom and
 * the uses a one way hash function in counter mode to generate a random
 * stream. However if /dev/urandom is not available, a weaker seed is used.
 *
 * This function is not thread safe, since the state is global. */
/**
 * 生成指定长度的随机字节序列
 * 
 * @param p 指向用于存储随机字节的缓冲区的指针
 * @param len 需要生成的随机字节的长度（以字节为单位）
 * 
 * @note 该函数生成的是密码学安全的随机数，适用于需要高安全性随机值的场景
 * @note 调用者必须确保缓冲区p的大小至少为len字节
 * @warning 如果无法获取足够的随机熵，函数可能会阻塞或返回不确定值
 */
void toolFunc::getRandomBytes(unsigned char *p, size_t len) 
{
    /* Global state. */
    static int seed_initialized = 0;
    static unsigned char seed[64]; /* 512 bit internal block size. */
    static uint64_t counter = 0; /* The counter we hash with the seed. */

    if (!seed_initialized) {
        /* Initialize a seed and use SHA1 in counter mode, where we hash
         * the same seed with a progressive counter. For the goals of this
         * function we just need non-colliding strings, there are no
         * cryptographic security needs. */
        FILE *fp = fopen("/dev/urandom","r");
        if (fp == NULL || fread(seed,sizeof(seed),1,fp) != 1) {
            /* Revert to a weaker seed, and in this case reseed again
             * at every call.*/
            for (unsigned int j = 0; j < sizeof(seed); j++) {
                struct timeval tv;
                gettimeofday(&tv,NULL);
                pid_t pid = getpid();
                seed[j] = tv.tv_sec ^ tv.tv_usec ^ pid ^ (long)fp;
            }
        } else {
            seed_initialized = 1;
        }
        if (fp) fclose(fp);
    }

    while(len) {
        /* This implements SHA256-HMAC. */
        unsigned char digest[SHA256_BLOCK_SIZE];
        unsigned char kxor[64];
        unsigned int copylen =
            len > SHA256_BLOCK_SIZE ? SHA256_BLOCK_SIZE : len;

        /* IKEY: key xored with 0x36. */
        memcpy(kxor,seed,sizeof(kxor));
        for (unsigned int i = 0; i < sizeof(kxor); i++) kxor[i] ^= 0x36;

        /* Obtain HASH(IKEY||MESSAGE). */
        SHA256_CTX ctx;
        sha256_init(&ctx);
        sha256_update(&ctx,kxor,sizeof(kxor));
        sha256_update(&ctx,(unsigned char*)&counter,sizeof(counter));
        sha256_final(&ctx,digest);

        /* OKEY: key xored with 0x5c. */
        memcpy(kxor,seed,sizeof(kxor));
        for (unsigned int i = 0; i < sizeof(kxor); i++) kxor[i] ^= 0x5C;

        /* Obtain HASH(OKEY || HASH(IKEY||MESSAGE)). */
        sha256_init(&ctx);
        sha256_update(&ctx,kxor,sizeof(kxor));
        sha256_update(&ctx,digest,SHA256_BLOCK_SIZE);
        sha256_final(&ctx,digest);

        /* Increment the counter for the next iteration. */
        counter++;

        memcpy(p,digest,copylen);
        len -= copylen;
        p += copylen;
    }
}

/* Generate the Redis "Run ID", a SHA1-sized random number that identifies a
 * given execution of Redis, so that if you are talking with an instance
 * having run_id == A, and you reconnect and it has run_id == B, you can be
 * sure that it is either a different instance or it was restarted. */
/**
 * 生成指定长度的随机十六进制字符序列
 * 
 * @param p 指向用于存储随机十六进制字符的缓冲区的指针
 * @param len 需要生成的随机十六进制字符的长度（以字符为单位）
 * 
 * @note 生成的字符范围为'0'-'9'、'a'-'f'，例如："b35fe8a1"
 * @note 调用者必须确保缓冲区p的大小至少为len字节
 * @note 内部会调用getRandomBytes()生成底层随机字节
 * @warning len应为偶数，否则最后一个字符将被截断
 */
void toolFunc::getRandomHexChars(char *p, size_t len) {
    char *charset = "0123456789abcdef";
    size_t j;

    getRandomBytes((unsigned char*)p,len);
    for (j = 0; j < len; j++) p[j] = charset[p[j] & 0x0F];
}

/* Given the filename, return the absolute path as an SDS string, or NULL
 * if it fails for some reason. Note that "filename" may be an absolute path
 * already, this will be detected and handled correctly.
 *
 * The function does not try to normalize everything, but only the obvious
 * case of one or more "../" appearing at the start of "filename"
 * relative path. */
/**
 * 获取文件的绝对路径
 * @param filename 相对路径文件名
 * @return 绝对路径字符串指针，失败返回NULL
 * @note 返回的指针可能指向静态缓冲区，后续调用会覆盖
 */
sds toolFunc::getAbsolutePath(char *filename) {
    char cwd[1024];
    sds abspath;
    sdsCreate sdsc;
    sds relpath = sdsc.sdsnew(filename);

    relpath = sdsc.sdstrim(relpath," \r\n\t");
    if (relpath[0] == '/') return relpath; /* Path is already absolute. */

    /* If path is relative, join cwd and relative path. */
    if (getcwd(cwd,sizeof(cwd)) == NULL) {
        sdsc.sdsfree(relpath);
        return NULL;
    }
    abspath = sdsc.sdsnew(cwd);
    if (sdsc.sdslen(abspath) && abspath[sdsc.sdslen(abspath)-1] != '/')
        abspath = sdsc.sdscat(abspath,"/");

    /* At this point we have the current path always ending with "/", and
     * the trimmed relative path. Try to normalize the obvious case of
     * trailing ../ elements at the start of the path.
     *
     * For every "../" we find in the filename, we remove it and also remove
     * the last element of the cwd, unless the current cwd is "/". */
    while (sdsc.sdslen(relpath) >= 3 &&
           relpath[0] == '.' && relpath[1] == '.' && relpath[2] == '/')
    {
        sdsc.sdsrange(relpath,3,-1);
        if (sdsc.sdslen(abspath) > 1) {
            char *p = abspath + sdsc.sdslen(abspath)-2;
            int trimlen = 1;

            while(*p != '/') {
                p--;
                trimlen++;
            }
            sdsc.sdsrange(abspath,0,-(trimlen+1));
        }
    }

    /* Finally glue the two parts together. */
    abspath = sdsc.sdscatsds(abspath,relpath);
    sdsc.sdsfree(relpath);
    return abspath;
}

/*
 * Gets the proper timezone in a more portable fashion
 * i.e timezone variables are linux specific.
 */
/**
 * 获取当前系统时区偏移(秒)
 * @return 时区偏移量，UTC时间返回0
 */
long toolFunc::getTimeZone(void) {
#if defined(__linux__) || defined(__sun)
    return timezone;
#else
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    return tz.tz_minuteswest * 60L;
#endif
}

/* Return true if the specified path is just a file basename without any
 * relative or absolute path. This function just checks that no / or \
 * character exists inside the specified path, that's enough in the
 * environments where Redis runs. */
/**
 * 判断路径是否为基础文件名(不包含目录)
 * @param path 路径字符串
 * @return 是返回1，否返回0
 */
int toolFunc::pathIsBaseName(char *path) {
    return strchr(path,'/') == NULL && strchr(path,'\\') == NULL;
}


/**
 * 执行单轮 SHA256 块转换（核心压缩函数）
 * 
 * @param ctx 指向 SHA256 上下文的指针，包含当前哈希状态
 * @param data 指向 64 字节输入数据块的指针（必须为完整块）
 * 
 * @note 此函数实现 SHA256 算法的核心压缩步骤，处理单个 512 位（64 字节）数据块
 * @note 内部会将输入数据扩展为 64 个 32 位字，并对 ctx->state 进行 64 轮迭代压缩
 * @note 该函数不会修改输入数据，也不更新 ctx->datalen 和 ctx->bitlen
 * 
 * @warning 调用前必须确保 data 包含完整的 64 字节数据块
 * @warning 此函数为内部实现的低级接口，通常由 sha256_update() 自动调用
 * @warning 直接调用可能导致哈希计算错误，需严格遵循 SHA256 处理流程
 */
void toolFunc::sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
	WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}
/**
 * 初始化SHA256哈希计算上下文
 * 
 * @param ctx 指向SHA256_CTX结构的指针，用于存储哈希计算的中间状态
 * 
 * @note 必须在调用sha256_update()或sha256_final()之前调用此函数
 * @note 该函数会将上下文初始化为SHA256算法定义的初始哈希值
 * @warning 重复初始化会重置已计算的哈希状态
 */
void toolFunc::sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}
/**
 * 向SHA256哈希计算过程中添加数据
 * 
 * @param ctx 指向已初始化的SHA256_CTX结构的指针
 * @param data 指向要添加的数据的指针
 * @param len 要添加的数据的长度（以字节为单位）
 * 
 * @note 可以多次调用此函数以分块处理大数据
 * @note 数据会被内部缓冲并按64字节块进行处理
 * @warning 在调用sha256_final()之后不应再调用此函数，需重新初始化上下文
 */
void toolFunc::sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
	WORD i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}
/**
 * 完成SHA256哈希计算并生成最终哈希值
 * 
 * @param ctx 指向已完成数据添加的SHA256_CTX结构的指针
 * @param hash 用于存储最终32字节哈希值的缓冲区
 * 
 * @note 此函数会执行SHA256的填充算法和最终计算步骤
 * @note 生成的哈希值为二进制格式（非ASCII编码）
 * @warning 调用此函数后上下文状态会失效，如需继续计算需重新初始化
 */
void toolFunc::sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
	WORD i;

	i = ctx->datalen;

	// Pad whatever data is left in the buffer.
	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	// Append to the padding the total message's length in bits and transform.
	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	// Since this implementation uses little endian byte ordering and SHA uses big endian,
	// reverse all the bytes when copying the final state to the output hash.
	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}



/* Toggle the 16 bit unsigned integer pointed by *p from little endian to
 * big endian */
/**
 * 反转16位数据的字节序（大端<->小端转换）
 * 
 * @param p 指向16位数据的指针（按字节修改内存）
 * 
 * @note 直接修改内存中的数据，例如：0x1234 → 0x3412
 * @note 适用于需要手动处理字节序的场景（如网络协议解析）
 * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
 */
void toolFunc::memrev16(void *p) {
    unsigned char *x = static_cast<unsigned char*>(p), t;

    t = x[0];
    x[0] = x[1];
    x[1] = t;
}

/* Toggle the 32 bit unsigned integer pointed by *p from little endian to
 * big endian */
/**
 * 反转32位数据的字节序（大端<->小端转换）
 * 
 * @param p 指向32位数据的指针（按字节修改内存）
 * 
 * @note 直接修改内存中的数据，例如：0x12345678 → 0x78563412
 * @note 常用于网络字节序与主机字节序的转换
 * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
 */
void toolFunc::memrev32(void *p) {
    unsigned char *x = static_cast<unsigned char*>(p), t;

    t = x[0];
    x[0] = x[3];
    x[3] = t;
    t = x[1];
    x[1] = x[2];
    x[2] = t;
}

/* Toggle the 64 bit unsigned integer pointed by *p from little endian to
 * big endian */
/**
 * 反转64位数据的字节序（大端<->小端转换）
 * 
 * @param p 指向64位数据的指针（按字节修改内存）
 * 
 * @note 直接修改内存中的数据，例如：0x0123456789ABCDEF → 0xEFCDAB8967452301
 * @note 适用于处理64位整数的字节序转换（如某些加密算法）
 * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
 */
void toolFunc::memrev64(void *p) {
    unsigned char *x = static_cast<unsigned char*>(p), t;

    t = x[0];
    x[0] = x[7];
    x[7] = t;
    t = x[1];
    x[1] = x[6];
    x[6] = t;
    t = x[2];
    x[2] = x[5];
    x[5] = t;
    t = x[3];
    x[3] = x[4];
    x[4] = t;
}
/**
 * 返回16位整数反转字节序后的结果（非内存操作）
 * 
 * @param v 要处理的16位无符号整数
 * @return 字节序反转后的新值
 * 
 * @note 不修改原始值，例如：intrev16(0x1234) → 0x3412
 * @note 纯数值计算，不涉及内存访问，适合嵌入式系统优化
 */
uint16_t toolFunc::intrev16(uint16_t v) {
    memrev16(&v);
    return v;
}
/**
 * 返回32位整数反转字节序后的结果（非内存操作）
 * 
 * @param v 要处理的32位无符号整数
 * @return 字节序反转后的新值
 * 
 * @note 不修改原始值，例如：intrev32(0x12345678) → 0x78563412
 * @note 等效于htonl()/ntohl()，但不依赖系统库
 */
uint32_t toolFunc::intrev32(uint32_t v) {
    memrev32(&v);
    return v;
}
/**
 * 返回64位整数反转字节序后的结果（非内存操作）
 * 
 * @param v 要处理的64位无符号整数
 * @return 字节序反转后的新值
 * 
 * @note 不修改原始值，例如：intrev64(0x0123456789ABCDEF) → 0xEFCDAB8967452301
 * @note 适用于需要手动处理64位数据字节序的场景
 */
uint64_t toolFunc::intrev64(uint64_t v) {
    memrev64(&v);
    return v;
}

/**
 * SHA-1算法核心变换函数
 * 将64字节(512位)数据块进行压缩，更新当前哈希状态
 * 
 * @param state  指向5个32位哈希状态数组的指针 (A, B, C, D, E)
 * @param buffer 待处理的64字节数据块
 */
void toolFunc::SHA1Transform(uint32_t state[5], const unsigned char buffer[64])
{
    uint32_t a, b, c, d, e;
    typedef union {
        unsigned char c[64];
        uint32_t l[16];
    } CHAR64LONG16;
#ifdef SHA1HANDSOFF
    CHAR64LONG16 block[1];  /* use array to appear as a pointer */
    memcpy(block, buffer, 64);
#else
    /* The following had better never be used because it causes the
     * pointer-to-const buffer to be cast into a pointer to non-const.
     * And the result is written through.  I threw a "const" in, hoping
     * this will cause a diagnostic.
     */
    //CHAR64LONG16* block = (const CHAR64LONG16*)buffer;
    CHAR64LONG16 blockCopy;
    memcpy(&blockCopy, buffer, sizeof(CHAR64LONG16));
    CHAR64LONG16* block = &blockCopy;

// 后续处理代码保持不变
#endif
    /* Copy context->state[] to working vars */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    /* 4 rounds of 20 operations each. Loop unrolled. */
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    /* Add the working vars back into context.state[] */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    /* Wipe variables */
    a = b = c = d = e = 0;
#ifdef SHA1HANDSOFF
    memset(block, '\0', sizeof(block));
#endif
}

/**
 * 初始化SHA-1上下文
 * 重置内部状态，准备开始哈希计算
 * 
 * @param context 指向SHA1_CTX结构体的指针，存储计算状态
 */
void toolFunc::SHA1Init(SHA1_CTX* context)
{
    /* SHA1 initialization constants */
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}

/**
 * 向SHA-1计算过程添加数据
 * 可多次调用以处理分段数据
 * 
 * @param context 指向SHA1_CTX结构体的指针
 * @param data    待处理的数据缓冲区
 * @param len     数据长度(字节)
 */
void toolFunc::SHA1Update(SHA1_CTX* context, const unsigned char* data, uint32_t len)
{
    uint32_t i, j;

    j = context->count[0];
    if ((context->count[0] += len << 3) < j)
        context->count[1]++;
    context->count[1] += (len>>29);
    j = (j >> 3) & 63;
    if ((j + len) > 63) {
        memcpy(&context->buffer[j], data, (i = 64-j));
        SHA1Transform(context->state, context->buffer);
        for ( ; i + 63 < len; i += 64) {
            SHA1Transform(context->state, &data[i]);
        }
        j = 0;
    }
    else i = 0;
    memcpy(&context->buffer[j], &data[i], len - i);
}


/**
 * 完成SHA-1计算并生成最终哈希值
 * 调用后会重置上下文状态
 * 
 * @param digest  输出20字节(160位)哈希结果的缓冲区
 * @param context 指向已初始化并更新数据的SHA1_CTX结构体指针
 */
void toolFunc::SHA1Final(unsigned char digest[20], SHA1_CTX* context)
{
    unsigned i;
    unsigned char finalcount[8];
    unsigned char c;

#if 0	/* untested "improvement" by DHR */
    /* Convert context->count to a sequence of bytes
     * in finalcount.  Second element first, but
     * big-endian order within element.
     * But we do it all backwards.
     */
    unsigned char *fcp = &finalcount[8];

    for (i = 0; i < 2; i++)
       {
        uint32_t t = context->count[i];
        int j;

        for (j = 0; j < 4; t >>= 8, j++)
	          *--fcp = (unsigned char) t;
    }
#else
    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */
    }
#endif
    c = 0200;
    SHA1Update(context, &c, 1);
    while ((context->count[0] & 504) != 448) {
	c = 0000;
        SHA1Update(context, &c, 1);
    }
    SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform() */
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)
         ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
    /* Wipe variables */
    memset(context, '\0', sizeof(*context));
    memset(&finalcount, '\0', sizeof(finalcount));
}
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/21
 * All rights reserved. No one may copy or transfer.
 * Description: tool function implementation
 */

#ifndef REDIS_BASE_TOOLFUNC_H
#define REDIS_BASE_TOOLFUNC_H
#include "define.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//

#define MAX_LONG_DOUBLE_CHARS 5*1024

/* 
 * long double到字符串的转换模式选项
 */
typedef enum {
    LD_STR_AUTO,     /* 使用%.17Lg格式化，自动选择定点或科学计数法 */
    LD_STR_HUMAN,    /* 使用%.17Lf格式化，保留固定小数位并去除尾部多余的零 */
    LD_STR_HEX       /* 使用%La格式化，以十六进制浮点数表示 */
} ld2string_mode;


/****************************** MACROS ******************************/
/** SHA256算法输出的摘要长度（以字节为单位） */
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
/** 8位无符号字节类型定义 */
typedef uint8_t BYTE;   // 8-bit byte
/** 32位无符号字类型定义（用于内部处理） */
typedef uint32_t WORD;  // 32-bit word

/**
 * SHA256哈希计算的上下文结构
 * 包含算法执行过程中的中间状态和数据缓冲区
 */
typedef struct {
    BYTE data[64];      // 数据块缓冲区（SHA256按64字节块处理数据）
    WORD datalen;       // 当前数据块中已填充的字节数
    unsigned long long bitlen;  // 已处理数据的总位数（用于填充算法）
    WORD state[8];      // 哈希中间状态（8个32位字，对应SHA256的8个寄存器）
} SHA256_CTX;

typedef uint64_t (*crcfn64)(uint64_t, const void *, const uint64_t);
typedef uint16_t (*crcfn16)(uint16_t, const void *, const uint64_t);

class toolFunc;
#if (BYTE_ORDER == LITTLE_ENDIAN)
#define memrev16ifbe(p) ((void)(0))
#define memrev32ifbe(p) ((void)(0))
#define memrev64ifbe(p) ((void)(0))
#define intrev16ifbe(v) (v)
#define intrev32ifbe(v) (v)
#define intrev64ifbe(v) (v)
#else
#define memrev16ifbe(p) toolFunc::memrev16(p)
#define memrev32ifbe(p) toolFunc::memrev32(p)
#define memrev64ifbe(p) toolFunc::memrev64(p)
#define intrev16ifbe(v) toolFunc::intrev16(v)
#define intrev32ifbe(v) toolFunc::intrev32(v)
#define intrev64ifbe(v) toolFunc::intrev64(v)
#endif

/* The functions htonu64() and ntohu64() convert the specified value to
 * network byte ordering and back. In big endian systems they are no-ops. */
#if (BYTE_ORDER == BIG_ENDIAN)
#define htonu64(v) (v)
#define ntohu64(v) (v)
#else
#define htonu64(v) toolFunc::intrev64(v)
#define ntohu64(v) toolFunc::intrev64(v)
#endif

/**
 * SHA1算法的上下文结构
 * 用于存储SHA1计算过程中的中间状态和数据
 */
typedef struct {
    uint32_t state[5];    // SHA1算法的5个32位哈希状态变量 (A, B, C, D, E)
    uint32_t count[2];    // 记录已处理消息的比特数（高32位和低32位）
    unsigned char buffer[64];  // 用于缓存待处理的消息块（64字节=512位）
} SHA1_CTX;

/**
 * toolFunc工具类 - 提供通用字符串处理、数值转换和系统操作功能
 * 该类采用单例模式设计，禁止拷贝和移动操作
 */
class toolFunc
{
public:
    toolFunc();             /* 默认构造函数 */
    ~toolFunc();            /* 默认析构函数 */
    toolFunc(const toolFunc&) = delete;       /* 禁用拷贝构造函数 */
    toolFunc& operator=(const toolFunc&) = delete;  /* 禁用拷贝赋值运算符 */
    toolFunc(toolFunc&&) = delete;            /* 禁用移动构造函数 */

public:
    /**
     * 字符串模式匹配（带长度限制）
     * @param p 模式字符串
     * @param plen 模式长度
     * @param s 待匹配字符串
     * @param slen 待匹配字符串长度
     * @param nocase 是否忽略大小写(非零为忽略)
     * @return 匹配成功返回1，失败返回0
     */
    int stringmatchlen(const char *p, int plen, const char *s, int slen, int nocase);
    
    /**
     * 字符串模式匹配（完整长度）
     * @param p 模式字符串(以'\0'结尾)
     * @param s 待匹配字符串(以'\0'结尾)
     * @param nocase 是否忽略大小写(非零为忽略)
     * @return 匹配成功返回1，失败返回0
     */
    int stringmatch(const char *p, const char *s, int nocase);    
    
    /**
     * 将内存表示的数值字符串转换为long long类型
     * 支持后缀：k/K(千), m/M(兆), g/G(吉), t/T(太)
     * @param p 输入字符串
     * @param err 错误指示器(成功返回0，失败返回非零)
     * @return 转换后的数值
     */
    long long memtoll(const char *p, int *err);
    
    /**
     * 在指定内存区域查找任意字符
     * @param s 源内存区域
     * @param len 源内存长度
     * @param chars 待查找字符集
     * @param charslen 字符集长度
     * @return 找到的第一个匹配字符的指针，未找到返回NULL
     */
    const char *mempbrk(const char *s, size_t len, const char *chars, size_t charslen);
    
    /**
     * 字符映射替换
     * @param s 源字符串
     * @param len 字符串长度
     * @param from 源字符集
     * @param to 目标字符集
     * @param setlen 字符集长度
     * @return 处理后的字符串指针
     */
    char *memmapchars(char *s, size_t len, const char *from, const char *to, size_t setlen);
    
    /**
     * 计算无符号64位整数的十进制位数
     * @param v 输入值
     * @return 十进制位数
     */
    uint32_t digits10(uint64_t v);
    
    /**
     * 计算有符号64位整数的十进制位数
     * @param v 输入值
     * @return 十进制位数(包括符号位)
     */
    uint32_t sdigits10(int64_t v);
    
    /**
     * 将long long类型整数转换为字符串
     * @param s 输出缓冲区
     * @param len 缓冲区长度
     * @param value 输入值
     * @return 写入的字符数，失败返回-1
     */
    int ll2string(char *s, size_t len, long long value);
    
    /**
     * 将字符串转换为long long类型整数
     * @param s 输入字符串
     * @param slen 字符串长度
     * @param value 输出值
     * @return 成功返回1，失败返回0
     */
    int string2ll(const char *s, size_t slen, long long *value);
    
    /**
     * 将字符串转换为unsigned long long类型整数
     * @param s 输入字符串
     * @param value 输出值
     * @return 成功返回1，失败返回0
     */
    int string2ull(const char *s, unsigned long long *value);
    
    /**
     * 将字符串转换为long类型整数
     * @param s 输入字符串
     * @param slen 字符串长度
     * @param value 输出值
     * @return 成功返回1，失败返回0
     */
    int string2l(const char *s, size_t slen, long *value);
    
    /**
     * 将字符串转换为long double类型浮点数
     * @param s 输入字符串
     * @param slen 字符串长度
     * @param dp 输出值
     * @return 成功返回1，失败返回0
     */
    int string2ld(const char *s, size_t slen, long double *dp);
    
    /**
     * 将字符串转换为double类型浮点数
     * @param s 输入字符串
     * @param slen 字符串长度
     * @param dp 输出值
     * @return 成功返回1，失败返回0
     */
    int string2d(const char *s, size_t slen, double *dp);
    
    /**
     * 将double类型浮点数转换为字符串
     * @param buf 输出缓冲区
     * @param len 缓冲区长度
     * @param value 输入值
     * @return 写入的字符数，失败返回-1
     */
    int d2string(char *buf, size_t len, double value);
    
    /**
     * 将long double类型浮点数转换为字符串
     * @param buf 输出缓冲区
     * @param len 缓冲区长度
     * @param value 输入值
     * @param mode 转换模式(见ld2string_mode枚举)
     * @return 写入的字符数，失败返回-1
     */
    int ld2string(char *buf, size_t len, long double value, ld2string_mode mode);
    
    /**
     * 获取文件的绝对路径
     * @param filename 相对路径文件名
     * @return 绝对路径字符串指针，失败返回NULL
     * @note 返回的指针可能指向静态缓冲区，后续调用会覆盖
     */
    char* getAbsolutePath(char *filename);
    
    /**
     * 获取当前系统时区偏移(秒)
     * @return 时区偏移量，UTC时间返回0
     */
    long getTimeZone(void);
    
    /**
     * 判断路径是否为基础文件名(不包含目录)
     * @param path 路径字符串
     * @return 是返回1，否返回0
     */
    int pathIsBaseName(char *path);

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
    void getRandomBytes(unsigned char *p, size_t len);

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
    void getRandomHexChars(char *p, size_t len);

    /**
     * 初始化SHA256哈希计算上下文
     * 
     * @param ctx 指向SHA256_CTX结构的指针，用于存储哈希计算的中间状态
     * 
     * @note 必须在调用sha256_update()或sha256_final()之前调用此函数
     * @note 该函数会将上下文初始化为SHA256算法定义的初始哈希值
     * @warning 重复初始化会重置已计算的哈希状态
     */
    void sha256_init(SHA256_CTX *ctx);

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
    void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len);

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
    void sha256_final(SHA256_CTX *ctx, BYTE hash[]);


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
    void sha256_transform(SHA256_CTX *ctx, const BYTE data[]);

    /**
     * 反转16位数据的字节序（大端<->小端转换）
     * 
     * @param p 指向16位数据的指针（按字节修改内存）
     * 
     * @note 直接修改内存中的数据，例如：0x1234 → 0x3412
     * @note 适用于需要手动处理字节序的场景（如网络协议解析）
     * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
     */
    static void memrev16(void *p);

    /**
     * 反转32位数据的字节序（大端<->小端转换）
     * 
     * @param p 指向32位数据的指针（按字节修改内存）
     * 
     * @note 直接修改内存中的数据，例如：0x12345678 → 0x78563412
     * @note 常用于网络字节序与主机字节序的转换
     * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
     */
    static void memrev32(void *p);

    /**
     * 反转64位数据的字节序（大端<->小端转换）
     * 
     * @param p 指向64位数据的指针（按字节修改内存）
     * 
     * @note 直接修改内存中的数据，例如：0x0123456789ABCDEF → 0xEFCDAB8967452301
     * @note 适用于处理64位整数的字节序转换（如某些加密算法）
     * @warning 不会检查指针有效性，调用者需确保p指向合法内存区域
     */
    static void memrev64(void *p);

    /**
     * 返回16位整数反转字节序后的结果（非内存操作）
     * 
     * @param v 要处理的16位无符号整数
     * @return 字节序反转后的新值
     * 
     * @note 不修改原始值，例如：intrev16(0x1234) → 0x3412
     * @note 纯数值计算，不涉及内存访问，适合嵌入式系统优化
     */
    static uint16_t intrev16(uint16_t v);

    /**
     * 返回32位整数反转字节序后的结果（非内存操作）
     * 
     * @param v 要处理的32位无符号整数
     * @return 字节序反转后的新值
     * 
     * @note 不修改原始值，例如：intrev32(0x12345678) → 0x78563412
     * @note 等效于htonl()/ntohl()，但不依赖系统库
     */
    static uint32_t intrev32(uint32_t v);

    /**
     * 返回64位整数反转字节序后的结果（非内存操作）
     * 
     * @param v 要处理的64位无符号整数
     * @return 字节序反转后的新值
     * 
     * @note 不修改原始值，例如：intrev64(0x0123456789ABCDEF) → 0xEFCDAB8967452301
     * @note 适用于需要手动处理64位数据字节序的场景
     */
    static uint64_t intrev64(uint64_t v);

    /**
     * SHA-1算法核心变换函数
     * 将64字节(512位)数据块进行压缩，更新当前哈希状态
     * 
     * @param state  指向5个32位哈希状态数组的指针 (A, B, C, D, E)
     * @param buffer 待处理的64字节数据块
     */
    void SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);

    /**
     * 初始化SHA-1上下文
     * 重置内部状态，准备开始哈希计算
     * 
     * @param context 指向SHA1_CTX结构体的指针，存储计算状态
     */
    void SHA1Init(SHA1_CTX* context);

    /**
     * 向SHA-1计算过程添加数据
     * 可多次调用以处理分段数据
     * 
     * @param context 指向SHA1_CTX结构体的指针
     * @param data    待处理的数据缓冲区
     * @param len     数据长度(字节)
     */
    void SHA1Update(SHA1_CTX* context, const unsigned char* data, uint32_t len);

    /**
     * 完成SHA-1计算并生成最终哈希值
     * 调用后会重置上下文状态
     * 
     * @param digest  输出20字节(160位)哈希结果的缓冲区
     * @param context 指向已初始化并更新数据的SHA1_CTX结构体指针
     */
    void SHA1Final(unsigned char digest[20], SHA1_CTX* context);
    
public:
    /**
     * Mersenne Twister 64位随机数生成器核心函数
     * 基于松本真和西村拓士开发的MT19937-64算法实现
     * 特点：周期长度为2^19937-1，具有优秀的统计特性
     */

    /**
     * 使用单个种子值初始化随机数生成器
     * @param seed 用于初始化的64位无符号整数种子值
     * @note 相同的种子将产生相同的随机数序列，便于重现结果
     */
    void init_genrand64(unsigned long long seed);

    /**
     * 使用种子数组初始化随机数生成器（增强随机性）
     * @param init_key 用于初始化的64位无符号整数数组
     * @param key_length 种子数组的长度
     * @note 适用于需要更高熵值的场景，如密码学应用
     */
    void init_by_array64(unsigned long long init_key[],
                        unsigned long long key_length);

    /**
     * 生成64位无符号整数随机数
     * @return [0, 2^64-1]范围内的均匀分布随机整数
     * @note 此为核心生成函数，其他分布基于此实现
     */
    unsigned long long genrand64_int64(void);

    /**
     * 生成63位有符号整数随机数
     * @return [0, 2^63-1]范围内的均匀分布随机整数
     * @note 避免符号位影响，适用于需要正整数的场景
     */
    long long genrand64_int63(void);

    /**
     * 生成闭区间[0,1]上的双精度浮点数随机数
     * @return [0.0, 1.0]范围内的均匀分布随机浮点数（包含0和1）
     * @note 精度约为2^-53
     */
    double genrand64_real1(void);

    /**
     * 生成半开区间[0,1)上的双精度浮点数随机数
     * @return [0.0, 1.0)范围内的均匀分布随机浮点数（包含0但不包含1）
     * @note 最常用的浮点数分布，精度约为2^-53
     */
    double genrand64_real2(void);

    /**
     * 生成开区间(0,1)上的双精度浮点数随机数
     * @return (0.0, 1.0)范围内的均匀分布随机浮点数（不包含0和1）
     * @note 适用于需要非零值的概率场景
     */
    double genrand64_real3(void);

    /**
     * 生成半开区间(0,1]上的双精度浮点数随机数
     * @return (0.0, 1.0]范围内的均匀分布随机浮点数（不包含0但包含1）
     * @note 特定场景使用，精度约为2^-53
     */
    double genrand64_real4(void);



public:
    unsigned int lzf_compress (const void *const in_data,  unsigned int in_len, void *out_data, unsigned int out_len);
    unsigned int lzf_decompress (const void *const in_data,  unsigned int in_len,void *out_data, unsigned int out_len);
 
    /**
     * CRC64 算法实现 - 提供64位循环冗余校验功能
     * 支持标准CRC64计算及优化的查表加速算法
     * 多项式定义: POLY = 0xad93d23594c935a9
     */

    /**
     * 初始化CRC64计算所需的查找表
     * 必须在调用crc64()之前调用此函数进行初始化
     */
    void crc64_init(void);

    /**
     * 计算数据的CRC64校验值
     * 
     * @param crc 初始CRC值（通常为0或预计算的种子值）
     * @param s   待计算CRC的输入数据缓冲区
     * @param l   输入数据的长度（字节）
     * @return    计算得到的CRC64校验值
     */
    uint64_t crc64(uint64_t crc, const unsigned char *s, uint64_t l);

    /**
     * 初始化小端序优化的CRC64查找表
     * 
     * @param fn    CRC计算函数指针
     * @param table 用于存储8x256项的CRC64查找表数组
     */
    void crcspeed64little_init(crcfn64 fn, uint64_t table[8][256]);

    /**
     * 初始化大端序优化的CRC64查找表
     * 
     * @param fn    CRC计算函数指针
     * @param table 用于存储8x256项的CRC64查找表数组
     */
    void crcspeed64big_init(crcfn64 fn, uint64_t table[8][256]);

    /**
     * 初始化本地字节序优化的CRC64查找表
     * 
     * @param fn    CRC计算函数指针
     * @param table 用于存储8x256项的CRC64查找表数组
     */
    void crcspeed64native_init(crcfn64 fn, uint64_t table[8][256]);

    /**
     * 使用小端序优化表计算CRC64（每次处理8字节）
     * 
     * @param table 预计算的8x256项查找表
     * @param crc   初始CRC值
     * @param buf   输入数据缓冲区
     * @param len   数据长度（字节）
     * @return      计算得到的CRC64值
     */
    uint64_t crcspeed64little(uint64_t table[8][256], uint64_t crc, void *buf, size_t len);

    /**
     * 使用大端序优化表计算CRC64（每次处理8字节）
     * 
     * @param table 预计算的8x256项查找表
     * @param crc   初始CRC值
     * @param buf   输入数据缓冲区
     * @param len   数据长度（字节）
     * @return      计算得到的CRC64值
     */
    uint64_t crcspeed64big(uint64_t table[8][256], uint64_t crc, void *buf, size_t len);

    /**
     * 使用本地字节序优化表计算CRC64（每次处理8字节）
     * 
     * @param table 预计算的8x256项查找表
     * @param crc   初始CRC值
     * @param buf   输入数据缓冲区
     * @param len   数据长度（字节）
     * @return      计算得到的CRC64值
     */
    uint64_t crcspeed64native(uint64_t table[8][256], uint64_t crc, void *buf, size_t len);

    /**
     * 内部使用的CRC64计算函数
     * 
     * @param crc      初始CRC值
     * @param in_data  输入数据缓冲区
     * @param len      数据长度（字节）
     * @return         计算得到的CRC64值
     */
    static uint64_t _crc64(uint_fast64_t crc, const void *in_data, const uint64_t len);

    /**
     * 按位反射数据（用于CRC计算中的位序调整）
     * 
     * @param data     待反射的数据
     * @param data_len 数据长度（位）
     * @return         反射后的数据
     */
    static uint_fast64_t crc_reflect(uint_fast64_t data, size_t data_len);

    /**
     * 反转8字节数据的字节序
     * 
     * @param a 输入的64位数据
     * @return  字节序反转后的64位数据
     */
    uint64_t rev8(uint64_t a);
};

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
 #endif
#ifndef REDIS_BASE_DEFINE_H
#define REDIS_BASE_DEFINE_H

//================================全局=========================//
#define BEGIN_NAMESPACE(name) namespace name {
#define END_NAMESPACE(name) } // namespace name

//================================adlist=========================//
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)
#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))
#define listGetDupMethod(l) ((l)->dup)
#define listGetFreeMethod(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1



//================================dict=========================//
#define DICT_OK 0
#define DICT_ERR 1
#define DICT_NOTUSED(V) ((void) V)

/* This is the initial size of every hash table */
#define DICT_HT_INITIAL_SIZE     4

/* 哈希表初始大小 - 必须为2的幂 */
#define DICT_HT_INITIAL_SIZE     4

#define dictFreeVal(d, entry) \
    if ((d)->type->valDestructor) \
        (d)->type->valDestructor((d)->privdata, (entry)->v.val)

#define dictSetVal(d, entry, _val_) do { \
    if ((d)->type->valDup) \
        (entry)->v.val = (d)->type->valDup((d)->privdata, _val_); \
    else \
        (entry)->v.val = (_val_); \
} while(0)

#define dictSetSignedIntegerVal(entry, _val_) \
    do { (entry)->v.s64 = _val_; } while(0)
#define dictSetUnsignedIntegerVal(entry, _val_) \
    do { (entry)->v.u64 = _val_; } while(0)

#define dictSetDoubleVal(entry, _val_) \
    do { (entry)->v.d = _val_; } while(0)
#define dictFreeKey(d, entry) \
    if ((d)->type->keyDestructor) \
        (d)->type->keyDestructor((d)->privdata, (entry)->key)
#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)
#define dictCompareKeys(d, key1, key2) \
    (((d)->type->keyCompare) ? \
        (d)->type->keyCompare((d)->privdata, key1, key2) : \
        (key1) == (key2))
#define dictHashKey(d, key) (d)->type->hashFunction(key)
#define dictGetKey(he) ((he)->key)
#define dictGetVal(he) ((he)->v.val)
#define dictGetSignedIntegerVal(he) ((he)->v.s64)
#define dictGetUnsignedIntegerVal(he) ((he)->v.u64)
#define dictGetDoubleVal(he) ((he)->v.d)
#define dictSlots(d) ((d)->ht[0].size+(d)->ht[1].size)
#define dictSize(d) ((d)->ht[0].used+(d)->ht[1].used)
#define dictIsRehashing(d) ((d)->rehashidx != -1)
#define dictPauseRehashing(d) (d)->pauserehash++
#define dictResumeRehashing(d) (d)->pauserehash--

#define DICT_STATS_VECTLEN 50
/* Test of the CPU is Little Endian and supports not aligned accesses.
 * Two interesting conditions to speedup the function that happen to be
 * in most of x86 servers. */
#if defined(__X86_64__) || defined(__x86_64__) || defined (__i386__) \
	|| defined (__aarch64__) || defined (__arm64__)
#define UNALIGNED_LE_CPU
#endif

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)                                                        \
    (p)[0] = (uint8_t)((v));                                                   \
    (p)[1] = (uint8_t)((v) >> 8);                                              \
    (p)[2] = (uint8_t)((v) >> 16);                                             \
    (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)                                                        \
    U32TO8_LE((p), (uint32_t)((v)));                                           \
    U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#ifdef UNALIGNED_LE_CPU
#define U8TO64_LE(p) (*((uint64_t*)(p)))
#else
#define U8TO64_LE(p)                                                           \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |                        \
     ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) |                 \
     ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |                 \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))
#endif

#define U8TO64_LE_NOCASE(p)                                                    \
    (((uint64_t)(siptlw((p)[0]))) |                                           \
     ((uint64_t)(siptlw((p)[1])) << 8) |                                      \
     ((uint64_t)(siptlw((p)[2])) << 16) |                                     \
     ((uint64_t)(siptlw((p)[3])) << 24) |                                     \
     ((uint64_t)(siptlw((p)[4])) << 32) |                                              \
     ((uint64_t)(siptlw((p)[5])) << 40) |                                              \
     ((uint64_t)(siptlw((p)[6])) << 48) |                                              \
     ((uint64_t)(siptlw((p)[7])) << 56))

#define SIPROUND                                                               \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 32);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 16);                                                     \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 21);                                                     \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 17);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 32);                                                     \
    } while (0)





//================================intset======================================//
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))





//================================randomNumGenerator=========================//
#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */




//================================redisObject==============================//
#define OBJ_ENCODING_RAW 0     /* Raw representation */
#define OBJ_ENCODING_INT 1     /* Encoded as integer */
#define OBJ_ENCODING_HT 2      /* Encoded as hash table */
#define OBJ_ENCODING_ZIPMAP 3  /* Encoded as zipmap */
#define OBJ_ENCODING_LINKEDLIST 4 /* No longer used: old list encoding. */
#define OBJ_ENCODING_ZIPLIST 5 /* Encoded as ziplist */
#define OBJ_ENCODING_INTSET 6  /* Encoded as intset */
#define OBJ_ENCODING_SKIPLIST 7  /* Encoded as skiplist */
#define OBJ_ENCODING_EMBSTR 8  /* Embedded sds string encoding */
#define OBJ_ENCODING_QUICKLIST 9 /* Encoded as linked list of ziplists */
#define OBJ_ENCODING_STREAM 10 /* Encoded as a radix tree of listpacks */
#define LRU_BITS 24
#define LRU_CLOCK_MAX ((1<<LRU_BITS)-1) /* Max value of obj->lru */
#define LRU_CLOCK_RESOLUTION 1000 /* LRU clock resolution in ms */
#define OBJ_SHARED_REFCOUNT INT_MAX     /* Global object never destroyed. */
#define OBJ_STATIC_REFCOUNT (INT_MAX-1) /* Object allocated in the stack. */
#define OBJ_FIRST_SPECIAL_REFCOUNT OBJ_STATIC_REFCOUNT
/* Redis maxmemory strategies. Instead of using just incremental number
 * for this defines, we use a set of flags so that testing for certain
 * properties common to multiple policies is faster. */
#define MAXMEMORY_FLAG_LRU (1<<0)
#define MAXMEMORY_FLAG_LFU (1<<1)
#define MAXMEMORY_FLAG_ALLKEYS (1<<2)
#define MAXMEMORY_FLAG_NO_SHARED_INTEGERS \
    (MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_LFU)
#define MAXMEMORY_VOLATILE_LRU ((0<<8)|MAXMEMORY_FLAG_LRU)
#define MAXMEMORY_VOLATILE_LFU ((1<<8)|MAXMEMORY_FLAG_LFU)
#define MAXMEMORY_VOLATILE_TTL (2<<8)
#define MAXMEMORY_VOLATILE_RANDOM (3<<8)
#define MAXMEMORY_ALLKEYS_LRU ((4<<8)|MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_LFU ((5<<8)|MAXMEMORY_FLAG_LFU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_RANDOM ((6<<8)|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_NO_EVICTION (7<<8)
#define PROTO_SHARED_SELECT_CMDS 10
#define OBJ_SHARED_INTEGERS 10000
#define OBJ_SHARED_BULKHDR_LEN 32
#define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
#define REDIS_COMPARE_BINARY (1<<0)
#define REDIS_COMPARE_COLL (1<<1)
#define OBJ_COMPUTE_SIZE_DEF_SAMPLES 5 /* Default sample size. */
#define CLIENT_TYPE_NORMAL 0 /* Normal req-reply clients + MONITORs */
#define CLIENT_TYPE_SLAVE 1  /* Slaves. */
#define CLIENT_TYPE_PUBSUB 2 /* Clients subscribed to PubSub channels. */
#define CLIENT_TYPE_MASTER 3 /* Master. */
#define CLIENT_TYPE_COUNT 4  /* Total number of client types. */
#define CLIENT_TYPE_OBUF_COUNT 3 /* Number of clients to expose to output
                                    buffer configuration. Just the first
                                    three: normal, slave, pubsub. */
#define sdsEncodedObject(objptr) (objptr->encoding == OBJ_ENCODING_RAW || objptr->encoding == OBJ_ENCODING_EMBSTR)

//================================sds=========================//
#define SDS_TYPE_5  0
#define SDS_TYPE_8  1
#define SDS_TYPE_16 2
#define SDS_TYPE_32 3
#define SDS_TYPE_64 4
#define SDS_TYPE_MASK 7
#define SDS_TYPE_BITS 3
#define SDS_MAX_PREALLOC (1024*1024)

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
typedef sds (*sdstemplate_callback_t)(const sds variable, void *arg);
#define SDS_LLSTR_SIZE 21




//================================ziplist=========================//
/**
 * ZIPLIST_HEAD 和 ZIPLIST_TAIL 是 ziplistPush 函数的方向参数
 * 分别表示在列表头部插入和在列表尾部插入
 */
#define ZIPLIST_HEAD 0
#define ZIPLIST_TAIL 1
/* 压缩列表的特殊标记和结构定义 */

#define ZIP_END 255         /* 特殊"压缩列表结束"标记，值为0xFF */
#define ZIP_BIG_PREVLEN 254 /* 当前一个条目的长度小于254字节时，"prevlen"字段用1字节表示；
                             * 否则用5字节表示：首字节为0xFE，后4字节为无符号整数表示长度 */

/* 不同数据类型的编码和长度表示 */
#define ZIP_STR_MASK 0xc0   /* 用于判断字符串编码的掩码（高2位不为11） */
#define ZIP_INT_MASK 0x30   /* 用于提取整数编码类型的掩码 */
#define ZIP_STR_06B (0 << 6) /* 6位字符串编码（长度最大63字节） */
#define ZIP_STR_14B (1 << 6) /* 14位字符串编码（长度最大16383字节） */
#define ZIP_STR_32B (2 << 6) /* 32位字符串编码（长度最大4GB） */
#define ZIP_INT_16B (0xc0 | 0<<4) /* 16位整数编码 */
#define ZIP_INT_32B (0xc0 | 1<<4) /* 32位整数编码 */
#define ZIP_INT_64B (0xc0 | 2<<4) /* 64位整数编码 */
#define ZIP_INT_24B (0xc0 | 3<<4) /* 24位整数编码 */
#define ZIP_INT_8B 0xfe          /* 8位整数编码 */

/* 4位立即数整数编码，格式为|1111xxxx|，xxxx范围0001-1101 */
#define ZIP_INT_IMM_MASK 0x0f   /* 提取4位立即数的掩码，需加1还原实际值 */
#define ZIP_INT_IMM_MIN 0xf1    /* 最小4位立即数编码值（0001） */
#define ZIP_INT_IMM_MAX 0xfd    /* 最大4位立即数编码值（1101） */

/* 24位整数的范围定义 */
#define INT24_MAX 0x7fffff      /* 24位整数最大值 */
#define INT24_MIN (-INT24_MAX - 1) /* 24位整数最小值 */

/* 判断是否为字符串条目的宏：字符串编码首字节高2位不为11 */
#define ZIP_IS_STR(enc) (((enc) & ZIP_STR_MASK) < ZIP_STR_MASK)

/* 压缩列表头部和结构操作宏 */

/* 获取压缩列表的总字节数 */
#define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))

/* 获取压缩列表最后一个条目的偏移量 */
#define ZIPLIST_TAIL_OFFSET(zl) (*((uint32_t*)((zl)+sizeof(uint32_t))))

/* 获取压缩列表的条目数量（最多65535，超过时需遍历整个列表） */
#define ZIPLIST_LENGTH(zl)      (*((uint16_t*)((zl)+sizeof(uint32_t)*2)))

/* 压缩列表头部大小：两个32位整数（总字节数和尾部偏移）+ 一个16位整数（条目数量） */
#define ZIPLIST_HEADER_SIZE     (sizeof(uint32_t)*2+sizeof(uint16_t))

/* 压缩列表结束标记的大小（1字节） */
#define ZIPLIST_END_SIZE        (sizeof(uint8_t))

/* 获取压缩列表第一个数据条目的指针 */
#define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)

/* 获取压缩列表最后一个数据条目的指针 */
#define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)))

/* 获取压缩列表结束标记的指针（0xFF） */
#define ZIPLIST_ENTRY_END(zl)   ((zl)+intrev32ifbe(ZIPLIST_BYTES(zl))-1)

/* 增加压缩列表的条目计数。当达到65535时不再增加，需遍历列表获取实际数量 */
#define ZIPLIST_INCR_LENGTH(zl,incr) { \
    if (intrev16ifbe(ZIPLIST_LENGTH(zl)) < UINT16_MAX) \
        ZIPLIST_LENGTH(zl) = intrev16ifbe(intrev16ifbe(ZIPLIST_LENGTH(zl))+incr); \
}




//================================zset=========================//
#define C_OK                    0
#define C_ERR                   -1
/* Input flags. */
#define ZADD_IN_NONE 0
#define ZADD_IN_INCR (1<<0)    /* Increment the score instead of setting it. */
#define ZADD_IN_NX (1<<1)      /* Don't touch elements not already existing. */
#define ZADD_IN_XX (1<<2)      /* Only touch elements already existing. */
#define ZADD_IN_GT (1<<3)      /* Only update existing when new scores are higher. */
#define ZADD_IN_LT (1<<4)      /* Only update existing when new scores are lower. */

/* Output flags. */
#define ZADD_OUT_NOP (1<<0)     /* Operation not performed because of conditionals.*/
#define ZADD_OUT_NAN (1<<1)     /* Only touch elements already existing. */
#define ZADD_OUT_ADDED (1<<2)   /* The element was new and was added. */
#define ZADD_OUT_UPDATED (1<<3) /* The element already existed, score updated. */

/* Hash table parameters */
#define HASHTABLE_MIN_FILL        10      /* Minimal hash table fill 10% */
#define HASHTABLE_MAX_LOAD_FACTOR 1.618   /* Maximum hash table load factor. */

/* The actual Redis Object */
#define OBJ_STRING 0    /* String object. */
#define OBJ_LIST 1      /* List object. */
#define OBJ_SET 2       /* Set object. */
#define OBJ_ZSET 3      /* Sorted set object. */
#define OBJ_HASH 4      /* Hash object. */
#define OBJ_MODULE 5    /* Module object. */
#define OBJ_STREAM 6    /* Stream object. */


//================================zskiplist=========================//
#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/4 */


//================================quicklist=========================//
#define QUICKLIST_HEAD 0
#define QUICKLIST_TAIL -1

/* quicklist node encodings */
#define QUICKLIST_NODE_ENCODING_RAW 1
#define QUICKLIST_NODE_ENCODING_LZF 2

/* quicklist compression disable */
#define QUICKLIST_NOCOMPRESS 0

/* quicklist container formats */
#define QUICKLIST_NODE_CONTAINER_NONE 1
#define QUICKLIST_NODE_CONTAINER_ZIPLIST 2

#define quicklistNodeIsCompressed(node)                                        \
    ((node)->encoding == QUICKLIST_NODE_ENCODING_LZF)





//================================toolFunc=========================//    

#define STANDALONE 1 /* at the moment, this is ok. */

#ifndef STANDALONE
# include "lzf.h"
#endif

#ifndef HLOG
# define HLOG 16
#endif

#ifndef VERY_FAST
# define VERY_FAST 1
#endif

#ifndef ULTRA_FAST
# define ULTRA_FAST 0
#endif

#ifndef STRICT_ALIGN
# if !(defined(__i386) || defined (__amd64))
#  define STRICT_ALIGN 1
# else
#  define STRICT_ALIGN 0
# endif
#endif

#ifndef INIT_HTAB
# define INIT_HTAB 0
#endif

#ifndef AVOID_ERRNO
# define AVOID_ERRNO 0
#endif

#ifndef LZF_STATE_ARG
# define LZF_STATE_ARG 0
#endif

#ifndef CHECK_INPUT
# define CHECK_INPUT 1
#endif

#ifdef __cplusplus
# include <cstring>
# include <climits>
using namespace std;
#else
# include <string.h>
# include <limits.h>
#endif

#ifndef LZF_USE_OFFSETS
# if defined (WIN32)
#  define LZF_USE_OFFSETS defined(_M_X64)
# else
#  if __cplusplus > 199711L
#   include <cstdint>
#  else
#   include <stdint.h>
#  endif
#  define LZF_USE_OFFSETS (UINTPTR_MAX > 0xffffffffU)
# endif
#endif

typedef unsigned char u8;

#if LZF_USE_OFFSETS
# define LZF_HSLOT_BIAS ((const u8 *)in_data)
  typedef unsigned int LZF_HSLOT;
#else
# define LZF_HSLOT_BIAS 0
  typedef const u8 *LZF_HSLOT;
#endif

typedef LZF_HSLOT LZF_STATE[1 << (HLOG)];

#if !STRICT_ALIGN
/* for unaligned accesses we need a 16 bit datatype. */
# if USHRT_MAX == 65535
    typedef unsigned short u16;
# elif UINT_MAX == 65535
    typedef unsigned int u16;
# else
#  undef STRICT_ALIGN
#  define STRICT_ALIGN 1
# endif
#endif

#if ULTRA_FAST
# undef VERY_FAST
#endif


#define HSIZE (1 << (HLOG))

#ifndef FRST
# define FRST(p) (((p[0]) << 8) | p[1])
# define NEXT(v,p) (((v) << 8) | p[2])
# if ULTRA_FAST
#  define IDX(h) ((( h             >> (3*8 - HLOG)) - h  ) & (HSIZE - 1))
# elif VERY_FAST
#  define IDX(h) ((( h             >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
# else
#  define IDX(h) ((((h ^ (h << 5)) >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
# endif
#endif

#define        MAX_LIT        (1 <<  5)
#define        MAX_OFF        (1 << 13)
#define        MAX_REF        ((1 << 8) + (1 << 3))

#if __GNUC__ >= 3
# define expect(expr,value)         __builtin_expect ((expr),(value))
# define inline                     inline
#else
# define expect(expr,value)         (expr)
# define inline                     static
#endif

#define expect_false(expr) expect ((expr) != 0, 0)
#define expect_true(expr)  expect ((expr) != 0, 1)


#if AVOID_ERRNO
# define SET_ERRNO(n)
#else
# include <errno.h>
# define SET_ERRNO(n) errno = (n)
#endif

#if USE_REP_MOVSB /* small win on amd, big loss on intel */
#if (__i386 || __amd64) && __GNUC__ >= 3
# define lzf_movsb(dst, src, len)                \
   asm ("rep movsb"                              \
        : "=D" (dst), "=S" (src), "=c" (len)     \
        :  "0" (dst),  "1" (src),  "2" (len));
#endif
#endif

#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif



#define LP_INTBUF_SIZE 21 
#define LP_BEFORE 0
#define LP_AFTER 1
#define LP_REPLACE 2
#define LP_HDR_SIZE 6       /* 32 bit total len + 16 bit number of elements. */
#define LP_HDR_NUMELE_UNKNOWN UINT16_MAX
#define LP_MAX_INT_ENCODING_LEN 9
#define LP_MAX_BACKLEN_SIZE 5
#define LP_MAX_ENTRY_BACKLEN 34359738367ULL
#define LP_ENCODING_INT 0
#define LP_ENCODING_STRING 1

typedef long long mstime_t; /* millisecond time type. */
typedef long long ustime_t; /* microsecond time type. */
#endif
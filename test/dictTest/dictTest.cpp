/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: adlist test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "dict.h"
#include "zmalloc.h"
#include "fmacros.h"
int __failed_tests = 0;
int __test_num = 0;
#define test_cond(descr,_c) do { \
    __test_num++; printf("%d - %s: ", __test_num, descr); \
    if(_c) printf("PASSED\n"); else {printf("FAILED\n"); __failed_tests++;} \
} while(0)

#define test_report() do { \
    printf("%d tests, %d passed, %d failed\n", __test_num, \
                    __test_num-__failed_tests, __failed_tests); \
    if (__failed_tests) { \
        printf("=== WARNING === We have failed tests here...\n"); \
        exit(1); \
    } \
} while(0)
// 基础内存分配宏（内部使用）
#define __ZMALLOC_BASE(func, ...) \
    ({ \
        void* __restrict __zm_ptr = nullptr; \
        __zm_ptr = zmalloc::getInstance()->func(__VA_ARGS__); \
        __zm_ptr; \
    })
// 内存分配宏
#define zmalloc(a) __ZMALLOC_BASE(zzmalloc, (a))
#define zrealloc(p, a) __ZMALLOC_BASE(zrealloc, (p), (a))
#define ztrymalloc(a) __ZMALLOC_BASE(ztrymalloc, (a))
#define ztryrealloc(p, a) __ZMALLOC_BASE(ztryrealloc, (p), (a))
#define zcalloc(a) __ZMALLOC_BASE(zcalloc, (a))
#define ztrycalloc(a) __ZMALLOC_BASE(ztrycalloc, (a))

// 带usable参数的版本
#define zmalloc_usable(a, u) __ZMALLOC_BASE(zmalloc_usable, (a), (u))
#define zrealloc_usable(p, a, u) __ZMALLOC_BASE(zrealloc_usable, (p), (a), (u))
#define ztrymalloc_usable(a, u) __ZMALLOC_BASE(ztrymalloc_usable, (a), (u))
#define ztryrealloc_usable(p, a, u) __ZMALLOC_BASE(ztryrealloc_usable, (p), (a), (u))

// 特殊处理free函数（不返回指针）
#define zfree(p) \
    do { \
        if (p) { \
            zmalloc::getInstance()->zfree(p); \
            p = nullptr; \
        } \
    } while(0)

// 带usable参数的free
#define zfree_usable(p, u) \
    do { \
        if (p) { \
            zmalloc::getInstance()->zfree_usable(p, u); \
            p = nullptr; \
        } \
    } while(0)

static dictionaryCreate dictCrt;

uint64_t hashCallback(const void *key) {
    return dictCrt.dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

int compareCallback(void *privdata, const void *key1, const void *key2) {
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

void freeCallback(void *privdata, void *val) {
    DICT_NOTUSED(privdata);

    zfree(val);
}

char *stringFromLongLong(long long value) {
    char buf[32];
    int len;
    char *s;

    len = sprintf(buf,"%lld",value);
    s = static_cast<char*>(zmalloc(len+1));
    memcpy(s, buf, len);
    s[len] = '\0';
    return s;
}

dictType BenchmarkDictType = {
    hashCallback,
    NULL,
    NULL,
    compareCallback,
    freeCallback,
    NULL,
    NULL
};

#define start_benchmark() start = dictCrt.timeInMilliseconds()
#define end_benchmark(msg) do { \
    elapsed = dictCrt.timeInMilliseconds()-start; \
    printf(msg ": %ld items in %lld ms\n", count, elapsed); \
} while(0)



// 辅助函数
void dummyCallback(void* data) {}
void dummyScanFunction(void *privdata, const dictEntry *de) {}
void dummyScanBucketFunction(void *privdata, dictEntry **bucket) {}
int main(int argc,char* argv[])
{
    long j;
    long long start, elapsed;
    dict *dict = dictCrt.dictCreate(&BenchmarkDictType,NULL);
     // 测试字典创建
    test_cond("Dictionary creation", dict != nullptr);

    //对于count = 8,rehashidx = -1,无需进行dictRehashMilliseconds
    long count = 8000;
    start_benchmark();
    for (j = 0; j < count; j++) 
    {
        int retval = dictCrt.dictAdd(dict,stringFromLongLong(j),(void*)j);
        assert(retval == DICT_OK);
    }
    end_benchmark("Inserting");

    //内存结构如下
    // $1 = {type = 0x555555559020 <BenchmarkDictType>, privdata = 0x0, ht = {{table = 0x503000000040, size = 4, sizemask = 3, used = 4}, {
    //     table = 0x506000000020, size = 8, sizemask = 7, used = 1}}, rehashidx = 0, pauserehash = 0}
    // (gdb) p $1.ht[0].table[0]
    // $2 = (dictEntry *) 0x503000000100
    // (gdb) p ($1.ht[0].table[0])
    // $3 = (dictEntry *) 0x503000000100
    // (gdb) p *($1.ht[0].table[0])
    // $4 = {key = 0x502000000090, v = {val = 0x3, u64 = 3, s64 = 3, d = 1.4821969375237396e-323}, next = 0x0}
    // (gdb) p *($1.ht[0].table[1])
    // Cannot access memory at address 0x0
    // (gdb) p *($1.ht[0].table[2])
    // $5 = {key = 0x502000000070, v = {val = 0x2, u64 = 2, s64 = 2, d = 9.8813129168249309e-324}, next = 0x5030000000a0}
    // (gdb) p *($1.ht[0].table[3])
    // $6 = {key = 0x502000000030, v = {val = 0x0, u64 = 0, s64 = 0, d = 0}, next = 0x0}
    // (gdb) p *($1.ht[0].table[4])
    // $7 = {key = 0x0, v = {val = 0x0, u64 = 0, s64 = 0, d = 0}, next = 0x0}
    // (gdb) p *($1.ht[0].table[5])
    // Cannot access memory at address 0xe
    // (gdb) p *($1.ht[0].table[6])
    // $8 = {key = 0x30, v = {val = 0x0, u64 = 0, s64 = 0, d = 0}, next = 0x200001102}
    // (gdb) p *($1.ht[0].table[7])
    // Cannot access memory at address 0x0
    // (gdb) p *($1.ht[1].table[6])
    // $9 = {key = 0x5020000000b0, v = {val = 0x4, u64 = 4, s64 = 4, d = 1.9762625833649862e-323}, next = 0x0}


    // assert((long)dictSize(dict) == count);

    //     $1 = {type = 0x555555559020 <BenchmarkDictType>, privdata = 0x0, ht = {{table = 0x503000000040, size = 4, sizemask = 3, used = 4}, {
    //       table = 0x506000000020, size = 8, sizemask = 7, used = 1}}, rehashidx = 0, pauserehash = 0}
    // (gdb) n
    // 171         while (dictIsRehashing(dict)) {
    // (gdb) p *dict
    // $2 = {type = 0x555555559020 <BenchmarkDictType>, privdata = 0x0, ht = {{table = 0x506000000020, size = 8, sizemask = 7, used = 5}, {
    //       table = 0x0, size = 0, sizemask = 0, used = 0}}, rehashidx = -1, pauserehash = 0}
    /* Wait for rehashing. */
    while (dictIsRehashing(dict)) {
        dictCrt.dictRehashMilliseconds(dict,100);
    }

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        dictEntry *de = dictCrt.dictFind(dict,key);
        assert(de != NULL);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        void *fetchedValue = dictCrt.dictFetchValue(dict, key);
        //printf("Fetched value for key %s: %p\n", key, fetchedValue);
        zfree(key);
    }
    end_benchmark("Linear access of existing elements (2nd round)");


    start_benchmark();
    for (j = 0; j < count; j++) {
        dictEntry *de = dictCrt.dictGetRandomKey(dict);
        assert(de != NULL);
    }
    end_benchmark("Accessing random keys");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(rand() % count);
        key[0] = 'X';
        dictEntry *de = dictCrt.dictFind(dict,key);
        assert(de == NULL);
        zfree(key);
    }
    end_benchmark("Accessing missing");

    start_benchmark();
    for (j = 0; j < count; j++) {
        char *key = stringFromLongLong(j);
        int retval = dictCrt.dictDelete(dict,key);
        assert(retval == DICT_OK);
        key[0] += 17; /* Change first number to letter. */
        retval = dictCrt.dictAdd(dict,key,(void*)j);
        assert(retval == DICT_OK);
    }
    end_benchmark("Removing and adding");
    
    dictCrt.dictRelease(dict);


    return 0; 
}



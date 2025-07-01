/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/12
 * Description: ziplistTest test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include "zmallocDf.h"
#include "sds.h"
#include "ziplist.h"
#include "toolFunc.h"
#include "define.h"
using namespace REDIS_BASE;

// 定义测试宏
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
ziplistCreate ziplistCrt;
void ziplistRepr(unsigned char *zl) {
    unsigned char *p;
    int index = 0;
    zlentry entry;
    size_t zlbytes = ziplistCrt.ziplistBlobLen(zl);

    printf(
        "{total bytes %u} "
        "{num entries %u}\n"
        "{tail offset %u}\n",
        intrev32ifbe(ZIPLIST_BYTES(zl)),
        intrev16ifbe(ZIPLIST_LENGTH(zl)),
        intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)));
    p = ZIPLIST_ENTRY_HEAD(zl);
    while(*p != ZIP_END) {
        assert(ziplistCrt.zipEntrySafe(zl, zlbytes, p, &entry, 1));
        printf(
            "{\n"
                "\taddr 0x%08lx,\n"
                "\tindex %2d,\n"
                "\toffset %5lu,\n"
                "\thdr+entry len: %5u,\n"
                "\thdr len%2u,\n"
                "\tprevrawlen: %5u,\n"
                "\tprevrawlensize: %2u,\n"
                "\tpayload %5u\n",
            (long unsigned)p,
            index,
            (unsigned long) (p-zl),
            entry.headersize+entry.len,
            entry.headersize,
            entry.prevrawlen,
            entry.prevrawlensize,
            entry.len);
        printf("\tbytes: ");
        for (unsigned int i = 0; i < entry.headersize+entry.len; i++) {
            printf("%02x|",p[i]);
        }
        printf("\n");
        p += entry.headersize;
        if (ZIP_IS_STR(entry.encoding)) {
            printf("\t[str]");
            if (entry.len > 40) {
                if (fwrite(p,40,1,stdout) == 0) perror("fwrite");
                printf("...");
            } else {
                if (entry.len &&
                    fwrite(p,entry.len,1,stdout) == 0) perror("fwrite");
            }
        } else {
            printf("\t[int]%lld", (long long) ziplistCrt.zipLoadInteger(p,entry.encoding));
        }
        printf("\n}\n");
        p += entry.len;
        index++;
    }
    printf("{end}\n\n");
}



int main(int argc, char **argv)
{
    unsigned char *entry, *p;
    unsigned int elen;
    long long value;
    
    printf("=== Starting ziplist Tests ===\n");
    
    // 测试基本push操作
    {
        unsigned char *zl = ziplistCrt.ziplistNew();
        test_cond("Create new ziplist", zl != NULL);
        
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"foo", 3, ZIPLIST_TAIL);
        test_cond("Push 'foo' to tail", zl != NULL);
        
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"quux", 4, ZIPLIST_TAIL);
        test_cond("Push 'quux' to tail", zl != NULL);
        
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"hello", 5, ZIPLIST_HEAD);
        test_cond("Push 'hello' to head", zl != NULL);
        
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"1024", 4, ZIPLIST_TAIL);
        test_cond("Push '1024' to tail", zl != NULL);
        
        // 测试索引访问
        p = ziplistCrt.ziplistIndex(zl, 0);
        test_cond("Access index 0", p != NULL);
        test_cond("Index 0 is 'hello'", ziplistCrt.ziplistCompare(p, (unsigned char*)"hello", 5));
        
	    ziplistRepr(zl);
        
	    // 测试越界索引
        p = ziplistCrt.ziplistIndex(zl, 5);
        test_cond("Access invalid index 5", !ziplistCrt.ziplistGet(p, &entry, &elen, &value));
        
        // 测试索引2
        p = ziplistCrt.ziplistIndex(zl, 2);
        test_cond("Access index 2", p != NULL);
        
        // 测试删除范围
        zl = ziplistCrt.ziplistDeleteRange(zl, 0, 1);
        test_cond("Delete range [0,1]", zl != NULL);
        
        // 测试逐个删除
        p = ziplistCrt.ziplistIndex(zl, -1);
        int deleted = 0;
        while(ziplistCrt.ziplistGet(p, &entry, &elen, &value)) {
            zl = ziplistCrt.ziplistDelete(zl, &p);
            p = ziplistCrt.ziplistPrev(zl, p);
            deleted++;
        }
        test_cond("Delete all entries", deleted > 0);
        
        zfree(zl);
    }
    
    // 测试整数存储
    {
        unsigned char *zl = ziplistCrt.ziplistNew();
        char buf[32];
        
        sprintf(buf, "100");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
        test_cond("Push integer '100'", zl != NULL);
        
        sprintf(buf, "128000");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
        test_cond("Push integer '128000'", zl != NULL);
        
        sprintf(buf, "-100");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_HEAD);
        test_cond("Push integer '-100'", zl != NULL);
        
        sprintf(buf, "4294967296");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_HEAD);
        test_cond("Push integer '4294967296'", zl != NULL);
        
        sprintf(buf, "non integer");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
        test_cond("Push string 'non integer'", zl != NULL);
        
        sprintf(buf, "much much longer non integer");
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
        test_cond("Push long string", zl != NULL);
        
        // 测试越界索引返回NULL
        p = ziplistCrt.ziplistIndex(zl, 10);
        test_cond("Out of range index returns NULL", p == NULL);
        
        zfree(zl);
    }
    
    // 测试替换功能
    {
        unsigned char *zl = ziplistCrt.ziplistNew();
        zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"abcd", 4, ZIPLIST_TAIL);
        test_cond("Create ziplist with 'abcd'", zl != NULL);
        
        p = ziplistCrt.ziplistIndex(zl, 0);
        test_cond("Access entry to replace", p != NULL);
        
        zl = ziplistCrt.ziplistReplace(zl, p, (unsigned char*)"zhao", 4);
        test_cond("Replace entry with 'zhao'", zl != NULL);
        
        p = ziplistCrt.ziplistIndex(zl, 0);
        test_cond("Access replaced entry", p != NULL);
        test_cond("Replaced entry is 'zhao'", ziplistCrt.ziplistCompare(p, (unsigned char*)"zhao", 4));
        
        zfree(zl);
    }
    
    // 输出测试报告
    test_report();
    
    return 0;
}
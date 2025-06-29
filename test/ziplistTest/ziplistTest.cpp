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
    unsigned char *entry,*p;
    unsigned int elen;
    long long value;
    unsigned char *zl = ziplistCrt.ziplistNew();
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"foo", 3, ZIPLIST_TAIL);
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"quux", 4, ZIPLIST_TAIL);
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"hello", 5, ZIPLIST_HEAD);
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"1024", 4, ZIPLIST_TAIL);

    p = ziplistCrt.ziplistIndex(zl,0);
    if (!ziplistCrt.ziplistCompare(p,(unsigned char*)"hello",5)) {
        printf("ERROR: not \"hello\"\n");
    }

    //ziplistRepr(zl);
    p = ziplistCrt.ziplistIndex(zl, 5);
    if (!ziplistCrt.ziplistGet(p, &entry, &elen, &value)) {
        printf("ERROR: Could not access index 5\n");
    }
    if (entry) {
        if (elen && fwrite(entry,elen,1,stdout) == 0) perror("fwrite");
        printf("\n");
    } else {
        printf("%lld\n", value);
    }

    //bug -start 
    //zl = ziplistCrt.ziplistReplace(zl, p, (unsigned char*)"zhao", 4);
    // p = ziplistCrt.ziplistIndex(zl,0);
    // if (!ziplistCrt.ziplistCompare(p,(unsigned char*)"zhao",4)) 
    // {
    // printf("ERROR: not \"hello\"\n");
    // return 1;
    // }
    //bug -end

    p = ziplistCrt.ziplistIndex(zl, 2);
    if (!ziplistCrt.ziplistGet(p, &entry, &elen, &value)) {
        printf("ERROR: Could not access index 2\n");
    }
    if (entry) {
        if (elen && fwrite(entry,elen,1,stdout) == 0) perror("fwrite");
        printf("\n");
    } else {
        printf("%lld\n", value);
    }
    zl = ziplistCrt.ziplistDeleteRange(zl, 0, 1);
    p = ziplistCrt.ziplistIndex(zl, -1);
    while(ziplistCrt.ziplistGet(p, &entry, &elen, &value))
    {
        if (entry) 
        {
            if (elen && fwrite(entry,elen,1,stdout) == 0) perror("fwrite");
        } 
        else 
        {
            printf("%lld\n", value);
        }
        zl = ziplistCrt.ziplistDelete(zl, &p);
        p = ziplistCrt.ziplistPrev(zl, p);
    }
    zfree(zl);

    zl = ziplistCrt.ziplistNew();
    char buf[32];
    sprintf(buf, "100");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
    sprintf(buf, "128000");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
    sprintf(buf, "-100");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_HEAD);
    sprintf(buf, "4294967296");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_HEAD);
    sprintf(buf, "non integer");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
    sprintf(buf, "much much longer non integer");
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)buf, strlen(buf), ZIPLIST_TAIL);
    //ziplistRepr(zl);
    p = ziplistCrt.ziplistIndex(zl, 10);
    if (p == NULL) {
        printf("No entry\n");
    } else {
        printf("ERROR: Out of range index should return NULL, returned offset: %ld\n", (long)(p-zl));
    }
    zfree(zl); 


    zl = ziplistCrt.ziplistNew();
    zl = ziplistCrt.ziplistPush(zl, (unsigned char*)"abcd", 4, ZIPLIST_TAIL);
    p = ziplistCrt.ziplistIndex(zl,0);
    ziplistCrt.ziplistReplace(zl, p, (unsigned char*)"zhao", 4);

    if (!ziplistCrt.ziplistCompare(p,(unsigned char*)"zhao",4)) 
    {
        printf("ERROR: not \"zhao\"\n");
    }

    zfree(zl); 



    return 0;
}

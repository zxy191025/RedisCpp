/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/12
 * Description: sds test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include "zmalloc.h"
#include "sds.h"

#define UNUSED(x) (void)(x)

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

static sdsCreate sdsC;
static sds sdsTestTemplateCallback(sds varname, void *arg) {
    UNUSED(arg);
    static const char *_var1 = "variable1";
    static const char *_var2 = "variable2";

    if (!strcmp(varname, _var1)) return sdsC.sdsnew("value1");
    else if (!strcmp(varname, _var2)) return sdsC.sdsnew("value2");
    else return NULL;
}



int main(int argc, char **argv)
{   
    //测试sdsnew
    sds x = sdsC.sdsnew("foo");
    test_cond("sdsnew",sdsC.sdslen(x) == 3 && memcmp(x,"foo\0",4) == 0);
    sdsC.sdsfree(x);
    //测试sdsnewlen
    x = sdsC.sdsnewlen("foo",2);
    test_cond("Create a string with specified length", sdsC.sdslen(x) == 2 && memcmp(x,"fo\0",3) == 0);  
    //测试sdscat
    x = sdsC.sdscat(x,"bar");
    test_cond("Strings concatenation",sdsC.sdslen(x) == 5 && memcmp(x,"fobar\0",6) == 0);
    //测试sdscpy
    x = sdsC.sdscpy(x,"a");
    test_cond("sdscpy() against an originally longer string",sdsC.sdslen(x) == 1 && memcmp(x,"a\0",2) == 0);
    x = sdsC.sdscpy(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");
    test_cond("sdscpy() against an originally shorter string",sdsC.sdslen(x) == 33 &&memcmp(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk\0",33) == 0);
    sdsC.sdsfree(x);
    //测试sdscatprintf
    x = sdsC.sdscatprintf(sdsC.sdsempty(),"%d",123);
    test_cond("sdscatprintf() seems working in the base case",sdsC.sdslen(x) == 3 && memcmp(x,"123\0",4) == 0);
    sdsC.sdsfree(x);
    x = sdsC.sdscatprintf(sdsC.sdsempty(),"a%cb",0);
    test_cond("sdscatprintf() seems working with \\0 inside of result",sdsC.sdslen(x) == 3 && memcmp(x,"a\0""b\0",4) == 0);
    sdsC.sdsfree(x);
    char etalon[1024*1024];
    for (size_t i = 0; i < sizeof(etalon); i++) {etalon[i] = '0';}
    x = sdsC.sdscatprintf(sdsC.sdsempty(),"%0*d",(int)sizeof(etalon),0);
    test_cond("sdscatprintf() can print 1MB",sdsC.sdslen(x) == sizeof(etalon) && memcmp(x,etalon,sizeof(etalon)) == 0);
    sdsC.sdsfree(x);
    //测试sdscatfmt
    x = sdsC.sdsnew("--");
    x = sdsC.sdscatfmt(x, "Hello %s World %I,%I--", "Hi!", LLONG_MIN,LLONG_MAX);
    test_cond("sdscatfmt() seems working in the base case",sdsC.sdslen(x) == 60 &&memcmp(x,"--Hello Hi! World -9223372036854775808,""9223372036854775807--",60) == 0);
    printf("[%s]\n",x);
    sdsC.sdsfree(x);
    x = sdsC.sdsnew("--");
    x = sdsC.sdscatfmt(x, "%u,%U--", UINT_MAX, ULLONG_MAX);
    test_cond("sdscatfmt() seems working with unsigned numbers",sdsC.sdslen(x) == 35 &&memcmp(x,"--4294967295,18446744073709551615--",35) == 0);
    sdsC.sdsfree(x);
    
    //测试sdstrim
    x = sdsC.sdsnew(" x ");
    sdsC.sdstrim(x," x");
    test_cond("sdstrim() works when all chars match",sdsC.sdslen(x) == 0);
    sdsC.sdsfree(x);
    x = sdsC.sdsnew(" x ");
    sdsC.sdstrim(x," ");
    test_cond("sdstrim() works when a single char remains",sdsC.sdslen(x) == 1 && x[0] == 'x');
    sdsC.sdsfree(x);
    x = sdsC.sdsnew("xxciaoyyy");
    sdsC.sdstrim(x,"xy");
    test_cond("sdstrim() correctly trims characters",sdsC.sdslen(x) == 4 && memcmp(x,"ciao\0",5) == 0);

    //测试sdstrim sdsrange
    sds y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,1,1);
    test_cond("sdsrange(...,1,1)",sdsC.sdslen(y) == 1 && memcmp(y,"i\0",2) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,1,-1);
    test_cond("sdsrange(...,1,-1)",sdsC.sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,-2,-1);
    test_cond("sdsrange(...,-2,-1)",sdsC.sdslen(y) == 2 && memcmp(y,"ao\0",3) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,2,1);
    test_cond("sdsrange(...,2,1)",sdsC.sdslen(y) == 0 && memcmp(y,"\0",1) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,1,100);
    test_cond("sdsrange(...,1,100)",sdsC.sdslen(y) == 3 && memcmp(y,"iao\0",4) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,100,100);
    test_cond("sdsrange(...,100,100)",sdsC.sdslen(y) == 0 && memcmp(y,"\0",1) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,4,6);
    test_cond("sdsrange(...,4,6)",sdsC.sdslen(y) == 0 && memcmp(y,"\0",1) == 0);
    sdsC.sdsfree(y);
    y = sdsC.sdsdup(x);
    sdsC.sdsrange(y,3,6);
    test_cond("sdsrange(...,3,6)",sdsC.sdslen(y) == 1 && memcmp(y,"o\0",2) == 0);
    sdsC.sdsfree(y);
    sdsC.sdsfree(x);
    //测试sdstrim sdscmp
    x = sdsC.sdsnew("foo");
    y = sdsC.sdsnew("foa");
    test_cond("sdscmp(foo,foa)", sdsC.sdscmp(x,y) > 0);

    sdsC.sdsfree(y);
    sdsC.sdsfree(x);
    x = sdsC.sdsnew("bar");
    y = sdsC.sdsnew("bar");
    test_cond("sdscmp(bar,bar)", sdsC.sdscmp(x,y) == 0);

    sdsC.sdsfree(y);
    sdsC.sdsfree(x);
    x = sdsC.sdsnew("aar");
    y = sdsC.sdsnew("bar");
    test_cond("sdscmp(bar,bar)", sdsC.sdscmp(x,y) < 0);

    sdsC.sdsfree(y);
    sdsC.sdsfree(x);
     //测试sdscatrepr  转义
    x = sdsC.sdsnewlen("\a\n\0foo\r",7);
    y = sdsC.sdscatrepr(sdsC.sdsempty(),x,sdsC.sdslen(x));
    test_cond("sdscatrepr(...data...)",memcmp(y,"\"\\a\\n\\x00foo\\r\"",15) == 0);

    {
        unsigned int oldfree;
        char *p;
        int i;
        size_t step = 10, j;

        sdsC.sdsfree(x);
        sdsC.sdsfree(y);
        x = sdsC.sdsnew("0");
        test_cond("sdsnew() free/len buffers", sdsC.sdslen(x) == 1 && sdsC.sdsavail(x) == 0);

        /* Run the test a few times in order to hit the first two
            * SDS header types. */
        for (i = 0; i < 10; i++) {
            size_t oldlen = sdsC.sdslen(x);
            x = sdsC.sdsMakeRoomFor(x,step);
            int type = x[-1]&SDS_TYPE_MASK;

            test_cond("sdsMakeRoomFor() len", sdsC.sdslen(x) == oldlen);
            if (type != SDS_TYPE_5) {
                test_cond("sdsMakeRoomFor() free", sdsC.sdsavail(x) >= step);
                oldfree = sdsC.sdsavail(x);
                UNUSED(oldfree);
            }
            p = x+oldlen;
            for (j = 0; j < step; j++) {
                p[j] = 'A'+j;
            }
            sdsC.sdsIncrLen(x,step);
        }
        test_cond("sdsMakeRoomFor() content",
        memcmp("0ABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJABCDEFGHIJ",x,101) == 0);
        test_cond("sdsMakeRoomFor() final length",sdsC.sdslen(x)==101);
        sdsC.sdsfree(x);
    }

    //测试 sdstemplate  
    /* Simple template */
    x = sdsC.sdstemplate("v1={variable1} v2={variable2}", sdsTestTemplateCallback, NULL);
    test_cond("sdstemplate() normal flow",memcmp(x,"v1=value1 v2=value2",19) == 0);
    sdsC.sdsfree(x);

    /* Template with callback error */
    x = sdsC.sdstemplate("v1={variable1} v3={doesnotexist}", sdsTestTemplateCallback, NULL);
    test_cond("sdstemplate() with callback error", x == NULL);

    /* Template with empty var name */
    x = sdsC.sdstemplate("v1={", sdsTestTemplateCallback, NULL);
    test_cond("sdstemplate() with empty var name", x == NULL);

    /* Template with truncated var name */
    x = sdsC.sdstemplate("v1={start", sdsTestTemplateCallback, NULL);
    test_cond("sdstemplate() with truncated var name", x == NULL);

    /* Template with quoting */
    x = sdsC.sdstemplate("v1={{{variable1}} {{} v2={variable2}", sdsTestTemplateCallback, NULL);
    test_cond("sdstemplate() with quoting",memcmp(x,"v1={value1} {} v2=value2",24) == 0);
    sdsC.sdsfree(x);
    
    return 0;
}
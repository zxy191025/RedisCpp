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

int main(int argc, char **argv)
{   
    sds x = sdsC.sdsnew("foo");
    test_cond("sdsnew",sdsC.sdslen(x) == 3 && memcmp(x,"foo\0",4) == 0);
    sdsC.sdsfree(x);
    
    x = sdsC.sdsnewlen("foo",2);
    test_cond("Create a string with specified length", sdsC.sdslen(x) == 2 && memcmp(x,"fo\0",3) == 0);
       
    x = sdsC.sdscat(x,"bar");
    test_cond("Strings concatenation",sdsC.sdslen(x) == 5 && memcmp(x,"fobar\0",6) == 0);

    x = sdsC.sdscpy(x,"a");
    test_cond("sdscpy() against an originally longer string",sdsC.sdslen(x) == 1 && memcmp(x,"a\0",2) == 0);

    x = sdsC.sdscpy(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk");
    test_cond("sdscpy() against an originally shorter string",sdsC.sdslen(x) == 33 &&memcmp(x,"xyzxxxxxxxxxxyyyyyyyyyykkkkkkkkkk\0",33) == 0);

    sdsC.sdsfree(x);


    return 0;
}
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/12
 * Description: toolFunc test
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include "zmalloc.h"
#include "sds.h"
#include "toolFunc.h"

toolFunc tool;
static void test_string2ll(void) {
    char buf[32];
    long long v;

    /* May not start with +. */
    strcpy(buf,"+1");
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);

    /* Leading space. */
    strcpy(buf," 1");
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);

    /* Trailing space. */
    strcpy(buf,"1 ");
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);

    /* May not start with 0. */
    strcpy(buf,"01");
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);

    strcpy(buf,"-1");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == -1);

    strcpy(buf,"0");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == 0);

    strcpy(buf,"1");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == 1);

    strcpy(buf,"99");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == 99);

    strcpy(buf,"-99");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == -99);

    strcpy(buf,"-9223372036854775808");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == LLONG_MIN);

    strcpy(buf,"-9223372036854775809"); /* overflow */
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);

    strcpy(buf,"9223372036854775807");
    assert(tool.string2ll(buf,strlen(buf),&v) == 1);
    assert(v == LLONG_MAX);

    strcpy(buf,"9223372036854775808"); /* overflow */
    assert(tool.string2ll(buf,strlen(buf),&v) == 0);
}

static void test_string2l(void) {
    char buf[32];
    long v;

    /* May not start with +. */
    strcpy(buf,"+1");
    assert(tool.string2l(buf,strlen(buf),&v) == 0);

    /* May not start with 0. */
    strcpy(buf,"01");
    assert(tool.string2l(buf,strlen(buf),&v) == 0);

    strcpy(buf,"-1");
    assert(tool.string2l(buf,strlen(buf),&v) == 1);
    assert(v == -1);

    strcpy(buf,"0");
    assert(tool.string2l(buf,strlen(buf),&v) == 1);
    assert(v == 0);

    strcpy(buf,"1");
    assert(tool.string2l(buf,strlen(buf),&v) == 1);
    assert(v == 1);

    strcpy(buf,"99");
    assert(tool.string2l(buf,strlen(buf),&v) == 1);
    assert(v == 99);

    strcpy(buf,"-99");
    assert(tool.string2l(buf,strlen(buf),&v) == 1);
    assert(v == -99);

#if LONG_MAX != LLONG_MAX
    strcpy(buf,"-2147483648");
    assert(string2l(buf,strlen(buf),&v) == 1);
    assert(v == LONG_MIN);

    strcpy(buf,"-2147483649"); /* overflow */
    assert(string2l(buf,strlen(buf),&v) == 0);

    strcpy(buf,"2147483647");
    assert(string2l(buf,strlen(buf),&v) == 1);
    assert(v == LONG_MAX);

    strcpy(buf,"2147483648"); /* overflow */
    assert(string2l(buf,strlen(buf),&v) == 0);
#endif
}

static void test_ll2string(void) {
    char buf[32];
    long long v;
    int sz;

    v = 0;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 1);
    assert(!strcmp(buf, "0"));

    v = -1;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 2);
    assert(!strcmp(buf, "-1"));

    v = 99;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 2);
    assert(!strcmp(buf, "99"));

    v = -99;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 3);
    assert(!strcmp(buf, "-99"));

    v = -2147483648;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 11);
    assert(!strcmp(buf, "-2147483648"));

    v = LLONG_MIN;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 20);
    assert(!strcmp(buf, "-9223372036854775808"));

    v = LLONG_MAX;
    sz = tool.ll2string(buf, sizeof buf, v);
    assert(sz == 19);
    assert(!strcmp(buf, "9223372036854775807"));
}

int main(int argc, char **argv)
{
    test_string2ll();
    test_string2l();
    test_ll2string();
    return 0;
}


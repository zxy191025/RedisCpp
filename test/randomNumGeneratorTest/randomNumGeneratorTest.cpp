/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: randomNumGenerator test
 */
#include "randomNumGenerator.h"
#include <stdio.h>
#include <stdlib.h>
#include "define.h"
using namespace REDIS_BASE;
// 测试宏定义
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

int main() {
    randomNumGenerator rng;

    // 测试 init_genrand64 方法
    unsigned long long seed = 123456789;
    rng.init_genrand64(seed);
    test_cond("init_genrand64 should initialize without errors", 1);

    // 测试 genrand64_int64 方法
    unsigned long long rand64 = rng.genrand64_int64();
    printf("Generated 64-bit unsigned integer: %llu\n", rand64);
    test_cond("genrand64_int64 should return a valid 64-bit unsigned integer", rand64 >= 0 && rand64 <= 0xFFFFFFFFFFFFFFFFULL);

    // 测试 genrand64_int63 方法
    long long rand63 = rng.genrand64_int63();
    printf("Generated 63-bit signed integer: %lld\n", rand63);
    test_cond("genrand64_int63 should return a valid 63-bit signed integer", rand63 >= 0 && rand63 <= 0x7FFFFFFFFFFFFFFFLL);

    // 测试 genrand64_real1 方法
    double randReal1 = rng.genrand64_real1();
    printf("Generated real number in [0.0, 1.0]: %f\n", randReal1);
    test_cond("genrand64_real1 should return a double in [0.0, 1.0]", randReal1 >= 0.0 && randReal1 <= 1.0);

    // 测试 genrand64_real2 方法
    double randReal2 = rng.genrand64_real2();
    printf("Generated real number in [0.0, 1.0): %f\n", randReal2);
    test_cond("genrand64_real2 should return a double in [0.0, 1.0)", randReal2 >= 0.0 && randReal2 < 1.0);

    // 测试 genrand64_real3 方法
    double randReal3 = rng.genrand64_real3();
    printf("Generated real number in (0.0, 1.0): %f\n", randReal3);
    test_cond("genrand64_real3 should return a double in (0.0, 1.0)", randReal3 > 0.0 && randReal3 < 1.0);

    // 测试 init_by_array64 方法
    unsigned long long init_key[] = {1, 2, 3, 4, 5};
    unsigned long long key_length = sizeof(init_key) / sizeof(init_key[0]);
    rng.init_by_array64(init_key, key_length);
    test_cond("init_by_array64 should initialize without errors", 1);

    // 生成新的随机数验证初始化
    rand64 = rng.genrand64_int64();
    printf("Generated 64-bit unsigned integer after init_by_array64: %llu\n", rand64);
    test_cond("genrand64_int64 after init_by_array64 should return a valid 64-bit unsigned integer", rand64 >= 0 && rand64 <= 0xFFFFFFFFFFFFFFFFULL);

    // 报告测试结果
    test_report();

    return 0;
}

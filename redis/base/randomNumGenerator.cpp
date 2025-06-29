/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: 借助梅森旋转算法（Mersenne Twister algorithm），能够生成高质量的伪随机数序列。
 */
#include "randomNumGenerator.h"
#include <stdio.h>

#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */


/* The array for the state vector */
static unsigned long long mt[NN];
/* mti==NN+1 means mt[NN] is not initialized */
static int mti=NN+1;
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//

/**
 * 使用单个种子值初始化随机数生成器
 * @param seed 用于初始化的64位无符号整数种子值
 * @note 相同的种子将产生相同的随机数序列，便于重现结果
 */
void randomNumGenerator::init_genrand64(unsigned long long seed)
{
    mt[0] = seed;
    for (mti=1; mti<NN; mti++)
        mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti); 
}

/**
 * 使用种子数组初始化随机数生成器（增强随机性）
 * @param init_key 用于初始化的64位无符号整数数组
 * @param key_length 种子数组的长度
 * @note 适用于需要更高熵值的场景，如密码学应用
 */
void randomNumGenerator::init_by_array64(unsigned long long init_key[],unsigned long long key_length)
{
    unsigned long long i, j, k;
    init_genrand64(19650218ULL);
    i=1; j=0;
    k = (NN>key_length ? NN : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 3935559000370003845ULL))
          + init_key[j] + j; /* non linear */
        i++; j++;
        if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=NN-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 2862933555777941757ULL))
          - i; /* non linear */
        i++;
        if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
    }

    mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */
}

/**
 * 生成64位无符号整数随机数
 * @return [0, 2^64-1]范围内的均匀分布随机整数
 * @note 此为核心生成函数，其他分布基于此实现
 */
unsigned long long randomNumGenerator::genrand64_int64(void)
{
    int i;
    unsigned long long x;
    static unsigned long long mag01[2]={0ULL, MATRIX_A};

    if (mti >= NN) { /* generate NN words at one time */

        /* if init_genrand64() has not been called, */
        /* a default initial seed is used     */
        if (mti == NN+1)
            init_genrand64(5489ULL);

        for (i=0;i<NN-MM;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        for (;i<NN-1;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        x = (mt[NN-1]&UM)|(mt[0]&LM);
        mt[NN-1] = mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

        mti = 0;
    }

    x = mt[mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

/**
 * 生成63位有符号整数随机数
 * @return [0, 2^63-1]范围内的均匀分布随机整数
 * @note 避免符号位影响，适用于需要正整数的场景
 */
long long randomNumGenerator::genrand64_int63(void)
{
    return (long long)(genrand64_int64() >> 1);
}

/**
 * 生成闭区间[0,1]上的双精度浮点数随机数
 * @return [0.0, 1.0]范围内的均匀分布随机浮点数（包含0和1）
 * @note 精度约为2^-53
 */
double randomNumGenerator::genrand64_real1(void)
{
    return (genrand64_int64() >> 11) * (1.0/9007199254740991.0);
}

/**
 * 生成半开区间[0,1)上的双精度浮点数随机数
 * @return [0.0, 1.0)范围内的均匀分布随机浮点数（包含0但不包含1）
 * @note 最常用的浮点数分布，精度约为2^-53
 */
double randomNumGenerator::genrand64_real2(void)
{
    return (genrand64_int64() >> 11) * (1.0/9007199254740992.0);
}

/**
 * 生成开区间(0,1)上的双精度浮点数随机数
 * @return (0.0, 1.0)范围内的均匀分布随机浮点数（不包含0和1）
 * @note 适用于需要非零值的概率场景
 */
double randomNumGenerator::genrand64_real3(void)
{
    return ((genrand64_int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
}

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
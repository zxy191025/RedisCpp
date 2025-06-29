/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/18
 * All rights reserved. No one may copy or transfer.
 * Description: 借助梅森旋转算法（Mersenne Twister algorithm），能够生成高质量的伪随机数序列。
 */
#ifndef __RANDOMNUMGENERATOR_H__ 
#define __RANDOMNUMGENERATOR_H__
#include "define.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
 class randomNumGenerator
 {
 public:
     randomNumGenerator() = default;
     ~randomNumGenerator()= default;
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

 };
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif



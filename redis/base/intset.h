/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/27
 * All rights reserved. No one may copy or transfer.
 * Description: intset implementation
 */
#ifndef REDIS_BASE_INTSET_H
#define REDIS_BASE_INTSET_H
#include "define.h"

#include <stdint.h>
#include <stddef.h>
typedef struct intset {
    uint32_t encoding;
    uint32_t length;
    int8_t contents[];
} intset;
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class toolFunc;
class intsetCreate
{
public:
    intsetCreate();
    ~intsetCreate();
public:
    /**
     * 创建一个新的整数集合
     * @return 指向新创建的intset的指针
     */
    intset *intsetNew(void);

    /**
     * 向整数集合中添加一个元素
     * @param is 目标整数集合
     * @param value 要添加的整数值
     * @param success 输出参数，指示添加是否成功（1表示成功，0表示失败）
     * @return 可能是修改后的原集合，或重新分配的新集合
     */
    intset *intsetAdd(intset *is, int64_t value, uint8_t *success);

    /**
     * 从整数集合中移除一个元素
     * @param is 目标整数集合
     * @param value 要移除的整数值
     * @param success 输出参数，指示移除是否成功（1表示成功，0表示失败）
     * @return 可能是修改后的原集合，或重新分配的新集合
     */
    intset *intsetRemove(intset *is, int64_t value, int *success);

    /**
     * 检查整数集合中是否存在指定值
     * @param is 目标整数集合
     * @param value 要查找的整数值
     * @return 1表示存在，0表示不存在
     */
    uint8_t intsetFind(intset *is, int64_t value);

    /**
     * 从整数集合中随机获取一个元素
     * @param is 目标整数集合
     * @return 随机选中的整数值
     */
    int64_t intsetRandom(intset *is);

    /**
     * 获取整数集合中指定位置的元素
     * @param is 目标整数集合
     * @param pos 元素位置（0-based）
     * @param value 输出参数，存储获取到的整数值
     * @return 1表示获取成功，0表示位置无效
     */
    uint8_t intsetGet(intset *is, uint32_t pos, int64_t *value);

    /**
     * 获取整数集合的元素个数
     * @param is 目标整数集合
     * @return 集合中的元素数量
     */
    uint32_t intsetLen(const intset *is);

    /**
     * 获取整数集合的内存占用大小（以字节为单位）
     * @param is 目标整数集合
     * @return 集合占用的总字节数
     */
    size_t intsetBlobLen(intset *is);

    /**
     * 验证整数集合的完整性
     * @param is 指向整数集合的指针
     * @param size 集合的预期大小
     * @param deep 是否进行深度验证（1表示进行深度验证，0表示只进行基本验证）
     * @return 1表示完整，0表示损坏
     */
    int intsetValidateIntegrity(const unsigned char *p, size_t size, int deep);

    /**
     * 确定给定整数值所需的最小编码类型
     * @param v 待检查的整数值
     * @return 编码类型常量: 
     *         INTSET_ENC_INT16 (16位), 
     *         INTSET_ENC_INT32 (32位), 
     *         INTSET_ENC_INT64 (64位)
     */
    uint8_t _intsetValueEncoding(int64_t v);

    /**
     * 升级整数集合的编码类型并添加新元素
     * @param is 待升级的整数集合
     * @param value 要添加的新元素(需更高编码)
     * @return 升级后的整数集合指针
     * @throws 内存分配失败时返回NULL
     */
    intset *intsetUpgradeAndAdd(intset *is, int64_t value);

    /**
     * 在整数集合中搜索指定值
     * @param is 目标整数集合
     * @param value 要搜索的值
     * @param pos [可选]输出参数，存储值应插入的位置或已存在位置
     * @return 存在返回1，不存在返回0
     */
    uint8_t intsetSearch(intset *is, int64_t value, uint32_t *pos);

    /**
     * 调整整数集合的内存大小
     * @param is 目标整数集合
     * @param len 新的元素数量
     * @return 调整后的整数集合指针
     * @throws 内存分配失败时返回NULL
     */
    intset *intsetResize(intset *is, uint32_t len);

    /**
     * 获取指定位置的元素值(使用集合当前编码)
     * @param is 目标整数集合
     * @param pos 元素位置(0-based)
     * @return 指定位置的整数值
     * @throws 位置越界时行为未定义
     */
    int64_t _intsetGet(intset *is, int pos);

    /**
     * 设置指定位置的元素值(使用集合当前编码)
     * @param is 目标整数集合
     * @param pos 元素位置(0-based)
     * @param value 要设置的新值
     * @throws 位置越界时行为未定义
     */
    void _intsetSet(intset *is, int pos, int64_t value);

    /**
     * 获取指定位置的元素值(使用指定编码)
     * @param is 目标整数集合
     * @param pos 元素位置(0-based)
     * @param enc 使用的编码类型
     * @return 指定位置的整数值
     * @throws 位置越界或编码不匹配时行为未定义
     */
    int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc);

    /**
     * 将集合中从from位置开始的所有元素后移到to位置
     * @param is 目标整数集合
     * @param from 起始位置(包含)
     * @param to 目标位置
     * @implNote 用于插入/删除元素时移动后续数据
     */
    void intsetMoveTail(intset *is, uint32_t from, uint32_t to);
private:
    toolFunc* toolFuncInstance;
};

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
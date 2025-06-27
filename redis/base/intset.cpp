/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/27
 * All rights reserved. No one may copy or transfer.
 * Description: intset implementation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intset.h"
#include "zmallocDf.h"
#include <string.h>
#include "toolFunc.h"
#include <assert.h>
#define INTSET_ENC_INT16 (sizeof(int16_t))
#define INTSET_ENC_INT32 (sizeof(int32_t))
#define INTSET_ENC_INT64 (sizeof(int64_t))

intsetCreate::intsetCreate()
{
    toolFuncInstance = static_cast<toolFunc *>(zmalloc(sizeof(toolFunc)));
}
intsetCreate::~intsetCreate()
{
    zfree(toolFuncInstance);
}

/**
 * 创建一个新的整数集合
 * @return 指向新创建的intset的指针
 */
intset *intsetCreate::intsetNew(void)
{
    intset *is =   static_cast<intset*>(zmalloc(sizeof(intset)));
    is->encoding = intrev32ifbe(INTSET_ENC_INT16);
    is->length = 0;
    return is;
}

/**
 * 向整数集合中添加一个元素
 * @param is 目标整数集合
 * @param value 要添加的整数值
 * @param success 输出参数，指示添加是否成功（1表示成功，0表示失败）
 * @return 可能是修改后的原集合，或重新分配的新集合
 */
intset *intsetCreate::intsetAdd(intset *is, int64_t value, uint8_t *success)
{
    uint8_t valenc = _intsetValueEncoding(value);
    uint32_t pos;
    if (success) *success = 1;

    /* Upgrade encoding if necessary. If we need to upgrade, we know that
     * this value should be either appended (if > 0) or prepended (if < 0),
     * because it lies outside the range of existing values. */
    if (valenc > intrev32ifbe(is->encoding)) {
        /* This always succeeds, so we don't need to curry *success. */
        return intsetUpgradeAndAdd(is,value);
    } else {
        /* Abort if the value is already present in the set.
         * This call will populate "pos" with the right position to insert
         * the value when it cannot be found. */
        if (intsetSearch(is,value,&pos)) {
            if (success) *success = 0;
            return is;
        }

        is = intsetResize(is,intrev32ifbe(is->length)+1);
        if (pos < intrev32ifbe(is->length)) intsetMoveTail(is,pos,pos+1);
    }

    _intsetSet(is,pos,value);
    is->length = intrev32ifbe(intrev32ifbe(is->length)+1);
    return is;
}

/**
 * 从整数集合中移除一个元素
 * @param is 目标整数集合
 * @param value 要移除的整数值
 * @param success 输出参数，指示移除是否成功（1表示成功，0表示失败）
 * @return 可能是修改后的原集合，或重新分配的新集合
 */
intset *intsetCreate::intsetRemove(intset *is, int64_t value, int *success)
{
    uint8_t valenc = _intsetValueEncoding(value);
    uint32_t pos;
    if (success) *success = 0;

    if (valenc <= intrev32ifbe(is->encoding) && intsetSearch(is,value,&pos)) {
        uint32_t len = intrev32ifbe(is->length);

        /* We know we can delete */
        if (success) *success = 1;

        /* Overwrite value with tail and update length */
        if (pos < (len-1)) intsetMoveTail(is,pos+1,pos);
        is = intsetResize(is,len-1);
        is->length = intrev32ifbe(len-1);
    }
    return is;
}

/**
 * 检查整数集合中是否存在指定值
 * @param is 目标整数集合
 * @param value 要查找的整数值
 * @return 1表示存在，0表示不存在
 */
uint8_t intsetCreate::intsetFind(intset *is, int64_t value)
{
    uint8_t valenc = _intsetValueEncoding(value);
    return valenc <= intrev32ifbe(is->encoding) && intsetSearch(is,value,NULL);
}

/**
 * 从整数集合中随机获取一个元素
 * @param is 目标整数集合
 * @return 随机选中的整数值
 */
int64_t intsetCreate::intsetRandom(intset *is)
{
    uint32_t len = intrev32ifbe(is->length);
    assert(len); /* avoid division by zero on corrupt intset payload. */
    return _intsetGet(is,rand()%len);
}

/**
 * 获取整数集合中指定位置的元素
 * @param is 目标整数集合
 * @param pos 元素位置（0-based）
 * @param value 输出参数，存储获取到的整数值
 * @return 1表示获取成功，0表示位置无效
 */
uint8_t intsetCreate::intsetGet(intset *is, uint32_t pos, int64_t *value)
{
    // uint8_t valenc = _intsetValueEncoding(value);
    // return valenc <= intrev32ifbe(is->encoding) && intsetSearch(is,value,NULL);

    // 先解引用 value 指针，获取存储的值
    uint8_t valenc = _intsetValueEncoding(*value);
    return valenc <= intrev32ifbe(is->encoding) && intsetSearch(is, *value, NULL);
}

/**
 * 获取整数集合的元素个数
 * @param is 目标整数集合
 * @return 集合中的元素数量
 */
uint32_t intsetCreate::intsetLen(const intset *is)
{
    return intrev32ifbe(is->length);
}

/**
 * 获取整数集合的内存占用大小（以字节为单位）
 * @param is 目标整数集合
 * @return 集合占用的总字节数
 */
size_t intsetCreate::intsetBlobLen(intset *is)
{
    return sizeof(intset)+(size_t)intrev32ifbe(is->length)*intrev32ifbe(is->encoding);
}

/**
 * 验证整数集合的完整性
 * @param is 指向整数集合的指针
 * @param size 集合的预期大小
 * @param deep 是否进行深度验证（1表示进行深度验证，0表示只进行基本验证）
 * @return 1表示完整，0表示损坏
 */
int intsetCreate::intsetValidateIntegrity(const unsigned char *p, size_t size, int deep)
{
    intset *is = (intset *)p;
    /* check that we can actually read the header. */
    if (size < sizeof(*is))
        return 0;

    uint32_t encoding = intrev32ifbe(is->encoding);

    size_t record_size;
    if (encoding == INTSET_ENC_INT64) {
        record_size = INTSET_ENC_INT64;
    } else if (encoding == INTSET_ENC_INT32) {
        record_size = INTSET_ENC_INT32;
    } else if (encoding == INTSET_ENC_INT16){
        record_size = INTSET_ENC_INT16;
    } else {
        return 0;
    }

    /* check that the size matchies (all records are inside the buffer). */
    uint32_t count = intrev32ifbe(is->length);
    if (sizeof(*is) + count*record_size != size)
        return 0;

    /* check that the set is not empty. */
    if (count==0)
        return 0;

    if (!deep)
        return 1;

    /* check that there are no dup or out of order records. */
    int64_t prev = _intsetGet(is,0);
    for (uint32_t i=1; i<count; i++) {
        int64_t cur = _intsetGet(is,i);
        if (cur <= prev)
            return 0;
        prev = cur;
    }

    return 1;
}

/* Return the required encoding for the provided value. */
uint8_t intsetCreate::_intsetValueEncoding(int64_t v) 
{
    if (v < INT32_MIN || v > INT32_MAX)
        return INTSET_ENC_INT64;
    else if (v < INT16_MIN || v > INT16_MAX)
        return INTSET_ENC_INT32;
    else
        return INTSET_ENC_INT16;
}

intset *intsetCreate::intsetUpgradeAndAdd(intset *is, int64_t value) 
{
    uint8_t curenc = intrev32ifbe(is->encoding);
    uint8_t newenc = _intsetValueEncoding(value);
    int length = intrev32ifbe(is->length);
    int prepend = value < 0 ? 1 : 0;

    /* First set new encoding and resize */
    is->encoding = intrev32ifbe(newenc);
    is = intsetResize(is,intrev32ifbe(is->length)+1);

    /* Upgrade back-to-front so we don't overwrite values.
     * Note that the "prepend" variable is used to make sure we have an empty
     * space at either the beginning or the end of the intset. */
    while(length--)
        _intsetSet(is,length+prepend,_intsetGetEncoded(is,length,curenc));

    /* Set the value at the beginning or the end. */
    if (prepend)
        _intsetSet(is,0,value);
    else
        _intsetSet(is,intrev32ifbe(is->length),value);
    is->length = intrev32ifbe(intrev32ifbe(is->length)+1);
    return is;
}

uint8_t intsetCreate::intsetSearch(intset *is, int64_t value, uint32_t *pos) 
{
    int min = 0, max = intrev32ifbe(is->length)-1, mid = -1;
    int64_t cur = -1;

    /* The value can never be found when the set is empty */
    if (intrev32ifbe(is->length) == 0) {
        if (pos) *pos = 0;
        return 0;
    } else {
        /* Check for the case where we know we cannot find the value,
         * but do know the insert position. */
        if (value > _intsetGet(is,max)) {
            if (pos) *pos = intrev32ifbe(is->length);
            return 0;
        } else if (value < _intsetGet(is,0)) {
            if (pos) *pos = 0;
            return 0;
        }
    }

    while(max >= min) {
        mid = ((unsigned int)min + (unsigned int)max) >> 1;
        cur = _intsetGet(is,mid);
        if (value > cur) {
            min = mid+1;
        } else if (value < cur) {
            max = mid-1;
        } else {
            break;
        }
    }

    if (value == cur) {
        if (pos) *pos = mid;
        return 1;
    } else {
        if (pos) *pos = min;
        return 0;
    }
}

/* Resize the intset */
intset *intsetCreate::intsetResize(intset *is, uint32_t len) 
{
    uint64_t size = (uint64_t)len*intrev32ifbe(is->encoding);
    assert(size <= SIZE_MAX - sizeof(intset));
    is =   static_cast<intset *>(zrealloc(is,sizeof(intset)+size));
    return is;
}

/* Return the value at pos, using the configured encoding. */
int64_t intsetCreate::_intsetGet(intset *is, int pos)
 {
    return _intsetGetEncoded(is,pos,intrev32ifbe(is->encoding));
}

/* Set the value at pos, using the configured encoding. */
void intsetCreate::_intsetSet(intset *is, int pos, int64_t value) 
{
    uint32_t encoding = intrev32ifbe(is->encoding);

    if (encoding == INTSET_ENC_INT64) {
        ((int64_t*)is->contents)[pos] = value;
        memrev64ifbe(((int64_t*)is->contents)+pos);
    } else if (encoding == INTSET_ENC_INT32) {
        ((int32_t*)is->contents)[pos] = value;
        memrev32ifbe(((int32_t*)is->contents)+pos);
    } else {
        ((int16_t*)is->contents)[pos] = value;
        memrev16ifbe(((int16_t*)is->contents)+pos);
    }
}

/* Return the value at pos, given an encoding. */
int64_t intsetCreate::_intsetGetEncoded(intset *is, int pos, uint8_t enc) 
{
    int64_t v64;
    int32_t v32;
    int16_t v16;

    if (enc == INTSET_ENC_INT64) {
        memcpy(&v64,((int64_t*)is->contents)+pos,sizeof(v64));
        memrev64ifbe(&v64);
        return v64;
    } else if (enc == INTSET_ENC_INT32) {
        memcpy(&v32,((int32_t*)is->contents)+pos,sizeof(v32));
        memrev32ifbe(&v32);
        return v32;
    } else {
        memcpy(&v16,((int16_t*)is->contents)+pos,sizeof(v16));
        memrev16ifbe(&v16);
        return v16;
    }
}

void intsetCreate::intsetMoveTail(intset *is, uint32_t from, uint32_t to) 
{
    void *src, *dst;
    uint32_t bytes = intrev32ifbe(is->length)-from;
    uint32_t encoding = intrev32ifbe(is->encoding);

    if (encoding == INTSET_ENC_INT64) {
        src = (int64_t*)is->contents+from;
        dst = (int64_t*)is->contents+to;
        bytes *= sizeof(int64_t);
    } else if (encoding == INTSET_ENC_INT32) {
        src = (int32_t*)is->contents+from;
        dst = (int32_t*)is->contents+to;
        bytes *= sizeof(int32_t);
    } else {
        src = (int16_t*)is->contents+from;
        dst = (int16_t*)is->contents+to;
        bytes *= sizeof(int16_t);
    }
    memmove(dst,src,bytes);
}




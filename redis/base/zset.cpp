/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/27
 * All rights reserved. No one may copy or transfer.
 * Description: zset implementation
 */
#include "zset.h"
#include "zmallocDf.h"
#include "sds.h"
#include "toolFunc.h"
#include <string.h>

zsetCreate::zsetCreate()
{
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
    ziplistCreateInstance = static_cast<ziplistCreate *>(zmalloc(sizeof(ziplistCreate)));
    //serverAssert(sdsCreateInstance != NULL && ziplistCreateInstance != NULL);
    toolFuncInstance = static_cast<toolFunc *>(zmalloc(sizeof(toolFunc)));
    //serverAssert(toolFuncInstance != NULL);
    zskiplistCreateInstance = static_cast<zskiplistCreate *>(zmalloc(sizeof(zskiplistCreate)));
    //serverAssert(zskiplistCreateInstance != NULL);
}

zsetCreate::~zsetCreate()
{
    zfree(sdsCreateInstance);
    zfree(ziplistCreateInstance);
    zfree(toolFuncInstance);
    zfree(zskiplistCreateInstance);
}

/**
 * 向压缩列表中插入元素和分数
 * @param zl 目标压缩列表指针
 * @param ele 待插入的元素字符串
 * @param score 待插入的分数值
 * @return 返回新的压缩列表指针（可能已重新分配内存）
 */
unsigned char *zsetCreate::zzlInsert(unsigned char *zl, sds ele, double score)
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,0), *sptr;
    double s;

    while (eptr != NULL) {
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssert(sptr != NULL);
        s = zzlGetScore(sptr);

        if (s > score) {
            /* First element with score larger than score for element to be
             * inserted. This means we should take its spot in the list to
             * maintain ordering. */
            zl = zzlInsertAt(zl,eptr,ele,score);
            break;
        } else if (s == score) {
            /* Ensure lexicographical ordering for elements. */
            if (zzlCompareElements(eptr,(unsigned char*)ele,sdsCreateInstance->sdslen(ele)) > 0) {
                zl = zzlInsertAt(zl,eptr,ele,score);
                break;
            }
        }

        /* Move to next element. */
        eptr = ziplistCreateInstance->ziplistNext(zl,sptr);
    }

    /* Push on tail of list when it was not yet inserted. */
    if (eptr == NULL)
        zl = zzlInsertAt(zl,NULL,ele,score);
    return zl;
}

/**
 * 从压缩列表中获取元素的分数值
 * @param sptr 指向分数存储位置的指针（通过zzlNext/zzlPrev获取）
 * @return 返回解析出的分数值
 */
double zsetCreate::zzlGetScore(unsigned char *sptr)
{
    unsigned char *vstr;
    unsigned int vlen;
    long long vlong;
    double score;

    //serverAssert(sptr != NULL);
    //serverAssert(ziplistGet(sptr,&vstr,&vlen,&vlong));

    if (vstr) {
        score = zzlStrtod(vstr,vlen);
    } else {
        score = vlong;
    }

    return score;
}

/**
 * 移动到压缩列表的下一个元素
 * @param zl 压缩列表指针
 * @param eptr 输出参数，指向当前元素的指针
 * @param sptr 输出参数，指向当前元素分数的指针
 */
void zsetCreate::zzlNext(unsigned char *zl, unsigned char **eptr, unsigned char **sptr)
{
    unsigned char *_eptr, *_sptr;
    //serverAssert(*eptr != NULL && *sptr != NULL);

    _eptr = ziplistCreateInstance->ziplistNext(zl,*sptr);
    if (_eptr != NULL) {
        _sptr = ziplistCreateInstance->ziplistNext(zl,_eptr);
        //serverAssert(_sptr != NULL);
    } else {
        /* No next entry. */
        _sptr = NULL;
    }

    *eptr = _eptr;
    *sptr = _sptr;
}


/**
 * 移动到压缩列表的前一个元素
 * @param zl 压缩列表指针
 * @param eptr 输出参数，指向当前元素的指针
 * @param sptr 输出参数，指向当前元素分数的指针
 */
void zsetCreate::zzlPrev(unsigned char *zl, unsigned char **eptr, unsigned char **sptr)
{
    unsigned char *_eptr, *_sptr;
    //serverAssert(*eptr != NULL && *sptr != NULL);

    _sptr = ziplistCreateInstance->ziplistPrev(zl,*eptr);
    if (_sptr != NULL) {
        _eptr = ziplistCreateInstance->ziplistPrev(zl,_sptr);
        //serverAssert(_eptr != NULL);
    } else {
        /* No previous entry. */
        _eptr = NULL;
    }

    *eptr = _eptr;
    *sptr = _sptr;
}

/**
 * 获取压缩列表中分数范围内的第一个元素
 * @param zl 压缩列表指针
 * @param range 分数范围规范结构体，包含min/max及边界规则
 * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
 */
unsigned char *zsetCreate::zzlFirstInRange(unsigned char *zl, zrangespec *range)
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,0), *sptr;
    double score;

    /* If everything is out of range, return early. */
    if (!zzlIsInRange(zl,range)) return NULL;

    while (eptr != NULL) {
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssert(sptr != NULL);

        score = zzlGetScore(sptr);
        if (zskiplistCreateInstance->zslValueGteMin(score,range)) {
            /* Check if score <= max. */
            if (zskiplistCreateInstance->zslValueLteMax(score,range))
                return eptr;
            return NULL;
        }

        /* Move to next element. */
        eptr = ziplistCreateInstance->ziplistNext(zl,sptr);
    }

    return NULL;
}

/**
 * 获取压缩列表中分数范围内的最后一个元素
 * @param zl 压缩列表指针
 * @param range 分数范围规范结构体，包含min/max及边界规则
 * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
 */
unsigned char *zsetCreate::zzlLastInRange(unsigned char *zl, zrangespec *range)
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,-2), *sptr;
    double score;

    /* If everything is out of range, return early. */
    if (!zzlIsInRange(zl,range)) return NULL;

    while (eptr != NULL) {
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssert(sptr != NULL);

        score = zzlGetScore(sptr);
        if (zskiplistCreateInstance->zslValueLteMax(score,range)) {
            /* Check if score >= min. */
            if (zskiplistCreateInstance->zslValueGteMin(score,range))
                return eptr;
            return NULL;
        }

        /* Move to previous element by moving to the score of previous element.
         * When this returns NULL, we know there also is no element. */
        sptr = ziplistCreateInstance->ziplistPrev(zl,eptr);
        if (sptr != NULL)
            //serverAssert((eptr = ziplistCreateInstance->ziplistPrev(zl,sptr)) != NULL);
            ;
        else
            eptr = NULL;
    }

    return NULL;
}
/**
 * 在跳跃表(zskiplist)的底层链表中插入一个新元素
 * 
 * @param zl        指向跳跃表底层压缩列表的指针
 * @param eptr      插入位置的指针（NULL表示插入到尾部）
 * @param ele       要插入的元素值（SDS字符串）
 * @param score     元素的分数（排序依据）
 * @return          插入新元素后的压缩列表指针
 */
unsigned char *zsetCreate::zzlInsertAt(unsigned char *zl, unsigned char *eptr, sds ele, double score) 
{
    unsigned char *sptr;
    char scorebuf[128];
    int scorelen;
    size_t offset;

    scorelen = toolFuncInstance->d2string(scorebuf,sizeof(scorebuf),score);
    if (eptr == NULL) {
        zl = ziplistCreateInstance->ziplistPush(zl,(unsigned char*)ele,sdsCreateInstance->sdslen(ele),ZIPLIST_TAIL);
        zl = ziplistCreateInstance->ziplistPush(zl,(unsigned char*)scorebuf,scorelen,ZIPLIST_TAIL);
    } else {
        /* Keep offset relative to zl, as it might be re-allocated. */
        offset = eptr-zl;
        zl = ziplistCreateInstance->ziplistInsert(zl,eptr,(unsigned char*)ele,sdsCreateInstance->sdslen(ele));
        eptr = zl+offset;

        /* Insert score after the element. */
        //serverAssert((sptr = ziplistNext(zl,eptr)) != NULL);
        zl = ziplistCreateInstance->ziplistInsert(zl,sptr,(unsigned char*)scorebuf,scorelen);
    }
    return zl;
}

/**
 * 比较压缩列表中的元素与指定字符串
 * 
 * @param eptr      指向压缩列表中元素的指针
 * @param cstr      要比较的目标字符串
 * @param clen      目标字符串的长度
 * @return          比较结果：0表示相等，非0表示不相等
 */
int zsetCreate::zzlCompareElements(unsigned char *eptr, unsigned char *cstr, unsigned int clen) 
{
    unsigned char *vstr;
    unsigned int vlen;
    long long vlong;
    unsigned char vbuf[32];
    int minlen, cmp;

    //serverAssert(ziplistGet(eptr,&vstr,&vlen,&vlong));
    if (vstr == NULL) {
        /* Store string representation of long long in buf. */
        vlen = toolFuncInstance->ll2string((char*)vbuf,sizeof(vbuf),vlong);
        vstr = vbuf;
    }

    minlen = (vlen < clen) ? vlen : clen;
    cmp = memcmp(vstr,cstr,minlen);
    if (cmp == 0) return vlen-clen;
    return cmp;
}
/**
 * 将压缩列表中的字符串转换为double类型数值
 * 
 * @param vstr      指向字符串值的指针
 * @param vlen      字符串的长度
 * @return          转换后的double值
 */
double zsetCreate::zzlStrtod(unsigned char *vstr, unsigned int vlen) 
{
    char buf[128];
    if (vlen > sizeof(buf))
        vlen = sizeof(buf);
    memcpy(buf,vstr,vlen);
    buf[vlen] = '\0';
    return strtod(buf,NULL);
 }

 /* Returns if there is a part of the zset is in range. Should only be used
 * internally by zzlFirstInRange and zzlLastInRange. */
/**
 * 检查压缩列表中的元素是否在指定范围内
 * 
 * @param zl        指向压缩列表的指针
 * @param range     范围规范结构体指针
 * @return          1表示元素在范围内，0表示不在范围内
 */
int zsetCreate::zzlIsInRange(unsigned char *zl, zrangespec *range) 
{
    unsigned char *p;
    double score;

    /* Test for ranges that will always be empty. */
    if (range->min > range->max ||
            (range->min == range->max && (range->minex || range->maxex)))
        return 0;

    p = ziplistCreateInstance->ziplistIndex(zl,-1); /* Last score. */
    if (p == NULL) return 0; /* Empty sorted set */
    score = zzlGetScore(p);
    if (!zskiplistCreateInstance->zslValueGteMin(score,range))
        return 0;

    p = ziplistCreateInstance->ziplistIndex(zl,1); /* First score. */
    //serverAssert(p != NULL);
    score = zzlGetScore(p);
    if (!zskiplistCreateInstance->zslValueLteMax(score,range))
        return 0;

    return 1;
}

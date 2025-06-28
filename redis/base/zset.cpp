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
#include <cmath>
zsetCreate::zsetCreate()
{
    sdsCreateInstance = static_cast<sdsCreate *>(zmalloc(sizeof(sdsCreate)));
    //serverAssert(sdsCreateInstance != NULL);
    ziplistCreateInstance = static_cast<ziplistCreate *>(zmalloc(sizeof(ziplistCreate)));
    //serverAssert(sdsCreateInstance != NULL && ziplistCreateInstance != NULL);
    toolFuncInstance = static_cast<toolFunc *>(zmalloc(sizeof(toolFunc)));
    //serverAssert(toolFuncInstance != NULL);
    zskiplistCreateInstance = static_cast<zskiplistCreate *>(zmalloc(sizeof(zskiplistCreate)));
    //serverAssert(zskiplistCreateInstance != NULL);
    dictionaryCreateInstance = static_cast<dictionaryCreate *>(zmalloc(sizeof(dictionaryCreate)));
    //serverAssert(dictionaryCreateInstance != NULL);
    redisObjectCreateInstance = static_cast<redisObjectCreate *>(zmalloc(sizeof(redisObjectCreate)));
    //serverAssert(redisObjectCreateInstance != NULL);
}

zsetCreate::~zsetCreate()
{
    zfree(sdsCreateInstance);
    zfree(ziplistCreateInstance);
    zfree(toolFuncInstance);
    zfree(zskiplistCreateInstance);
    zfree(dictionaryCreateInstance);
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
        //serverAssert(sptr != NULL);    //delete by zhenjia.zhao
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
    //serverAssert(ziplistGet(sptr,&vstr,&vlen,&vlong));    //delete by zhenjia.zhao

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
    //serverAssert(*eptr != NULL && *sptr != NULL);    //delete by zhenjia.zhao

    _eptr = ziplistCreateInstance->ziplistNext(zl,*sptr);
    if (_eptr != NULL) {
        _sptr = ziplistCreateInstance->ziplistNext(zl,_eptr);
        //serverAssert(_sptr != NULL);    //delete by zhenjia.zhao
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
    //serverAssert(*eptr != NULL && *sptr != NULL);    //delete by zhenjia.zhao

    _sptr = ziplistCreateInstance->ziplistPrev(zl,*eptr);
    if (_sptr != NULL) {
        _eptr = ziplistCreateInstance->ziplistPrev(zl,_sptr);
        //serverAssert(_eptr != NULL);    //delete by zhenjia.zhao
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
        //serverAssert(sptr != NULL);    //delete by zhenjia.zhao

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
        //serverAssert(sptr != NULL);    //delete by zhenjia.zhao

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
        //serverAssert((sptr = ziplistNext(zl,eptr)) != NULL);    //delete by zhenjia.zhao
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
    //serverAssert(p != NULL);    //delete by zhenjia.zhao
    score = zzlGetScore(p);
    if (!zskiplistCreateInstance->zslValueLteMax(score,range))
        return 0;

    return 1;
}

/**
 * 获取有序集合的元素数量
 * @param zobj 有序集合对象指针
 * @return 返回元素数量
 */
unsigned long zsetCreate::zsetLength(const robj *zobj)
{
    unsigned long length = 0;
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        length = zzlLength(static_cast<unsigned char*>(zobj->ptr));
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        length = ((const zset*)zobj->ptr)->zsl->length;
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
    return length;
}

/**
 * 转换有序集合的编码方式（如压缩列表与跳跃表互转）
 * @param zobj 有序集合对象指针
 * @param encoding 目标编码类型（OBJ_ENCODING_ZIPLIST 或 OBJ_ENCODING_SKIPLIST）
 */
void zsetCreate::zsetConvert(robj *zobj, int encoding)
{
    zset *zs;
    zskiplistNode *node, *next;
    sds ele;
    double score;

    if (zobj->encoding == encoding) return;
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        unsigned char *zl = static_cast<unsigned char*>(zobj->ptr);
        unsigned char *eptr, *sptr;
        unsigned char *vstr;
        unsigned int vlen;
        long long vlong;

        if (encoding != OBJ_ENCODING_SKIPLIST)
            //serverPanic("Unknown target encoding");    //delete by zhenjia.zhao

        zs = static_cast<zset *>(zmalloc(sizeof(*zs)));
        zs->dictl = dictionaryCreateInstance->dictCreate(&zsetDictType,NULL);
        zs->zsl = zskiplistCreateInstance->zslCreate();

        eptr = ziplistCreateInstance->ziplistIndex(zl,0);
        //serverAssertWithInfo(NULL,zobj,eptr != NULL);    //delete by zhenjia.zhao
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssertWithInfo(NULL,zobj,sptr != NULL);    //delete by zhenjia.zhao

        while (eptr != NULL) {
            score = zzlGetScore(sptr);
            //serverAssertWithInfo(NULL,zobj,ziplistGet(eptr,&vstr,&vlen,&vlong));    //delete by zhenjia.zhao
            if (vstr == NULL)
                ele = sdsCreateInstance->sdsfromlonglong(vlong);
            else
                ele = sdsCreateInstance->sdsnewlen((char*)vstr,vlen);

            node = zskiplistCreateInstance->zslInsert(zs->zsl,score,ele);
            //serverAssert(dictAdd(zs->dict,ele,&node->score) == DICT_OK);    //delete by zhenjia.zhao
            zzlNext(zl,&eptr,&sptr);
        }

        zfree(zobj->ptr);
        zobj->ptr = zs;
        zobj->encoding = OBJ_ENCODING_SKIPLIST;
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        unsigned char *zl = ziplistCreateInstance->ziplistNew();

        if (encoding != OBJ_ENCODING_ZIPLIST)
            //serverPanic("Unknown target encoding");    //delete by zhenjia.zhao

        /* Approach similar to zslFree(), since we want to free the skiplist at
         * the same time as creating the ziplist. */
        zs = static_cast<zset *>(zobj->ptr);
        dictionaryCreateInstance->dictRelease(zs->dictl);
        node = zs->zsl->header->level[0].forward;
        zfree(zs->zsl->header);
        zfree(zs->zsl);

        while (node) {
            zl = zzlInsertAt(zl,NULL,node->ele,node->score);
            next = node->level[0].forward;
            zskiplistCreateInstance->zslFreeNode(node);
            node = next;
        }

        zfree(zs);
        zobj->ptr = zl;
        zobj->encoding = OBJ_ENCODING_ZIPLIST;
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
}

/**
 * 当满足条件时自动将有序集合转换为压缩列表编码
 * @param zobj 有序集合对象指针
 * @param maxelelen 元素最大长度阈值
 * @param totelelen 集合总长度阈值
 */
void zsetCreate::zsetConvertToZiplistIfNeeded(robj *zobj, size_t maxelelen, size_t totelelen)
{
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) return;
    zset *zsetl =static_cast<zset*>(zobj->ptr);
    //delete by zhenjia.zhao 
    // if (zsetl->zsl->length <= server.zset_max_ziplist_entries &&
    //     maxelelen <= server.zset_max_ziplist_value &&
    //     ziplistCreateInstance->ziplistSafeToAdd(NULL, totelelen))
    // {
    //     zsetConvert(zobj,OBJ_ENCODING_ZIPLIST);
    // }
}

/**
 * 获取有序集合中成员的分数
 * @param zobj 有序集合对象指针
 * @param member 待查询的成员名称
 * @param score 输出参数，存储查询到的分数值
 * @return 成员存在返回1，不存在返回0
 */
int zsetCreate::zsetScore(robj *zobj, sds member, double *score)
{
    if (!zobj || !member) return C_ERR;

    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        if (zzlFind(static_cast<unsigned char*>(zobj->ptr), member, score) == NULL) return C_ERR;
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs =static_cast<zset*> (zobj->ptr);
        dictEntry *de = dictionaryCreateInstance->dictFind(zs->dictl, member);
        if (de == NULL) return C_ERR;
        *score = *(double*)dictGetVal(de);
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
    return C_OK;
}

/**
 * 向有序集合中添加或更新成员的分数
 * @param zobj 有序集合对象指针
 * @param score 新分数值
 * @param ele 成员名称
 * @param in_flags 输入标志（如ZADD_IN_FLAG_NX表示不更新已存在成员）
 * @param out_flags 输出标志（如ZADD_OUT_FLAG_ADDED表示成功添加）
 * @param newscore 输出参数，存储最终生效的分数值
 * @return 操作成功返回1，失败返回0
 */
int zsetCreate::zsetAdd(robj *zobj, double score, sds ele, int in_flags, int *out_flags, double *newscore)
{
    /* Turn options into simple to check vars. */
    int incr = (in_flags & ZADD_IN_INCR) != 0;
    int nx = (in_flags & ZADD_IN_NX) != 0;
    int xx = (in_flags & ZADD_IN_XX) != 0;
    int gt = (in_flags & ZADD_IN_GT) != 0;
    int lt = (in_flags & ZADD_IN_LT) != 0;
    *out_flags = 0; /* We'll return our response flags. */
    double curscore;

    /* NaN as input is an error regardless of all the other parameters. */
    if (std::isnan(score)) {
        *out_flags = ZADD_OUT_NAN;
        return 0;
    }

    /* Update the sorted set according to its encoding. */
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        unsigned char *eptr;

        if ((eptr = zzlFind(static_cast<unsigned char *>(zobj->ptr),ele,&curscore)) != NULL) {
            /* NX? Return, same element already exists. */
            if (nx) {
                *out_flags |= ZADD_OUT_NOP;
                return 1;
            }

            /* Prepare the score for the increment if needed. */
            if (incr) {
                score += curscore;
                if (std::isnan(score)) {
                    *out_flags |= ZADD_OUT_NAN;
                    return 0;
                }
            }

            /* GT/LT? Only update if score is greater/less than current. */
            if ((lt && score >= curscore) || (gt && score <= curscore)) {
                *out_flags |= ZADD_OUT_NOP;
                return 1;
            }

            if (newscore) *newscore = score;

            /* Remove and re-insert when score changed. */
            if (score != curscore) {
                zobj->ptr = zzlDelete( static_cast<unsigned char*>(zobj->ptr),eptr);
                zobj->ptr = zzlInsert(static_cast<unsigned char*>(zobj->ptr),ele,score);
                *out_flags |= ZADD_OUT_UPDATED;
            }
            return 1;
        } else if (!xx) {
            /* check if the element is too large or the list
             * becomes too long *before* executing zzlInsert. */
            // if (zzlLength( static_cast<unsigned char*>(zobj->ptr))+1 > server.zset_max_ziplist_entries ||
            //     sdsCreateInstance->sdslen(ele) > server.zset_max_ziplist_value ||
            //     !ziplistCreateInstance->ziplistSafeToAdd(static_cast<unsigned char*>(zobj->ptr), sdsCreateInstance->sdslen(ele)))
            //delete by zhenjia.zhao 
            if (!ziplistCreateInstance->ziplistSafeToAdd(static_cast<unsigned char*>(zobj->ptr), sdsCreateInstance->sdslen(ele)))
            {
                zsetConvert(zobj,OBJ_ENCODING_SKIPLIST);
            } 
            else 
            {
                zobj->ptr = zzlInsert(static_cast<unsigned char*>(zobj->ptr),ele,score);
                if (newscore) *newscore = score;
                *out_flags |= ZADD_OUT_ADDED;
                return 1;
            }
        } else {
            *out_flags |= ZADD_OUT_NOP;
            return 1;
        }
    }

    /* Note that the above block handling ziplist would have either returned or
     * converted the key to skiplist. */
    if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs =static_cast<zset*>(zobj->ptr);
        zskiplistNode *znode;
        dictEntry *de;

        de = dictionaryCreateInstance->dictFind(zs->dictl,ele);
        if (de != NULL) {
            /* NX? Return, same element already exists. */
            if (nx) {
                *out_flags |= ZADD_OUT_NOP;
                return 1;
            }

            curscore = *(double*)dictGetVal(de);

            /* Prepare the score for the increment if needed. */
            if (incr) {
                score += curscore;
                if (std::isnan(score)) {
                    *out_flags |= ZADD_OUT_NAN;
                    return 0;
                }
            }

            /* GT/LT? Only update if score is greater/less than current. */
            if ((lt && score >= curscore) || (gt && score <= curscore)) {
                *out_flags |= ZADD_OUT_NOP;
                return 1;
            }

            if (newscore) *newscore = score;

            /* Remove and re-insert when score changes. */
            if (score != curscore) {
                znode = zslUpdateScore(zs->zsl,curscore,ele,score);
                /* Note that we did not removed the original element from
                 * the hash table representing the sorted set, so we just
                 * update the score. */
                dictGetVal(de) = &znode->score; /* Update score ptr. */
                *out_flags |= ZADD_OUT_UPDATED;
            }
            return 1;
        } else if (!xx) {
            ele = sdsCreateInstance->sdsdup(ele);
            znode = zskiplistCreateInstance->zslInsert(zs->zsl,score,ele);
            //serverAssert(dictAdd(zs->dict,ele,&znode->score) == DICT_OK);    //delete by zhenjia.zhao
            *out_flags |= ZADD_OUT_ADDED;
            if (newscore) *newscore = score;
            return 1;
        } else {
            *out_flags |= ZADD_OUT_NOP;
            return 1;
        }
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
    return 0; /* Never reached. */
}

/**
 * 获取有序集合中成员的排名（支持升序/降序）
 * @param zobj 有序集合对象指针
 * @param ele 成员名称
 * @param reverse 排序方向（1表示降序，0表示升序）
 * @return 返回成员排名（从0开始），成员不存在返回-1
 */
long zsetCreate::zsetRank(robj *zobj, sds ele, int reverse)
{
    unsigned long llen;
    unsigned long rank;

    llen = zsetLength(zobj);

    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        unsigned char *zl =static_cast<unsigned char*>(zobj->ptr);
        unsigned char *eptr, *sptr;

        eptr = ziplistCreateInstance->ziplistIndex(zl,0);
        //serverAssert(eptr != NULL);    //delete by zhenjia.zhao
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssert(sptr != NULL);    //delete by zhenjia.zhao

        rank = 1;
        while(eptr != NULL) {
            if (ziplistCreateInstance->ziplistCompare(eptr,(unsigned char*)ele,sdsCreateInstance->sdslen(ele)))
                break;
            rank++;
            zzlNext(zl,&eptr,&sptr);
        }

        if (eptr != NULL) {
            if (reverse)
                return llen-rank;
            else
                return rank-1;
        } else {
            return -1;
        }
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs =static_cast<zset*>(zobj->ptr);
        zskiplist *zsl = zs->zsl;
        dictEntry *de;
        double score;

        de = dictionaryCreateInstance->dictFind(zs->dictl,ele);
        if (de != NULL) {
            score = *(double*)dictGetVal(de);
            rank = zskiplistCreateInstance->zslGetRank(zsl,score,ele);
            /* Existing elements always have a rank. */
            //serverAssert(rank != 0);    //delete by zhenjia.zhao
            if (reverse)
                return llen-rank;
            else
                return rank-1;
        } else {
            return -1;
        }
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
}

/**
 * 从有序集合中删除成员
 * @param zobj 有序集合对象指针
 * @param ele 待删除的成员名称
 * @return 成功删除返回1，成员不存在返回0
 */
int zsetCreate::zsetDel(robj *zobj, sds ele)
{
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST) {
        unsigned char *eptr;

        if ((eptr = zzlFind(static_cast<unsigned char*>(zobj->ptr),ele,NULL)) != NULL) {
            zobj->ptr = zzlDelete(static_cast<unsigned char*> (zobj->ptr),eptr);
            return 1;
        }
    } else if (zobj->encoding == OBJ_ENCODING_SKIPLIST) {
        zset *zs = static_cast<zset*>(zobj->ptr);
        if (zsetRemoveFromSkiplist(zs, ele)) {
            if (htNeedsResize(zs->dictl)) dictionaryCreateInstance->dictResize(zs->dictl);
            return 1;
        }
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
    return 0; /* No such element found. */
}

/**
 * 复制有序集合对象
 * @param o 源有序集合对象指针
 * @return 返回新复制的有序集合对象指针
 */
robj *zsetCreate::zsetDup(robj *o)
{
    robj *zobj;
    zset *zs;
    zset *new_zs;

    //serverAssert(o->type == OBJ_ZSET);    //delete by zhenjia.zhao

    /* Create a new sorted set object that have the same encoding as the original object's encoding */
    if (o->encoding == OBJ_ENCODING_ZIPLIST) 
    {
        unsigned char *zl = static_cast<unsigned char*>(o->ptr);
        size_t sz = ziplistCreateInstance->ziplistBlobLen(zl);
        unsigned char *new_zl = static_cast<unsigned char*>(zmalloc(sz));
        memcpy(new_zl, zl, sz);
        zobj = redisObjectCreateInstance->createObject(OBJ_ZSET, new_zl);
        zobj->encoding = OBJ_ENCODING_ZIPLIST;
    } 
    else if (o->encoding == OBJ_ENCODING_SKIPLIST) 
    {
        zobj = redisObjectCreateInstance->createZsetObject();
        zs = static_cast<zset*>(o->ptr);
        new_zs =static_cast<zset*>(zobj->ptr);
        dictionaryCreateInstance->dictExpand(new_zs->dictl,dictSize(zs->dictl));
        zskiplist *zsl = zs->zsl;
        zskiplistNode *ln;
        sds ele;
        long llen = zsetLength(o);

        /* We copy the skiplist elements from the greatest to the
         * smallest (that's trivial since the elements are already ordered in
         * the skiplist): this improves the load process, since the next loaded
         * element will always be the smaller, so adding to the skiplist
         * will always immediately stop at the head, making the insertion
         * O(1) instead of O(log(N)). */
        ln = zsl->tail;
        while (llen--) {
            ele = ln->ele;
            sds new_ele = sdsCreateInstance->sdsdup(ele);
            zskiplistNode *znode = zskiplistCreateInstance->zslInsert(new_zs->zsl,ln->score,new_ele);
            dictionaryCreateInstance->dictAdd(new_zs->dictl,new_ele,&znode->score);
            ln = ln->backward;
        }
    } else {
        //serverPanic("Unknown sorted set encoding");    //delete by zhenjia.zhao
    }
    return zobj;
}
/**
 * Redis有序集合(zset)底层实现相关的核心数据结构和函数。
 * 有序集合同时使用哈希表(dict)和跳跃表(zskiplist)实现，
 * 以支持O(1)时间复杂度的成员查找和O(logN)的范围操作。
 */

/**
 * 计算压缩列表(ziplist)的长度。
 * 
 * @param zl 指向压缩列表的指针
 * @return 压缩列表中的元素数量
 */
unsigned int zsetCreate::zzlLength(unsigned char *zl) 
{
    return ziplistCreateInstance->ziplistLen(zl)/2;
}
/**
 * 比较两个SDS字符串键是否相等。
 * 用于字典的键比较回调函数。
 * 
 * @param privdata 私有数据（未使用）
 * @param key1 第一个键
 * @param key2 第二个键
 * @return 相等返回1，否则返回0
 */
int zsetCreate::dictSdsKeyCompare(void *privdata, const void *key1,const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);
    sdsCreate sdsCreateInst;
    l1 = sdsCreateInst.sdslen((sds)key1);
    l2 = sdsCreateInst.sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}
/**
 * 计算SDS字符串的哈希值。
 * 用于字典的哈希函数回调。
 * 
 * @param key 指向SDS字符串的指针
 * @return 计算得到的哈希值
 */
uint64_t zsetCreate::dictSdsHash(const void *key) 
{
    sdsCreate sdsCreateInst;
    dictionaryCreate dictionaryCreateInst;
    return dictionaryCreateInst.dictGenHashFunction((unsigned char*)key, sdsCreateInst.sdslen((char*)key));
}
/**
 * 在压缩列表中查找指定元素。
 * 
 * @param zl 指向压缩列表的指针
 * @param ele 要查找的元素(SDS字符串)
 * @param score 用于存储找到元素的分数(如果不为NULL)
 * @return 指向元素的指针，如果未找到则返回NULL
 */
unsigned char *zsetCreate::zzlFind(unsigned char *zl, sds ele, double *score) 
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,0), *sptr;

    while (eptr != NULL) {
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr);
        //serverAssert(sptr != NULL);    //delete by zhenjia.zhao

        if (ziplistCreateInstance->ziplistCompare(eptr,(unsigned char*)ele,sdsCreateInstance->sdslen(ele))) {
            /* Matching element, pull out score. */
            if (score != NULL) *score = zzlGetScore(sptr);
            return eptr;
        }

        /* Move to next element. */
        eptr = ziplistCreateInstance->ziplistNext(zl,sptr);
    }
    return NULL;
}
/**
 * 从压缩列表中删除指定元素。
 * 
 * @param zl 指向压缩列表的指针
 * @param eptr 指向要删除元素的指针
 * @return 删除元素后的压缩列表指针
 */
/* Delete (element,score) pair from ziplist. Use local copy of eptr because we
 * don't want to modify the one given as argument. */
unsigned char *zsetCreate::zzlDelete(unsigned char *zl, unsigned char *eptr) 
{
    unsigned char *p = eptr;

    /* TODO: add function to ziplist API to delete N elements from offset. */
    zl = ziplistCreateInstance->ziplistDelete(zl,&p);
    zl = ziplistCreateInstance->ziplistDelete(zl,&p);
    return zl;
}

/* Update the score of an element inside the sorted set skiplist.
 * Note that the element must exist and must match 'score'.
 * This function does not update the score in the hash table side, the
 * caller should take care of it.
 *
 * Note that this function attempts to just update the node, in case after
 * the score update, the node would be exactly at the same position.
 * Otherwise the skiplist is modified by removing and re-adding a new
 * element, which is more costly.
 *
 * The function returns the updated element skiplist node pointer. */
/**
 * 更新跳跃表中元素的分数。
 * 如果分数变化导致元素排序位置改变，会调整元素在跳跃表中的位置。
 * 
 * @param zsl 指向跳跃表的指针
 * @param curscore 当前分数
 * @param ele 元素(SDS字符串)
 * @param newscore 新分数
 * @return 更新后的跳跃表节点指针
 */
zskiplistNode *zsetCreate::zslUpdateScore(zskiplist *zsl, double curscore, sds ele, double newscore) 
{
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i;

    /* We need to seek to element to update to start: this is useful anyway,
     * we'll have to update or remove it. */
    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                (x->level[i].forward->score < curscore ||
                    (x->level[i].forward->score == curscore &&
                     sdsCreateInstance->sdscmp(x->level[i].forward->ele,ele) < 0)))
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }

    /* Jump to our element: note that this function assumes that the
     * element with the matching score exists. */
    x = x->level[0].forward;
    //serverAssert(x && curscore == x->score && sdscmp(x->ele,ele) == 0);

    /* If the node, after the score update, would be still exactly
     * at the same position, we can just update the score without
     * actually removing and re-inserting the element in the skiplist. */
    if ((x->backward == NULL || x->backward->score < newscore) &&
        (x->level[0].forward == NULL || x->level[0].forward->score > newscore))
    {
        x->score = newscore;
        return x;
    }

    /* No way to reuse the old node: we need to remove and insert a new
     * one at a different place. */
    zskiplistCreateInstance->zslDeleteNode(zsl, x, update);
    zskiplistNode *newnode = zskiplistCreateInstance->zslInsert(zsl,newscore,x->ele);
    /* We reused the old node x->ele SDS string, free the node now
     * since zslInsert created a new one. */
    x->ele = NULL;
    zskiplistCreateInstance->zslFreeNode(x);
    return newnode;
}

/* Deletes the element 'ele' from the sorted set encoded as a skiplist+dict,
 * returning 1 if the element existed and was deleted, 0 otherwise (the
 * element was not there). It does not resize the dict after deleting the
 * element. */
/**
 * 从有序集合中删除指定元素。
 * 
 * @param zs 指向有序集合的指针
 * @param ele 要删除的元素(SDS字符串)
 * @return 成功删除返回1，未找到元素返回0
 */
int zsetCreate::zsetRemoveFromSkiplist(zset *zs, sds ele) 
{
    dictEntry *de;
    double score;

    de = dictionaryCreateInstance->dictUnlink(zs->dictl,ele);
    if (de != NULL) {
        /* Get the score in order to delete from the skiplist later. */
        score = *(double*)dictGetVal(de);

        /* Delete from the hash table and later from the skiplist.
         * Note that the order is important: deleting from the skiplist
         * actually releases the SDS string representing the element,
         * which is shared between the skiplist and the hash table, so
         * we need to delete from the skiplist as the final step. */
        dictionaryCreateInstance->dictFreeUnlinkedEntry(zs->dictl,de);

        /* Delete from skiplist. */
        int retval = zskiplistCreateInstance->zslDelete(zs->zsl,score,ele,NULL);
        //serverAssert(retval);    //delete by zhenjia.zhao

        return 1;
    }

    return 0;
}
/**
 * 判断字典是否需要调整大小。
 * 
 * @param dict 指向字典的指针
 * @return 如果需要调整返回1，否则返回0
 */
int zsetCreate::htNeedsResize(dict *dict) 
{
    long long size, used;

    size = dictSlots(dict);
    used = dictSize(dict);
    return (size > DICT_HT_INITIAL_SIZE &&
            (used*100/size < HASHTABLE_MIN_FILL));
}

/**
 * 解析字典序范围参数（如"[min" "[max"）
 * @param min 最小值对象（字符串）
 * @param max 最大值对象（字符串）
 * @param spec 输出参数，存储解析后的范围规范结构体
 * @return 解析成功返回1，参数格式错误返回0
 */
int zsetCreate::zslParseLexRange(robj *min, robj *max, zlexrangespec *spec)
{
    /* The range can't be valid if objects are integer encoded.
     * Every item must start with ( or [. */
    if (min->encoding == OBJ_ENCODING_INT ||
        max->encoding == OBJ_ENCODING_INT) return C_ERR;

    spec->min = spec->max = NULL;
    if (zslParseLexRangeItem(min, &spec->min, &spec->minex) == C_ERR ||
        zslParseLexRangeItem(max, &spec->max, &spec->maxex) == C_ERR)
    {
        zslFreeLexRange(spec);
        return C_ERR;
    } 
    else
    {
        return C_OK;
    }
}

/**
 * 释放字典序范围规范结构体占用的内存
 * @param spec 待释放的范围规范结构体指针
 */
void zsetCreate::zslFreeLexRange(zlexrangespec *spec)
{
    if (spec->min != redisObjectCreateInstance->shared.minstring &&
        spec->min != redisObjectCreateInstance->shared.maxstring) sdsCreateInstance->sdsfree(spec->min);
    if (spec->max != redisObjectCreateInstance->shared.minstring &&
        spec->max != redisObjectCreateInstance->shared.maxstring) sdsCreateInstance->sdsfree(spec->max);
}

/**
 * 获取跳跃表中字典序范围内的第一个节点
 * @param zsl 目标跳跃表指针
 * @param range 字典序范围规范结构体
 * @return 返回符合条件的第一个节点指针，若无匹配则返回NULL
 */
zskiplistNode *zsetCreate::zslFirstInLexRange(zskiplist *zsl, zlexrangespec *range)
{
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInLexRange(zsl,range)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward &&
            !zslLexValueGteMin(x->level[i].forward->ele,range))
                x = x->level[i].forward;
    }

    /* This is an inner range, so the next node cannot be NULL. */
    x = x->level[0].forward;
    //serverAssert(x != NULL);

    /* Check if score <= max. */
    if (!zslLexValueLteMax(x->ele,range)) return NULL;
    return x;
}

/**
 * 获取跳跃表中字典序范围内的最后一个节点
 * @param zsl 目标跳跃表指针
 * @param range 字典序范围规范结构体
 * @return 返回符合条件的最后一个节点指针，若无匹配则返回NULL
 */
zskiplistNode *zsetCreate::zslLastInLexRange(zskiplist *zsl, zlexrangespec *range)
{
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInLexRange(zsl,range)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward &&
            zslLexValueLteMax(x->level[i].forward->ele,range))
                x = x->level[i].forward;
    }

    /* This is an inner range, so this node cannot be NULL. */
    //serverAssert(x != NULL);

    /* Check if score >= min. */
    if (!zslLexValueGteMin(x->ele,range)) return NULL;
    return x;
}

/**
 * 获取压缩列表中字典序范围内的第一个元素
 * @param zl 压缩列表指针
 * @param range 字典序范围规范结构体
 * @return 返回指向第一个符合条件元素的指针，若无匹配则返回NULL
 */
unsigned char *zsetCreate::zzlFirstInLexRange(unsigned char *zl, zlexrangespec *range)
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,0), *sptr;

    /* If everything is out of range, return early. */
    if (!zzlIsInLexRange(zl,range)) return NULL;

    while (eptr != NULL) {
        if (zzlLexValueGteMin(eptr,range)) {
            /* Check if score <= max. */
            if (zzlLexValueLteMax(eptr,range))
                return eptr;
            return NULL;
        }

        /* Move to next element. */
        sptr = ziplistCreateInstance->ziplistNext(zl,eptr); /* This element score. Skip it. */
        //serverAssert(sptr != NULL);
        eptr = ziplistCreateInstance->ziplistNext(zl,sptr); /* Next element. */
    }

    return NULL;
}

/**
 * 获取压缩列表中字典序范围内的最后一个元素
 * @param zl 压缩列表指针
 * @param range 字典序范围规范结构体
 * @return 返回指向最后一个符合条件元素的指针，若无匹配则返回NULL
 */
unsigned char *zsetCreate::zzlLastInLexRange(unsigned char *zl, zlexrangespec *range)
{
    unsigned char *eptr = ziplistCreateInstance->ziplistIndex(zl,-2), *sptr;

    /* If everything is out of range, return early. */
    if (!zzlIsInLexRange(zl,range)) return NULL;

    while (eptr != NULL) {
        if (zzlLexValueLteMax(eptr,range)) {
            /* Check if score >= min. */
            if (zzlLexValueGteMin(eptr,range))
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
 * 判断元素是否大于等于字典序最小值
 * @param value 待判断的元素字符串
 * @param spec 字典序范围规范结构体
 * @return 满足条件返回1，否则返回0
 */
int zsetCreate::zslLexValueGteMin(sds value, zlexrangespec *spec)
{
    return spec->minex ?
        (sdscmplex(value,spec->min) > 0) :
        (sdscmplex(value,spec->min) >= 0);
}

/**
 * 判断元素是否小于等于字典序最大值
 * @param value 待判断的元素字符串
 * @param spec 字典序范围规范结构体
 * @return 满足条件返回1，否则返回0
 */
int zsetCreate::zslLexValueLteMax(sds value, zlexrangespec *spec)
{
    return spec->maxex ?
        (sdscmplex(value,spec->max) < 0) :
        (sdscmplex(value,spec->max) <= 0);
}

//其他辅助函数
/**
 * 验证压缩列表编码的有序集合完整性
 * @param zl 压缩列表指针
 * @param size 压缩列表大小（字节数）
 * @param deep 是否进行深度验证（验证每个元素内容）
 * @return 验证通过返回1，发现错误返回0
 */
int zsetCreate::zsetZiplistValidateIntegrity(unsigned char *zl, size_t size, int deep)
{
    if (!deep)
        return ziplistCreateInstance->ziplistValidateIntegrity(zl, size, 0, NULL, NULL);

    /* Keep track of the field names to locate duplicate ones */
    struct {
        long count;
        dict *fields;
    } data = {0, dictionaryCreateInstance->dictCreate(&hashDictType, NULL)};

    int ret = ziplistCreateInstance->ziplistValidateIntegrity(zl, size, 1, _zsetZiplistValidateIntegrity, &data);

    /* make sure we have an even number of records. */
    if (data.count & 1)
        ret = 0;

    dictionaryCreateInstance->dictRelease(data.fields);
    return ret;
}

/**
 * 处理ZPOP系列命令（如ZPOPMIN/ZPOPMAX）
 * @param c 客户端连接对象
 * @param keyv 键名数组
 * @param keyc 键名数量
 * @param where 弹出方向（ZPOP_MIN或ZPOP_MAX）
 * @param emitkey 是否返回键名（用于多键操作）
 * @param countarg 弹出数量参数对象
 */
// void zsetCreate::genericZpopCommand(client *c, robj **keyv, int keyc, int where, int emitkey, robj *countarg)
// {

// }

/**
 * 从压缩列表中提取元素对象
 * @param sptr 指向元素存储位置的指针
 * @return 返回解析出的字符串对象
 */
sds zsetCreate::ziplistGetObject(unsigned char *sptr)
{
    unsigned char *vstr;
    unsigned int vlen;
    long long vlong;

    //serverAssert(sptr != NULL);
    //serverAssert(ziplistGet(sptr,&vstr,&vlen,&vlong));

    if (vstr) {
        return sdsCreateInstance->sdsnewlen((char*)vstr,vlen);
    } else {
        return sdsCreateInstance->sdsfromlonglong(vlong);
    }
}

/**
 * 解析有序集合的字典序范围项
 * @param item 待解析的对象
 * @param dest 输出参数，存储解析后的SDS字符串
 * @param ex 输出参数，存储是否为排他范围标记
 * @return 成功返回1，失败返回0
 */
int zsetCreate::zslParseLexRangeItem(robj *item, sds *dest, int *ex) 
{
    char *c =  static_cast<char*>(item->ptr);

    switch(c[0]) {
    case '+':
        if (c[1] != '\0') return C_ERR;
        *ex = 1;
        *dest = redisObjectCreateInstance->shared.maxstring;
        return C_OK;
    case '-':
        if (c[1] != '\0') return C_ERR;
        *ex = 1;
        *dest = redisObjectCreateInstance->shared.minstring;
        return C_OK;
    case '(':
        *ex = 1;
        *dest = sdsCreateInstance->sdsnewlen(c+1,sdsCreateInstance->sdslen(c)-1);
        return C_OK;
    case '[':
        *ex = 0;
        *dest = sdsCreateInstance->sdsnewlen(c+1,sdsCreateInstance->sdslen(c)-1);
        return C_OK;
    default:
        return C_ERR;
    }
}

/**
 * 判断有序集合节点是否在指定字典序范围内
 * @param zsl 有序集合指针
 * @param range 字典序范围规范
 * @return 若在范围内返回1，否则返回0
 */
/* Returns if there is a part of the zset is in the lex range. */
int zsetCreate::zslIsInLexRange(zskiplist *zsl, zlexrangespec *range) 
{
    zskiplistNode *x;

    /* Test for ranges that will always be empty. */
    int cmp = sdscmplex(range->min,range->max);
    if (cmp > 0 || (cmp == 0 && (range->minex || range->maxex)))
        return 0;
    x = zsl->tail;
    if (x == NULL || !zslLexValueGteMin(x->ele,range))
        return 0;
    x = zsl->header->level[0].forward;
    if (x == NULL || !zslLexValueLteMax(x->ele,range))
        return 0;
    return 1;
}
/**
 * 按字典序比较两个SDS字符串
 * @param a 第一个SDS字符串
 * @param b 第二个SDS字符串
 * @return 比较结果：a<b返回负数，a==b返回0，a>b返回正数
 */
/* This is just a wrapper to sdscmp() that is able to
 * handle shared.minstring and shared.maxstring as the equivalent of
 * -inf and +inf for strings */
int zsetCreate::sdscmplex(sds a, sds b) 
{
    if (a == b) return 0;
    if (a == redisObjectCreateInstance->shared.minstring || b == redisObjectCreateInstance->shared.maxstring) return -1;
    if (a == redisObjectCreateInstance->shared.maxstring || b == redisObjectCreateInstance->shared.minstring) return 1;
    return sdsCreateInstance->sdscmp(a,b);
}

/**
 * 判断压缩列表表示的有序集合是否在字典序范围内
 * @param zl 压缩列表指针
 * @param range 字典序范围规范
 * @return 若在范围内返回1，否则返回0
 */
/* Returns if there is a part of the zset is in range. Should only be used
 * internally by zzlFirstInRange and zzlLastInRange. */
int zsetCreate::zzlIsInLexRange(unsigned char *zl, zlexrangespec *range) 
{
    unsigned char *p;

    /* Test for ranges that will always be empty. */
    int cmp = sdscmplex(range->min,range->max);
    if (cmp > 0 || (cmp == 0 && (range->minex || range->maxex)))
        return 0;

    p = ziplistCreateInstance->ziplistIndex(zl,-2); /* Last element. */
    if (p == NULL) return 0;
    if (!zzlLexValueGteMin(p,range))
        return 0;

    p = ziplistCreateInstance->ziplistIndex(zl,0); /* First element. */
    //serverAssert(p != NULL);
    if (!zzlLexValueLteMax(p,range))
        return 0;

    return 1;
}
/**
 * 检查压缩列表中的元素是否大于等于字典序最小值
 * @param p 压缩列表中元素的指针
 * @param spec 字典序范围规范
 * @return 满足条件返回1，否则返回0
 */
int zsetCreate::zzlLexValueGteMin(unsigned char *p, zlexrangespec *spec) 
{
    sds value = ziplistGetObject(p);
    int res = zslLexValueGteMin(value,spec);
    sdsCreateInstance->sdsfree(value);
    return res;
}
/**
 * 检查压缩列表中的元素是否小于等于字典序最大值
 * @param p 压缩列表中元素的指针
 * @param spec 字典序范围规范
 * @return 满足条件返回1，否则返回0
 */
int zsetCreate::zzlLexValueLteMax(unsigned char *p, zlexrangespec *spec) 
{
    sds value = ziplistGetObject(p);
    int res = zslLexValueLteMax(value,spec);
    sdsCreateInstance->sdsfree(value);
    return res;
}

/**
 * 释放SDS字符串的内存
 * @param privdata 私有数据（未使用）
 * @param val 待释放的SDS字符串指针
 */
void zsetCreate::dictSdsDestructor(void *privdata, void *val)
{
    DICT_NOTUSED(privdata);
    sdsCreate sdsCreateInstancel;
    sdsCreateInstancel.sdsfree(static_cast<char*>(val));
}

/**
 * 验证压缩列表表示的有序集合的完整性
 * 用于内部一致性检查和调试
 * @param p 压缩列表的指针
 * @param userdata 用户数据，包含验证所需的上下文
 * @return 验证通过返回1，否则返回0
 */
int zsetCreate::_zsetZiplistValidateIntegrity(unsigned char *p, void *userdata) 
{
    struct {
        long count;
        dict *fields;
    } *data = static_cast<decltype(data)>(userdata);

    /* Even records are field names, add to dict and check that's not a dup */
    if (((data->count) & 1) == 0) {
        unsigned char *str;
        unsigned int slen;
        long long vll;
        ziplistCreate ziplistCreateInstancel;
        sdsCreate sdsCreateInstancel;
        dictionaryCreate dictionaryCreateInstancel;
        if (!ziplistCreateInstancel.ziplistGet(p, &str, &slen, &vll))
            return 0;
        sds field = str? sdsCreateInstancel.sdsnewlen(str, slen): sdsCreateInstancel.sdsfromlonglong(vll);
        if (dictionaryCreateInstancel.dictAdd(data->fields, field, NULL) != DICT_OK) {
            /* Duplicate, return an error */
            sdsCreateInstancel.sdsfree(field);
            return 0;
        }
    }

    (data->count)++;
    return 1;
}

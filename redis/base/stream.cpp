/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/06
 * All rights reserved. No one may copy or transfer.
 * Description: 久化的、有序的、支持多播的消息队列，适合用于实现消息队列（MQ）系统、事件溯源、实时数据分析等场景
 */
#include "stream.h"
#include "sds.h"
#include "redisObject.h"
#include "zmallocDf.h"
#include "toolFunc.h"
#include "debugDf.h"
#include "listPack.h"
#include <sys/time.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
#define STREAM_ITEM_FLAG_NONE 0             /* No special flags. */
#define STREAM_ITEM_FLAG_DELETED (1<<0)     /* Entry is deleted. Skip it. */
#define STREAM_ITEM_FLAG_SAMEFIELDS (1<<1)  /* Same fields as master entry. */
#define LONG_STR_SIZE      21          /* Bytes needed for long -> str + '\0' */
/* Flags for streamLookupConsumer */
#define SLC_NONE      0
#define SLC_NOCREAT   (1<<0) /* Do not create the consumer if it doesn't exist */
#define SLC_NOREFRESH (1<<1) /* Do not update consumer's seen-time */
#define lpGetInteger(ele) lpGetIntegerIfValid(ele, NULL)

#define STREAM_LISTPACK_MAX_SIZE (1<<30)
#define STREAM_LISTPACK_MAX_PRE_ALLOCATE 4096

#define TRIM_STRATEGY_NONE 0
#define TRIM_STRATEGY_MAXLEN 1
#define TRIM_STRATEGY_MINID 2

/* Return the UNIX time in microseconds */
static long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec) * 1000000;
    ust += tv.tv_usec;
    return ust;
}

/* Return the UNIX time in milliseconds */
static long long mstime(void) { return ustime() / 1000; }

static sdsCreate sdsCreatel;
static raxCreate raxCreatel;
static listPackCreate listPackCreatel;
static toolFunc toolFuncl;
streamCreate::streamCreate()
{
    raxCreateInstance =static_cast<raxCreate*>(zmalloc(sizeof(raxCreate)));
    sdsCreateInstance = static_cast<sdsCreate*>(zmalloc(sizeof(sdsCreate)));
    listPackCreateInstance = static_cast<listPackCreate*>(zmalloc(sizeof(listPackCreate)));
    toolFuncInstance = static_cast<toolFunc*>(zmalloc(sizeof(toolFunc)));
    redisObjectCreateInstance = static_cast<redisObjectCreate*>(zmalloc(sizeof(redisObjectCreate)));
}
streamCreate::~streamCreate()
{
    zfree(raxCreateInstance);
    zfree(sdsCreateInstance);
    zfree(listPackCreateInstance);
    zfree(toolFuncInstance);
    zfree(redisObjectCreateInstance);
}

/**
 * 创建一个新的Stream对象
 * 
 * @return 返回新创建的Stream对象指针
 */
stream *streamCreate::streamNew(void)
{
    stream *s =static_cast<stream*>(zmalloc(sizeof(*s)));
    s->raxl = raxCreateInstance->raxNew();
    s->length = 0;
    s->last_id.ms = 0;
    s->last_id.seq = 0;
    s->cgroups = NULL; /* Created on demand to save memory when not used. */
    return s;
}

/**
 * 释放Stream对象及其关联的所有资源
 * 
 * @param s 要释放的Stream对象指针
 */
void streamCreate::freeStream(stream *s)
{
    raxCreateInstance->raxFreeWithCallback(s->raxl,(void(*)(void*))lpFree);
    if (s->cgroups)
        raxCreateInstance->raxFreeWithCallback(s->cgroups,(void(*)(void*))streamFreeCG);
    zfree(s);
}

/**
 * 获取Stream的消息条目数量
 * 
 * @param subject 存储Stream的Redis对象
 * @return Stream中消息条目的数量
 */
unsigned long streamCreate::streamLength(const robj *subject)
{
    stream *s = static_cast<stream*>(subject->ptr);
    return s->length;
}

/**
 * 将Stream指定范围内的消息回复给客户端
 * 
 * @param c        客户端上下文
 * @param s        Stream对象
 * @param start    起始消息ID（包含）
 * @param end      结束消息ID（包含）
 * @param count    返回的最大消息数量
 * @param rev      是否反向遍历（true表示从最新到最旧）
 * @param group    消费者组（可为NULL）
 * @param consumer 消费者（可为NULL）
 * @param flags    操作标志位
 * @param spi      Stream属性信息（可为NULL）
 * @return 返回回复的消息数量
 */
size_t streamCreate::streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, 
                        size_t count, int rev, streamCG *group, streamConsumer *consumer, 
                        int flags, streamPropInfo *spi)
{
    // void *arraylen_ptr = NULL;
    // size_t arraylen = 0;
    // streamIterator si;
    // int64_t numfields;
    // streamID id;
    // int propagate_last_id = 0;
    // int noack = flags & STREAM_RWR_NOACK;

    // /* If the client is asking for some history, we serve it using a
    //  * different function, so that we return entries *solely* from its
    //  * own PEL. This ensures each consumer will always and only see
    //  * the history of messages delivered to it and not yet confirmed
    //  * as delivered. */
    // if (group && (flags & STREAM_RWR_HISTORY)) {
    //     return streamReplyWithRangeFromConsumerPEL(c,s,start,end,count,
    //                                                consumer);
    // }

    // if (!(flags & STREAM_RWR_RAWENTRIES))
    //     arraylen_ptr = addReplyDeferredLen(c);
    // streamIteratorStart(&si,s,start,end,rev);
    // while(streamIteratorGetID(&si,&id,&numfields)) {
    //     /* Update the group last_id if needed. */
    //     if (group && streamCompareID(&id,&group->last_id) > 0) {
    //         group->last_id = id;
    //         /* Group last ID should be propagated only if NOACK was
    //          * specified, otherwise the last id will be included
    //          * in the propagation of XCLAIM itself. */
    //         if (noack) propagate_last_id = 1;
    //     }

    //     /* Emit a two elements array for each item. The first is
    //      * the ID, the second is an array of field-value pairs. */
    //     addReplyArrayLen(c,2);
    //     addReplyStreamID(c,&id);

    //     addReplyArrayLen(c,numfields*2);

    //     /* Emit the field-value pairs. */
    //     while(numfields--) {
    //         unsigned char *key, *value;
    //         int64_t key_len, value_len;
    //         streamIteratorGetField(&si,&key,&value,&key_len,&value_len);
    //         addReplyBulkCBuffer(c,key,key_len);
    //         addReplyBulkCBuffer(c,value,value_len);
    //     }

    //     /* If a group is passed, we need to create an entry in the
    //      * PEL (pending entries list) of this group *and* this consumer.
    //      *
    //      * Note that we cannot be sure about the fact the message is not
    //      * already owned by another consumer, because the admin is able
    //      * to change the consumer group last delivered ID using the
    //      * XGROUP SETID command. So if we find that there is already
    //      * a NACK for the entry, we need to associate it to the new
    //      * consumer. */
    //     if (group && !noack) {
    //         unsigned char buf[sizeof(streamID)];
    //         streamEncodeID(buf,&id);

    //         /* Try to add a new NACK. Most of the time this will work and
    //          * will not require extra lookups. We'll fix the problem later
    //          * if we find that there is already a entry for this ID. */
    //         streamNACK *nack = streamCreateNACK(consumer);
    //         int group_inserted =
    //             raxTryInsert(group->pel,buf,sizeof(buf),nack,NULL);
    //         int consumer_inserted =
    //             raxTryInsert(consumer->pel,buf,sizeof(buf),nack,NULL);

    //         /* Now we can check if the entry was already busy, and
    //          * in that case reassign the entry to the new consumer,
    //          * or update it if the consumer is the same as before. */
    //         if (group_inserted == 0) {
    //             streamFreeNACK(nack);
    //             nack = raxFind(group->pel,buf,sizeof(buf));
    //             serverAssert(nack != raxNotFound);
    //             raxRemove(nack->consumer->pel,buf,sizeof(buf),NULL);
    //             /* Update the consumer and NACK metadata. */
    //             nack->consumer = consumer;
    //             nack->delivery_time = mstime();
    //             nack->delivery_count = 1;
    //             /* Add the entry in the new consumer local PEL. */
    //             raxInsert(consumer->pel,buf,sizeof(buf),nack,NULL);
    //         } else if (group_inserted == 1 && consumer_inserted == 0) {
    //             serverPanic("NACK half-created. Should not be possible.");
    //         }

    //         /* Propagate as XCLAIM. */
    //         if (spi) {
    //             robj *idarg = createObjectFromStreamID(&id);
    //             streamPropagateXCLAIM(c,spi->keyname,group,spi->groupname,idarg,nack);
    //             decrRefCount(idarg);
    //         }
    //     }

    //     arraylen++;
    //     if (count && count == arraylen) break;
    // }

    // if (spi && propagate_last_id)
    //     streamPropagateGroupID(c,spi->keyname,group,spi->groupname);

    // streamIteratorStop(&si);
    // if (arraylen_ptr) setDeferredArrayLen(c,arraylen_ptr,arraylen);
    // return arraylen;
}

/**
 * 初始化一个Stream迭代器
 * 
 * @param si    迭代器对象
 * @param s     要迭代的Stream对象
 * @param start 起始消息ID（可为NULL，表示从第一个/最后一个消息开始）
 * @param end   结束消息ID（可为NULL，表示到最后一个/第一个消息结束）
 * @param rev   是否反向遍历（true表示从最新到最旧）
 */
void streamCreate::streamIteratorStart(streamIterator *si, stream *s, streamID *start, streamID *end, int rev)
{
    /* Initialize the iterator and translates the iteration start/stop
     * elements into a 128 big big-endian number. */
    if (start) {
        streamEncodeID(si->start_key,start);
    } else {
        si->start_key[0] = 0;
        si->start_key[1] = 0;
    }

    if (end) {
        streamEncodeID(si->end_key,end);
    } else {
        si->end_key[0] = UINT64_MAX;
        si->end_key[1] = UINT64_MAX;
    }

    /* Seek the correct node in the radix tree. */
    raxCreateInstance->raxStart(&si->ri,s->raxl);
    if (!rev) {
        if (start && (start->ms || start->seq)) {
            raxCreateInstance->raxSeek(&si->ri,"<=",(unsigned char*)si->start_key,
                    sizeof(si->start_key));
            if (raxCreateInstance->raxEOF(&si->ri)) raxCreateInstance->raxSeek(&si->ri,"^",NULL,0);
        } else {
            raxCreateInstance->raxSeek(&si->ri,"^",NULL,0);
        }
    } else {
        if (end && (end->ms || end->seq)) {
            raxCreateInstance->raxSeek(&si->ri,"<=",(unsigned char*)si->end_key,
                    sizeof(si->end_key));
            if (raxCreateInstance->raxEOF(&si->ri)) raxCreateInstance->raxSeek(&si->ri,"$",NULL,0);
        } else {
            raxCreateInstance->raxSeek(&si->ri,"$",NULL,0);
        }
    }
    si->streaml = s;
    si->lp = NULL; /* There is no current listpack right now. */
    si->lp_ele = NULL; /* Current listpack cursor. */
    si->rev = rev;  /* Direction, if non-zero reversed, from end to start. */
}

/**
 * 获取当前迭代器指向的消息ID和字段数量
 * 
 * @param si        迭代器对象
 * @param id        用于存储消息ID的结构体
 * @param numfields 用于存储消息字段数量的指针
 * @return 如果成功获取返回1，否则返回0
 */
int streamCreate::streamIteratorGetID(streamIterator *si, streamID *id, int64_t *numfields)
{
    while(1) { /* Will stop when element > stop_key or end of radix tree. */
        /* If the current listpack is set to NULL, this is the start of the
         * iteration or the previous listpack was completely iterated.
         * Go to the next node. */
        if (si->lp == NULL || si->lp_ele == NULL) {
            if (!si->rev && !raxCreateInstance->raxNext(&si->ri)) return 0;
            else if (si->rev && !raxCreateInstance->raxPrev(&si->ri)) return 0;
            serverAssert(si->ri.key_len == sizeof(streamID));
            /* Get the master ID. */
            streamDecodeID(si->ri.key,&si->master_id);
            /* Get the master fields count. */
            si->lp =static_cast<unsigned char*>(si->ri.data);
            si->lp_ele = listPackCreateInstance->lpFirst(si->lp);           /* Seek items count */
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele); /* Seek deleted count. */
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele); /* Seek num fields. */
            si->master_fields_count = lpGetInteger(si->lp_ele);
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele); /* Seek first field. */
            si->master_fields_start = si->lp_ele;
            /* We are now pointing to the first field of the master entry.
             * We need to seek either the first or the last entry depending
             * on the direction of the iteration. */
            if (!si->rev) {
                /* If we are iterating in normal order, skip the master fields
                 * to seek the first actual entry. */
                for (uint64_t i = 0; i < si->master_fields_count; i++)
                    si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
            } else {
                /* If we are iterating in reverse direction, just seek the
                 * last part of the last entry in the listpack (that is, the
                 * fields count). */
                si->lp_ele = listPackCreateInstance->lpLast(si->lp);
            }
        } else if (si->rev) {
            /* If we are iterating in the reverse order, and this is not
             * the first entry emitted for this listpack, then we already
             * emitted the current entry, and have to go back to the previous
             * one. */
            int lp_count = lpGetInteger(si->lp_ele);
            while(lp_count--) si->lp_ele = listPackCreateInstance->lpPrev(si->lp,si->lp_ele);
            /* Seek lp-count of prev entry. */
            si->lp_ele = listPackCreateInstance->lpPrev(si->lp,si->lp_ele);
        }

        /* For every radix tree node, iterate the corresponding listpack,
         * returning elements when they are within range. */
        while(1) {
            if (!si->rev) {
                /* If we are going forward, skip the previous entry
                 * lp-count field (or in case of the master entry, the zero
                 * term field) */
                si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
                if (si->lp_ele == NULL) break;
            } else {
                /* If we are going backward, read the number of elements this
                 * entry is composed of, and jump backward N times to seek
                 * its start. */
                int64_t lp_count = lpGetInteger(si->lp_ele);
                if (lp_count == 0) { /* We reached the master entry. */
                    si->lp = NULL;
                    si->lp_ele = NULL;
                    break;
                }
                while(lp_count--) si->lp_ele = listPackCreateInstance->lpPrev(si->lp,si->lp_ele);
            }

            /* Get the flags entry. */
            si->lp_flags = si->lp_ele;
            int flags = lpGetInteger(si->lp_ele);
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele); /* Seek ID. */

            /* Get the ID: it is encoded as difference between the master
             * ID and this entry ID. */
            *id = si->master_id;
            id->ms += lpGetInteger(si->lp_ele);
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
            id->seq += lpGetInteger(si->lp_ele);
            si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
            unsigned char buf[sizeof(streamID)];
            streamEncodeID(buf,id);

            /* The number of entries is here or not depending on the
             * flags. */
            if (flags & STREAM_ITEM_FLAG_SAMEFIELDS) {
                *numfields = si->master_fields_count;
            } else {
                *numfields = lpGetInteger(si->lp_ele);
                si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
            }
            serverAssert(*numfields>=0);

            /* If current >= start, and the entry is not marked as
             * deleted, emit it. */
            if (!si->rev) {
                if (memcmp(buf,si->start_key,sizeof(streamID)) >= 0 &&
                    !(flags & STREAM_ITEM_FLAG_DELETED))
                {
                    if (memcmp(buf,si->end_key,sizeof(streamID)) > 0)
                        return 0; /* We are already out of range. */
                    si->entry_flags = flags;
                    if (flags & STREAM_ITEM_FLAG_SAMEFIELDS)
                        si->master_fields_ptr = si->master_fields_start;
                    return 1; /* Valid item returned. */
                }
            } else {
                if (memcmp(buf,si->end_key,sizeof(streamID)) <= 0 &&
                    !(flags & STREAM_ITEM_FLAG_DELETED))
                {
                    if (memcmp(buf,si->start_key,sizeof(streamID)) < 0)
                        return 0; /* We are already out of range. */
                    si->entry_flags = flags;
                    if (flags & STREAM_ITEM_FLAG_SAMEFIELDS)
                        si->master_fields_ptr = si->master_fields_start;
                    return 1; /* Valid item returned. */
                }
            }

            /* If we do not emit, we have to discard if we are going
             * forward, or seek the previous entry if we are going
             * backward. */
            if (!si->rev) {
                int64_t to_discard = (flags & STREAM_ITEM_FLAG_SAMEFIELDS) ?
                                      *numfields : *numfields*2;
                for (int64_t i = 0; i < to_discard; i++)
                    si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
            } else {
                int64_t prev_times = 4; /* flag + id ms + id seq + one more to
                                           go back to the previous entry "count"
                                           field. */
                /* If the entry was not flagged SAMEFIELD we also read the
                 * number of fields, so go back one more. */
                if (!(flags & STREAM_ITEM_FLAG_SAMEFIELDS)) prev_times++;
                while(prev_times--) si->lp_ele = listPackCreateInstance->lpPrev(si->lp,si->lp_ele);
            }
        }

        /* End of listpack reached. Try the next/prev radix tree node. */
    }
}

/**
 * 获取当前迭代器指向的消息的下一个字段和值
 * 
 * @param si          迭代器对象
 * @param fieldptr    用于存储字段名指针的指针
 * @param valueptr    用于存储值指针的指针
 * @param fieldlen    用于存储字段名长度的指针
 * @param valuelen    用于存储值长度的指针
 */
void streamCreate::streamIteratorGetField(streamIterator *si, unsigned char **fieldptr, unsigned char **valueptr, 
                        int64_t *fieldlen, int64_t *valuelen)
{
    if (si->entry_flags & STREAM_ITEM_FLAG_SAMEFIELDS) 
    {
        *fieldptr = listPackCreateInstance->lpGet(si->master_fields_ptr,fieldlen,si->field_buf);
        si->master_fields_ptr = listPackCreateInstance->lpNext(si->lp,si->master_fields_ptr);
    } 
    else 
    {
        *fieldptr = listPackCreateInstance->lpGet(si->lp_ele,fieldlen,si->field_buf);
        si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
    }
    *valueptr = listPackCreateInstance->lpGet(si->lp_ele,valuelen,si->value_buf);
    si->lp_ele = listPackCreateInstance->lpNext(si->lp,si->lp_ele);
}

/**
 * 从Stream中删除当前迭代器指向的消息
 * 
 * @param si      迭代器对象
 * @param current 用于存储当前消息ID的结构体
 */
void streamCreate::streamIteratorRemoveEntry(streamIterator *si, streamID *current)
{
    unsigned char *lp = si->lp;
    int64_t aux;

    /* We do not really delete the entry here. Instead we mark it as
     * deleted flagging it, and also incrementing the count of the
     * deleted entries in the listpack header.
     *
     * We start flagging: */
    int flags = lpGetInteger(si->lp_flags);
    flags |= STREAM_ITEM_FLAG_DELETED;
    lp = lpReplaceInteger(lp,&si->lp_flags,flags);

    /* Change the valid/deleted entries count in the master entry. */
    unsigned char *p = listPackCreateInstance->lpFirst(lp);
    aux = lpGetInteger(p);

    if (aux == 1) {
        /* If this is the last element in the listpack, we can remove the whole
         * node. */
        lpFree(lp);
        raxCreateInstance->raxRemove(si->streaml->raxl,si->ri.key,si->ri.key_len,NULL);
    } else {
        /* In the base case we alter the counters of valid/deleted entries. */
        lp = lpReplaceInteger(lp,&p,aux-1);
        p = listPackCreateInstance->lpNext(lp,p); /* Seek deleted field. */
        aux = lpGetInteger(p);
        lp = lpReplaceInteger(lp,&p,aux+1);

        /* Update the listpack with the new pointer. */
        if (si->lp != lp)
            raxCreateInstance->raxInsert(si->streaml->raxl,si->ri.key,si->ri.key_len,lp,NULL);
    }

    /* Update the number of entries counter. */
    si->streaml->length--;

    /* Re-seek the iterator to fix the now messed up state. */
    streamID start, end;
    if (si->rev) {
        streamDecodeID(si->start_key,&start);
        end = *current;
    } else {
        start = *current;
        streamDecodeID(si->end_key,&end);
    }
    streamIteratorStop(si);
    streamIteratorStart(si,si->streaml,&start,&end,si->rev);

    /* TODO: perform a garbage collection here if the ration between
     * deleted and valid goes over a certain limit. */
}

/**
 * 停止并释放Stream迭代器
 * 
 * @param si 迭代器对象
 */
void streamCreate::streamIteratorStop(streamIterator *si)
{
    raxCreateInstance->raxStop(&si->ri);
}

/**
 * 在Stream中查找指定名称的消费者组
 * 
 * @param s         Stream对象
 * @param groupname 消费者组名称
 * @return 如果找到返回消费者组指针，否则返回NULL
 */
streamCG *streamCreate::streamLookupCG(stream *s, sds groupname)
{
    if (s->cgroups == NULL) return NULL;
    streamCG *cg =static_cast<streamCG*>(raxCreateInstance->raxFind(s->cgroups,(unsigned char*)groupname,sdsCreateInstance->sdslen(groupname)));
    return (cg == raxNotFound) ? NULL : cg;
}

/**
 * 在消费者组中查找指定名称的消费者
 * 
 * @param cg       消费者组
 * @param name     消费者名称
 * @param flags    查找标志
 * @param created  用于存储是否创建了新消费者的指针（可为NULL）
 * @return 如果找到或创建成功返回消费者指针，否则返回NULL
 */
streamConsumer *streamCreate::streamLookupConsumer(streamCG *cg, sds name, int flags, int *created)
{
    if (created) *created = 0;
    int create = !(flags & SLC_NOCREAT);
    int refresh = !(flags & SLC_NOREFRESH);
    streamConsumer *consumer =static_cast<streamConsumer*>(raxCreateInstance->raxFind(cg->consumers,(unsigned char*)name,sdsCreateInstance->sdslen(name)));
    if (consumer == raxNotFound) {
        if (!create) return NULL;
        consumer =static_cast<streamConsumer*>(zmalloc(sizeof(*consumer)));
        consumer->name = sdsCreateInstance->sdsdup(name);
        consumer->pel = raxCreateInstance->raxNew();
        raxCreateInstance->raxInsert(cg->consumers,(unsigned char*)name,sdsCreateInstance->sdslen(name),
                  consumer,NULL);
        consumer->seen_time = mstime();
        if (created) *created = 1;
    } else if (refresh)
        consumer->seen_time = mstime();
    return consumer;
}

/**
 * 在Stream中创建新的消费者组
 * 
 * @param s         Stream对象
 * @param name      消费者组名称
 * @param namelen   消费者组名称长度
 * @param id        消费者组的起始ID
 * @return 如果创建成功返回新消费者组指针，否则返回NULL
 */
streamCG *streamCreate::streamCreateCG(stream *s, char *name, size_t namelen, streamID *id)
{
    if (s->cgroups == NULL) s->cgroups = raxCreateInstance->raxNew();
    if (raxCreateInstance->raxFind(s->cgroups,(unsigned char*)name,namelen) != raxNotFound)
        return NULL;

    streamCG *cg = static_cast<streamCG*>(zmalloc(sizeof(*cg)));
    cg->pel = raxCreateInstance->raxNew();
    cg->consumers = raxCreateInstance->raxNew();
    cg->last_id = *id;
    raxCreateInstance->raxInsert(s->cgroups,(unsigned char*)name,namelen,cg,NULL);
    return cg;
}

/**
 * 在消费者中创建新的未确认消息（NACK）
 * 
 * @param consumer 消费者对象
 * @return 返回新创建的NACK对象指针
 */
streamNACK *streamCreate::streamCreateNACK(streamConsumer *consumer)
{
    streamNACK *nack =static_cast<streamNACK*>(zmalloc(sizeof(*nack)));
    nack->delivery_time = mstime();
    nack->delivery_count = 1;
    nack->consumer = consumer;
    return nack;
}

/**
 * 从缓冲区解码Stream消息ID
 * 
 * @param buf 包含消息ID的缓冲区
 * @param id  用于存储解码后消息ID的结构体
 */
void streamCreate::streamDecodeID(void *buf, streamID *id)
{
    uint64_t e[2];
    memcpy(e,buf,sizeof(e));
    id->ms = ntohu64(e[0]);
    id->seq = ntohu64(e[1]);
}

/**
 * 比较两个Stream消息ID的大小
 * 
 * @param a 第一个消息ID
 * @param b 第二个消息ID
 * @return 如果a小于b返回负数，如果相等返回0，如果a大于b返回正数
 */
int streamCreate::streamCompareID(streamID *a, streamID *b)
{
    if (a->ms > b->ms) return 1;
    else if (a->ms < b->ms) return -1;
    /* The ms part is the same. Check the sequence part. */
    else if (a->seq > b->seq) return 1;
    else if (a->seq < b->seq) return -1;
    /* Everything is the same: IDs are equal. */
    return 0;
}

/**
 * 释放NACK对象
 * 
 * @param na 要释放的NACK对象指针
 */
void streamCreate::streamFreeNACK(streamNACK *na)
{
    zfree(na);
}

/**
 * 将消息ID递增
 * 
 * @param id 要递增的消息ID
 * @return 成功返回1，失败返回0
 */
int streamCreate::streamIncrID(streamID *id)
{
    int ret = C_OK;
    if (id->seq == UINT64_MAX) {
        if (id->ms == UINT64_MAX) {
            /* Special case where 'id' is the last possible streamID... */
            id->ms = id->seq = 0;
            ret = C_ERR;
        } else {
            id->ms++;
            id->seq = 0;
        }
    } else {
        id->seq++;
    }
    return ret;
}

/**
 * 将消息ID递减
 * 
 * @param id 要递减的消息ID
 * @return 成功返回1，失败返回0
 */
int streamCreate::streamDecrID(streamID *id)
{
    int ret = C_OK;
    if (id->seq == 0) {
        if (id->ms == 0) {
            /* Special case where 'id' is the first possible streamID... */
            id->ms = id->seq = UINT64_MAX;
            ret = C_ERR;
        } else {
            id->ms--;
            id->seq = UINT64_MAX;
        }
    } else {
        id->seq--;
    }
    return ret;
}

/**
 * 传播消费者创建操作到从节点和AOF文件
 * 
 * @param c            客户端上下文
 * @param key          Stream键对象
 * @param groupname    消费者组名称对象
 * @param consumername 消费者名称
 */
void streamCreate::streamPropagateConsumerCreation(client *c, robj *key, robj *groupname, sds consumername)
{
    robj *argv[5];
    argv[0] = redisObjectCreateInstance->shared.xgroup;
    argv[1] = redisObjectCreateInstance->shared.createconsumer;
    argv[2] = key;
    argv[3] = groupname;
    argv[4] = redisObjectCreateInstance->createObject(OBJ_STRING,sdsCreateInstance->sdsdup(consumername));
    //delete by zhenjia.zhao 20250708
    // propagate(server.xgroupCommand,c->db->id,argv,5,PROPAGATE_AOF|PROPAGATE_REPL);
    // decrRefCount(argv[4]);
}

/**
 * 复制Stream对象
 * 
 * @param o 要复制的Stream对象
 * @return 返回复制后的新对象
 */
robj *streamCreate::streamDup(robj *o)
{
    robj *sobj;

    serverAssert(o->type == OBJ_STREAM);

    switch (o->encoding) {
        case OBJ_ENCODING_STREAM:
            sobj = redisObjectCreateInstance->createStreamObject();
            break;
        default:
            serverPanic("Wrong encoding.");
            break;
    }

    stream *s;
    stream *new_s;
    s = static_cast<stream*>(o->ptr);
    new_s =static_cast<stream*>(sobj->ptr);

    raxIterator ri;
    uint64_t rax_key[2];
    raxCreateInstance->raxStart(&ri, s->raxl);
    raxCreateInstance->raxSeek(&ri, "^", NULL, 0);
    size_t lp_bytes = 0;      /* Total bytes in the listpack. */
    unsigned char *lp = NULL; /* listpack pointer. */
    /* Get a reference to the listpack node. */
    while (raxCreateInstance->raxNext(&ri)) 
    {
        lp =static_cast<unsigned char*>(ri.data);
        lp_bytes = listPackCreateInstance->lpBytes(lp);
        unsigned char *new_lp =static_cast<unsigned char*>(zmalloc(lp_bytes));
        memcpy(new_lp, lp, lp_bytes);
        memcpy(rax_key, ri.key, sizeof(rax_key));
        raxCreateInstance->raxInsert(new_s->raxl, (unsigned char *)&rax_key, sizeof(rax_key),
                  new_lp, NULL);
    }
    new_s->length = s->length;
    new_s->last_id = s->last_id;
    raxCreateInstance->raxStop(&ri);

    if (s->cgroups == NULL) return sobj;

    /* Consumer Groups */
    raxIterator ri_cgroups;
    raxCreateInstance->raxStart(&ri_cgroups, s->cgroups);
    raxCreateInstance->raxSeek(&ri_cgroups, "^", NULL, 0);
    while (raxCreateInstance->raxNext(&ri_cgroups)) 
    {
        streamCG *cg =static_cast<streamCG*>(ri_cgroups.data);
        streamCG *new_cg = streamCreateCG(new_s, (char *)ri_cgroups.key,
                                          ri_cgroups.key_len, &cg->last_id);

        serverAssert(new_cg != NULL);

        /* Consumer Group PEL */
        raxIterator ri_cg_pel;
        raxCreateInstance->raxStart(&ri_cg_pel,cg->pel);
        raxCreateInstance->raxSeek(&ri_cg_pel,"^",NULL,0);
        while(raxCreateInstance->raxNext(&ri_cg_pel)){
            streamNACK *nack =static_cast<streamNACK *>(ri_cg_pel.data);
            streamNACK *new_nack = streamCreateNACK(NULL);
            new_nack->delivery_time = nack->delivery_time;
            new_nack->delivery_count = nack->delivery_count;
            raxCreateInstance->raxInsert(new_cg->pel, ri_cg_pel.key, sizeof(streamID), new_nack, NULL);
        }
        raxCreateInstance->raxStop(&ri_cg_pel);

        /* Consumers */
        raxIterator ri_consumers;
        raxCreateInstance->raxStart(&ri_consumers, cg->consumers);
        raxCreateInstance->raxSeek(&ri_consumers, "^", NULL, 0);
        while (raxCreateInstance->raxNext(&ri_consumers)) {
            streamConsumer *consumer =static_cast<streamConsumer*>(ri_consumers.data);
            streamConsumer *new_consumer;
            new_consumer =static_cast<streamConsumer*>(zmalloc(sizeof(*new_consumer)));
            new_consumer->name = sdsCreateInstance->sdsdup(consumer->name);
            new_consumer->pel = raxCreateInstance->raxNew();
            raxCreateInstance->raxInsert(new_cg->consumers,(unsigned char *)new_consumer->name,
                        sdsCreateInstance->sdslen(new_consumer->name), new_consumer, NULL);
            new_consumer->seen_time = consumer->seen_time;

            /* Consumer PEL */
            raxIterator ri_cpel;
            raxCreateInstance->raxStart(&ri_cpel, consumer->pel);
            raxCreateInstance->raxSeek(&ri_cpel, "^", NULL, 0);
            while (raxCreateInstance->raxNext(&ri_cpel)) {
                streamNACK *new_nack =static_cast<streamNACK*>(raxCreateInstance->raxFind(new_cg->pel,ri_cpel.key,sizeof(streamID)));

                serverAssert(new_nack != raxNotFound);

                new_nack->consumer = new_consumer;
                raxCreateInstance->raxInsert(new_consumer->pel,ri_cpel.key,sizeof(streamID),new_nack,NULL);
            }
            raxCreateInstance->raxStop(&ri_cpel);
        }
        raxCreateInstance->raxStop(&ri_consumers);
    }
    raxCreateInstance->raxStop(&ri_cgroups);
    return sobj;
}

/**
 * 验证Stream底层Listpack的完整性
 * 
 * @param lp    Listpack缓冲区指针
 * @param size  Listpack大小
 * @param deep  是否进行深度验证（true表示进行深度验证）
 * @return 验证通过返回1，否则返回0
 */
int streamCreate::streamValidateListpackIntegrity(unsigned char *lp, size_t size, int deep)
{
    int valid_record;
    unsigned char *p, *next;

    /* Since we don't want to run validation of all records twice, we'll
     * run the listpack validation of just the header and do the rest here. */
    if (!listPackCreateInstance->lpValidateIntegrity(lp, size, 0))
        return 0;

    /* In non-deep mode we just validated the listpack header (encoded size) */
    if (!deep) return 1;

    next = p = listPackCreateInstance->lpValidateFirst(lp);
    if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
    if (!p) return 0;

    /* entry count */
    int64_t entry_count = lpGetIntegerIfValid(p, &valid_record);
    if (!valid_record) return 0;
    p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

    /* deleted */
    int64_t deleted_count = lpGetIntegerIfValid(p, &valid_record);
    if (!valid_record) return 0;
    p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

    /* num-of-fields */
    int64_t master_fields = lpGetIntegerIfValid(p, &valid_record);
    if (!valid_record) return 0;
    p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

    /* the field names */
    for (int64_t j = 0; j < master_fields; j++) {
        p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
    }

    /* the zero master entry terminator. */
    int64_t zero = lpGetIntegerIfValid(p, &valid_record);
    if (!valid_record || zero != 0) return 0;
    p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

    entry_count += deleted_count;
    while (entry_count--) 
    {
        if (!p) return 0;
        int64_t fields = master_fields, extra_fields = 3;
        int64_t flags = lpGetIntegerIfValid(p, &valid_record);
        if (!valid_record) return 0;
        p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

        /* entry id */
        lpGetIntegerIfValid(p, &valid_record);
        if (!valid_record) return 0;
        p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
        lpGetIntegerIfValid(p, &valid_record);
        if (!valid_record) return 0;
        p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

        if (!(flags & STREAM_ITEM_FLAG_SAMEFIELDS)) 
        {
            /* num-of-fields */
            fields = lpGetIntegerIfValid(p, &valid_record);
            if (!valid_record) return 0;
            p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;

            /* the field names */
            for (int64_t j = 0; j < fields; j++) {
                p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
            }

            extra_fields += fields + 1;
        }

        /* the values */
        for (int64_t j = 0; j < fields; j++) {
            p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
        }

        /* lp-count */
        int64_t lp_count = lpGetIntegerIfValid(p, &valid_record);
        if (!valid_record) return 0;
        if (lp_count != fields + extra_fields) return 0;
        p = next; if (!listPackCreateInstance->lpValidateNext(lp, &next, size)) return 0;
    }

    if (next)
        return 0;

    return 1;
}

/**
 * 解析字符串形式的消息ID
 * 
 * @param o 包含消息ID的Redis对象
 * @param id 用于存储解析后消息ID的结构体
 * @return 解析成功返回1，失败返回0
 */
int streamCreate::streamParseID(const robj *o, streamID *id)
{
    return streamGenericParseIDOrReply(NULL, o, id, 0, 0);
}

/**
 * 从消息ID创建Redis对象
 * 
 * @param id 消息ID
 * @return 返回新创建的Redis对象
 */
robj *streamCreate::createObjectFromStreamID(streamID *id)
{
    return redisObjectCreateInstance->createObject(OBJ_STRING, sdsCreateInstance->sdscatfmt(sdsCreateInstance->sdsempty(),"%U-%U",id->ms,id->seq));
}

/**
 * 向Stream中追加新消息
 * 
 * @param s         Stream对象
 * @param argv      消息字段和值数组
 * @param numfields 字段数量（值数量等于字段数量）
 * @param added_id  用于存储添加的消息ID的指针
 * @param use_id    要使用的消息ID（可为NULL，表示自动生成）
 * @return 成功返回1，失败返回0
 */
int streamCreate::streamAppendItem(stream *s, robj **argv, int64_t numfields, streamID *added_id, streamID *use_id)
{
    /* Generate the new entry ID. */
    streamID id;
    if (use_id)
        id = *use_id;
    else
        streamNextID(&s->last_id,&id);

    /* Check that the new ID is greater than the last entry ID
     * or return an error. Automatically generated IDs might
     * overflow (and wrap-around) when incrementing the sequence
       part. */
    if (streamCompareID(&id,&s->last_id) <= 0) {
        errno = EDOM;
        return C_ERR;
    }

    /* Avoid overflow when trying to add an element to the stream (listpack
     * can only host up to 32bit length sttrings, and also a total listpack size
     * can't be bigger than 32bit length. */
    size_t totelelen = 0;
    for (int64_t i = 0; i < numfields*2; i++) {
        sds ele =static_cast<char*>(argv[i]->ptr);
        totelelen += sdsCreateInstance->sdslen(ele);
    }
    if (totelelen > STREAM_LISTPACK_MAX_SIZE) {
        errno = ERANGE;
        return C_ERR;
    }

    /* Add the new entry. */
    raxIterator ri;
    raxCreateInstance->raxStart(&ri,s->raxl);
    raxCreateInstance->raxSeek(&ri,"$",NULL,0);

    size_t lp_bytes = 0;        /* Total bytes in the tail listpack. */
    unsigned char *lp = NULL;   /* Tail listpack pointer. */

    /* Get a reference to the tail node listpack. */
    if (raxCreateInstance->raxNext(&ri)) {
        lp =static_cast<unsigned char*> (ri.data);
        lp_bytes = listPackCreateInstance->lpBytes(lp);
    }
    raxCreateInstance->raxStop(&ri);

    /* We have to add the key into the radix tree in lexicographic order,
     * to do so we consider the ID as a single 128 bit number written in
     * big endian, so that the most significant bytes are the first ones. */
    uint64_t rax_key[2];    /* Key in the radix tree containing the listpack.*/
    streamID master_id;     /* ID of the master entry in the listpack. */

    /* Create a new listpack and radix tree node if needed. Note that when
     * a new listpack is created, we populate it with a "master entry". This
     * is just a set of fields that is taken as references in order to compress
     * the stream entries that we'll add inside the listpack.
     *
     * Note that while we use the first added entry fields to create
     * the master entry, the first added entry is NOT represented in the master
     * entry, which is a stand alone object. But of course, the first entry
     * will compress well because it's used as reference.
     *
     * The master entry is composed like in the following example:
     *
     * +-------+---------+------------+---------+--/--+---------+---------+-+
     * | count | deleted | num-fields | field_1 | field_2 | ... | field_N |0|
     * +-------+---------+------------+---------+--/--+---------+---------+-+
     *
     * count and deleted just represent respectively the total number of
     * entries inside the listpack that are valid, and marked as deleted
     * (deleted flag in the entry flags set). So the total number of items
     * actually inside the listpack (both deleted and not) is count+deleted.
     *
     * The real entries will be encoded with an ID that is just the
     * millisecond and sequence difference compared to the key stored at
     * the radix tree node containing the listpack (delta encoding), and
     * if the fields of the entry are the same as the master entry fields, the
     * entry flags will specify this fact and the entry fields and number
     * of fields will be omitted (see later in the code of this function).
     *
     * The "0" entry at the end is the same as the 'lp-count' entry in the
     * regular stream entries (see below), and marks the fact that there are
     * no more entries, when we scan the stream from right to left. */

    /* First of all, check if we can append to the current macro node or
     * if we need to switch to the next one. 'lp' will be set to NULL if
     * the current node is full. */
    if (lp != NULL) {
        //size_t node_max_bytes = server.stream_node_max_bytes; //delete by zhenjia.zhao
        size_t node_max_bytes = 8192;                           //modified by zhenjia.zhao 同上
        if (node_max_bytes == 0 || node_max_bytes > STREAM_LISTPACK_MAX_SIZE)
            node_max_bytes = STREAM_LISTPACK_MAX_SIZE;
        if (lp_bytes + totelelen >= node_max_bytes) {
            lp = NULL;
        } 
        //else if (server.stream_node_max_entries) //delete by zhenjia.zhao
        else if (true)                              //modified by zhenjia.zhao
        {
            unsigned char *lp_ele = listPackCreateInstance->lpFirst(lp);
            /* Count both live entries and deleted ones. */
            int64_t count = lpGetInteger(lp_ele) + lpGetInteger(listPackCreateInstance->lpNext(lp,lp_ele));
            //if (count >= server.stream_node_max_entries) //delete by zhenjia.zhao
            if (true)  //modified by zhenjia.zhao
            {
                /* Shrink extra pre-allocated memory */
                lp = listPackCreateInstance->lpShrinkToFit(lp);
                if (ri.data != lp)
                    raxCreateInstance->raxInsert(s->raxl,ri.key,ri.key_len,lp,NULL);
                lp = NULL;
            }
        }
    }

    int flags = STREAM_ITEM_FLAG_NONE;
    if (lp == NULL) {
        master_id = id;
        streamEncodeID(rax_key,&id);
        /* Create the listpack having the master entry ID and fields.
         * Pre-allocate some bytes when creating listpack to avoid realloc on
         * every XADD. Since listpack.c uses malloc_size, it'll grow in steps,
         * and won't realloc on every XADD.
         * When listpack reaches max number of entries, we'll shrink the
         * allocation to fit the data. */
        size_t prealloc = STREAM_LISTPACK_MAX_PRE_ALLOCATE;
        //delete by zhenjia.zhao
        // if (server.stream_node_max_bytes > 0 && server.stream_node_max_bytes < prealloc) {
        //     prealloc = server.stream_node_max_bytes;
        // }
        //end
        lp = listPackCreateInstance->lpNew(prealloc);
        lp = lpAppendInteger(lp,1); /* One item, the one we are adding. */
        lp = lpAppendInteger(lp,0); /* Zero deleted so far. */
        lp = lpAppendInteger(lp,numfields);
        for (int64_t i = 0; i < numfields; i++) {
            sds field =static_cast<char*>(argv[i*2]->ptr);
            lp = listPackCreateInstance->lpAppend(lp,(unsigned char*)field,sdsCreateInstance->sdslen(field));
        }
        lp = lpAppendInteger(lp,0); /* Master entry zero terminator. */
        raxCreateInstance->raxInsert(s->raxl,(unsigned char*)&rax_key,sizeof(rax_key),lp,NULL);
        /* The first entry we insert, has obviously the same fields of the
         * master entry. */
        flags |= STREAM_ITEM_FLAG_SAMEFIELDS;
    } else {
        serverAssert(ri.key_len == sizeof(rax_key));
        memcpy(rax_key,ri.key,sizeof(rax_key));

        /* Read the master ID from the radix tree key. */
        streamDecodeID(rax_key,&master_id);
        unsigned char *lp_ele = listPackCreateInstance->lpFirst(lp);

        /* Update count and skip the deleted fields. */
        int64_t count = lpGetInteger(lp_ele);
        lp = lpReplaceInteger(lp,&lp_ele,count+1);
        lp_ele = listPackCreateInstance->lpNext(lp,lp_ele); /* seek deleted. */
        lp_ele = listPackCreateInstance->lpNext(lp,lp_ele); /* seek master entry num fields. */

        /* Check if the entry we are adding, have the same fields
         * as the master entry. */
        int64_t master_fields_count = lpGetInteger(lp_ele);
        lp_ele = listPackCreateInstance->lpNext(lp,lp_ele);
        if (numfields == master_fields_count) {
            int64_t i;
            for (i = 0; i < master_fields_count; i++) {
                sds field = static_cast<char*>(argv[i*2]->ptr);
                int64_t e_len;
                unsigned char buf[LP_INTBUF_SIZE];
                unsigned char *e = listPackCreateInstance->lpGet(lp_ele,&e_len,buf);
                /* Stop if there is a mismatch. */
                if (sdsCreateInstance->sdslen(field) != (size_t)e_len ||
                    memcmp(e,field,e_len) != 0) break;
                lp_ele = listPackCreateInstance->lpNext(lp,lp_ele);
            }
            /* All fields are the same! We can compress the field names
             * setting a single bit in the flags. */
            if (i == master_fields_count) flags |= STREAM_ITEM_FLAG_SAMEFIELDS;
        }
    }

    /* Populate the listpack with the new entry. We use the following
     * encoding:
     *
     * +-----+--------+----------+-------+-------+-/-+-------+-------+--------+
     * |flags|entry-id|num-fields|field-1|value-1|...|field-N|value-N|lp-count|
     * +-----+--------+----------+-------+-------+-/-+-------+-------+--------+
     *
     * However if the SAMEFIELD flag is set, we have just to populate
     * the entry with the values, so it becomes:
     *
     * +-----+--------+-------+-/-+-------+--------+
     * |flags|entry-id|value-1|...|value-N|lp-count|
     * +-----+--------+-------+-/-+-------+--------+
     *
     * The entry-id field is actually two separated fields: the ms
     * and seq difference compared to the master entry.
     *
     * The lp-count field is a number that states the number of listpack pieces
     * that compose the entry, so that it's possible to travel the entry
     * in reverse order: we can just start from the end of the listpack, read
     * the entry, and jump back N times to seek the "flags" field to read
     * the stream full entry. */
    lp = lpAppendInteger(lp,flags);
    lp = lpAppendInteger(lp,id.ms - master_id.ms);
    lp = lpAppendInteger(lp,id.seq - master_id.seq);
    if (!(flags & STREAM_ITEM_FLAG_SAMEFIELDS))
        lp = lpAppendInteger(lp,numfields);
    for (int64_t i = 0; i < numfields; i++) {
        sds field =static_cast<char*>(argv[i*2]->ptr), value = static_cast<char*>(argv[i*2+1]->ptr);
        if (!(flags & STREAM_ITEM_FLAG_SAMEFIELDS))
            lp = listPackCreateInstance->lpAppend(lp,(unsigned char*)field,sdsCreateInstance->sdslen(field));
        lp = listPackCreateInstance->lpAppend(lp,(unsigned char*)value,sdsCreateInstance->sdslen(value));
    }
    /* Compute and store the lp-count field. */
    int64_t lp_count = numfields;
    lp_count += 3; /* Add the 3 fixed fields flags + ms-diff + seq-diff. */
    if (!(flags & STREAM_ITEM_FLAG_SAMEFIELDS)) {
        /* If the item is not compressed, it also has the fields other than
         * the values, and an additional num-fileds field. */
        lp_count += numfields+1;
    }
    lp = lpAppendInteger(lp,lp_count);

    /* Insert back into the tree in order to update the listpack pointer. */
    if (ri.data != lp)
        raxCreateInstance->raxInsert(s->raxl,(unsigned char*)&rax_key,sizeof(rax_key),lp,NULL);
    s->length++;
    s->last_id = id;
    if (added_id) *added_id = id;
    return C_OK;
}

/**
 * 从Stream中删除指定ID的消息
 * 
 * @param s  Stream对象
 * @param id 要删除的消息ID
 * @return 成功删除返回1，未找到返回0
 */
int streamCreate::streamDeleteItem(stream *s, streamID *id)
{
    int deleted = 0;
    streamIterator si;
    streamIteratorStart(&si,s,id,id,0);
    streamID myid;
    int64_t numfields;
    if (streamIteratorGetID(&si,&myid,&numfields)) {
        streamIteratorRemoveEntry(&si,&myid);
        deleted = 1;
    }
    streamIteratorStop(&si);
    return deleted;
}

/**
 * 按消息数量修剪Stream
 * 
 * @param s       Stream对象
 * @param maxlen  保留的最大消息数量
 * @param approx  是否进行近似修剪（true表示允许近似，性能更好）
 * @return 被删除的消息数量
 */
int64_t streamCreate::streamTrimByLength(stream *s, long long maxlen, int approx)
{
    streamAddTrimArgs args = {
        .trim_strategy = TRIM_STRATEGY_MAXLEN,
        .approx_trim = approx,
        //.limit = approx ? 100 * server.stream_node_max_entries : 0,//delete by zhenjia.zhao 
        .limit = 60000, //add by zhenjia.zhao
        .maxlen = maxlen
    };
    return streamTrim(s, &args);
}

/**
 * 按消息ID修剪Stream
 * 
 * @param s      Stream对象
 * @param minid  保留的最小消息ID（小于此ID的消息将被删除）
 * @param approx 是否进行近似修剪（true表示允许近似，性能更好）
 * @return 被删除的消息数量
 */
int64_t streamCreate::streamTrimByID(stream *s, streamID minid, int approx)
{
    streamAddTrimArgs args = {
        .trim_strategy = TRIM_STRATEGY_MINID,
        .approx_trim = approx,
        //.limit = approx ? 100 * server.stream_node_max_entries : 0,//delete by zhenjia.zhao 
        .limit = 60000, //add by zhenjia.zhao
        .minid = minid
    };
    return streamTrim(s, &args);
}

/* Free a whole radix tree. */
/**
 * 释放一个rax树结构占用的内存。
 * 
 * @param rax 指向要释放的rax树的指针
 */
void streamCreate::raxFree(rax *rax) {
    raxCreatel.raxFreeWithCallback(rax,NULL);
}

/**
 * 释放一个流消费者结构占用的内存。
 * 
 * @param sc 指向要释放的流消费者结构的指针
 */
void streamCreate::streamFreeConsumer(streamConsumer *sc) 
{
    raxFree(sc->pel); /* No value free callback: the PEL entries are shared
                         between the consumer and the main stream PEL. */
    sdsCreatel.sdsfree(sc->name);
    zfree(sc);
}

/**
 * 释放一个消费组结构占用的内存。
 * 
 * @param cg 指向要释放的消费组结构的指针
 */
void streamCreate::streamFreeCG(streamCG *cg) 
{
    raxCreatel.raxFreeWithCallback(cg->pel,(void(*)(void*))streamFreeNACK);
    raxCreatel.raxFreeWithCallback(cg->consumers,(void(*)(void*))streamFreeConsumer);
    zfree(cg);
}

/**
 * 释放一个压缩列表占用的内存。
 * 
 * @param lp 指向要释放的压缩列表的指针
 */
void streamCreate::lpFree(unsigned char *lp)
{
    zfree(lp);
}

/**
 * 将流ID编码到指定的缓冲区。
 * 
 * @param buf 目标缓冲区
 * @param id  指向要编码的流ID结构的指针
 */
void streamCreate::streamEncodeID(void *buf, streamID *id) 
{
    uint64_t e[2];
    e[0] = htonu64(id->ms);
    e[1] = htonu64(id->seq);
    memcpy(buf,e,sizeof(e));
}

/* This is just a wrapper for lpReplace() to directly use a 64 bit integer
 * instead of a string to replace the current element. The function returns
 * the new listpack as return value, and also updates the current cursor
 * by updating '*pos'. */
/**
 * 在压缩列表中替换指定位置的整数值。
 * 
 * @param lp    压缩列表指针
 * @param pos   指向要替换的位置的指针
 * @param value 新的整数值
 * @return 替换后的压缩列表指针
 */
unsigned char *streamCreate::lpReplaceInteger(unsigned char *lp, unsigned char **pos, int64_t value) 
{
    char buf[LONG_STR_SIZE];
    int slen = toolFuncInstance->ll2string(buf,sizeof(buf),value);
    return listPackCreateInstance->lpInsert(lp, (unsigned char*)buf, slen, *pos, LP_REPLACE, pos);
}
/**
 * 获取压缩列表元素中的整数值（如果有效）。
 * 
 * @param ele   指向压缩列表元素的指针
 * @param valid 输出参数，指示值是否有效
 * @return 如果有效，返回元素中的整数值；否则返回未定义值
 */
int64_t streamCreate::lpGetIntegerIfValid(unsigned char *ele, int *valid) 
{
    int64_t v;
    unsigned char *e = listPackCreatel.lpGet(ele,&v,NULL);
    if (e == NULL) {
        if (valid)
            *valid = 1;
        return v;
    }
    /* The following code path should never be used for how listpacks work:
     * they should always be able to store an int64_t value in integer
     * encoded form. However the implementation may change. */
    long long ll;
    int ret = toolFuncl.string2ll((char*)e,v,&ll);
    if (valid)
        *valid = ret;
    else
        serverAssert(ret != 0);
    v = ll;
    return v;
}

/* Parse a stream ID in the format given by clients to Redis, that is
 * <ms>-<seq>, and converts it into a streamID structure. If
 * the specified ID is invalid C_ERR is returned and an error is reported
 * to the client, otherwise C_OK is returned. The ID may be in incomplete
 * form, just stating the milliseconds time part of the stream. In such a case
 * the missing part is set according to the value of 'missing_seq' parameter.
 *
 * The IDs "-" and "+" specify respectively the minimum and maximum IDs
 * that can be represented. If 'strict' is set to 1, "-" and "+" will be
 * treated as an invalid ID.
 *
 * If 'c' is set to NULL, no reply is sent to the client. */
/**
 * 解析流ID参数并设置到streamID结构中，或在解析失败时向客户端发送错误回复。
 * 
 * @param c          客户端上下文
 * @param o          包含ID字符串的对象
 * @param id         输出参数，用于存储解析后的流ID
 * @param missing_seq 当ID中缺少序列号部分时使用的默认序列号
 * @param strict     是否启用严格模式（禁止某些特殊值）
 * @return C_OK表示解析成功，C_ERR表示解析失败
 */
int streamCreate::streamGenericParseIDOrReply(client *c, const robj *o, streamID *id, uint64_t missing_seq, int strict) 
{
    char buf[128];
    unsigned long long ms, seq;  // 提前声明变量
    
    if (sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr)) > sizeof(buf)-1) 
    {
        //delete by zhenjia.zhao 
        //if (c) 
            //addReplyError(c,"Invalid stream ID specified as stream " "command argument");
        return C_ERR;
    }
        
    memcpy(buf,o->ptr,sdsCreateInstance->sdslen(static_cast<const char*>(o->ptr))+1);

    if (strict && (buf[0] == '-' || buf[0] == '+') && buf[1] == '\0')
    {
        //delete by zhenjia.zhao
        //if (c) 
        //addReplyError(c,"Invalid stream ID specified as stream " "command argument");
        return C_ERR;
    }

    /* Handle the "-" and "+" special cases. */
    if (buf[0] == '-' && buf[1] == '\0') {
        id->ms = 0;
        id->seq = 0;
        return C_OK;
    } else if (buf[0] == '+' && buf[1] == '\0') {
        id->ms = UINT64_MAX;
        id->seq = UINT64_MAX;
        return C_OK;
    }

    /* Parse <ms>-<seq> form. */
    char *dot = strchr(buf,'-');
    if (dot) *dot = '\0';
    if (toolFuncInstance->string2ull(buf,&ms) == 0) 
    {    
        //delete by zhenjia.zhao
        //if (c) 
        //addReplyError(c,"Invalid stream ID specified as stream " "command argument");
        return C_ERR;
    }
    if (dot && toolFuncInstance->string2ull(dot+1,&seq) == 0)
    {
        //delete by zhenjia.zhao
        //if (c) 
        //addReplyError(c,"Invalid stream ID specified as stream " "command argument");
        return C_ERR;
    }
    if (!dot) seq = missing_seq;
    id->ms = ms;
    id->seq = seq;
    return C_OK;
}

/* Generate the next stream item ID given the previous one. If the current
 * milliseconds Unix time is greater than the previous one, just use this
 * as time part and start with sequence part of zero. Otherwise we use the
 * previous time (and never go backward) and increment the sequence. */
/**
 * 生成下一个流ID（基于当前ID递增）。
 * 
 * @param last_id  当前ID
 * @param new_id   输出参数，用于存储生成的下一个ID
 */
void streamCreate::streamNextID(streamID *last_id, streamID *new_id) 
{
    uint64_t ms = mstime();
    if (ms > last_id->ms) {
        new_id->ms = ms;
        new_id->seq = 0;
    } else {
        *new_id = *last_id;
        streamIncrID(new_id);
    }
}

/* This is just a wrapper for lpAppend() to directly use a 64 bit integer
 * instead of a string. */
/**
 * 向压缩列表追加一个整数值。
 * 
 * @param lp    压缩列表指针
 * @param value 要追加的整数值
 * @return 追加后的压缩列表指针
 */
unsigned char *streamCreate::lpAppendInteger(unsigned char *lp, int64_t value) 
{
    char buf[LONG_STR_SIZE];
    int slen = toolFuncInstance->ll2string(buf,sizeof(buf),value);
    return listPackCreateInstance->lpAppend(lp,(unsigned char*)buf,slen);
}

/**
 * 对流进行修剪，移除旧的条目以控制流的大小。
 * 
 * @param s     要修剪的流
 * @param args  修剪参数，包含修剪策略和阈值
 * @return 被移除的条目数量
 */
int64_t streamCreate::streamTrim(stream *s, streamAddTrimArgs *args) 
{
    size_t maxlen = args->maxlen;
    streamID *id = &args->minid;
    int approx = args->approx_trim;
    int64_t limit = args->limit;
    int trim_strategy = args->trim_strategy;

    if (trim_strategy == TRIM_STRATEGY_NONE)
        return 0;

    raxIterator ri;
    raxCreateInstance->raxStart(&ri,s->raxl);
    raxCreateInstance->raxSeek(&ri,"^",NULL,0);

    int64_t deleted = 0;
    while (raxCreateInstance->raxNext(&ri)) {
        if (trim_strategy == TRIM_STRATEGY_MAXLEN && s->length <= maxlen)
            break;

        unsigned char *lp =static_cast<unsigned char*>(ri.data), *p = listPackCreateInstance->lpFirst(lp);
        int64_t entries = lpGetInteger(p);

        /* Check if we exceeded the amount of work we could do */
        if (limit && (deleted + entries) > limit)
            break;

        /* Check if we can remove the whole node. */
        int remove_node;
        streamID master_id = {0}; /* For MINID */
        if (trim_strategy == TRIM_STRATEGY_MAXLEN) {
            remove_node = s->length - entries >= maxlen;
        } else {
            /* Read the master ID from the radix tree key. */
            streamDecodeID(ri.key, &master_id);

            /* Read last ID. */
            streamID last_id;
            lpGetEdgeStreamID(lp, 0, &master_id, &last_id);

            /* We can remove the entire node id its last ID < 'id' */
            remove_node = streamCompareID(&last_id, id) < 0;
        }

        if (remove_node) {
            lpFree(lp);
            raxCreateInstance->raxRemove(s->raxl,ri.key,ri.key_len,NULL);
            raxCreateInstance->raxSeek(&ri,">=",ri.key,ri.key_len);
            s->length -= entries;
            deleted += entries;
            continue;
        }

        /* If we cannot remove a whole element, and approx is true,
         * stop here. */
        if (approx) break;

        /* Now we have to trim entries from within 'lp' */
        int64_t deleted_from_lp = 0;

        p = listPackCreateInstance->lpNext(lp, p); /* Skip deleted field. */
        p = listPackCreateInstance->lpNext(lp, p); /* Skip num-of-fields in the master entry. */

        /* Skip all the master fields. */
        int64_t master_fields_count = lpGetInteger(p);
        p = listPackCreateInstance->lpNext(lp,p); /* Skip the first field. */
        for (int64_t j = 0; j < master_fields_count; j++)
            p = listPackCreateInstance->lpNext(lp,p); /* Skip all master fields. */
        p = listPackCreateInstance->lpNext(lp,p); /* Skip the zero master entry terminator. */

        /* 'p' is now pointing to the first entry inside the listpack.
         * We have to run entry after entry, marking entries as deleted
         * if they are already not deleted. */
        while (p) {
            /* We keep a copy of p (which point to flags part) in order to
             * update it after (and if) we actually remove the entry */
            unsigned char *pcopy = p;

            int flags = lpGetInteger(p);
            p = listPackCreateInstance->lpNext(lp, p); /* Skip flags. */
            int to_skip;

            int ms_delta = lpGetInteger(p);
            p = listPackCreateInstance->lpNext(lp, p); /* Skip ID ms delta */
            int seq_delta = lpGetInteger(p);
            p = listPackCreateInstance->lpNext(lp, p); /* Skip ID seq delta */

            streamID currid = {0}; /* For MINID */
            if (trim_strategy == TRIM_STRATEGY_MINID) {
                currid.ms = master_id.ms + ms_delta;
                currid.seq = master_id.seq + seq_delta;
            }

            int stop;
            if (trim_strategy == TRIM_STRATEGY_MAXLEN) {
                stop = s->length <= maxlen;
            } else {
                /* Following IDs will definitely be greater because the rax
                 * tree is sorted, no point of continuing. */
                stop = streamCompareID(&currid, id) >= 0;
            }
            if (stop)
                break;

            if (flags & STREAM_ITEM_FLAG_SAMEFIELDS) {
                to_skip = master_fields_count;
            } else {
                to_skip = lpGetInteger(p); /* Get num-fields. */
                p = listPackCreateInstance->lpNext(lp,p); /* Skip num-fields. */
                to_skip *= 2; /* Fields and values. */
            }

            while(to_skip--) p = listPackCreateInstance->lpNext(lp,p); /* Skip the whole entry. */
            p = listPackCreateInstance->lpNext(lp,p); /* Skip the final lp-count field. */

            /* Mark the entry as deleted. */
            if (!(flags & STREAM_ITEM_FLAG_DELETED)) {
                intptr_t delta = p - lp;
                flags |= STREAM_ITEM_FLAG_DELETED;
                lp = lpReplaceInteger(lp, &pcopy, flags);
                deleted_from_lp++;
                s->length--;
                p = lp + delta;
            }
        }
        deleted += deleted_from_lp;

        /* Now we the entries/deleted counters. */
        p = listPackCreateInstance->lpFirst(lp);
        lp = lpReplaceInteger(lp,&p,entries-deleted_from_lp);
        p = listPackCreateInstance->lpNext(lp,p); /* Skip deleted field. */
        int64_t marked_deleted = lpGetInteger(p);
        lp = lpReplaceInteger(lp,&p,marked_deleted+deleted_from_lp);
        p = listPackCreateInstance->lpNext(lp,p); /* Skip num-of-fields in the master entry. */

        /* Here we should perform garbage collection in case at this point
         * there are too many entries deleted inside the listpack. */
        entries -= deleted_from_lp;
        marked_deleted += deleted_from_lp;
        if (entries + marked_deleted > 10 && marked_deleted > entries/2) {
            /* TODO: perform a garbage collection. */
        }

        /* Update the listpack with the new pointer. */
        raxCreateInstance->raxInsert(s->raxl,ri.key,ri.key_len,lp,NULL);

        break; /* If we are here, there was enough to delete in the current
                  node, so no need to go to the next node. */
    }

    raxCreateInstance->raxStop(&ri);
    return deleted;
}


/* Get an edge streamID of a given listpack.
 * 'master_id' is an input param, used to build the 'edge_id' output param */
/**
 * 获取压缩列表中第一个或最后一个条目的流ID（用于确定流的边界）。
 * 
 * @param lp        压缩列表指针
 * @param first     1表示获取第一个条目，0表示获取最后一个条目
 * @param master_id 输出参数，主ID
 * @param edge_id   输出参数，边界ID
 * @return 成功返回1，失败返回0
 */
int streamCreate::lpGetEdgeStreamID(unsigned char *lp, int first, streamID *master_id, streamID *edge_id)
{
   if (lp == NULL)
       return 0;

   unsigned char *lp_ele;

   /* We need to seek either the first or the last entry depending
    * on the direction of the iteration. */
   if (first) {
       /* Get the master fields count. */
       lp_ele = listPackCreateInstance->lpFirst(lp);        /* Seek items count */
       lp_ele = listPackCreateInstance->lpNext(lp, lp_ele); /* Seek deleted count. */
       lp_ele = listPackCreateInstance->lpNext(lp, lp_ele); /* Seek num fields. */
       int64_t master_fields_count = lpGetInteger(lp_ele);
       lp_ele = listPackCreateInstance->lpNext(lp, lp_ele); /* Seek first field. */

       /* If we are iterating in normal order, skip the master fields
        * to seek the first actual entry. */
       for (int64_t i = 0; i < master_fields_count; i++)
           lp_ele = listPackCreateInstance->lpNext(lp, lp_ele);

       /* If we are going forward, skip the previous entry's
        * lp-count field (or in case of the master entry, the zero
        * term field) */
       lp_ele = listPackCreateInstance->lpNext(lp, lp_ele);
       if (lp_ele == NULL)
           return 0;
   } else {
       /* If we are iterating in reverse direction, just seek the
        * last part of the last entry in the listpack (that is, the
        * fields count). */
       lp_ele = listPackCreateInstance->lpLast(lp);

       /* If we are going backward, read the number of elements this
        * entry is composed of, and jump backward N times to seek
        * its start. */
       int64_t lp_count = lpGetInteger(lp_ele);
       if (lp_count == 0) /* We reached the master entry. */
           return 0;

       while (lp_count--)
           lp_ele = listPackCreateInstance->lpPrev(lp, lp_ele);
   }

   lp_ele = listPackCreateInstance->lpNext(lp, lp_ele); /* Seek ID (lp_ele currently points to 'flags'). */

   /* Get the ID: it is encoded as difference between the master
    * ID and this entry ID. */
   streamID id = *master_id;
   id.ms += lpGetInteger(lp_ele);
   lp_ele = listPackCreateInstance->lpNext(lp, lp_ele);
   id.seq += lpGetInteger(lp_ele);
   *edge_id = id;
   return 1;
}
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
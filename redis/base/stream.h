/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/06
 * All rights reserved. No one may copy or transfer.
 * Description: 久化的、有序的、支持多播的消息队列，适合用于实现消息队列（MQ）系统、事件溯源、实时数据分析等场景
 */

#ifndef REDIS_BASE_STREAM_H
#define REDIS_BASE_STREAM_H
#include "define.h"
#include "rax.h"
#include <stdint.h>
#include <stdlib.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class redisObject;
typedef class redisObject robj;

typedef struct streamID {
    uint64_t ms;        /* Unix time in milliseconds. */
    uint64_t seq;       /* Sequence number. */
} streamID;

typedef struct stream {
    rax *raxl;               /* The radix tree holding the stream. */
    uint64_t length;        /* Number of elements inside this stream. */
    streamID last_id;       /* Zero if there are yet no items. */
    rax *cgroups;           /* Consumer groups dictionary: name -> streamCG */
} stream;

typedef struct streamIterator {
    stream *streaml;         /* The stream we are iterating. */
    streamID master_id;     /* ID of the master entry at listpack head. */
    uint64_t master_fields_count;       /* Master entries # of fields. */
    unsigned char *master_fields_start; /* Master entries start in listpack. */
    unsigned char *master_fields_ptr;   /* Master field to emit next. */
    int entry_flags;                    /* Flags of entry we are emitting. */
    int rev;                /* True if iterating end to start (reverse). */
    uint64_t start_key[2];  /* Start key as 128 bit big endian. */
    uint64_t end_key[2];    /* End key as 128 bit big endian. */
    raxIterator ri;         /* Rax iterator. */
    unsigned char *lp;      /* Current listpack. */
    unsigned char *lp_ele;  /* Current listpack cursor. */
    unsigned char *lp_flags; /* Current entry flags pointer. */
    /* Buffers used to hold the string of lpGet() when the element is
     * integer encoded, so that there is no string representation of the
     * element inside the listpack itself. */
    unsigned char field_buf[LP_INTBUF_SIZE];
    unsigned char value_buf[LP_INTBUF_SIZE];
} streamIterator;

/* Consumer group. */
typedef struct streamCG {
    streamID last_id;       /* Last delivered (not acknowledged) ID for this
                               group. Consumers that will just ask for more
                               messages will served with IDs > than this. */
    rax *pel;               /* Pending entries list. This is a radix tree that
                               has every message delivered to consumers (without
                               the NOACK option) that was yet not acknowledged
                               as processed. The key of the radix tree is the
                               ID as a 64 bit big endian number, while the
                               associated value is a streamNACK structure.*/
    rax *consumers;         /* A radix tree representing the consumers by name
                               and their associated representation in the form
                               of streamConsumer structures. */
} streamCG;

/* A specific consumer in a consumer group.  */
typedef struct streamConsumer {
    mstime_t seen_time;         /* Last time this consumer was active. */
    sds name;                   /* Consumer name. This is how the consumer
                                   will be identified in the consumer group
                                   protocol. Case sensitive. */
    rax *pel;                   /* Consumer specific pending entries list: all
                                   the pending messages delivered to this
                                   consumer not yet acknowledged. Keys are
                                   big endian message IDs, while values are
                                   the same streamNACK structure referenced
                                   in the "pel" of the consumer group structure
                                   itself, so the value is shared. */
} streamConsumer;

typedef struct streamNACK {
    mstime_t delivery_time;     /* Last time this message was delivered. */
    uint64_t delivery_count;    /* Number of times this message was delivered.*/
    streamConsumer *consumer;   /* The consumer this message was delivered to
                                   in the last delivery. */
} streamNACK;


typedef struct streamPropInfo {
    robj *keyname;
    robj *groupname;
} streamPropInfo;

struct client;
class sdsCreate;
class streamCreate
{
public:
    streamCreate();
    ~streamCreate();

public:
    /**
     * 创建一个新的Stream对象
     * 
     * @return 返回新创建的Stream对象指针
     */
    stream *streamNew(void);

    /**
     * 释放Stream对象及其关联的所有资源
     * 
     * @param s 要释放的Stream对象指针
     */
    void freeStream(stream *s);

    /**
     * 获取Stream的消息条目数量
     * 
     * @param subject 存储Stream的Redis对象
     * @return Stream中消息条目的数量
     */
    unsigned long streamLength(const robj *subject);

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
    size_t streamReplyWithRange(client *c, stream *s, streamID *start, streamID *end, 
                            size_t count, int rev, streamCG *group, streamConsumer *consumer, 
                            int flags, streamPropInfo *spi);

    /**
     * 初始化一个Stream迭代器
     * 
     * @param si    迭代器对象
     * @param s     要迭代的Stream对象
     * @param start 起始消息ID（可为NULL，表示从第一个/最后一个消息开始）
     * @param end   结束消息ID（可为NULL，表示到最后一个/第一个消息结束）
     * @param rev   是否反向遍历（true表示从最新到最旧）
     */
    void streamIteratorStart(streamIterator *si, stream *s, streamID *start, streamID *end, int rev);

    /**
     * 获取当前迭代器指向的消息ID和字段数量
     * 
     * @param si        迭代器对象
     * @param id        用于存储消息ID的结构体
     * @param numfields 用于存储消息字段数量的指针
     * @return 如果成功获取返回1，否则返回0
     */
    int streamIteratorGetID(streamIterator *si, streamID *id, int64_t *numfields);

    /**
     * 获取当前迭代器指向的消息的下一个字段和值
     * 
     * @param si          迭代器对象
     * @param fieldptr    用于存储字段名指针的指针
     * @param valueptr    用于存储值指针的指针
     * @param fieldlen    用于存储字段名长度的指针
     * @param valuelen    用于存储值长度的指针
     */
    void streamIteratorGetField(streamIterator *si, unsigned char **fieldptr, unsigned char **valueptr, 
                            int64_t *fieldlen, int64_t *valuelen);

    /**
     * 从Stream中删除当前迭代器指向的消息
     * 
     * @param si      迭代器对象
     * @param current 用于存储当前消息ID的结构体
     */
    void streamIteratorRemoveEntry(streamIterator *si, streamID *current);

    /**
     * 停止并释放Stream迭代器
     * 
     * @param si 迭代器对象
     */
    void streamIteratorStop(streamIterator *si);

    /**
     * 在Stream中查找指定名称的消费者组
     * 
     * @param s         Stream对象
     * @param groupname 消费者组名称
     * @return 如果找到返回消费者组指针，否则返回NULL
     */
    streamCG *streamLookupCG(stream *s, sds groupname);

    /**
     * 在消费者组中查找指定名称的消费者
     * 
     * @param cg       消费者组
     * @param name     消费者名称
     * @param flags    查找标志
     * @param created  用于存储是否创建了新消费者的指针（可为NULL）
     * @return 如果找到或创建成功返回消费者指针，否则返回NULL
     */
    streamConsumer *streamLookupConsumer(streamCG *cg, sds name, int flags, int *created);

    /**
     * 在Stream中创建新的消费者组
     * 
     * @param s         Stream对象
     * @param name      消费者组名称
     * @param namelen   消费者组名称长度
     * @param id        消费者组的起始ID
     * @return 如果创建成功返回新消费者组指针，否则返回NULL
     */
    streamCG *streamCreateCG(stream *s, char *name, size_t namelen, streamID *id);

    /**
     * 在消费者中创建新的未确认消息（NACK）
     * 
     * @param consumer 消费者对象
     * @return 返回新创建的NACK对象指针
     */
    streamNACK *streamCreateNACK(streamConsumer *consumer);

    /**
     * 从缓冲区解码Stream消息ID
     * 
     * @param buf 包含消息ID的缓冲区
     * @param id  用于存储解码后消息ID的结构体
     */
    void streamDecodeID(void *buf, streamID *id);

    /**
     * 比较两个Stream消息ID的大小
     * 
     * @param a 第一个消息ID
     * @param b 第二个消息ID
     * @return 如果a小于b返回负数，如果相等返回0，如果a大于b返回正数
     */
    int streamCompareID(streamID *a, streamID *b);

    /**
     * 释放NACK对象
     * 
     * @param na 要释放的NACK对象指针
     */
    static void streamFreeNACK(streamNACK *na);

    /**
     * 将消息ID递增
     * 
     * @param id 要递增的消息ID
     * @return 成功返回1，失败返回0
     */
    int streamIncrID(streamID *id);

    /**
     * 将消息ID递减
     * 
     * @param id 要递减的消息ID
     * @return 成功返回1，失败返回0
     */
    int streamDecrID(streamID *id);

    /**
     * 传播消费者创建操作到从节点和AOF文件
     * 
     * @param c            客户端上下文
     * @param key          Stream键对象
     * @param groupname    消费者组名称对象
     * @param consumername 消费者名称
     */
    void streamPropagateConsumerCreation(client *c, robj *key, robj *groupname, sds consumername);

    /**
     * 复制Stream对象
     * 
     * @param o 要复制的Stream对象
     * @return 返回复制后的新对象
     */
    robj *streamDup(robj *o);

    /**
     * 验证Stream底层Listpack的完整性
     * 
     * @param lp    Listpack缓冲区指针
     * @param size  Listpack大小
     * @param deep  是否进行深度验证（true表示进行深度验证）
     * @return 验证通过返回1，否则返回0
     */
    int streamValidateListpackIntegrity(unsigned char *lp, size_t size, int deep);

    /**
     * 解析字符串形式的消息ID
     * 
     * @param o 包含消息ID的Redis对象
     * @param id 用于存储解析后消息ID的结构体
     * @return 解析成功返回1，失败返回0
     */
    int streamParseID(const robj *o, streamID *id);

    /**
     * 从消息ID创建Redis对象
     * 
     * @param id 消息ID
     * @return 返回新创建的Redis对象
     */
    robj *createObjectFromStreamID(streamID *id);

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
    int streamAppendItem(stream *s, robj **argv, int64_t numfields, streamID *added_id, streamID *use_id);

    /**
     * 从Stream中删除指定ID的消息
     * 
     * @param s  Stream对象
     * @param id 要删除的消息ID
     * @return 成功删除返回1，未找到返回0
     */
    int streamDeleteItem(stream *s, streamID *id);

    /**
     * 按消息数量修剪Stream
     * 
     * @param s       Stream对象
     * @param maxlen  保留的最大消息数量
     * @param approx  是否进行近似修剪（true表示允许近似，性能更好）
     * @return 被删除的消息数量
     */
    int64_t streamTrimByLength(stream *s, long long maxlen, int approx);

    /**
     * 按消息ID修剪Stream
     * 
     * @param s      Stream对象
     * @param minid  保留的最小消息ID（小于此ID的消息将被删除）
     * @param approx 是否进行近似修剪（true表示允许近似，性能更好）
     * @return 被删除的消息数量
     */
    int64_t streamTrimByID(stream *s, streamID minid, int approx);

public:
    raxCreate  *raxCreateInstance;
    sdsCreate  *sdsCreateInstance;
public:
    static void raxFree(rax *rax);
    static void streamFreeConsumer(streamConsumer *sc);
    static void streamFreeCG(streamCG *cg);
    static void lpFree(unsigned char *lp);
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
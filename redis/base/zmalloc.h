/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> 
 * All rights reserved.
 * Date: 2025/06/12
 * Description: zmalloc - total amount of allocated memory aware version of malloc()
 */
 #ifndef __ZMALLOC_H
 #define __ZMALLOC_H

 /* Double expansion needed for stringification of macro values. */
#define __xstr(s) __str(s)//这个宏的作用很直接，就是把参数 s 字符串化。假设调用 __str(HELLO)，它会直接扩展成 "HELLO"。
#define __str(s) #s//乍一看，__xstr 和 __str 好像没什么区别，但实际上它们有着关键差异。__xstr 宏会进行两次宏展开。当你调用 __xstr(FOO) 时，首先会把 FOO 展开，接着再对展开后的结果进行字符串化操作。
/*
#define FOO bar
#define STR(s) #s
#define XSTR(s) STR(s)
STR(FOO)  // 会变成 "FOO"
XSTR(FOO) // 会变成 "bar"
*/

#if defined(USE_TCMALLOC)
#define ZMALLOC_LIB ("tcmalloc-" __xstr(TC_VERSION_MAJOR) "." __xstr(TC_VERSION_MINOR))
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif

#elif defined(USE_JEMALLOC)
#define ZMALLOC_LIB ("jemalloc-" __xstr(JEMALLOC_VERSION_MAJOR) "." __xstr(JEMALLOC_VERSION_MINOR) "." __xstr(JEMALLOC_VERSION_BUGFIX))
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif

#elif defined(__APPLE__)
#include <malloc/malloc.h>
#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_size(p)
#endif

#ifndef ZMALLOC_LIB
#define ZMALLOC_LIB "libc"

// !defined(NO_MALLOC_USABLE_SIZE)：判断是否没有定义 NO_MALLOC_USABLE_SIZE 宏。要是这个宏已经被定义了，那就意味着要禁用 malloc_usable_size 函数的使用。
// defined(__GLIBC__)：检测系统是否使用 GNU C 库（GLIBC），因为 GLIBC 提供了 malloc_usable_size 函数。
// defined(__FreeBSD__)：判断系统是否为 FreeBSD，FreeBSD 系统同样提供了 malloc_usable_size 函数。
#if !defined(NO_MALLOC_USABLE_SIZE) && \
    (defined(__GLIBC__) || defined(__FreeBSD__) || \
     defined(USE_MALLOC_USABLE_SIZE))

/* Includes for malloc_usable_size() */
#ifdef __FreeBSD__
#include <malloc_np.h>
#else
#include <malloc.h>
#endif

#define HAVE_MALLOC_SIZE 1
#define zmalloc_size(p) malloc_usable_size(p)

#endif
#endif

/* We can enable the Redis defrag capabilities only if we are using Jemalloc
 * and the version used is our special version modified for Redis having
 * the ability to return per-allocation fragmentation hints. */
#if defined(USE_JEMALLOC) && defined(JEMALLOC_FRAG_HINT)
#define HAVE_DEFRAG
#endif

class  zmalloc
{
    private:
        zmalloc(){}
        zmalloc(const zmalloc&) = delete;
        zmalloc& operator=(const zmalloc&) = delete;
        static zmalloc* instance;
    public:
        static zmalloc* getInstance(){
            if (instance == nullptr)
            {
                instance = new zmalloc();
            }
            return instance;
        }  
    public:
        /**
         * zzmalloc - 分配指定大小的内存块
         * @size: 需要分配的字节数
         *
         * 分配指定大小的内存块，不对内存内容进行初始化。
         * 如果分配失败，会调用内存不足处理函数(oom_handler)。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *zzmalloc(size_t size);

        /**
         * ztrymalloc - 尝试分配指定大小的内存块
         * @size: 需要分配的字节数
         *
         * 分配指定大小的内存块，不对内存内容进行初始化。
         * 与zzmalloc不同，分配失败时不会调用oom_handler，而是直接返回NULL。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *ztrymalloc(size_t size);

        /**
         * zmalloc_usable - 分配指定大小的内存块并获取实际可用大小
         * @size: 需要分配的字节数
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 分配指定大小的内存块，不对内存内容进行初始化，
         * 并通过usable参数返回实际分配的内存大小（可能大于请求的大小）。
         * 如果分配失败，会调用oom_handler。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *zmalloc_usable(size_t size, size_t *usable);

        /**
         * ztrymalloc_usable - 尝试分配内存并获取实际可用大小
         * @size: 需要分配的字节数
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 分配指定大小的内存块，不对内存内容进行初始化，
         * 并通过usable参数返回实际分配的内存大小。
         * 分配失败时不调用oom_handler，直接返回NULL。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *ztrymalloc_usable(size_t size, size_t *usable);

        /**
         * zcalloc - 分配并清零内存块
         * @size: 需要分配的字节数
         *
         * 分配指定大小的内存块，并将所有字节初始化为0。
         * 如果分配失败，会调用oom_handler。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *zcalloc(size_t size);

        /**
         * ztrycalloc - 尝试分配并清零内存块
         * @size: 需要分配的字节数
         *
         * 分配指定大小的内存块，并将所有字节初始化为0。
         * 分配失败时不调用oom_handler，直接返回NULL。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *ztrycalloc(size_t size);

        /**
         * zcalloc_usable - 分配、清零内存并获取实际可用大小
         * @size: 需要分配的字节数
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 分配指定大小的内存块，将所有字节初始化为0，
         * 并通过usable参数返回实际分配的内存大小。
         * 分配失败时调用oom_handler。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *zcalloc_usable(size_t size, size_t *usable);

        /**
         * ztrycalloc_usable - 尝试分配、清零内存并获取可用大小
         * @size: 需要分配的字节数
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 分配指定大小的内存块，将所有字节初始化为0，
         * 并通过usable参数返回实际分配的内存大小。
         * 分配失败时不调用oom_handler，直接返回NULL。
         *
         * 返回值: 成功时返回分配内存的指针，失败时返回NULL
         */
        void *ztrycalloc_usable(size_t size, size_t *usable);

        /**
         * zrealloc - 调整已分配内存块的大小
         * @ptr: 指向需要调整大小的内存块的指针
         * @size: 新的大小（字节）
         *
         * 调整ptr指向的内存块大小为size字节。
         * 如果ptr为NULL，等效于调用zzmalloc(size)。
         * 如果size为0且ptr不为NULL，等效于调用zfree(ptr)并返回NULL。
         * 可能会移动内存位置，移动后原内存区域内容会被复制到新位置。
         * 如果分配失败，会调用oom_handler，原内存块不会被释放。
         *
         * 返回值: 成功时返回调整后内存的指针，失败时返回NULL
         */
        void *zrealloc(void *ptr, size_t size);

        /**
         * ztryrealloc - 尝试调整已分配内存块的大小
         * @ptr: 指向需要调整大小的内存块的指针
         * @size: 新的大小（字节）
         *
         * 功能与zrealloc类似，但分配失败时不调用oom_handler，直接返回NULL，
         * 原内存块不会被释放。
         *
         * 返回值: 成功时返回调整后内存的指针，失败时返回NULL
         */
        void *ztryrealloc(void *ptr, size_t size);

        /**
         * zrealloc_usable - 调整内存大小并获取实际可用大小
         * @ptr: 指向需要调整大小的内存块的指针
         * @size: 新的大小（字节）
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 调整内存块大小并通过usable参数返回实际分配的内存大小。
         * 其他行为与zrealloc相同。
         *
         * 返回值: 成功时返回调整后内存的指针，失败时返回NULL
         */
        void *zrealloc_usable(void *ptr, size_t size, size_t *usable);

        /**
         * ztryrealloc_usable - 尝试调整内存大小并获取可用大小
         * @ptr: 指向需要调整大小的内存块的指针
         * @size: 新的大小（字节）
         * @usable: 用于存储实际分配内存大小的指针
         *
         * 功能与zrealloc_usable类似，但分配失败时不调用oom_handler，
         * 直接返回NULL，原内存块不会被释放。
         *
         * 返回值: 成功时返回调整后内存的指针，失败时返回NULL
         */
        void *ztryrealloc_usable(void *ptr, size_t size, size_t *usable);

        /**
         * zfree - 释放已分配的内存块
         * @ptr: 指向需要释放的内存块的指针
         *
         * 释放ptr指向的内存块。如果ptr为NULL，不执行任何操作。
         * 释放后，ptr变为无效指针，不应再被使用。
         */
        void zfree(void *ptr);

        /**
         * zfree_usable - 释放已分配的内存块并获取可用大小
         * @ptr: 指向需要释放的内存块的指针
         * @usable: 用于存储内存块实际大小的指针
         *
         * 释放ptr指向的内存块，并通过usable参数返回该内存块的实际大小。
         * 如果ptr为NULL，不执行任何操作，usable会被设置为0。
         */
        void zfree_usable(void *ptr, size_t *usable);

        /**
         * zstrdup - 复制字符串
         * @s: 指向需要复制的字符串的指针
         *
         * 分配足够的内存来存储字符串s的副本（包括终止符'\0'），
         * 并将s的内容复制到新分配的内存中。
         * 如果分配失败，会调用oom_handler。
         *
         * 返回值: 成功时返回指向新字符串的指针，失败时返回NULL
         */
        char *zstrdup(const char *s);

        /**
         * zmalloc_used_memory - 获取已使用的内存总量
         *
         * 返回当前程序通过本内存分配器分配的内存总量（字节）。
         * 该值反映了应用程序的内存使用情况，包括所有已分配但尚未释放的内存。
         *
         * 返回值: 已使用的内存总量（字节）
         */
        size_t zmalloc_used_memory(void);

        /**
         * zmalloc_set_oom_handler - 设置内存不足处理函数
         * @oom_handler: 内存不足时调用的函数指针，参数为请求分配的大小
         *
         * 设置当内存分配失败时调用的处理函数。
         * 传入NULL可恢复默认行为（打印错误信息并终止程序）。
         */
        void zmalloc_set_oom_handler(void (*oom_handler)(size_t));

        /**
         * zmalloc_get_rss - 获取常驻内存大小
         *
         * 返回当前进程的常驻集大小(Resident Set Size, RSS)，
         * 即当前实际驻留在物理内存中的内存量（字节）。
         *
         * 返回值: 常驻内存大小（字节），失败时返回0
         */
        size_t zmalloc_get_rss(void);

        /**
         * zmalloc_get_allocator_info - 获取内存分配器的详细信息
         * @allocated: 用于存储已分配内存总量的指针
         * @active: 用于存储活跃内存块大小的指针
         * @resident: 用于存储常驻内存大小的指针
         *
         * 获取内存分配器的详细统计信息，包括：
         * - 已分配的内存总量（包括元数据）
         * - 活跃内存块的大小（实际使用的内存）
         * - 常驻内存大小（物理内存中占用的部分）
         *
         * 返回值: 成功时返回0，失败时返回-1
         */
        int zmalloc_get_allocator_info(size_t *allocated, size_t *active, size_t *resident);

        /**
         * set_jemalloc_bg_thread - 启用或禁用jemalloc后台线程
         * @enable: 非零值启用后台线程，0禁用后台线程
         *
         * 如果使用jemalloc作为内存分配器，此函数可控制是否启用其后台线程。
         * 后台线程用于执行内存碎片整理等维护任务。
         */
        void set_jemalloc_bg_thread(int enable);

        /**
         * jemalloc_purge - 强制jemalloc释放未使用的物理内存
         *
         * 通知jemalloc将未使用的物理内存归还给操作系统。
         * 这可能会减少进程的RSS，但可能会影响后续内存分配的性能。
         *
         * 返回值: 成功时返回0，失败时返回非零值
         */
        int jemalloc_purge();

        /**
         * zmalloc_get_private_dirty - 获取进程的私有脏页内存大小
         * @pid: 进程ID，指定为0表示当前进程
         *
         * 返回指定进程的私有脏页(private dirty pages)占用的内存大小（字节）。
         * 私有脏页是指该进程独有的、已被修改且尚未写入磁盘的内存页。
         *
         * 返回值: 私有脏页内存大小（字节），失败时返回0
         */
        size_t zmalloc_get_private_dirty(long pid);

        /**
         * zmalloc_get_smap_bytes_by_field - 从/proc/pid/smaps获取特定字段的内存信息
         * @field: 要查找的字段名称，如"Rss:"、"Pss:"等
         * @pid: 进程ID，指定为0表示当前进程
         *
         * 解析/proc/pid/smaps文件，累加所有内存区域中指定字段的值，
         * 返回该字段表示的内存总量（字节）。
         *
         * 返回值: 指定字段的内存总量（字节），失败时返回0
         */
        size_t zmalloc_get_smap_bytes_by_field(char *field, long pid);

        /**
         * zmalloc_get_memory_size - 获取系统总内存大小
         *
         * 返回系统的物理内存总量（字节）。
         *
         * 返回值: 系统总内存大小（字节），失败时返回0
         */
        size_t zmalloc_get_memory_size(void);

        /**
         * zlibc_free - 使用标准C库的free函数释放内存
         * @ptr: 指向需要释放的内存块的指针
         *
         * 使用标准C库的free函数释放内存，而不是当前内存分配器的释放函数。
         * 用于释放由标准C库的malloc、calloc、realloc等函数分配的内存。
         * 如果ptr为NULL，不执行任何操作。
         */
        void zlibc_free(void *ptr);
        //程序终止回调
        static void zmalloc_default_oom(size_t size);

        #ifdef HAVE_DEFRAG
        void zfree_no_tcache(void *ptr);
        void *zmalloc_no_tcache(size_t size);
        #endif

        #ifndef HAVE_MALLOC_SIZE
        size_t zmalloc_size(void *ptr);
        size_t zmalloc_usable_size(void *ptr);
        #else
        #define zmalloc_usable_size(p) zmalloc_size(p)
        #endif
};
 #endif
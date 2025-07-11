/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> 
 * All rights reserved.
 * Date: 2025/06/12
 * Description: zmalloc - total amount of allocated memory aware version of malloc()
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "atomicvar.h"
#include "zmalloc.h"

#ifdef HAVE_MALLOC_SIZE
#define PREFIX_SIZE (0)
#define ASSERT_NO_SIZE_OVERFLOW(sz)
#else
#if defined(__sun) || defined(__sparc) || defined(__sparc__)
#define PREFIX_SIZE (sizeof(long long))
#else
#define PREFIX_SIZE (sizeof(size_t))
#endif
//哪些平台定义了HAVE_MALLOC_SIZE
// Linux (GLIBC)	✅ 是	定义了 __GLIBC__ 且默认提供 malloc_usable_size()
// FreeBSD	✅ 是	定义了 __FreeBSD__ 且提供 malloc_usable_size()（通过 <malloc_np.h>）
// macOS (libc)	❌ 否	标准 libc 不提供 malloc_usable_size()，需依赖 malloc_size()
// Windows (MSVC)	❌ 否	无 malloc_usable_size() 函数，需自定义实现
// Alpine Linux (musl)	❌ 否	musl libc 可能不提供 malloc_usable_size()
//总分配大小 = 头部大小（PREFIX_SIZE） + 用户请求大小（sz）
#define ASSERT_NO_SIZE_OVERFLOW(sz) assert((sz) + PREFIX_SIZE > (sz))
#endif
#define MALLOC_MIN_SIZE(x) ((x) > 0 ? (x) : sizeof(long))
#define update_zmalloc_stat_alloc(__n) atomicIncr(used_memory,(__n))
#define update_zmalloc_stat_free(__n) atomicDecr(used_memory,(__n))
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
static void (*zmalloc_oom_handler)(size_t) = zmalloc::zmalloc_default_oom;
static redisAtomic size_t used_memory = 0;
//程序终止回调
void zmalloc::zmalloc_default_oom(size_t size) {
    fprintf(stderr, "zmalloc: Out of memory trying to allocate %zu bytes\n",
        size);
    fflush(stderr);
    abort();
}
zmalloc* zmalloc::instance = nullptr;
/**
 * zzmalloc - 分配指定大小的内存块
 * @size: 需要分配的字节数
 *
 * 分配指定大小的内存块，不对内存内容进行初始化。
 * 如果分配失败，会调用内存不足处理函数(oom_handler)。
 *
 * 返回值: 成功时返回分配内存的指针，失败时返回NULL
 */
void * zmalloc::zzmalloc(size_t size)
{
    void *ptr = ztrymalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}
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
void *zmalloc::ztrymalloc_usable(size_t size, size_t *usable) 
{
    ASSERT_NO_SIZE_OVERFLOW(size);
    void *ptr = malloc(MALLOC_MIN_SIZE(size)+PREFIX_SIZE);

    if (!ptr) return NULL;
#ifdef HAVE_MALLOC_SIZE
    size = zmalloc_size(ptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return ptr;
#else
    *((size_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    if (usable) *usable = size;
    return (char*)ptr+PREFIX_SIZE;
#endif

}
/**
 * zcalloc - 分配并清零内存块
 * @size: 需要分配的字节数
 *
 * 分配指定大小的内存块，并将所有字节初始化为0。
 * 如果分配失败，会调用oom_handler。
 *
 * 返回值: 成功时返回分配内存的指针，失败时返回NULL
 */
void *zmalloc::zcalloc(size_t size)
{
    void *ptr = ztrycalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}
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
void *zmalloc::zrealloc(void *ptr, size_t size)
{
    ptr = ztryrealloc_usable(ptr, size, NULL);
    if (!ptr && size != 0) zmalloc_oom_handler(size);
    return ptr;
}
/**
 * ztrymalloc - 尝试分配指定大小的内存块
 * @size: 需要分配的字节数
 *
 * 分配指定大小的内存块，不对内存内容进行初始化。
 * 与zzmalloc不同，分配失败时不会调用oom_handler，而是直接返回NULL。
 *
 * 返回值: 成功时返回分配内存的指针，失败时返回NULL
 */
void *zmalloc::ztrymalloc(size_t size)
{
    void *ptr = ztrymalloc_usable(size, NULL);
    return ptr;
}
/**
 * ztrycalloc - 尝试分配并清零内存块
 * @size: 需要分配的字节数
 *
 * 分配指定大小的内存块，并将所有字节初始化为0。
 * 分配失败时不调用oom_handler，直接返回NULL。
 *
 * 返回值: 成功时返回分配内存的指针，失败时返回NULL
 */
void *zmalloc::ztrycalloc(size_t size)
{
    void *ptr = ztrycalloc_usable(size, NULL);
    return ptr;
}
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
void *zmalloc::ztryrealloc(void *ptr, size_t size)
{
    ptr = ztryrealloc_usable(ptr, size, NULL);
    return ptr;
}
/**
 * zfree - 释放已分配的内存块
 * @ptr: 指向需要释放的内存块的指针
 *
 * 释放ptr指向的内存块。如果ptr为NULL，不执行任何操作。
 * 释放后，ptr变为无效指针，不应再被使用。
 */
void zmalloc::zfree(void *ptr)
{
    #ifndef HAVE_MALLOC_SIZE
    void *realptr;
    size_t oldsize;
#endif

    if (ptr == NULL) return;
#ifdef HAVE_MALLOC_SIZE
    update_zmalloc_stat_free(zmalloc_size(ptr));
    free(ptr);
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((size_t*)realptr);
    update_zmalloc_stat_free(oldsize+PREFIX_SIZE);
    free(realptr);
#endif
}
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
void *zmalloc::zmalloc_usable(size_t size, size_t *usable)
{
    void *ptr = ztrymalloc_usable(size, usable);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}
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
void *zmalloc::zcalloc_usable(size_t size, size_t *usable)
{
    void *ptr = ztrycalloc_usable(size, usable);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}
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
void *zmalloc::zrealloc_usable(void *ptr, size_t size, size_t *usable)
{
    ptr = ztryrealloc_usable(ptr, size, usable);
    if (!ptr && size != 0) zmalloc_oom_handler(size);
    return ptr;
}

// *使用 malloc 的场景：
// 当你要立即用数据填充分配的内存时
// 当性能是关键因素时
// 当你要加载已有数据到内存时

// *使用 calloc 的场景：
// 当你需要初始化为 0 的内存时（如用于数值计算）
// 当你需要清零的内存用于敏感数据时
// 当你要分配结构体数组并希望所有字段初始化为 0 时
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
void *zmalloc::ztrycalloc_usable(size_t size, size_t *usable)
{
    ASSERT_NO_SIZE_OVERFLOW(size);
    void *ptr = calloc(1, MALLOC_MIN_SIZE(size)+PREFIX_SIZE);
    if (ptr == NULL) return NULL;

#ifdef HAVE_MALLOC_SIZE
    size = zmalloc_size(ptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return ptr;
#else
    *((size_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    if (usable) *usable = size;
    return (char*)ptr+PREFIX_SIZE;
#endif
}

// 尝试重新分配已分配的内存块
// 如果分配失败，返回 NULL
// 通过 usable 指针返回实际可用的内存大小
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
void *zmalloc::ztryrealloc_usable(void *ptr, size_t size, size_t *usable)
{
    ASSERT_NO_SIZE_OVERFLOW(size);
#ifndef HAVE_MALLOC_SIZE
    void *realptr;
#endif
    size_t oldsize;
    void *newptr;

    /* not allocating anything, just redirect to free. */
    if (size == 0 && ptr != NULL) {
        zfree(ptr);
        if (usable) *usable = 0;
        return NULL;
    }
    /* Not freeing anything, just redirect to malloc. */
    if (ptr == NULL)
        return ztrymalloc_usable(size, usable);

#ifdef HAVE_MALLOC_SIZE
    oldsize = zmalloc_size(ptr);
    newptr = realloc(ptr,size);
    if (newptr == NULL) {
        if (usable) *usable = 0;
        return NULL;
    }

    update_zmalloc_stat_free(oldsize);
    size = zmalloc_size(newptr);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return newptr;
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((size_t*)realptr);
    newptr = realloc(realptr,size+PREFIX_SIZE);
    if (newptr == NULL) {
        if (usable) *usable = 0;
        return NULL;
    }

    *((size_t*)newptr) = size;
    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(size);
    if (usable) *usable = size;
    return (char*)newptr+PREFIX_SIZE;
#endif
}
/**
 * zfree_usable - 释放已分配的内存块并获取可用大小
 * @ptr: 指向需要释放的内存块的指针
 * @usable: 用于存储内存块实际大小的指针
 *
 * 释放ptr指向的内存块，并通过usable参数返回该内存块的实际大小。
 * 如果ptr为NULL，不执行任何操作，usable会被设置为0。
 */
void zmalloc::zfree_usable(void *ptr, size_t *usable)
{
    #ifndef HAVE_MALLOC_SIZE
    void *realptr;
    size_t oldsize;
#endif

    if (ptr == NULL) return;
#ifdef HAVE_MALLOC_SIZE
    update_zmalloc_stat_free(*usable = zmalloc_size(ptr));
    free(ptr);
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    *usable = oldsize = *((size_t*)realptr);
    update_zmalloc_stat_free(oldsize+PREFIX_SIZE);
    free(realptr);
#endif
}
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
// 复制输入字符串 s 到新分配的内存中
// 返回指向新字符串的指针
// 使用 Redis 自己的内存分配函数 zmalloc
char *zmalloc::zstrdup(const char *s)
{
    size_t l = strlen(s)+1;
    char *p = (char*)zzmalloc(l);
    memcpy(p,s,l);
    return p;
}
/**
 * zmalloc_used_memory - 获取已使用的内存总量
 *
 * 返回当前程序通过本内存分配器分配的内存总量（字节）。
 * 该值反映了应用程序的内存使用情况，包括所有已分配但尚未释放的内存。
 *
 * 返回值: 已使用的内存总量（字节）
 */
size_t zmalloc::zmalloc_used_memory(void)
{
    size_t um;
    atomicGet(used_memory,um);
    return um;
}
// 允许调用者注册一个自定义的内存分配失败处理函数
// 将传入的函数指针保存到全局变量中
// 当 zmalloc 等内存分配函数失败时，会调用这个处理函数

/**
 * zmalloc_set_oom_handler - 设置内存不足处理函数
 * @oom_handler: 内存不足时调用的函数指针，参数为请求分配的大小
 *
 * 设置当内存分配失败时调用的处理函数。
 * 传入NULL可恢复默认行为（打印错误信息并终止程序）。
 */
void zmalloc::zmalloc_set_oom_handler(void (*oom_handler)(size_t))
{
    zmalloc_oom_handler = oom_handler;
}


// 以操作系统特定的方式获取进程的实际内存使用量
// 返回值单位为字节
// 注意：该函数不是为高性能设计的，不应在关键路径中频繁调用
#if defined(HAVE_PROC_STAT)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
/**
 * zmalloc_get_rss - 获取常驻内存大小
 *
 * 返回当前进程的常驻集大小(Resident Set Size, RSS)，
 * 即当前实际驻留在物理内存中的内存量（字节）。
 *
 * 返回值: 常驻内存大小（字节），失败时返回0
 */
size_t zmalloc::zmalloc_get_rss(void) {
    int page = sysconf(_SC_PAGESIZE);
    size_t rss;
    char buf[4096];
    char filename[256];
    int fd, count;
    char *p, *x;

    snprintf(filename,256,"/proc/%ld/stat",(long) getpid());
    if ((fd = open(filename,O_RDONLY)) == -1) return 0;
    if (read(fd,buf,4096) <= 0) {
        close(fd);
        return 0;
    }
    close(fd);

    p = buf;
    count = 23; /* RSS is the 24th field in /proc/<pid>/stat */
    while(p && count--) {
        p = strchr(p,' ');
        if (p) p++;
    }
    if (!p) return 0;
    x = strchr(p,' ');
    if (!x) return 0;
    *x = '\0';

    rss = strtoll(p,NULL,10);
    rss *= page;
    return rss;
}
#elif defined(HAVE_TASKINFO)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/task.h>
#include <mach/mach_init.h>

size_t zmalloc::zmalloc_get_rss(void) {
    task_t task = MACH_PORT_NULL;
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (task_for_pid(current_task(), getpid(), &task) != KERN_SUCCESS)
        return 0;
    task_info(task, TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);

    return t_info.resident_size;
}
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>

size_t zmalloc::zmalloc_get_rss(void) {
    struct kinfo_proc info;
    size_t infolen = sizeof(info);
    int mib[4];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    if (sysctl(mib, 4, &info, &infolen, NULL, 0) == 0)
#if defined(__FreeBSD__)
        return (size_t)info.ki_rssize * getpagesize();
#else
        return (size_t)info.kp_vm_rssize * getpagesize();
#endif

    return 0L;
}
#elif defined(__NetBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>

size_t zmalloc::zmalloc_get_rss(void) {
    struct kinfo_proc2 info;
    size_t infolen = sizeof(info);
    int mib[6];
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();
    mib[4] = sizeof(info);
    mib[5] = 1;
    if (sysctl(mib, 4, &info, &infolen, NULL, 0) == 0)
        return (size_t)info.p_vm_rssize * getpagesize();

    return 0L;
}
#elif defined(HAVE_PSINFO)
#include <unistd.h>
#include <sys/procfs.h>
#include <fcntl.h>

size_t zmalloc::zmalloc_get_rss(void) {
    struct prpsinfo info;
    char filename[256];
    int fd;

    snprintf(filename,256,"/proc/%ld/psinfo",(long) getpid());

    if ((fd = open(filename,O_RDONLY)) == -1) return 0;
    if (ioctl(fd, PIOCPSINFO, &info) == -1) {
        close(fd);
	return 0;
    }

    close(fd);
    return info.pr_rssize;
}
#else
size_t zmalloc::zmalloc_get_rss(void) {
    /* If we can't get the RSS in an OS-specific way for this system just
     * return the memory usage we estimated in zmalloc()..
     *
     * Fragmentation will appear to be always 1 (no fragmentation)
     * of course... */
    return zmalloc_used_memory();
}
#endif

// 这些函数在 Redis 中的主要用途：
// 内存监控：
// 在 INFO memory 命令中显示更详细的内存使用信息
// 帮助用户了解内存分配器的行为
// 内存碎片检测：
// 通过比较 allocated 和 active 可以检测内存碎片
// 高碎片率表示内存分配效率低下
// 内存优化：
// 可以在内存压力大时调用 jemalloc_purge()
// 可以在低峰期启用后台线程进行内存回收

#if defined(USE_JEMALLOC)

int zmalloc::zmalloc_get_allocator_info(size_t *allocated,
                               size_t *active,
                               size_t *resident) {
    uint64_t epoch = 1;
    size_t sz;
    *allocated = *resident = *active = 0;
    /* Update the statistics cached by mallctl. */
    sz = sizeof(epoch);
    je_mallctl("epoch", &epoch, &sz, &epoch, sz);
    sz = sizeof(size_t);
    /* Unlike RSS, this does not include RSS from shared libraries and other non
     * heap mappings. */
    je_mallctl("stats.resident", resident, &sz, NULL, 0);
    /* Unlike resident, this doesn't not include the pages jemalloc reserves
     * for re-use (purge will clean that). */
    je_mallctl("stats.active", active, &sz, NULL, 0);
    /* Unlike zmalloc_used_memory, this matches the stats.resident by taking
     * into account all allocations done by this process (not only zmalloc). */
    je_mallctl("stats.allocated", allocated, &sz, NULL, 0);
    return 1;
}

void zmalloc::set_jemalloc_bg_thread(int enable) {
    /* let jemalloc do purging asynchronously, required when there's no traffic 
     * after flushdb */
    char val = !!enable;
    je_mallctl("background_thread", NULL, 0, &val, 1);
}

int zmalloc::jemalloc_purge() {
    /* return all unused (reserved) pages to the OS */
    char tmp[32];
    unsigned narenas = 0;
    size_t sz = sizeof(unsigned);
    if (!je_mallctl("arenas.narenas", &narenas, &sz, NULL, 0)) {
        sprintf(tmp, "arena.%d.purge", narenas);
        if (!je_mallctl(tmp, NULL, 0, NULL, 0))
            return 0;
    }
    return -1;
}

#else
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
int zmalloc::zmalloc_get_allocator_info(size_t *allocated,
                               size_t *active,
                               size_t *resident) {
    *allocated = *resident = *active = 0;
    return 1;
}
/**
 * set_jemalloc_bg_thread - 启用或禁用jemalloc后台线程
 * @enable: 非零值启用后台线程，0禁用后台线程
 *
 * 如果使用jemalloc作为内存分配器，此函数可控制是否启用其后台线程。
 * 后台线程用于执行内存碎片整理等维护任务。
 */
void zmalloc::set_jemalloc_bg_thread(int enable) {
    ((void)(enable));
}
/**
 * jemalloc_purge - 强制jemalloc释放未使用的物理内存
 *
 * 通知jemalloc将未使用的物理内存归还给操作系统。
 * 这可能会减少进程的RSS，但可能会影响后续内存分配的性能。
 *
 * 返回值: 成功时返回0，失败时返回非零值
 */
int zmalloc::jemalloc_purge() {
    return 0;
}

#endif

//是一个用于精确测量进程实际消耗物理内存的工具，特别适合需要严格控制内存使用的场景（如高性能服务器、嵌入式系统）。
//通过监控私有脏页，可以更准确地定位内存问题，优化资源利用率。

//返回当前进程占用的私有脏页内存大小（字节）。
//帮助开发者识别内存泄漏或过度分配的问题（例如，持续增长的私有脏页可能表示内存泄漏）。
/**
 * zmalloc_get_private_dirty - 获取进程的私有脏页内存大小
 * @pid: 进程ID，指定为0表示当前进程
 *
 * 返回指定进程的私有脏页(private dirty pages)占用的内存大小（字节）。
 * 私有脏页是指该进程独有的、已被修改且尚未写入磁盘的内存页。
 *
 * 返回值: 私有脏页内存大小（字节），失败时返回0
 */
size_t zmalloc::zmalloc_get_private_dirty(long pid)
{
    return zmalloc_get_smap_bytes_by_field("Private_Dirty:",pid);
}

// 根据操作系统类型，以不同方式获取进程内存使用信息
// 统计指定内存字段（如 Rss:、Private_Dirty: 等）的总字节数
// 支持查询当前进程或指定进程的内存信息
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
/* Get the sum of the specified field (converted form kb to bytes) in
 * /proc/self/smaps. The field must be specified with trailing ":" as it
 * apperas in the smaps output.
 *
 * If a pid is specified, the information is extracted for such a pid,
 * otherwise if pid is -1 the information is reported is about the
 * current process.
 *
 * Example: zmalloc_get_smap_bytes_by_field("Rss:",-1);
 */
#if defined(HAVE_PROC_SMAPS)
size_t zmalloc::zmalloc_get_smap_bytes_by_field(char *field, long pid) {
    char line[1024];
    size_t bytes = 0;
    int flen = strlen(field);
    FILE *fp;

    if (pid == -1) {
        fp = fopen("/proc/self/smaps","r");
    } else {
        char filename[128];
        snprintf(filename,sizeof(filename),"/proc/%ld/smaps",pid);
        fp = fopen(filename,"r");
    }

    if (!fp) return 0;
    while(fgets(line,sizeof(line),fp) != NULL) {
        if (strncmp(line,field,flen) == 0) {
            char *p = strchr(line,'k');
            if (p) {
                *p = '\0';
                bytes += strtol(line+flen,NULL,10) * 1024;
            }
        }
    }
    fclose(fp);
    return bytes;
}
#else
/* Get sum of the specified field from libproc api call.
 * As there are per page value basis we need to convert
 * them accordingly.
 *
 * Note that AnonHugePages is a no-op as THP feature
 * is not supported in this platform
 */
size_t zmalloc::zmalloc_get_smap_bytes_by_field(char *field, long pid) {
#if defined(__APPLE__)
    struct proc_regioninfo pri;
    if (pid == -1) pid = getpid();
    if (proc_pidinfo(pid, PROC_PIDREGIONINFO, 0, &pri,
                     PROC_PIDREGIONINFO_SIZE) == PROC_PIDREGIONINFO_SIZE)
    {
        int pagesize = getpagesize();
        if (!strcmp(field, "Private_Dirty:")) {
            return (size_t)pri.pri_pages_dirtied * pagesize;
        } else if (!strcmp(field, "Rss:")) {
            return (size_t)pri.pri_pages_resident * pagesize;
        } else if (!strcmp(field, "AnonHugePages:")) {
            return 0;
        }
    }
    return 0;
#endif
    ((void) field);
    ((void) pid);
    return 0;
}
#endif

// 在 Unix/Linux 系统上，尝试通过不同方法获取系统的物理内存大小
// 根据不同的系统类型和可用 API，选择最合适的查询方式
// 统一返回size_t类型的内存大小值（以字节为单位）

// 代码中使用了三种主要方法来获取物理内存：
// sysctl(HW_MEMSIZE/HW_PHYSMEM64): 适用于 OSX、NetBSD、OpenBSD 等系统
// sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE): 适用于 FreeBSD、Linux、Solaris 等系统
// sysctl(HW_PHYSMEM/HW_REALMEM): 适用于 DragonFly BSD、FreeBSD 等系统
/**
 * zmalloc_get_memory_size - 获取系统总内存大小
 *
 * 返回系统的物理内存总量（字节）。
 *
 * 返回值: 系统总内存大小（字节），失败时返回0
 */
size_t zmalloc::zmalloc_get_memory_size(void) {
#if defined(__unix__) || defined(__unix) || defined(unix) || \
    (defined(__APPLE__) && defined(__MACH__))
#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
    int mib[2];
    mib[0] = CTL_HW;
#if defined(HW_MEMSIZE)
    mib[1] = HW_MEMSIZE;            /* OSX. --------------------- */
#elif defined(HW_PHYSMEM64)
    mib[1] = HW_PHYSMEM64;          /* NetBSD, OpenBSD. --------- */
#endif
    int64_t size = 0;               /* 64-bit */
    size_t len = sizeof(size);
    if (sysctl( mib, 2, &size, &len, NULL, 0) == 0)
        return (size_t)size;
    return 0L;          /* Failed? */

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
    /* FreeBSD, Linux, OpenBSD, and Solaris. -------------------- */
    return (size_t)sysconf(_SC_PHYS_PAGES) * (size_t)sysconf(_SC_PAGESIZE);

#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
    /* DragonFly BSD, FreeBSD, NetBSD, OpenBSD, and OSX. -------- */
    int mib[2];
    mib[0] = CTL_HW;
#if defined(HW_REALMEM)
    mib[1] = HW_REALMEM;        /* FreeBSD. ----------------- */
#elif defined(HW_PHYSMEM)
    mib[1] = HW_PHYSMEM;        /* Others. ------------------ */
#endif
    unsigned int size = 0;      /* 32-bit */
    size_t len = sizeof(size);
    if (sysctl(mib, 2, &size, &len, NULL, 0) == 0)
        return (size_t)size;
    return 0L;          /* Failed? */
#else
    return 0L;          /* Unknown method to get the data. */
#endif
#else
    return 0L;          /* Unknown OS. */
#endif
}
/**
 * zlibc_free - 使用标准C库的free函数释放内存
 * @ptr: 指向需要释放的内存块的指针
 *
 * 使用标准C库的free函数释放内存，而不是当前内存分配器的释放函数。
 * 用于释放由标准C库的malloc、calloc、realloc等函数分配的内存。
 * 如果ptr为NULL，不执行任何操作。
 */
void zmalloc::zlibc_free(void *ptr)
{
     free(ptr);
}


#ifdef HAVE_DEFRAG
void zmalloc::zfree_no_tcache(void *ptr)
{
    if (ptr == NULL) return;
    update_zmalloc_stat_free(zmalloc_size(ptr));
    dallocx(ptr, MALLOCX_TCACHE_NONE);
}
void *zmalloc::zmalloc_no_tcache(size_t size)
{
    ASSERT_NO_SIZE_OVERFLOW(size);
    void *ptr = mallocx(size+PREFIX_SIZE, MALLOCX_TCACHE_NONE);
    if (!ptr) zmalloc_oom_handler(size);
    update_zmalloc_stat_alloc(zmalloc_size(ptr));
    return ptr;
}
#endif
// [ size_t 类型的大小信息 | 用户数据 ]
// <---- PREFIX_SIZE ----><-- 用户空间 -->
#ifndef HAVE_MALLOC_SIZE
size_t zmalloc::zmalloc_size(void *ptr)
{
    void *realptr = (char*)ptr-PREFIX_SIZE;
    size_t size = *((size_t*)realptr);
    return size+PREFIX_SIZE;
}
size_t zmalloc::zmalloc_usable_size(void *ptr)
{
    return zmalloc_size(ptr)-PREFIX_SIZE;
}
#else
#define zmalloc_usable_size(p) zmalloc_size(p)
#endif

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
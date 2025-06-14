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
//总分配大小 = 头部大小（PREFIX_SIZE） + 用户请求大小（sz）
#define ASSERT_NO_SIZE_OVERFLOW(sz) assert((sz) + PREFIX_SIZE > (sz))
#endif
#define MALLOC_MIN_SIZE(x) ((x) > 0 ? (x) : sizeof(long))
#define update_zmalloc_stat_alloc(__n) atomicIncr(used_memory,(__n))
#define update_zmalloc_stat_free(__n) atomicDecr(used_memory,(__n))
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
void * zmalloc::zzmalloc(size_t size)
{
    void *ptr = ztrymalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

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

void *zmalloc::zcalloc(size_t size)
{
    void *ptr = ztrycalloc_usable(size, NULL);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

void *zmalloc::zrealloc(void *ptr, size_t size)
{
    ptr = ztryrealloc_usable(ptr, size, NULL);
    if (!ptr && size != 0) zmalloc_oom_handler(size);
    return ptr;
}

void *zmalloc::ztrymalloc(size_t size)
{
    void *ptr = ztrymalloc_usable(size, NULL);
    return ptr;
}

void *zmalloc::ztrycalloc(size_t size)
{
    void *ptr = ztrycalloc_usable(size, NULL);
    return ptr;
}

void *zmalloc::ztryrealloc(void *ptr, size_t size)
{
    ptr = ztryrealloc_usable(ptr, size, NULL);
    return ptr;
}

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

void *zmalloc::zmalloc_usable(size_t size, size_t *usable)
{
    void *ptr = ztrymalloc_usable(size, usable);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

void *zmalloc::zcalloc_usable(size_t size, size_t *usable)
{
    void *ptr = ztrycalloc_usable(size, usable);
    if (!ptr) zmalloc_oom_handler(size);
    return ptr;
}

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

size_t zmalloc::zmalloc_used_memory(void)
{
    size_t um;
    atomicGet(used_memory,um);
    return um;
}
// 允许调用者注册一个自定义的内存分配失败处理函数
// 将传入的函数指针保存到全局变量中
// 当 zmalloc 等内存分配函数失败时，会调用这个处理函数
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

int zmalloc::zmalloc_get_allocator_info(size_t *allocated,
                               size_t *active,
                               size_t *resident) {
    *allocated = *resident = *active = 0;
    return 1;
}
void zmalloc::set_jemalloc_bg_thread(int enable) {
    ((void)(enable));
}

int zmalloc::jemalloc_purge() {
    return 0;
}

#endif

//是一个用于精确测量进程实际消耗物理内存的工具，特别适合需要严格控制内存使用的场景（如高性能服务器、嵌入式系统）。
//通过监控私有脏页，可以更准确地定位内存问题，优化资源利用率。

//返回当前进程占用的私有脏页内存大小（字节）。
//帮助开发者识别内存泄漏或过度分配的问题（例如，持续增长的私有脏页可能表示内存泄漏）。

size_t zmalloc::zmalloc_get_private_dirty(long pid)
{
    return zmalloc_get_smap_bytes_by_field("Private_Dirty:",pid);
}

// 根据操作系统类型，以不同方式获取进程内存使用信息
// 统计指定内存字段（如 Rss:、Private_Dirty: 等）的总字节数
// 支持查询当前进程或指定进程的内存信息

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
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

// void *zmalloc::zcalloc(size_t size)
// {
    
// }

// void *zmalloc::zrealloc(void *ptr, size_t size)
// {
    
// }

// void *zmalloc::ztrymalloc(size_t size)
// {
    
// }

// void *zmalloc::ztrycalloc(size_t size)
// {
    
// }

// void *zmalloc::ztryrealloc(void *ptr, size_t size)
// {
    
// }

// void zmalloc::zfree(void *ptr)
// {
    
// }

// void *zmalloc::zmalloc_usable(size_t size, size_t *usable)
// {
    
// }

// void *zmalloc::zcalloc_usable(size_t size, size_t *usable)
// {
    
// }

// void *zmalloc::zrealloc_usable(void *ptr, size_t size, size_t *usable)
// {
    
// }

// void *zmalloc::ztrycalloc_usable(size_t size, size_t *usable)
// {
    
// }

// void *zmalloc::ztryrealloc_usable(void *ptr, size_t size, size_t *usable)
// {
    
// }

// void zmalloc::zfree_usable(void *ptr, size_t *usable)
// {
    
// }

// char *zmalloc::zstrdup(const char *s)
// {
    
// }

// size_t zmalloc::zmalloc_used_memory(void)
// {
    
// }

// void zmalloc::zmalloc_set_oom_handler(void (*oom_handler)(size_t))
// {
    
// }

// size_t zmalloc::zmalloc_get_rss(void)
// {
    
// }

// int zmalloc::zmalloc_get_allocator_info(size_t *allocated, size_t *active, size_t *resident)
// {
    
// }

// void zmalloc::set_jemalloc_bg_thread(int enable)
// {
    
// }

// int zmalloc::jemalloc_purge()
// {
    
// }

// size_t zmalloc::zmalloc_get_private_dirty(long pid)
// {
    
// }

// size_t zmalloc::zmalloc_get_smap_bytes_by_field(char *field, long pid)
// {
    
// }

// size_t zmalloc::zmalloc_get_memory_size(void)
// {
    
// }

// void zmalloc::zlibc_free(void *ptr)
// {
    
// }

/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> 
 * All rights reserved.
 * Date: 2025/06/12
 * Description: zmalloc - total amount of allocated memory aware version of malloc()
 */
 #ifndef __ZMALLOC_H
 #define __ZMALLOC_H

 /* Double expansion needed for stringification of macro values. */
#define __xstr(s) __str(s)
#define __str(s) #s

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
        //功能：分配指定大小的内存块，不对内存内容进行初始化。
        void *zzmalloc(size_t size);
        void *ztrymalloc(size_t size);
        void *zmalloc_usable(size_t size, size_t *usable);
        void *ztrymalloc_usable(size_t size, size_t *usable);
        
        //功能：分配指定大小的内存块，并将所有字节初始化为 0。
        void *zcalloc(size_t size);
        void *ztrycalloc(size_t size);
        void *zcalloc_usable(size_t size, size_t *usable);
        void *ztrycalloc_usable(size_t size, size_t *usable);
        
        //功能：调整已分配内存块的大小，可能会移动内存位置。
        void *zrealloc(void *ptr, size_t size);
        void *ztryrealloc(void *ptr, size_t size);
        void *zrealloc_usable(void *ptr, size_t size, size_t *usable);
        void *ztryrealloc_usable(void *ptr, size_t size, size_t *usable);
        
        //释放内存
        void zfree(void *ptr);
        void zfree_usable(void *ptr, size_t *usable);

        char *zstrdup(const char *s);

        size_t zmalloc_used_memory(void);
        void zmalloc_set_oom_handler(void (*oom_handler)(size_t));
        size_t zmalloc_get_rss(void);
        int zmalloc_get_allocator_info(size_t *allocated, size_t *active, size_t *resident);
        void set_jemalloc_bg_thread(int enable);
        int jemalloc_purge();
        size_t zmalloc_get_private_dirty(long pid);
        size_t zmalloc_get_smap_bytes_by_field(char *field, long pid);
        size_t zmalloc_get_memory_size(void);
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
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> 
 * All rights reserved.
 * Date: 2025/06/12
 * Description: zmalloc - total amount of allocated memory aware version of malloc()
 */
 #ifndef __ZMALLOC_H
 #define __ZMALLOC_H
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
        void *zzmalloc(size_t size);
        void *zcalloc(size_t size);
        void *zrealloc(void *ptr, size_t size);
        void *ztrymalloc(size_t size);
        void *ztrycalloc(size_t size);
        void *ztryrealloc(void *ptr, size_t size);
        void zfree(void *ptr);
        void *zmalloc_usable(size_t size, size_t *usable);
        void *zcalloc_usable(size_t size, size_t *usable);
        void *zrealloc_usable(void *ptr, size_t size, size_t *usable);
        void *ztrymalloc_usable(size_t size, size_t *usable);
        void *ztrycalloc_usable(size_t size, size_t *usable);
        void *ztryrealloc_usable(void *ptr, size_t size, size_t *usable);
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
};
 #endif
#include "zmalloc.h"
// 基础内存分配宏（内部使用）
#define __ZMALLOC_BASE(func, ...) \
    ({ \
        void* __restrict __zm_ptr = nullptr; \
        __zm_ptr = zmalloc::getInstance()->func(__VA_ARGS__); \
        __zm_ptr; \
    })

// 内存分配宏
#define zmalloc(a) __ZMALLOC_BASE(zzmalloc, (a))
#define zrealloc(p, a) __ZMALLOC_BASE(zrealloc, (p), (a))
#define ztrymalloc(a) __ZMALLOC_BASE(ztrymalloc, (a))
#define ztryrealloc(p, a) __ZMALLOC_BASE(ztryrealloc, (p), (a))
#define zcalloc(a) __ZMALLOC_BASE(zcalloc, (a))
#define ztrycalloc(a) __ZMALLOC_BASE(ztrycalloc, (a))
#define ZMALLOC(type, bytes) static_cast<type*>(zmalloc(bytes))

// 带usable参数的版本
#define zmalloc_usable(a, u) __ZMALLOC_BASE(zmalloc_usable, (a), (u))
#define zrealloc_usable(p, a, u) __ZMALLOC_BASE(zrealloc_usable, (p), (a), (u))
#define ztrymalloc_usable(a, u) __ZMALLOC_BASE(ztrymalloc_usable, (a), (u))
#define ztryrealloc_usable(p, a, u) __ZMALLOC_BASE(ztryrealloc_usable, (p), (a), (u))

// 特殊处理free函数（不返回指针）
#define zfree(p) \
    do { \
        if (p) { \
            zmalloc::getInstance()->zfree(p); \
            p = nullptr; \
        } \
    } while(0)

// 带usable参数的free
#define zfree_usable(p, u) \
    do { \
        if (p) { \
            zmalloc::getInstance()->zfree_usable(p, u); \
            p = nullptr; \
        } \
    } while(0)


#define BEGIN_NAMESPACE(name) namespace name {
#define END_NAMESPACE(name) }  
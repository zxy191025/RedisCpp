
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
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
#define ASSERT_NO_SIZE_OVERFLOW(sz) assert((sz) + PREFIX_SIZE > (sz))
#endif
#define UNUSED(x) ((void)(x))   // 用于标记未使用的变量或函数参数，以避免编译器发出警告

int main(int argc, char **argv)
{
    UNUSED(argc);
    UNUSED(argv);

    printf("prefix size :%d\n",(int)PREFIX_SIZE);
    size_t usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("used memory :%zu\n",usedmemory);
    void* c = zmalloc::getInstance()->zzmalloc(556);
    usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("allocated 556 bytes;used :%zu \n",usedmemory);

    return 0;    
}
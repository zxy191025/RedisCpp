#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
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

// 辅助函数：将字节转换为人类可读的格式（KB、MB等）
const char* format_bytes(size_t bytes, char* buf, size_t buf_size) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }
    
    snprintf(buf, buf_size, "%.2f %s", size, units[unit_idx]);
    return buf;
}

// 辅助函数：格式化字节为易读单位
void print_memory(const char* label, size_t bytes) {
    static char buffer[32];
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    printf("%-20s: %s\n", label, buffer);
}

int main(int argc, char **argv)
{
    printf("========================01 测试malloc================================\n");
    UNUSED(argc);
    UNUSED(argv);
    printf("prefix size :%d\n",(int)PREFIX_SIZE);
    size_t usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("used memory :%zu\n",usedmemory);
    void* ptr = zmalloc::getInstance()->zzmalloc(556);
    usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("allocated 556 bytes;used :%zu \n",usedmemory);
    ptr = zmalloc::getInstance()->zrealloc(ptr,888);
    usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("allocated 888 bytes;used :%zu \n",usedmemory);
    zmalloc::getInstance()->zfree(ptr);
    usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("used memory :%zu \n",usedmemory);
    //测试zmalloc_usable
    size_t request_size = 1024;
    size_t actual_size;
    ptr = zmalloc::getInstance()->zmalloc_usable(request_size,&actual_size);
    usedmemory = zmalloc::getInstance()->zmalloc_used_memory();
    printf("allocated 1024 bytes;actual_size: %zu,used :%zu \n",actual_size,usedmemory);
    zmalloc::getInstance()->zfree(ptr);

    printf("========================02 测试RSS================================\n");
    // 测试RSS
    char rss_buf[32];
    printf("初始RSS: %s\n", format_bytes(zmalloc::getInstance()->zmalloc_get_rss(), rss_buf, sizeof(rss_buf)));
    size_t alloc_size = 100 * 1024 * 1024; // 100MB
    char* large_buffer = static_cast<char*>(zmalloc::getInstance()->zzmalloc(alloc_size));
     // 填充内存（强制操作系统实际分配物理页）
    for (size_t i = 0; i < alloc_size; i += 4096) {
        large_buffer[i] = 0;
    }
    printf("分配100MB后RSS: %s\n", format_bytes(zmalloc::getInstance()->zmalloc_get_rss(), rss_buf, sizeof(rss_buf)));
    // 释放内存
    zmalloc::getInstance()->zfree(large_buffer);
 
    //测试脏页
    printf("========================03 内存监控测试================================\n");
    printf("进程ID: %d\n\n", (int)getpid());
    // 初始状态
    printf("初始状态:\n");
    print_memory("私有脏页", zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));//-1 代表当前进程
    printf("\n");
  // 分配100MB内存但不修改（干净页）
    printf("分配100MB内存但不修改...\n");
    char* buf1 = (char*)zmalloc::getInstance()->zzmalloc(100 * 1024 * 1024);
    if (!buf1) {
        printf("内存分配失败!\n");
        return 1;
    }
    print_memory("分配后私有脏页",zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("分配后RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));
    printf("\n");
    // 修改内存（使其变为脏页）
    printf("修改50MB内存...\n");
    memset(buf1, 'A', 50 * 1024 * 1024);  // 修改前50MB
    print_memory("修改后私有脏页", zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("修改后RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));
    printf("\n");
    // 再分配50MB并全部修改
    printf("再分配50MB并全部修改...\n");
    char* buf2 = (char*)zmalloc::getInstance()->zzmalloc(50 * 1024 * 1024);
    if (!buf2) {
        printf("内存分配失败!\n");
        zmalloc::getInstance()->zfree(buf1);
        return 1;
    }
    memset(buf2, 'B', 50 * 1024 * 1024);  // 全部修改
    print_memory("再次分配后私有脏页", zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("再次分配后RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));
    printf("\n");
    // 释放部分内存
    printf("释放第一次分配的内存...\n");
    zmalloc::getInstance()->zfree(buf1);
    print_memory("释放后私有脏页", zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("释放后RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));
    printf("\n");
    // 释放全部内存
    printf("释放全部内存...\n");
    zmalloc::getInstance()->zfree(buf2);
    print_memory("全部释放后私有脏页", zmalloc::getInstance()->zmalloc_get_private_dirty(-1));
    print_memory("全部释放后RSS总内存", zmalloc::getInstance()->zmalloc_get_smap_bytes_by_field("Rss:", -1));

    printf("========================04 内存zmalloc_get_memory_size================================\n");
    // 测试1：基本功能
    printf("\n【测试1】获取系统总内存:\n");
    size_t total_memory = zmalloc::getInstance()->zmalloc_get_memory_size();
    
    if (total_memory > 0) {
        char formatted[32];
        printf("系统总内存: %s\n", format_bytes(total_memory, formatted, sizeof(formatted)));
    } else {
        printf("错误: 无法获取系统内存信息\n");
        return 1;
    }

    return 0;
}
/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/07/17
 * All rights reserved. No one may copy or transfer.
 * Description: 服务器实现类
 */
#include "server.h"
#include <sys/time.h>
#include <locale.h>
#include <time.h>
#include <stdlib.h>  
#include <unistd.h>
#include "redis/base/zmallocDf.h" 
#include "redis/base/toolFunc.h"
using namespace REDIS_BASE;
static toolFunc toolFuncl;
static void redisOutOfMemoryHandler(size_t allocation_size) 
{
    // serverLog(LL_WARNING,"Out Of Memory allocating %zu bytes!",
    //     allocation_size);
    // serverPanic("Redis aborting for OUT OF MEMORY. Allocating %zu bytes!",
    //     allocation_size);
}

int main(int argc, char **argv) 
{
    struct timeval tv;
    int j;
    char config_from_stdin = 0;
#ifdef INIT_SETPROCTITLE_REPLACEMENT
    spt_init(argc, argv);
#endif
    setlocale(LC_COLLATE,"");
    tzset(); 
    zmalloc_set_oom_handler(redisOutOfMemoryHandler);
    srand(time(NULL)^getpid());
    srandom(time(NULL)^getpid());
    gettimeofday(&tv,NULL);
    toolFuncl.init_genrand64(((long long) tv.tv_sec * 1000000 + tv.tv_usec) ^ getpid());






    printf("Redis server starting...\n");

    return 0;
}

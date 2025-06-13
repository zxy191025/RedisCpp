
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include "zmalloc.h"
int main()
{
    void* c = zmalloc::getInstance()->zzmalloc(10);

    return 0;    
}
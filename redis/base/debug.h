/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: debug implementation
 */
#ifndef REDIS_BASE_DEBUG_H
#define REDIS_BASE_DEBUG_H
#include "define.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class toolFunc;
class sdsCreate;
struct redisObject;
typedef struct redisObject robj;
class debug
{
public:
    debug();
    ~debug();

public:
    void xorDigest(unsigned char *digest, void *ptr, size_t len);
    void xorStringObjectDigest(unsigned char *digest, robj *o);

private:
    toolFunc* toolFuncInstance;
    sdsCreate *sdsCreateInstance;
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
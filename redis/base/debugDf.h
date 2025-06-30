#include "config.h"
#include "debug.h"
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
/* We can print the stacktrace, so our assert is defined this way: */

#define serverAssertWithInfo(_c,_o,_e) ((_e)?(void)0 : (debug::getInstance()->_serverAssertWithInfo(_c,_o,#_e,__FILE__,__LINE__),redis_unreachable()))
#define serverAssert(_e) ((_e)?(void)0 : (debug::getInstance()->_serverAssert(#_e,__FILE__,__LINE__),redis_unreachable()))
#define serverPanic(...) debug::getInstance()->_serverPanic(__FILE__,__LINE__,__VA_ARGS__),redis_unreachable()

//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//


/* 
 * Copyright (c) 2025, JakeeZhao <zhaojakee@gmail.com> All rights reserved.
 * Date: 2025/06/28
 * All rights reserved. No one may copy or transfer.
 * Description: debug implementation
 */
#ifndef REDIS_BASE_DEBUG_H
#define REDIS_BASE_DEBUG_H
#include "define.h"
#include <signal.h>
//=====================================================================//
BEGIN_NAMESPACE(REDIS_BASE)
//=====================================================================//
class toolFunc;
class sdsCreate;
class redisObject;
typedef class redisObject robj;
class redisDb;
class debug
{
private:
    debug();
    ~debug();
    static debug* instance;

public:
    static debug* getInstance(){
        if (instance == nullptr)
        {
            instance = new debug();
        }
        return instance;
    }  
public:
    /**
     * 计算数据的异或摘要
     * 
     * @param digest 输出的摘要结果
     * @param ptr    输入数据指针
     * @param len    数据长度（字节）
     */
    void xorDigest(unsigned char *digest, void *ptr, size_t len);

    /**
     * 计算 Redis 对象的异或摘要
     * 
     * @param digest 输出的摘要结果
     * @param o      Redis 对象指针
     */
    void xorStringObjectDigest(unsigned char *digest, robj *o);

    /**
     * 将数据混合到现有摘要中
     * 
     * @param digest 现有摘要
     * @param ptr    输入数据指针
     * @param len    数据长度（字节）
     */
    void mixDigest(unsigned char *digest, void *ptr, size_t len);

    /**
     * 将 Redis 字符串对象混合到现有摘要中
     * 
     * @param digest 现有摘要
     * @param o      Redis 对象指针
     */
    void mixStringObjectDigest(unsigned char *digest, robj *o);

    /**
     * 计算 Redis 数据库中对象的异或摘要
     * 
     * @param db      Redis 数据库指针
     * @param keyobj  键对象指针
     * @param digest  输出的摘要结果
     * @param o       值对象指针
     */
    void xorObjectDigest(redisDb *db, robj *keyobj, unsigned char *digest, robj *o);

    /**
     * 计算整个数据集的摘要
     * 
     * @param final 输出的最终摘要结果
     */
    void computeDatasetDigest(unsigned char *final);

    /**
     * 处理 DEBUG 命令
     * 
     * @param c 客户端上下文
     */
    void debugCommand(client *c);

    /**
     * 内部使用的断言函数
     * 
     * @param estr  断言表达式字符串
     * @param file  发生断言的文件名
     * @param line  发生断言的行号
     */
    void _serverAssert(const char *estr, const char *file, int line);

    /**
     * 打印客户端信息（用于断言失败时）
     * 
     * @param c 客户端上下文
     */
    void _serverAssertPrintClientInfo(const client *c);

    /**
     * 打印 Redis 对象的调试信息
     * 
     * @param o Redis 对象指针
     */
    void serverLogObjectDebugInfo(const robj *o);

    /**
     * 打印 Redis 对象信息（用于断言失败时）
     * 
     * @param o Redis 对象指针
     */
    void _serverAssertPrintObject(const robj *o);

    /**
     * 带上下文信息的断言函数
     * 
     * @param c     客户端上下文
     * @param o     Redis 对象指针
     * @param estr  断言表达式字符串
     * @param file  发生断言的文件名
     * @param line  发生断言的行号
     */
    void _serverAssertWithInfo(const client *c, const robj *o, const char *estr, const char *file, int line);

    /**
     * 服务器严重错误处理（不可恢复）
     * 
     * @param file  发生错误的文件名
     * @param line  发生错误的行号
     * @param msg   错误信息（可变参数）
     */
    void _serverPanic(const char *file, int line, const char *msg, ...);

    /**
     * 开始生成错误报告
     */
    void bugReportStart(void);

    /**
     * 打开直接日志文件描述符
     * 
     * @return 文件描述符，失败时返回 -1
     */
    int openDirectLogFiledes(void);

    /**
     * 关闭直接日志文件描述符
     * 
     * @param fd 文件描述符
     */
    void closeDirectLogFiledes(int fd);

    /**
     * 记录服务器信息到日志
     */
    void logServerInfo(void);

    /**
     * 记录模块信息到日志
     */
    void logModulesInfo(void);

    /**
     * 记录当前客户端信息到日志
     */
    void logCurrentClient(void);

    /**
     * 终止主线程（静态方法）
     */
    static void killMainThread(void);

    /**
     * 终止所有工作线程
     */
    void killThreads(void);

    /**
     * 转储 x86 调用信息
     * 
     * @param addr 内存地址
     * @param len  长度（字节）
     */
    void dumpX86Calls(void *addr, size_t len);

    /**
     * 转储 EIP 附近的代码
     * 
     * @param eip 指令指针
     */
    void dumpCodeAroundEIP(void *eip);

    /**
     * 段错误信号处理函数
     * 
     * @param sig    信号编号
     * @param info   信号信息
     * @param secret 上下文信息
     */
    void sigsegvHandler(int sig, siginfo_t *info, void *secret);

    /**
     * 打印崩溃报告
     */
    void printCrashReport(void);

    /**
     * 结束错误报告生成
     * 
     * @param killViaSignal 是否通过信号终止
     * @param sig           终止信号编号
     */
    void bugReportEnd(int killViaSignal, int sig);

    /**
     * 以十六进制格式转储数据到日志
     * 
     * @param level  日志级别
     * @param descr  描述信息
     * @param value  数据指针
     * @param len    数据长度（字节）
     */
    void serverLogHexDump(int level, char *descr, void *value, size_t len);

    /**
     * 看门狗信号处理函数
     * 
     * @param sig    信号编号
     * @param info   信号信息
     * @param secret 上下文信息
     */
    void watchdogSignalHandler(int sig, siginfo_t *info, void *secret);

    /**
     * 安排看门狗信号触发
     * 
     * @param period 周期（毫秒）
     */
    void watchdogScheduleSignal(int period);

    /**
     * 启用看门狗定时器
     * 
     * @param period 检查周期（毫秒）
     */
    void enableWatchdog(int period);

    /**
     * 禁用看门狗定时器
     */
    void disableWatchdog(void);

    /**
     * 调试延迟函数
     * 
     * @param usec 延迟时间（微秒）
     */
    void debugDelay(int usec);

    /**
     * 从 ucontext 中获取指令指针（静态方法）
     * 
     * @param uc ucontext 上下文
     * @return 指令指针地址
     */
    static void *getMcontextEip(ucontext_t *uc);

    /**
     * 记录栈内容
     * 
     * @param sp 栈指针数组
     */
    void logStackContent(void **sp);

    /**
     * 记录寄存器信息
     * 
     * @param uc ucontext 上下文
     */
    void logRegisters(ucontext_t *uc);

    /**
     * 记录栈跟踪信息
     * 
     * @param eip     指令指针
     * @param uplevel 向上追溯的层数
     */
    void logStackTrace(void *eip, int uplevel);


    #ifdef USE_JEMALLOC
    /**
     * 处理 jemalloc 控制整数参数
     * 
     * @param c     客户端上下文
     * @param argv  参数数组
     * @param argc  参数数量
     */
    void mallctl_int(client *c, robj **argv, int argc);

    /**
     * 处理 jemalloc 控制字符串参数
     * 
     * @param c     客户端上下文
     * @param argv  参数数组
     * @param argc  参数数量
     */
    void mallctl_string(client *c, robj **argv, int argc);
    #endif

private:
    toolFunc* toolFuncInstance;
    sdsCreate *sdsCreateInstance;
    redisObjectCreate *redisObjectCreateInstance;
};
//=====================================================================//
END_NAMESPACE(REDIS_BASE)
//=====================================================================//
#endif
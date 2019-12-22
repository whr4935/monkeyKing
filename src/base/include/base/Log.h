#ifndef _LOG_H
#define _LOG_H

#include <sys/syscall.h>  
#include "Global.h"

class Log {
public:
    enum LogLevel {
        LOG_LEVEL_VERBOSE,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERR,
    };

    static int initLogMode(RunMode runMode);
    static void getCurrentTime(char *timebuf);
    static void doLog(int level, const char* format,...);

private:
    static LogLevel sLogLevel;
};

#define gettid()  syscall(__NR_gettid)

#define STRINGIFY(s)         TOSTRING(s)
#define TOSTRING(s) #s

#ifndef LOG_TAG
#define LOG_TAG __FILE__ ":" STRINGIFY(__LINE__) 
#endif

#define LOGV(format, ...) \
    do { \
        char timebuf[30]; \
        Log::getCurrentTime(timebuf); \
        Log::doLog(Log::LOG_LEVEL_VERBOSE, "%s %d %d %s %s " format "\n", timebuf, getpid(), gettid(), TOSTRING(V), LOG_TAG, ##__VA_ARGS__); \
    } while (0)

#define LOGI(format, ...) \
    do { \
        char timebuf[30]; \
        Log::getCurrentTime(timebuf); \
        Log::doLog(Log::LOG_LEVEL_INFO, "%s %d %d %s %s " format "\n", timebuf, getpid(), gettid(), TOSTRING(I), LOG_TAG, ##__VA_ARGS__); \
    } while (0)

#define LOGW(format, ...) \
    do { \
        char timebuf[30]; \
        Log::getCurrentTime(timebuf); \
        Log::doLog(Log::LOG_LEVEL_WARN, "%s %d %d %s %s " format "\n", timebuf, getpid(), gettid(), TOSTRING(W), LOG_TAG, ##__VA_ARGS__); \
    } while (0)

#define LOGE(format, ...) \
    do { \
        char timebuf[30]; \
        Log::getCurrentTime(timebuf); \
        Log::doLog(Log::LOG_LEVEL_ERR, "%s %d %d %s %s " format "\n", timebuf, getpid(), gettid(), TOSTRING(E), LOG_TAG, ##__VA_ARGS__); \
    } while (0)

#define PRINT(format,...) printf(format "\n", ##__VA_ARGS__)

#endif

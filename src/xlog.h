/*
 *  xutils/xlog.h
 *
 *  Copyleft (C) Sun Dro (a.k.a. kala13x)
 *
 * Advance logging library for C/C++
 */

#ifndef __XLOGGER_H__
#define __XLOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "stdinc.h"
#include <sys/time.h>
#include <inttypes.h>

/* Definations for version info */
#define XLOGGER_VERSION_MAJOR  1
#define XLOGGER_VERSION_MINOR  6
#define XLOGGER_BUILD_NUM      3

/* Loging flags */
#define XLOGGER_NONE   0
#define XLOGGER_LIVE   1
#define XLOGGER_INFO   2
#define XLOGGER_WARN   3
#define XLOGGER_DEBUG  4
#define XLOGGER_ERROR  5
#define XLOGGER_FATAL  6
#define XLOGGER_PANIC  7

/* Supported colors */
#define CLR_NORMAL   "\x1B[0m"
#define CLR_RED      "\x1B[31m"
#define CLR_GREEN    "\x1B[32m"
#define CLR_YELLOW   "\x1B[33m"
#define CLR_BLUE     "\x1B[34m"
#define CLR_NAGENTA  "\x1B[35m"
#define CLR_CYAN     "\x1B[36m"
#define CLR_WHITE    "\x1B[37m"
#define CLR_RESET    "\033[0m"

#define LVL1(x) #x
#define LVL2(x) LVL1(x)
#define SOURCE_THROW_LOCATION "<" __FILE__ ":" LVL2(__LINE__) "> - "

#define XLOGGER_PATH_MAX 2048
#define XLOGGER_FILE_MAX 4096

/* 
 * Define macros to allow us get further informations 
 * on the corresponding erros. These macros used as 
 * wrappers for XLogger_Log() function.
 */
#define XLog_None(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_NONE, __VA_ARGS__)

#define XLog_Live(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_LIVE, __VA_ARGS__)

#define XLog_Info(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_INFO, __VA_ARGS__)

#define XLog_Warn(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_WARN, __VA_ARGS__)

#define XLog_Debug(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_DEBUG, __VA_ARGS__)

#define XLog_Error(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_ERROR, SOURCE_THROW_LOCATION __VA_ARGS__)

#define XLog_Fatal(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_FATAL, SOURCE_THROW_LOCATION __VA_ARGS__)

#define XLog_Panic(LEVEL, ...) \
    XLogger_Log(LEVEL, XLOGGER_PANIC, SOURCE_THROW_LOCATION __VA_ARGS__)

typedef struct {
    int nYear;
    int nMonth; 
    int nDay;
    int nHour;
    int nMin;
    int nSec;
    int nUsec;
} XLoggerDate;

/* Flags */
typedef struct {
    char sFileName[XLOGGER_FILE_MAX];
    char sFilePath[XLOGGER_PATH_MAX];
    unsigned short nFileStamp:1;
    unsigned short nFileLevel;
    unsigned short nLogLevel;
    unsigned short nToFile:1;
    unsigned short nPretty:1;
    unsigned short nTdSafe:1;
    unsigned short nErrLog:1;
    unsigned short nSilent:1;
    pthread_mutexattr_t mutexAttr;
    pthread_mutex_t mutex;
} XLoggerConfig;

typedef struct {
    int nType;
    const char* pDesc;
    const char* pColor;
} XLoggerTag;

const char* XLogger_Version(int nMin);

void XLogger_ConfigGet(XLoggerConfig *pCfg);
void XLogger_ConfigSet(XLoggerConfig *pCfg);

void XLogger_Init(const char* pName, const char* pConf, int nLogLevel, int nTdSafe);
void XLogger_Flags(int nFlags);
void XLogger_Tag(int nLevel, char *pTag, const char *pMsg, ...);
void XLogger_Log(int nLevel, int nFlag, const char *pMsg, ...);
void XLogger_Destroy();

#ifdef __cplusplus
}
#endif

#endif /* __XLOGGER_H__ */
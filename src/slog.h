/*
 * The MIT License (MIT)
 *  
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 */

#ifndef __SLOG_H__
#define __SLOG_H__

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/* Definations for version info */
#define SLOGVERSION_MAX  1
#define SLOGVERSION_MIN  4
#define SLOGBUILD_NUM    82

/* Loging flags */
#define SLOG_NONE   0
#define SLOG_LIVE   1
#define SLOG_INFO   2
#define SLOG_WARN   3
#define SLOG_DEBUG  4
#define SLOG_ERROR  5
#define SLOG_FATAL  6
#define SLOG_PANIC  7

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

/* Flags */
typedef struct {
    const char* pFileName;
    short nFileStamp;
    short nFileLevel;
    short nLogLevel;
    short nToFile;
    short nPretty;
    short nTdSafe;
    short nSilent;
} SlogFlags;

/* Date variables */
typedef struct {
    int year; 
    int mon; 
    int day;
    int hour;
    int min;
    int sec;
    int usec;
} SlogDate;

const char* slog_version(int nMin);
char* slog_clr(const char* pColor, char* pStr, ...);
char* slog_get(SlogDate *pDate, char *pMsg, ...);
void slog_silent(int nSilent);

void slog_init(const char* pName, const char* pConf, int nLevel, int nFileLevel, int nTdSafe);
void slog(int level, int flag, const char *pMsg, ...);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __SLOG_H__ */
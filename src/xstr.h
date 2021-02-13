/*
 *  libxutils/src/xstr.h
 *
 *  This source is part of "libxutils" project
 *  2015-2020  Sun Dro (f4tb0y@protonmail.com)
 *
 * This source includes string operations.
 */

#ifndef __XUTILS_XSTR_H__
#define __XUTILS_XSTR_H__

#include "stdinc.h"

/* Supported colors */
#define XSTR_CLR_NORMAL     "\x1B[0m"
#define XSTR_CLR_RED        "\x1B[31m"
#define XSTR_CLR_GREEN      "\x1B[32m"
#define XSTR_CLR_YELLOW     "\x1B[33m"
#define XSTR_CLR_BLUE       "\x1B[34m"
#define XSTR_CLR_NAGENTA    "\x1B[35m"
#define XSTR_CLR_CYAN       "\x1B[36m"
#define XSTR_CLR_WHITE      "\x1B[37m"
#define XSTR_CLR_RESET      "\033[0m"

#define XSTRMAX             8196
#define XSTRNULL            '\0'
#define XSTREMPTY           ""

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XSTR_CASE_LOWER = 0,
    XSTR_CASE_UPPER
} XSTR_CASE_E;

char* xstrafmt(const char *pFmt, ...);
size_t xstrncpy(char *pDst, size_t nSize, const char* pSrc);
size_t xstrncpyf(char *pDst, size_t nSize, const char *pFmt, ...);
size_t xstrncatf(char *pDst, size_t nAvail, const char *pFmt, ...);
size_t xstracpyf(char **pDst, const char *pFmt, ...);
size_t xstrncase(char *pDst, size_t nSize, XSTR_CASE_E eCase, const char *pSrc);
size_t xstrnclr(char *pDst, size_t nSize, const char* pClr, const char* pStr, ...);
size_t xstrncut(char *pDst, size_t nSize, const char *pSrc, const char *pStart, const char *pEnd);

void xstrnull(char *pString, size_t nLength);
int xstrsrc(const char *pStr, const char *pSrc);
int xstrntok(char *pDst, size_t nSize, const char *pStr, size_t nPosit, const char *pDlmt);

char* xstrrep(const char *pOrig, const char *pRep, const char *pWith);
char* xstrcut(char *pData, const char *pStart, const char *pEnd);

#ifdef __cplusplus
}
#endif


#endif /* __XUTILS_XSTR_H__ */
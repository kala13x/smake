/*
 *  xutils/xstr.h
 * 
 *  Copyleft (C) 2015 Sun Dro (a.k.a. kala13x)
 *
 * This header includes string operations.
 */


#ifndef __XUTILS_XSTR_H__
#define __XUTILS_XSTR_H__

#include "stdinc.h"

/* Supported colors */
#define XSTR_CLR_NORMAL   "\x1B[0m"
#define XSTR_CLR_RED      "\x1B[31m"
#define XSTR_CLR_GREEN    "\x1B[32m"
#define XSTR_CLR_YELLOW   "\x1B[33m"
#define XSTR_CLR_BLUE     "\x1B[34m"
#define XSTR_CLR_NAGENTA  "\x1B[35m"
#define XSTR_CLR_CYAN     "\x1B[36m"
#define XSTR_CLR_WHITE    "\x1B[37m"
#define XSTR_CLR_RESET    "\033[0m"

#define XSTR_CASE_LOWER 0
#define XSTR_CASE_UPPER 1

#ifdef __cplusplus
extern "C" {
#endif

#define XSTR_IS_SPACE(x) isspace((int)(x))
#define XSTR_MAX 8196

/* Char array functions */
char *xstrrep(char *pOrig, const char *pRep, const char *pWith);
int xstrsrc(const char *pStr, const char *pSrch);
void xstrclr(const char* pClr, char *pOutput, char* pStr, ...);
void xstrtok(const char *pSrc, const char *pDlmt, void *pSave, char *pDst, int nSize);
void xstrtokf(const char *pSrc, const char *pDlmt, void *pSave, char *pDst, int nSize);
void xstrnul(void *pStr, size_t nLength);
void xstrncpy(char* pDst, const char* pSrc, size_t nSize);
void xstrncatf(char *pSrc, int nSize, char *pFmt, ...);
void xstrcatf(char *pSrc, char *pFmt, ...);
void xstrcase(const char *pSrc, int nCase, char* pStr, int nLength);

#ifdef __cplusplus
}
#endif


#endif /* __XUTILS_XSTR_H__ */
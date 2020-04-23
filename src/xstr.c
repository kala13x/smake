/*
 *  xutils/xstr.h
 * 
 *  Copyleft (C) 2015 Sun Dro (a.k.a. kala13x)
 *
 * This source includes string operations.
 */

#include "stdinc.h"
#include "xstr.h"

void xstrclr(const char* pClr, char *pOutput, char* pStr, ...) 
{
    char sBuffer[XSTR_MAX];
    size_t nSize = sizeof(sBuffer);

    va_list args;
    va_start(args, pStr);
    vsnprintf(sBuffer, nSize, pStr, args);
    va_end(args);

    sprintf(pOutput, "%s%s%s", pClr, sBuffer, XSTR_CLR_RESET);
}

int xstrsrc(const char *pStr, const char *pSrch)
{
    int nLenStr = strlen(pStr);
    int nLenSrch = strlen(pSrch);
    int i, x, nFound = -1;
 
    for (i = 0; i < nLenStr; i++)
    {
        if (pSrch[0] == pStr[i])
        {
            nFound = 1;
            for (x = 1; x < nLenSrch; x++) 
                if (pStr[i+x] != pSrch[x]) nFound = 0;
 
            if (nFound) return nFound;
        }
    }
 
    return nFound;
}

char *xstrrep(char *pOrig, const char *pRep, const char *pWith)
{
    if (!pOrig) return NULL;
    int nLenFront, nCount;
    char *pRes, *pInStr, *pTmp;

    if (!pRep) pRep = "";
    int nLenRep = strlen(pRep);
    if (!pWith) pWith = "";
    int nLenWith = strlen(pWith);

    pInStr = pOrig;
    for (nCount = 0; (pTmp = strstr(pInStr, pRep)) != NULL; ++nCount) 
        pInStr = pTmp + nLenRep;

    pTmp = pRes = malloc(strlen(pOrig) + (nLenWith - nLenRep) * nCount + 1);
    if (!pRes) return NULL;

    while (nCount--)
    {
        pInStr = strstr(pOrig, pRep);
        nLenFront = pInStr - pOrig;
        pTmp = strncpy(pTmp, pOrig, nLenFront) + nLenFront;
        pTmp = strcpy(pTmp, pWith) + nLenWith;
        pOrig += nLenFront + nLenRep;
    }

    strcpy(pTmp, pOrig);
    return pRes;
}

void xstrtok(const char *pSrc, const char *pDlmt, void *pSave, char *pDst, int nSize)
{
    register char *ptr = pSave;
    if (ptr == NULL) return;
    if (pSrc != NULL) 
    {
        strcpy(ptr, pSrc);
        strcat(ptr, pDlmt);
    }

    int i, length = strlen(ptr);
    for (i = 0; (i < length) && (i < nSize); i++)
    {
        if (ptr[i] == pDlmt[0]) break;
        if (ptr[i] == pDlmt[1])
        {
            ptr = NULL;
            break;
        }

        pDst[i] = ptr[i];
    }

    if (ptr != NULL) strcpy(ptr, &ptr[i+1]);
}

void xstrncpy(char* pDst, const char* pSrc, size_t nSize)
{
    size_t nCopySize = strnlen(pSrc, nSize-1);
    memcpy(pDst, pSrc, nCopySize);
    pDst[nCopySize] = '\0';
}

void xstrnul(void *pStr, size_t nLength)
{
    register char *ptr;
    for (ptr = pStr; nLength--;)
        *ptr++ = '\0';
}

void xstrncatf(char *pSrc, int nSize, char *pFmt, ...)
{
    char sBuffer[XSTR_MAX];
    size_t nBufSize = sizeof(sBuffer);

    va_list args;
    va_start(args, pFmt);
    vsnprintf(sBuffer, nBufSize, pFmt, args);
    va_end(args);

    strncat(pSrc, sBuffer, nSize);
}

void xstrcatf(char *pSrc, char *pFmt, ...)
{
    char sBuffer[XSTR_MAX];
    size_t nSize = sizeof(sBuffer);

    va_list args;
    va_start(args, pFmt);
    vsnprintf(sBuffer, nSize, pFmt, args);
    va_end(args);

    strcat(pSrc, sBuffer);
}

void xstrcase(const char *pSrc, int nCase, char* pStr, int nLength)
{
    memset(pStr, 0, nLength);
    int i, nSrcLen = strlen(pSrc);
    int nCopy = (nLength > nSrcLen) ? nSrcLen : nLength;

    for (i = 0; i < nCopy; i++)
    {
        pStr[i] = (nCase == XSTR_CASE_LOWER) ? 
            tolower(pSrc[i]) : toupper(pSrc[i]);
    }
}

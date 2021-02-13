/*
 *  libxutils/src/xstr.c
 *
 *  This source is part of "libxutils" project
 *  2015-2020  Sun Dro (f4tb0y@protonmail.com)
 *
 * This source includes string operations.
 */


#define _GNU_SOURCE
#include "xstr.h"

char* xstrafmt(const char *pFmt, ...)
{
    va_list args;
    char *pDest = NULL;
    va_start(args, pFmt);
    vasprintf(&pDest, pFmt, args);
    va_end(args);
    return pDest;
}

size_t xstrncpy(char *pDst, size_t nSize, const char* pSrc)
{
    if (pDst == NULL || pSrc == NULL) return 0;
    size_t nCopySize = strnlen(pSrc, nSize);
    memcpy(pDst, pSrc, nCopySize);
    pDst[nCopySize] = XSTRNULL;
    return nCopySize;
}

size_t xstracpyf(char **pDst, const char *pFmt, ...)
{
    va_list args;
    size_t nBytes = 0;
    va_start(args, pFmt);
    nBytes += vasprintf(pDst, pFmt, args);
    va_end(args);
    return nBytes;
}

size_t xstrncpyf(char *pDst, size_t nSize, const char *pFmt, ...)
{
    va_list args;
    size_t nBytes = 0;
    va_start(args, pFmt);
    nBytes += vsnprintf(pDst, nSize, pFmt, args);
    va_end(args);
    return nBytes;
}

size_t xstrncatf(char *pDst, size_t nAvail, const char *pFmt, ...)
{
    if (nAvail <= 0) return -1;
    char sBuffer[nAvail];
    size_t nBytes = 0;

    va_list args;
    va_start(args, pFmt);
    nBytes += vsnprintf(sBuffer, nAvail, pFmt, args);
    va_end(args);

    strncat(pDst, sBuffer, nAvail);
    return nAvail - nBytes;
}

size_t xstrnclr(char *pDst, size_t nSize, const char* pClr, const char* pStr, ...) 
{
    char sBuffer[nSize];
    va_list args;

    va_start(args, pStr);
    vsnprintf(sBuffer, nSize, pStr, args);
    va_end(args);

    return xstrncpyf(pDst, nSize, "%s%s%s", 
        pClr, sBuffer, XSTR_CLR_RESET);
}

size_t xstrncase(char* pDst, size_t nSize, XSTR_CASE_E eCase, const char *pSrc)
{
    size_t i, nCopyLength = strnlen(pSrc, nSize);

    for (i = 0; i < nCopyLength; i++)
        pDst[i] = (eCase == XSTR_CASE_LOWER) ? 
            tolower(pSrc[i]) : toupper(pSrc[i]);

    pDst[nCopyLength] = 0;
    return nCopyLength;
}

int xstrsrc(const char *pStr, const char *pSrc)
{
    if (pStr == NULL || pSrc == NULL) return -1;
    const char *pEndPosition = strstr(pStr, pSrc);
    if (pEndPosition == NULL) return -1;
    return (int)(pEndPosition - pStr);
}

int xstrntok(char *pDst, size_t nSize, const char *pStr, size_t nPosit, const char *pDlmt)
{
    if (pDst != NULL) pDst[0] = XSTRNULL;
    if (nPosit >= strlen(pStr)) return -1;

    int nOffset = xstrsrc(&pStr[nPosit], pDlmt);
    if (nOffset <= 0)
    {
        xstrncpy(pDst, nSize, &pStr[nPosit]);
        return 0;
    }

    size_t nCopySize = nSize < nOffset ? nSize : nOffset;
    xstrncpy(pDst, nCopySize, &pStr[nPosit]);
    return nPosit + nOffset + strlen(pDlmt);
}

size_t xstrncut(char *pDst, size_t nSize, const char *pSrc, const char *pStart, const char *pEnd)
{
    pDst[0] = XSTRNULL;
    if (pStart == NULL)
    {
        if (pEnd == NULL) return 0;
        int nStatus = xstrntok(pDst, nSize, pSrc, 0, pEnd);
        return (nStatus >= 0) ? strnlen(pDst, nSize) : 0;
    }

    size_t nPosit = xstrsrc(pSrc, pStart);
    if (nPosit < 0) return 0;
    nPosit += strlen(pStart);

    int nStatus = xstrntok(pDst, nSize, pSrc, nPosit, pEnd);
    return (nStatus >= 0) ? strnlen(pDst, nSize) : 0;
}

char* xstrcut(char *pData, const char *pStart, const char *pEnd)
{
    if (pStart == NULL)
    {
        if (pEnd == NULL) return NULL;
        char *pSavePtr = NULL;
        return strtok_r(pData, pEnd, &pSavePtr);
    }

    char *pLine = strstr(pData, pStart);
    if (pLine != NULL)
    {
        pLine += strlen(pStart);
        if (pEnd == NULL) return pLine;
        return xstrcut(pLine, NULL, pEnd);
    }

    return NULL;
}

char *xstrrep(const char *pOrig, const char *pRep, const char *pWith)
{
    size_t nOrigLen = strlen(pOrig);
    size_t nWithLen = strlen(pWith);
    size_t nRepLen = strlen(pRep);
    int nNext = 0, nCount = 0;

    while ((nNext = xstrntok(NULL, 0, pOrig, nNext, pRep)) > 0) nCount++;
    size_t nFreeBytes = ((nWithLen - nRepLen) * nCount) + nOrigLen + 1;
    char *pResult = (char*)malloc(nFreeBytes);

    if (pResult == NULL) return NULL;
    char *pOffset = pResult;

    while (nCount--)
    {
        int nCopyLen = xstrsrc(pOrig, pRep);
        if (nCopyLen <= 0) break;
    
        xstrncpy(pOffset, nFreeBytes, pOrig);
        nFreeBytes -= nCopyLen;
        pOffset += nCopyLen;

        xstrncpy(pOffset, nFreeBytes, pWith);
        nFreeBytes -= nWithLen;
        pOffset += nWithLen;
        pOrig += nCopyLen + nRepLen;
    }

    xstrncpy(pOffset, nFreeBytes, pOrig);
    return pResult;

}

void xstrnull(char *pString, size_t nLength)
{
    if (nLength <= 0) *pString = 0;
    else while (nLength--) *pString++ = 0;
}

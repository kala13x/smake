/*!
 *  @file smake/src/find.c
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Find dynamic and static libraries in the system.
 */

#include "find.h"
#include "cfg.h"

#define SMAKE_LIB_PATH \
    "/lib:" \
    "/lib64:" \
    "/usr/lib:" \
    "/usr/lib64:" \
    "/usr/local/lib:" \
    "/usr/local/lib64"

int SMake_SearchCb(xsearch_t *pSearch, xsearch_entry_t *pEntry, const char *pMsg)
{
    XSYNC_ATOMIC_SET(pSearch->pInterrupted, XTRUE);\
    (void)pEntry;
    (void)pMsg;
    return XSTDOK;
}

static XSTATUS SMake_FindLibrary(const smake_find_t *pFind, const char *pLib, const char *pPath)
{
    xsearch_t search;
    XSearch_Init(&search, pLib);

    search.callback = SMake_SearchCb;
    search.bInsensitive = pFind->bInsensitive;
    search.bRecursive = pFind->bRecursive;

    XSearch(&search, pPath);
    size_t nUsed = XArray_Used(&search.fileArray);

    if (nUsed > 0)
    {
        xsearch_entry_t *pFile = (xsearch_entry_t*)XArray_GetData(&search.fileArray, 0);
        XASSERT((pFile != NULL) && xstrused(pFile->sPath), XSTDNON);

        size_t nLentgh = strnlen(pFile->sPath, sizeof(pFile->sPath) - 1);
        while (pFile->sPath[--nLentgh] == '/') pFile->sPath[nLentgh] = '\0';
        xlogn("Found %s: %s/%s", pLib, pFile->sPath, pFile->sName);
    }

    XSearch_Destroy(&search);
    return nUsed ? XSTDOK : XSTDNON;
}

static XSTATUS SMake_FindLib(const smake_find_t *pFind, const char *pLib)
{
    XASSERT(pLib, XSTDINV);
    char sLDPath[XPATH_MAX];

    if (!xstrused(pFind->pPath)) xstrncpyf(sLDPath, sizeof(sLDPath), "%s", SMAKE_LIB_PATH);
    else if (pFind->bThisPathOnly) xstrncpyf(sLDPath, sizeof(sLDPath), "%s", pFind->pPath);
    else xstrncpyf(sLDPath, sizeof(sLDPath), "%s:%s", pFind->pPath, SMAKE_LIB_PATH);

    xarray_t *pPaths = xstrsplit(sLDPath, ":");
    XASSERT((pPaths && pPaths->nUsed), XSTDERR);

    size_t i, nUsed = XArray_Used(pPaths);
    XSTATUS nStatus = XSTDNON;

    for (i = 0; i < nUsed; i++)
    {
        const char *pPath = (const char*)XArray_GetData(pPaths, i);
        if (!xstrused(pPath)) continue;

        nStatus = SMake_FindLibrary(pFind, pLib, pPath);
        if (nStatus == XSTDOK) break;
    }

    return nStatus;
}

XSTATUS SMake_FindLibs(smake_ctx_t *pCtx, const smake_find_t *pFind)
{
    XASSERT_RET((pFind != NULL && xstrused(pFind->pFindStr)), XSTDINV);
    xarray_t *pLibs = xstrsplit(pFind->pFindStr, ":");
    XASSERT(pLibs, xthrow("Failed to split input: %s", pFind->pFindStr));

    size_t i, nUsed = XArray_Used(pLibs);
    XSTATUS nStatus = XSTDNON;

    for (i = 0; i < nUsed; i++)
    {
        const char *pLib = (const char*)XArray_GetData(pLibs, i);
        if (!xstrused(pLib)) continue;

        nStatus = SMake_FindLib(pFind, pLib);
        if (nStatus != XSTDOK) break;
    }

    (void)pCtx;
    XArray_Destroy(pLibs);
    return nStatus;
}
/*!
 *  @file smake/src/cfg.c
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Parse arguments and parse/generate config file.
 */

#include "stdinc.h"
#include "find.h"
#include "info.h"
#include "cfg.h"

extern char *optarg;
static void SMake_CopyPath(char *pDst, int nSize, const char *pSrc)
{
    xstrncpy(pDst, nSize, pSrc);
    int nLength = strlen(pDst);
    if (pDst[nLength-1] == '/')
        pDst[nLength-1] = '\0';
}

int SMake_GetLogFlags(uint8_t nVerbose)
{
    int nLogFlags = XLOG_ERROR | XLOG_WARN;
    if (nVerbose) nLogFlags |= XLOG_NOTE;
    if (nVerbose > 1) nLogFlags |= XLOG_INFO;
    if (nVerbose > 2) nLogFlags |= XLOG_DEBUG;
    if (nVerbose > 3) nLogFlags = XLOG_ALL;
    return nLogFlags;
}

xbool_t SMake_IsExcluded(smake_ctx_t *pCtx, const char *pPath)
{
    size_t i, nExcludes = XArray_Used(&pCtx->excludes);
    for (i = 0; i < nExcludes; i++)
    {
        const char *pExcl = (const char *)XArray_GetData(&pCtx->excludes, i);
        if (xstrused(pExcl) && !strcmp(pExcl, pPath)) return XTRUE;
    }

    return XFALSE;
}

xbool_t SMake_SerializeIncludes(xarray_t *pArr, const char *pDlmt, char *pOutput, size_t nSize)
{
    size_t nCount = XArray_Used(pArr);
    size_t i, nAvail = nSize - 1;
    xbool_t bStarted = XFALSE;

    for (i = 0; i < nCount; i++)
    {
        const char *pData = (const char*)XArray_GetData(pArr, i);
        if (!xstrused(pData)) continue;

        if (!bStarted)  bStarted = XTRUE;
        else if (pDlmt) nAvail = xstrncatf(pOutput, nAvail, "%s", pDlmt);
        nAvail = xstrncatf(pOutput, nAvail, "-I%s", pData);
    }

    return bStarted;
}

xbool_t SMake_SerializeArray(xarray_t *pArr, const char *pDlmt, char *pOutput, size_t nSize)
{
    size_t nCount = XArray_Used(pArr);
    size_t i, nAvail = nSize - 1;
    xbool_t bStarted = XFALSE;

    for (i = 0; i < nCount; i++)
    {
        const char *pData = (const char*)XArray_GetData(pArr, i);
        if (!xstrused(pData)) continue;

        if (!bStarted)  bStarted = XTRUE;
        else if (pDlmt) nAvail = xstrncatf(pOutput, nAvail, "%s", pDlmt);
        nAvail = xstrncatf(pOutput, nAvail, "%s", pData);
    }

    return bStarted;
}

xbool_t SMake_AddToArray(xarray_t *pArr, const char *pFmt, ...)
{
    va_list args;
    size_t nLength = 0;

    va_start(args, pFmt);
    char *pDest = xstracpyargs(pFmt, args, &nLength);
    va_end(args);
    XASSERT(pDest, XFALSE);

    size_t i, nCount = XArray_Used(pArr);
    for (i = 0; i < nCount; i++)
    {
        const char *pData = (const char*)XArray_GetData(pArr, i);
        if (!xstrused(pData)) continue;

        if (strlen(pData) == nLength &&
            !strncmp(pData, pDest, nLength))
        {
            free(pDest);
            return XTRUE;
        }
    }

    int nIndex = XArray_AddData(pArr, pDest, XSTDNON);
    xarray_data_t *pArrData = XArray_Get(pArr, nIndex);

    if (pArrData == NULL)
    {
        free(pDest);
        return XFALSE;
    }

    pArrData->nSize = nLength + 1;
    return XTRUE;
}

static xbool_t SMake_AddSourceFile(smake_ctx_t *pCtx, const char *pFullPath)
{
    if (SMake_IsExcluded(pCtx, pFullPath))
    {
        xlogi("Path is excluded: %s", pFullPath);
        return XTRUE;
    }

    if (!XPath_Exists(pFullPath))
    {
        xloge("Failed to stat file: %s (%s)", pFullPath, XSTRERR);
        return XFALSE;
    }

    int nLength = strlen(pFullPath);
    int nType = SMake_GetFileType(pFullPath, nLength);

    if (nType == SMAKE_FILE_UNF)
    {
        xloge("Invalid or unsupported file type: %s", pFullPath);
        return XFALSE;
    }

    xpath_t path;
    XPath_Parse(&path, pFullPath, XTRUE);

    if (!xstrused(path.sPath) || !xstrused(path.sFile))
    {
        xloge("Failed to parse path: %s", pFullPath);
        return XFALSE;
    }

    SMakeFile *pFile = SMake_FileNew(path.sPath, path.sFile, nType);
    if (pFile == NULL) return XFALSE;

    xlogd("Loading project file from config: %s/%s", pFile->sPath, pFile->sName);
    int nStatus = XArray_AddData(&pCtx->fileArr, pFile, XSTDNON);
    return nStatus >= 0 ? XTRUE : XFALSE;
}

static xbool_t SMake_AddIncludePath(smake_ctx_t *pCtx, const char *pFullPath)
{
    if (SMake_IsExcluded(pCtx, pFullPath))
    {
        xlogi("Path is excluded: %s", pFullPath);
        return XTRUE;
    }

    if (!XPath_Exists(pFullPath))
    {
        xloge("Failed to stat path: %s (%s)", pFullPath, XSTRERR);
        return XFALSE;
    }

    xpath_t path;
    XPath_Parse(&path, pFullPath, XTRUE);

    if (!xstrused(path.sPath))
    {
        xloge("Failed to parse path: %s", pFullPath);
        return XFALSE;
    }

    xlogd("Using include path from the config: %s", path.sPath);
    SMake_AddToArray(&pCtx->includes, "%s", path.sPath);

    return XTRUE;
}

xbool_t SMake_AddTokens(xarray_t *pArr, const char *pDlmt, const char *pInput)
{
    xarray_t *pTokens = xstrsplit(pInput, pDlmt);
    XASSERT(pTokens, XFALSE);

    size_t i, nUsed = XArray_Used(pTokens);
    for (i = 0; i < nUsed; i++)
    {
        const char *pToken = (const char*)XArray_GetData(pTokens, i);
        if (xstrused(pToken)) SMake_AddToArray(pArr, "%s", pToken);
    }

    XArray_Destroy(pTokens);
    return XSTDOK;
}

static xbool_t SMake_AddFindObject(smake_ctx_t *pCtx, xjson_obj_t *pFindObj, xbool_t bAppend)
{
    xjson_obj_t *pFlagsObj = XJSON_GetObject(pFindObj, "flags");
    xjson_obj_t *pLibsObj = XJSON_GetObject(pFindObj, "libs");
    xjson_obj_t *pLdObj = XJSON_GetObject(pFindObj, "ldLibs");

    const char *pFlags = pFlagsObj != NULL ? XJSON_GetString(pFlagsObj) : NULL;
    const char *pLibs = pLibsObj != NULL ? XJSON_GetString(pLibsObj) : NULL;
    const char *pLd = pLdObj != NULL ? XJSON_GetString(pLdObj) : NULL;

    if (xstrused(pFlags))
    {
        if (!bAppend) XArray_Clear(&pCtx->flagArr);
        SMake_AddTokens(&pCtx->flagArr, XSTR_SPACE, pFlags);
    }

    if (xstrused(pLibs))
    {
        if (!bAppend) XArray_Clear(&pCtx->libArr);
        SMake_AddTokens(&pCtx->libArr, XSTR_SPACE, pLibs);
    }

    if (xstrused(pLd))
    {
        if (!bAppend) XArray_Clear(&pCtx->ldArr);
        SMake_AddTokens(&pCtx->ldArr, XSTR_SPACE, pLd);
    }

    return XTRUE;
}

int SMake_ParseArgs(smake_ctx_t *pCtx, int argc, char *argv[])
{
    int nChar = 0;
    while ((nChar = getopt(argc, argv, "o:s:c:e:b:i:f:g:l:p:v:L:V1:I1:d1:j1:w1:x1:h1")) != -1)
    {
        switch (nChar)
        {
            case 'o':
                memset(pCtx->sOutDir, 0, sizeof(pCtx->sOutDir));
                SMake_CopyPath(pCtx->sOutDir, sizeof(pCtx->sOutDir), optarg);
                break;
            case 's':
                memset(pCtx->sPath, 0, sizeof(pCtx->sPath));
                SMake_CopyPath(pCtx->sPath, sizeof(pCtx->sPath), optarg);
                break;
            case 'c':
                xstrncpy(pCtx->sConfig, sizeof(pCtx->sConfig), optarg);
                break;
            case 'b':
                SMake_CopyPath(pCtx->sBinaryDst, sizeof(pCtx->sBinaryDst), optarg);
                break;
            case 'i':
                SMake_CopyPath(pCtx->sHeaderDst, sizeof(pCtx->sHeaderDst), optarg);
                break;
            case 'g':
                xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), optarg);
                break;
            case 'e':
                SMake_AddTokens(&pCtx->excludes, ":", optarg);
                break;
            case 'f':
                SMake_AddTokens(&pCtx->flagArr, XSTR_SPACE, optarg);
                break;
            case 'l':
                SMake_AddTokens(&pCtx->libArr, XSTR_SPACE, optarg);
                break;
            case 'L':
                SMake_AddTokens(&pCtx->ldArr, XSTR_SPACE, optarg);
                break;
            case 'p':
                xstrncpy(pCtx->sName, sizeof(pCtx->sName), optarg);
                break;
            case 'v':
                pCtx->nVerbose = atoi(optarg);
                break;
            case 'w':
                pCtx->bOverwrite = XTRUE;
                break;
            case 'd':
                pCtx->bVPath = XTRUE;
                break;
            case 'x':
                pCtx->bIsCPP = XTRUE;
                break;
            case 'j':
                pCtx->bWriteCfg = XTRUE;
                break;
            case 'I':
                pCtx->bInitProj = XTRUE;
                break;
            case 'V':
                SMake_PrintVersion(argv[0]);
                return XFALSE;
            case 'h':
            default:
                SMake_Usage(argv[0]);
                return XFALSE;
        }
    }

    return XTRUE;
}

int SMake_ParseConfig(smake_ctx_t *pCtx)
{
    XASSERT_RET(!pCtx->bWriteCfg, XTRUE);

    if (!xstrused(pCtx->sConfig) && XPath_Exists(SMAKE_CFG_FILE))
        xstrncpy(pCtx->sConfig, sizeof(pCtx->sConfig), SMAKE_CFG_FILE);

    size_t nSize = 0;
    char *pBuffer = (char*)XPath_Load(pCtx->sConfig, &nSize);

    if (pBuffer == NULL)
    {
        if (xstrused(pCtx->sConfig) && (!pCtx->bWriteCfg && !pCtx->bInitProj))
        {
            xloge("Failed to parse config: %s (%s)", pCtx->sConfig, XSTRERR);
            return XFALSE;
        }

        return XTRUE;
    }

    xjson_t json;
    if (!XJSON_Parse(&json, NULL, pBuffer, nSize))
    {
        char sError[256];
        XJSON_GetErrorStr(&json, sError, sizeof(sError));
        xloge("Failed to parse JSON: %s", sError);

        XJSON_Destroy(&json);
        free(pBuffer);
        return XFALSE;
    }

    xjson_obj_t *pBuildObj = XJSON_GetObject(json.pRootObj, "build");
    if (pBuildObj != NULL)
    {
        xjson_obj_t *pValueObj = XJSON_GetObject(pBuildObj, "verbose");
        if (pValueObj != NULL && !pCtx->nVerbose) pCtx->nVerbose = XJSON_GetInt(pValueObj);

        int nLogFlags = SMake_GetLogFlags(pCtx->nVerbose);
        xlog_setfl(nLogFlags);

        xjson_obj_t *pExcludeArr = XJSON_GetObject(pBuildObj, "excludes");
        if (pExcludeArr != NULL)
        {
            size_t i, nLength = XJSON_GetArrayLength(pExcludeArr);
            for (i = 0; i < nLength; i++)
            {
                pValueObj = XJSON_GetArrayItem(pExcludeArr, i);
                if (pValueObj != NULL)
                {
                    const char *pExcludeStr = XJSON_GetString(pValueObj);
                    SMake_AddToArray(&pCtx->excludes, "%s", pExcludeStr);
                }
            }
        }

        pValueObj = XJSON_GetObject(pBuildObj, "flags");
        if (pValueObj != NULL)
        {
            const char *pFlags = XJSON_GetString(pValueObj);
            SMake_AddTokens(&pCtx->flagArr, XSTR_SPACE, pFlags);
        }

        pValueObj = XJSON_GetObject(pBuildObj, "libs");
        if (pValueObj != NULL)
        {
            const char *pLibs = XJSON_GetString(pValueObj);
            SMake_AddTokens(&pCtx->libArr, XSTR_SPACE, pLibs);
        }

        pValueObj = XJSON_GetObject(pBuildObj, "ldLibs");
        if (pValueObj != NULL)
        {
            const char *pLd = XJSON_GetString(pValueObj);
            SMake_AddTokens(&pCtx->ldArr, XSTR_SPACE, pLd);
        }

        xjson_obj_t *pFindObj = XJSON_GetObject(pBuildObj, "find");
        if (pFindObj != NULL)
        {
            xarray_t *pObjects = XJSON_GetObjects(pFindObj);
            if (pObjects != NULL)
            {
                size_t i, nUsed = XArray_Used(pObjects);
                for (i = 0; i < nUsed; i++)
                {
                    xmap_pair_t *pPair = (xmap_pair_t*)XArray_GetData(pObjects, i);
                    if (pPair == NULL || pPair->pData == NULL|| !xstrused(pPair->pKey)) continue;

                    xjson_obj_t *pFoundObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "found");
                    xjson_obj_t *pNotFoundObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "notFound");
                    if (pFoundObj == NULL && pNotFoundObj == NULL) continue;

                    smake_find_t finder;
                    finder.pFindStr = pPair->pKey;

                    xjson_obj_t *pFindOptObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "path");
                    finder.pPath = pFindOptObj != NULL ? XJSON_GetString(pFindOptObj) : NULL;

                    pFindOptObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "thisPathOnly");
                    finder.bThisPathOnly = pFindOptObj != NULL ? XJSON_GetBool(pFindOptObj) : XFALSE;

                    pFindOptObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "insensitive");
                    finder.bInsensitive = pFindOptObj != NULL ? XJSON_GetBool(pFindOptObj) : XTRUE;

                    pFindOptObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "recursive");
                    finder.bRecursive = pFindOptObj != NULL ? XJSON_GetBool(pFindOptObj) : XTRUE;

                    XSTATUS nStatus = SMake_FindLibs(pCtx, &finder);
                    if (nStatus == XSTDOK)
                    {
                        xjson_obj_t *pAppendObj = XJSON_GetObject(pFoundObj, "append");
                        if (pAppendObj != NULL) SMake_AddFindObject(pCtx, pAppendObj, XTRUE);

                        xjson_obj_t *pSetObj = XJSON_GetObject(pFoundObj, "set");
                        if (pSetObj != NULL) SMake_AddFindObject(pCtx, pSetObj, XFALSE);
                    }
                    else
                    {
                        xjson_obj_t *pAppendObj = XJSON_GetObject(pNotFoundObj, "append");
                        if (pAppendObj != NULL) SMake_AddFindObject(pCtx, pAppendObj, XTRUE);

                        xjson_obj_t *pSetObj = XJSON_GetObject(pNotFoundObj, "set");
                        if (pSetObj != NULL) SMake_AddFindObject(pCtx, pSetObj, XFALSE);
                    }
                }

                XArray_Destroy(pObjects);
            }
        }

        pValueObj = XJSON_GetObject(pBuildObj, "name");
        if (pValueObj != NULL) xstrncpy(pCtx->sName, sizeof(pCtx->sName), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "outputDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sOutDir, sizeof(pCtx->sOutDir), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "compiler");
        if (pValueObj != NULL) xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "inject");
        if (pValueObj != NULL) xstrncpy(pCtx->sInjectPath, sizeof(pCtx->sInjectPath), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "ldflags");
        if (pValueObj != NULL) xstrncpy(pCtx->sLDFlags, sizeof(pCtx->sLDFlags), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "overwrite");
        if (pValueObj != NULL) pCtx->bOverwrite = XJSON_GetBool(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "vpath");
        if (pValueObj != NULL) pCtx->bVPath = XJSON_GetBool(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "cxx");
        if (pValueObj != NULL) pCtx->bIsCPP = XJSON_GetBool(pValueObj);

        xjson_obj_t *pSourceArr = XJSON_GetObject(pBuildObj, "sources");
        if (pSourceArr != NULL)
        {
            pCtx->bSrcFromCfg = XTRUE;
            size_t i, nLength = XJSON_GetArrayLength(pSourceArr);

            for (i = 0; i < nLength; i++)
            {
                pValueObj = XJSON_GetArrayItem(pSourceArr, i);
                if (pValueObj != NULL)
                {
                    const char *pSourceStr = XJSON_GetString(pValueObj);
                    if (!SMake_AddSourceFile(pCtx, pSourceStr))
                    {
                        XJSON_Destroy(&json);
                        free(pBuffer);
                        return XFALSE;
                    }
                }
            }
        }

        xjson_obj_t *pHeaderArr = XJSON_GetObject(pBuildObj, "includes");
        if (pHeaderArr != NULL)
        {
            size_t i, nLength = XJSON_GetArrayLength(pHeaderArr);
            for (i = 0; i < nLength; i++)
            {
                pValueObj = XJSON_GetArrayItem(pHeaderArr, i);
                if (pValueObj != NULL)
                {
                    const char *pIncludePath = XJSON_GetString(pValueObj);
                    if (!SMake_AddIncludePath(pCtx, pIncludePath))
                    {
                        XJSON_Destroy(&json);
                        free(pBuffer);
                        return XFALSE;
                    }
                }
            }
        }
    }

    xjson_obj_t *pInstallObj = XJSON_GetObject(json.pRootObj, "install");
    if (pInstallObj != NULL)
    {
        xjson_obj_t *pValueObj = XJSON_GetObject(pInstallObj, "binaryDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sBinaryDst, sizeof(pCtx->sBinaryDst), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pInstallObj, "headerDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sHeaderDst, sizeof(pCtx->sHeaderDst), XJSON_GetString(pValueObj));
    }

    XJSON_Destroy(&json);
    free(pBuffer);
    return XTRUE;
}

int SMake_WriteConfig(smake_ctx_t *pCtx)
{
    XASSERT_RET((pCtx->bWriteCfg || pCtx->bInitProj), XSTDOK);
    const char *pPath = xstrused(pCtx->sConfig) ? pCtx->sConfig : SMAKE_CFG_FILE;

    if (XPath_Exists(pPath) && !pCtx->bOverwrite)
    {
        xlogw("SMake config already exists: %s", pPath);

        char sAnswer[8];
        sAnswer[0] = '\0';

        XCLI_GetInput("Would you like to owerwrite? (Y/N): ", sAnswer, sizeof(sAnswer), XTRUE);
        if (!xstrused(sAnswer) || (sAnswer[0] != 'y' && sAnswer[0] != 'Y'))
        {
            xlogn("Stopping config file generation.");
            return XSTDOK;
        }
    }

    xjson_obj_t *pRootObj = XJSON_NewObject(NULL, NULL, XFALSE);
    if (pRootObj != NULL)
    {
        xjson_obj_t *pBuildObj = XJSON_NewObject(NULL, "build", XFALSE);
        if (pBuildObj != NULL)
        {
            if (xstrused(pCtx->sName)) XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "name", pCtx->sName));
            if (xstrused(pCtx->sCompiler)) XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "compiler", pCtx->sCompiler));
            if (xstrused(pCtx->sOutDir)) XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "outputDir", pCtx->sOutDir));
            if (xstrused(pCtx->sInjectPath)) XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "inject", pCtx->sInjectPath));
            if (xstrused(pCtx->sLDFlags)) XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "ldflags", pCtx->sLDFlags));

            if (XArray_Used(&pCtx->flagArr))
            {
                char sFlags[XSTR_MID];
                sFlags[0] = XSTR_NUL;

                SMake_SerializeArray(&pCtx->flagArr, XSTR_SPACE, sFlags, sizeof(sFlags));
                XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "flags", sFlags));
            }

            if (XArray_Used(&pCtx->libArr))
            {
                char sLibs[XSTR_MID];
                sLibs[0] = XSTR_NUL;

                SMake_SerializeArray(&pCtx->libArr, XSTR_SPACE, sLibs, sizeof(sLibs));
                XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "libs", sLibs));
            }

            if (XArray_Used(&pCtx->ldArr))
            {
                char sLd[XSTR_MID];
                sLd[0] = XSTR_NUL;

                SMake_SerializeArray(&pCtx->ldArr, XSTR_SPACE, sLd, sizeof(sLd));
                XJSON_AddObject(pBuildObj, XJSON_NewString(NULL, "ldLibs", sLd));
            }

            size_t i, nIncludes = XArray_Used(&pCtx->includes);
            if (nIncludes)
            {
                xjson_obj_t *pIncludesArr = XJSON_NewArray(NULL, "includes", XFALSE);
                if (pIncludesArr != NULL)
                {
                    for (i = 0; i < nIncludes; i++)
                    {
                        const char *pIncl = (const char *)XArray_GetData(&pCtx->includes, i);
                        if (!xstrused(pIncl)) continue;

                        XJSON_AddObject(pIncludesArr, XJSON_NewString(NULL, NULL, pIncl));
                    }

                    XJSON_AddObject(pBuildObj, pIncludesArr);
                }
            }

            size_t nExcludes = XArray_Used(&pCtx->excludes);
            if (nExcludes)
            {
                xjson_obj_t *pExcludesArr = XJSON_NewArray(NULL, "excludes", XFALSE);
                if (pExcludesArr != NULL)
                {
                    for (i = 0; i < nExcludes; i++)
                    {
                        const char *pExcl = (const char *)XArray_GetData(&pCtx->excludes, i);
                        if (!xstrused(pExcl)) continue;

                        XJSON_AddObject(pExcludesArr, XJSON_NewString(NULL, NULL, pExcl));
                    }

                    XJSON_AddObject(pBuildObj, pExcludesArr);
                }
            }

            XJSON_AddObject(pBuildObj, XJSON_NewInt(NULL, "verbose", pCtx->nVerbose));
            XJSON_AddObject(pBuildObj, XJSON_NewBool(NULL, "overwrite", pCtx->bOverwrite));
            XJSON_AddObject(pBuildObj, XJSON_NewBool(NULL, "cxx", pCtx->bIsCPP));
            XJSON_AddObject(pRootObj, pBuildObj);
        }

        if (xstrused(pCtx->sBinaryDst) || xstrused(pCtx->sHeaderDst))
        {
            xjson_obj_t *pInstallObj = XJSON_NewObject(NULL, "install", XFALSE);
            if (pInstallObj != NULL)
            {
                if (xstrused(pCtx->sBinaryDst)) XJSON_AddObject(pInstallObj, XJSON_NewString(NULL, "binaryDir", pCtx->sBinaryDst));
                if (xstrused(pCtx->sHeaderDst)) XJSON_AddObject(pInstallObj, XJSON_NewString(NULL, "headerDir", pCtx->sHeaderDst));
                XJSON_AddObject(pRootObj, pInstallObj);
            }
        }
    }

    xjson_writer_t linter;
    XJSON_InitWriter(&linter, NULL, NULL, 1); // Dynamic allocation
    linter.nTabSize = 4; // Enable linter and set tab size (4 spaces)
    XSTATUS nStatus = XSTDOK;

    /* Dump objects directly */
    if (XJSON_WriteObject(pRootObj, &linter))
    {
        if (XPath_Write(pPath, (const uint8_t*)linter.pData, linter.nLength, "cwt") <= 0)
        {
            xloge("Failed to wite data: %s (%s)", pPath, XSTRERR);
            nStatus = XSTDNON;
        }

        XJSON_DestroyWriter(&linter);
    }

    XJSON_FreeObject(pRootObj);
    return nStatus;
}
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
        if (xstrused(pData) && !strncmp(pData, pDest, strlen(pData)))
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

int SMake_ParseArgs(smake_ctx_t *pCtx, int argc, char *argv[])
{
    char sFlags[SMAKE_LINE_MAX];
    char sLibs[SMAKE_LINE_MAX];
    sFlags[0] = sLibs[0] = XSTR_NUL;

    int nChar = 0;
    while ((nChar = getopt(argc, argv, "o:s:c:e:b:i:f:g:l:p:v:I1:d1:w1:x1:h1")) != -1)
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
            case 'e':
                SMake_CopyPath(pCtx->sExcept, sizeof(pCtx->sExcept), optarg);
                break;
            case 'b':
                SMake_CopyPath(pCtx->sBinaryDst, sizeof(pCtx->sBinaryDst), optarg);
                break;
            case 'i':
                SMake_CopyPath(pCtx->sHeaderDst, sizeof(pCtx->sHeaderDst), optarg);
                break;
            case 'f':
                xstrncpy(sFlags, sizeof(sFlags), optarg);
                break;
            case 'g':
                xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), optarg);
                break;
            case 'l':
                xstrncpy(sLibs, sizeof(sLibs), optarg);
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
            case 'I':
                pCtx->bInitProj = XTRUE;
                break;
            case 'h':
            default:
                SMake_Usage(argv[0]);
                return -1;
        }
    }

    if (xstrused(sFlags)) SMake_AddTokens(&pCtx->flagArr, XSTR_SPACE, sFlags);
    if (xstrused(sLibs)) SMake_AddTokens(&pCtx->libArr, XSTR_SPACE, sLibs);

    return 0;
}

static void SMake_MergeConf(char *pOld, const char *pNew, const char *pDlmt)
{
    char sNew[SMAKE_LINE_MAX];
    char *pSavePtr = NULL;

    xstrncpyf(sNew, sizeof(sNew), pNew);
    char *pTok = strtok_r(sNew, pDlmt, &pSavePtr);

    while (pTok != NULL)
    {
        if (strstr(pOld, pTok) == NULL)
        {
            if (strlen(pOld)) strcat(pOld, pDlmt);
            strcat(pOld, pTok);
        }

        pTok = strtok_r(NULL, pDlmt, &pSavePtr);
    }
}

int SMake_ParseConfig(smake_ctx_t *pCtx, const char *pPath)
{
    const char *pCfgPath = xstrused(pCtx->sConfig) ? &pCtx->sConfig[0] : pPath;
    size_t nSize = 0;

    char *pBuffer = (char*)XPath_Load(pCfgPath, &nSize);
    if (pBuffer == NULL)
    {
        if (xstrused(pCtx->sConfig))
            xloge("Failed to parse config: %s (%s)", pCfgPath, XSTRERR);

        return 0;
    }

    xjson_t json;
    if (!XJSON_Parse(&json, pBuffer, nSize))
    {
        char sError[256];
        XJSON_GetErrorStr(&json, sError, sizeof(sError));
        xloge("Failed to parse JSON: %s", sError);

        XJSON_Destroy(&json);
        free(pBuffer);
        return 0;
    }

    xjson_obj_t *pBuildObj = XJSON_GetObject(json.pRootObj, "build");
    if (pBuildObj != NULL)
    {
        xjson_obj_t *pExcludeArr = XJSON_GetObject(pBuildObj, "excludes");
        if (pExcludeArr != NULL)
        {
            size_t i, nLength = XJSON_GetArrayLength(pExcludeArr);
            for (i = 0; i < nLength; i++)
            {
                xjson_obj_t *pValueObj = XJSON_GetArrayItem(pExcludeArr, i);
                if (pValueObj != NULL) SMake_MergeConf(pCtx->sExcept, XJSON_GetString(pValueObj), ";");
            }
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
                    if (pPair == NULL || !xstrused(pPair->pKey)) continue;

                    smake_find_t finder;
                    finder.pFindStr = pPair->pKey;

                    xjson_obj_t *pFlagsObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "flags");
                    xjson_obj_t *pLibsObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "libs");
                    xjson_obj_t *pPathObj = XJSON_GetObject((xjson_obj_t*)pPair->pData, "path");

                    finder.pFlags = pFlagsObj != NULL ? XJSON_GetString(pFlagsObj) : NULL;
                    finder.pLibs = pLibsObj != NULL ? XJSON_GetString(pLibsObj) : NULL;
                    finder.pPath = pPathObj != NULL ? XJSON_GetString(pPathObj) : NULL;
                    SMake_FindLibs(pCtx, &finder);
                }

                XArray_Destroy(pObjects);
            }
        }

        xjson_obj_t *pValueObj = XJSON_GetObject(pBuildObj, "flags");
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

        pValueObj = XJSON_GetObject(pBuildObj, "name");
        if (pValueObj != NULL) xstrncpy(pCtx->sName, sizeof(pCtx->sName), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "outputDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sOutDir, sizeof(pCtx->sOutDir), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "compiler");
        if (pValueObj != NULL) xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "verbose");
        if (pValueObj != NULL && !pCtx->nVerbose) pCtx->nVerbose = XJSON_GetInt(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "overwrite");
        if (pValueObj != NULL) pCtx->bOverwrite = XJSON_GetBool(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "vpath");
        if (pValueObj != NULL) pCtx->bVPath = XJSON_GetBool(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "cxx");
        if (pValueObj != NULL) pCtx->bIsCPP = XJSON_GetBool(pValueObj);
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
    return 1;
}

static void SMake_WriteExcluded(xjson_obj_t *pArrObj, const char *pExcludes)
{
    char sExcludes[SMAKE_LINE_MAX];
    char *pSavePtr = NULL;

    xstrncpyf(sExcludes, sizeof(sExcludes), pExcludes);
    char *pTok = strtok_r(sExcludes, ";", &pSavePtr);

    while (pTok != NULL)
    {
        XJSON_AddObject(pArrObj, XJSON_NewString(NULL, pTok));
        pTok = strtok_r(NULL, ";", &pSavePtr);
    }
}

int SMake_WriteConfig(smake_ctx_t *pCtx, const char *pPath)
{
    xjson_obj_t *pRootObj = XJSON_NewObject(NULL, XFALSE);
    if (pRootObj != NULL)
    {
        xjson_obj_t *pBuildObj = XJSON_NewObject("build", XFALSE);
        if (pBuildObj != NULL)
        {
            if (xstrused(pCtx->sName)) XJSON_AddObject(pBuildObj, XJSON_NewString("name", pCtx->sName));
            if (xstrused(pCtx->sCompiler)) XJSON_AddObject(pBuildObj, XJSON_NewString("compiler", pCtx->sCompiler));
            if (xstrused(pCtx->sOutDir)) XJSON_AddObject(pBuildObj, XJSON_NewString("outputDir", pCtx->sOutDir));

            if (XArray_Used(&pCtx->flagArr))
            {
                char sFlags[XSTR_MID];
                sFlags[0] = XSTR_NUL;

                SMake_SerializeArray(&pCtx->flagArr, XSTR_SPACE, sFlags, sizeof(sFlags));
                XJSON_AddObject(pBuildObj, XJSON_NewString("flags", sFlags));
            }

            if (XArray_Used(&pCtx->libArr))
            {
                char sLibs[XSTR_MID];
                sLibs[0] = XSTR_NUL;

                SMake_SerializeArray(&pCtx->libArr, XSTR_SPACE, sLibs, sizeof(sLibs));
                XJSON_AddObject(pBuildObj, XJSON_NewString("libs", sLibs));
            }

            if (xstrused(pCtx->sExcept))
            {
                xjson_obj_t *pExcludesArr = XJSON_NewArray("excludes", XFALSE);
                if (pExcludesArr != NULL)
                {
                    SMake_WriteExcluded(pExcludesArr, pCtx->sExcept);
                    XJSON_AddObject(pBuildObj, pExcludesArr);
                }
            }

            XJSON_AddObject(pBuildObj, XJSON_NewInt("verbose", pCtx->nVerbose));
            XJSON_AddObject(pBuildObj, XJSON_NewBool("overwrite", pCtx->bOverwrite));
            XJSON_AddObject(pBuildObj, XJSON_NewBool("cxx", pCtx->bIsCPP));
            XJSON_AddObject(pRootObj, pBuildObj);
        }

        if (xstrused(pCtx->sBinaryDst) || xstrused(pCtx->sHeaderDst))
        {
            xjson_obj_t *pInstallObj = XJSON_NewObject("install", XFALSE);
            if (pInstallObj != NULL)
            {
                if (xstrused(pCtx->sBinaryDst)) XJSON_AddObject(pInstallObj, XJSON_NewString("binaryDir", pCtx->sBinaryDst));
                if (xstrused(pCtx->sHeaderDst)) XJSON_AddObject(pInstallObj, XJSON_NewString("headerDir", pCtx->sHeaderDst));
                XJSON_AddObject(pRootObj, pInstallObj);
            }
        }
    }

    xjson_writer_t linter;
    XJSON_InitWriter(&linter, NULL, 1); // Dynamic allocation
    linter.nTabSize = 4; // Enable linter and set tab size (4 spaces)

    /* Dump objects directly */
    if (XJSON_WriteObject(pRootObj, &linter))
    {
        FILE *pFile = fopen(pPath, "w");
        if (pFile != NULL)
        {
            fwrite(linter.pData, linter.nLength, 1, pFile);
            fclose(pFile);
        }

        XJSON_DestroyWriter(&linter);
    }

    XJSON_FreeObject(pRootObj);
    return 1;
}
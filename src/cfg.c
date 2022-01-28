/*
 *  src/cfg.c
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#include "stdinc.h"
#include "xjson.h"
#include "slog.h"
#include "xstr.h"
#include "make.h"
#include "info.h"
#include "file.h"
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
    int nLogFlags = SLOG_ERROR | SLOG_WARN;
    if (nVerbose) nLogFlags |= SLOG_NOTAG;
    if (nVerbose > 1) nLogFlags |= SLOG_INFO;
    if (nVerbose > 2) nLogFlags |= SLOG_DEBUG;
    if (nVerbose > 3) nLogFlags = SLOG_FLAGS_ALL;
    return nLogFlags;
}

int SMake_ParseArgs(SMakeContext *pCtx, int argc, char *argv[])
{
    int nChar = 0;
    while ((nChar = getopt(argc, argv, "o:s:c:e:b:i:f:g:l:p:v:d1:x1:h1")) != -1) 
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
                SMake_CopyPath(pCtx->sBinary, sizeof(pCtx->sBinary), optarg);
                break;
            case 'i':
                SMake_CopyPath(pCtx->sIncludes, sizeof(pCtx->sIncludes), optarg);
                break;
            case 'f':
                xstrncpy(pCtx->sFlags, sizeof(pCtx->sFlags), optarg);
                break;
            case 'g':
                xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), optarg);
                break;
            case 'l':
                xstrncpy(pCtx->sLibs, sizeof(pCtx->sLibs), optarg);
                break;
            case 'p':
                xstrncpy(pCtx->sName, sizeof(pCtx->sName), optarg);
                break;
            case 'v':
                pCtx->nVerbose = atoi(optarg);
                break;
            case 'd':
                pCtx->nVPath = 1;
                break;
            case 'x':
                pCtx->nCPP = 1;
                break;
            case 'h':
            default:
                SMake_Usage(argv[0]);
                return -1;
        }
    }

    return 0;
}

int SMake_ParseConfig(SMakeContext *pCtx, const char *pPath)
{
    const char *pCfgPath = strlen(pCtx->sConfig) ? &pCtx->sConfig[0] : pPath;
    size_t nSize = 0;
    XFile file;

    if (XFile_Open(&file, pCfgPath, "r") < 0)
    {
        if (strlen(pCtx->sConfig)) 
            sloge("Can not open file: %s (%s)", pCfgPath, strerror(errno));
        return 0;
    }

    char *pBuffer = (char*)XFile_ReadBuffer(&file, &nSize);
    if (pBuffer == NULL)
    {
        sloge("Can not read file: %s (%s)", pCfgPath, strerror(errno));
        XFile_Close(&file);
        return 0;
    }

    XFile_Close(&file);
    xjson_t json;

    if (!XJSON_Parse(&json, pBuffer, nSize))
    {
        char sError[256];
        XJSON_GetErrorStr(&json, sError, sizeof(sError));
        sloge("Failed to parse JSON: %s", sError);

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
                if (pValueObj != NULL)
                {
                    if (strlen(pCtx->sExcept)) strcat(pCtx->sExcept, "|");
                    strcat(pCtx->sExcept, XJSON_GetString(pValueObj));
                }
            }
        }

        xjson_obj_t *pValueObj = XJSON_GetObject(pBuildObj, "libs");
        if (pValueObj != NULL)
        {
            if (strlen(pCtx->sLibs)) strcat(pCtx->sLibs, " ");
            strcat(pCtx->sLibs, XJSON_GetString(pValueObj));
        }

        pValueObj = XJSON_GetObject(pBuildObj, "flags");
        if (pValueObj != NULL)
        {
            if (strlen(pCtx->sFlags)) strcat(pCtx->sFlags, " ");
            strcat(pCtx->sFlags, XJSON_GetString(pValueObj));
        }

        pValueObj = XJSON_GetObject(pBuildObj, "name");
        if (pValueObj != NULL) xstrncpy(pCtx->sName, sizeof(pCtx->sName), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "outputDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sOutDir, sizeof(pCtx->sOutDir), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "compiler");
        if (pValueObj != NULL) xstrncpy(pCtx->sCompiler, sizeof(pCtx->sCompiler), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pBuildObj, "verbose");
        if (pValueObj != NULL) pCtx->nVerbose = XJSON_GetInt(pValueObj);

        pValueObj = XJSON_GetObject(pBuildObj, "cxx");
        if (pValueObj != NULL) pCtx->nCPP = XJSON_GetBool(pValueObj);
    }

    xjson_obj_t *pInstallObj = XJSON_GetObject(json.pRootObj, "install");
    if (pInstallObj != NULL)
    {
        xjson_obj_t *pValueObj = XJSON_GetObject(pInstallObj, "binaryDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sBinary, sizeof(pCtx->sBinary), XJSON_GetString(pValueObj));

        pValueObj = XJSON_GetObject(pInstallObj, "headerDir");
        if (pValueObj != NULL) xstrncpy(pCtx->sIncludes, sizeof(pCtx->sIncludes), XJSON_GetString(pValueObj));
    }

    XJSON_Destroy(&json);
    free(pBuffer);
    return 1;
}
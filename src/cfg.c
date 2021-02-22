/*
 *  src/cfg.c
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#include "stdinc.h"
#include "slog.h"
#include "xstr.h"
#include "make.h"
#include "sver.h"
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
    while ((nChar = getopt(argc, argv, "o:s:c:e:b:i:f:l:p:v:d1:x1:h1")) != -1) 
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
    char sName[SMAKE_NAME_MAX];
    char sArg[SMAKE_LINE_MAX];

    FILE* pFile = fopen(pCfgPath, "r");
    if (pFile == NULL) return 0;

    while(fscanf(pFile, "%s %[^:\n]\n", sName, sArg) == 2)
    {
        if ((strlen(sName) > 0) && (sName[0] == '#'))
        {
            /* Skip comment */
            continue;
        }
        else if (!strcmp(sName, "NAME"))
        {
            xstrncpy(pCtx->sName, sizeof(pCtx->sName), sArg);
        }
        else if (!strcmp(sName, "OUTPUT"))
        {
            xstrncpy(pCtx->sOutDir, sizeof(pCtx->sOutDir), sArg);
        }
        else if (!strcmp(sName, "BINARY_DIR"))
        {
            xstrncpy(pCtx->sBinary, sizeof(pCtx->sBinary), sArg);
        }
        else if (!strcmp(sName, "HEADER_DIR"))
        {
            xstrncpy(pCtx->sIncludes, sizeof(pCtx->sIncludes), sArg);
        }
        else if (!strcmp(sName, "EXCLUDE"))
        {
            if (strlen(pCtx->sExcept)) strcat(pCtx->sExcept, ":");
            strcat(pCtx->sExcept, sArg);
        }
        else if (!strcmp(sName, "FLAGS"))
        {
            if (strlen(pCtx->sFlags)) strcat(pCtx->sFlags, " ");
            strcat(pCtx->sFlags, sArg);
        }
        else if (!strcmp(sName, "LIBS"))
        {
            if (strlen(pCtx->sLibs)) strcat(pCtx->sLibs, " ");
            strcat(pCtx->sLibs, sArg);
        }
        else if (!strcmp(sName, "VERBOSE"))
        {
            pCtx->nVerbose = atoi(sArg);
        }
        else if (!strcmp(sName, "CXX"))
        {
            pCtx->nCPP = atoi(sArg);
        }
    }

    fclose(pFile);
    return 1;
}

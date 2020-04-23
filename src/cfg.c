/*
 *  src/cfg.c
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#include "stdinc.h"
#include "xlog.h"
#include "xstr.h"
#include "make.h"
#include "sver.h"
#include "cfg.h"

extern char *optarg;
static void SMake_CopyPath(char *pDst, int nSize, const char *pSrc)
{
    xstrncpy(pDst, pSrc, nSize);
    int nLength = strlen(pDst);
    if (pDst[nLength-1] == '/')
        pDst[nLength-1] = '\0';
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
                xstrncpy(pCtx->sConfig, optarg, sizeof(pCtx->sConfig));
                break;
            case 'e':
                SMake_CopyPath(pCtx->sExcept, sizeof(pCtx->sExcept), optarg);
                break;
            case 'b':
                SMake_CopyPath(pCtx->sBuild, sizeof(pCtx->sBuild), optarg);
                break;
            case 'i':
                SMake_CopyPath(pCtx->sInstall, sizeof(pCtx->sInstall), optarg);
                break;
            case 'f':
                xstrncpy(pCtx->sFlags, optarg, sizeof(pCtx->sFlags));
                break;
            case 'l':
                xstrncpy(pCtx->sLibs, optarg, sizeof(pCtx->sLibs));
                break;
            case 'p':
                xstrncpy(pCtx->sName, optarg, sizeof(pCtx->sName));
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
        else if (strcmp(sName, "NAME") == 0)
        {
            xstrncpy(pCtx->sName, sArg, sizeof(pCtx->sName));
        }
        else if (strcmp(sName, "BUILD") == 0)
        {
            xstrncpy(pCtx->sBuild, sArg, sizeof(pCtx->sBuild));
        }
        else if (strcmp(sName, "OUTPUT") == 0)
        {
            xstrncpy(pCtx->sOutDir, sArg, sizeof(pCtx->sOutDir));
        }
        else if (strcmp(sName, "INSTALL") == 0)
        {
            xstrncpy(pCtx->sInstall, sArg, sizeof(pCtx->sInstall));
        }
        else if (strcmp(sName, "EXCLUDE") == 0)
        {
            if (strlen(pCtx->sExcept)) strcat(pCtx->sExcept, ":");
            strcat(pCtx->sExcept, sArg);
        }
        else if (strcmp(sName, "FLAGS") == 0)
        {
            if (strlen(pCtx->sFlags)) strcat(pCtx->sFlags, " ");
            strcat(pCtx->sFlags, sArg);
        }
        else if (strcmp(sName, "LIBS") == 0)
        {
            if (strlen(pCtx->sLibs)) strcat(pCtx->sLibs, " ");
            strcat(pCtx->sLibs, sArg);
        }
        else if (strcmp(sName, "CXX") == 0)
        {
            pCtx->nCPP = atoi(sArg);
        }
    }

    fclose(pFile);
    return 1;
}

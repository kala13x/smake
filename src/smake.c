/*
 *  src/smake.c
 *
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main source file of project
 */

#include "stdinc.h"
#include "vector.h"
#include "version.h"
#include "config.h"
#include "files.h"
#include "make.h"
#include "slog.h"

static int ParseArguments(int argc, char *argv[], SMakeMap *pMap)
{
    int nChar, nSrc = 0;
    while ( (nChar = getopt(argc, argv, "s:c:b:i:f:l:p:d1:v:x1:h1")) != -1) 
    {
        switch (nChar)
        {
            case 's':
                strcpy(pMap->sPath, optarg); nSrc = 1;
                break;
            case 'c':
                strcpy(pMap->sCfgFile, optarg);
                break;
            case 'b':
                strcpy(pMap->sBuild, optarg);
                break;
            case 'i':
                strcpy(pMap->sInstall, optarg);
                break;
            case 'f':
                strcpy(pMap->sFlags, optarg);
                break;
            case 'l':
                strcpy(pMap->sLibs, optarg);
                break;
            case 'p':
                strcpy(pMap->sName, optarg);
                break;
            case 'v':
                pMap->nVerbose = atoi(optarg);
                break;
            case 'd':
                pMap->nVPath = 1;
                break;
            case 'x':
                pMap->nCPP = 1;
                break;
            case 'h':
            default:
                Greet("Simple-Make");
                Usage(argv[0]);
                return -1;
        }
    }

    if (pMap->nVPath && !nSrc)
    {
        Greet("Simple-Make");
        slog(0, SLOG_ERROR, "VPATH (-d) works with argument -s only\n");
        Usage(argv[0]);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    slog_init("smake", NULL, 2, 2, 0);

    SMakeMap objMap;
    SMakeMap_Init(&objMap);
    if (ParseArguments(argc, argv, &objMap)) return 0;

    int nSilent = objMap.nVerbose < 1 ? 1 : 0;
    if (!nSilent) Greet("Simple-Make");
    slog_silent(nSilent);

    int nRetVal = Files_GetList(&objMap);
    if (nRetVal) 
    {
        slog(0, SLOG_LIVE, "File list initialization done");
        nRetVal = SMakeMap_FillObjects(&objMap);
        if (nRetVal)
        {
            nRetVal = ConfigFile_Load(objMap.sCfgFile, &objMap);
            if (nRetVal) slog(0, SLOG_LIVE, "Config file initialization done");

            if (strlen(objMap.sName) < 1) SMake_FindMain(&objMap);
            slog(0, SLOG_LIVE, "Ready to write Makefile");

            nRetVal = SMake_WriteMake(&objMap);
            if (nRetVal) slog(0, SLOG_LIVE, "Succesfully generated Makefile");
        }
        else slog(0, SLOG_ERROR, "Unable to find source files: %s", objMap.sPath);
    }

    SMakeMap_Destroy(&objMap);
    return 0;
}

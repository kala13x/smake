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
    int c;
    while ( (c = getopt(argc, argv, "s:c:f:l:p:v1:h1")) != -1) 
    {
        switch (c)
        {
            case 's':
                strcpy(pMap->sPath, optarg);
                break;
            case 'c':
                strcpy(pMap->sCfgFile, optarg);
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
                pMap->nVerbose = 1;
                break;
            case 'h':
            default:
                Usage(argv[0]);
                return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    Greet("Simple-Make");
    slog_init("smake", NULL, 2, 2, 0);

    SMakeMap objMap;
    SMakeMap_Init(&objMap);
    if (ParseArguments(argc, argv, &objMap)) return 0;

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
    }

    SMakeMap_Destroy(&objMap);
    return 0;
}

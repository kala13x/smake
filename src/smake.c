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
#include "makes.h"
#include "slog.h"

typedef struct {
    char sPathSrc[PATH_MAX];
    char sCfgFile[PATH_MAX];
    int nVerbose;
} SMakeInfo;

void SMake_InitInfo(SMakeInfo *pInf)
{
    memset(pInf->sCfgFile, 0, PATH_MAX);
    memset(pInf->sPathSrc, 0, PATH_MAX);
    strcpy(pInf->sCfgFile, "smake.cfg");
    strcpy(pInf->sPathSrc, ".");
    pInf->nVerbose = 0;
}

static int ParseArguments(int argc, char *argv[], SMakeInfo *pInfo)
{
    int c;
    while ( (c = getopt(argc, argv, "s:c:f:l:p:v1:h1")) != -1) 
    {
        switch (c)
        {
            case 's':
                strcpy(pInfo->sPathSrc, optarg);
                break;
            case 'c':
                strcpy(pInfo->sCfgFile, optarg);
                break;
            case 'f':
                printf("%s\n",optarg);
                break;
            case 'l':
                printf("%s\n",optarg);
                break;
            case 'p':
                printf("%s\n",optarg);
                break;
            case 'v':
                pInfo->nVerbose = 1;
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

    SMakeInfo info;
    SMake_InitInfo(&info);
    if (ParseArguments(argc, argv, &info)) return 0;

    SMakeMap objMap;
    SMakeMap_Init(&objMap);
    strcpy(objMap.sPath, info.sPathSrc);
    objMap.nVerbose = info.nVerbose;

    int nRetVal = Files_GetList(info.sPathSrc, objMap.pFileList, info.nVerbose);
    if (nRetVal) 
    {
        slog(0, SLOG_LIVE, "File list initialization done");
        nRetVal = SMakeMap_FillObjects(&objMap);
        if (nRetVal)
        {
            slog(0, SLOG_LIVE, "Object list initialization done");

            nRetVal = ConfigFile_Load(info.sCfgFile, &objMap);
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

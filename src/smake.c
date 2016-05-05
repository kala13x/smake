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
#include "files.h"
#include "slog.h"

typedef struct {
    char sPathSrc[PATH_MAX];
    int nVerbose;
} SMakeInfo;

void SMake_InitInfo(SMakeInfo *pInf)
{
    memset(pInf->sPathSrc, 0, PATH_MAX);
    strcpy(pInf->sPathSrc, ".");
    pInf->nVerbose = 0;
}

static int ParseArguments(int argc, char *argv[], SMakeInfo *pInfo)
{
    int c;
    while ( (c = getopt(argc, argv, "s:v1:h1")) != -1) 
    {
        switch (c)
        {
            case 's':
                strcpy(pInfo->sPathSrc, optarg);
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
    Greet("Sun-Makefile");
    slog_init("smake", NULL, 2, 2, 0);

    SMakeInfo info;
    SMake_InitInfo(&info);
    if (ParseArguments(argc, argv, &info)) 
        return 0;

    vector *pFileList = vector_new(3);
    int nRetVal = Files_GetList(info.sPathSrc, pFileList);
    if (nRetVal > 0)
    {
        int i, nEntryes = vector_size(pFileList);
        for (i = 0; i < nEntryes; i++)
        {
            char *pFile = (char*)vector_get(pFileList, i);
            printf("%s\n", pFile);
        }
    }
    else if (!nRetVal) slog(0, SLOG_ERROR, "Can not open directory: %s", info.sPathSrc);
    else slog(0, SLOG_ERROR, "Directory is emplty: %s", info.sPathSrc);

    vector_free(pFileList);
    return 0;
}
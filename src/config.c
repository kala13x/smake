/*
 *  src/config.c
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#include "stdinc.h"
#include "config.h"
#include "make.h"
#include "slog.h"

int ConfigFile_Load(const char *pPath, SMakeMap *pMap)
{
    char sName[32];
    char sArg[256];

    FILE* pFile = fopen(pPath, "r");
    if (pFile == NULL) return 0;

    while(fscanf(pFile, "%s %s", sName, sArg) == 2)
    {
        if ((strlen(sName) > 0) && (sName[0] == '#'))
        {
            /* Skip comment */
            continue;
        }
        else if (strcmp(sName, "PROGRAM") == 0)
        {
            strcpy(pMap->sName, sArg);
        }
        else if (strcmp(sName, "FLAGS") == 0)
        {
            strcpy(pMap->sFlags, sArg);
        }
        else if (strcmp(sName, "LIBS") == 0)
        {
            strcpy(pMap->sLibs, sArg);
        }
    }

    fclose(pFile);

    return 1;
}

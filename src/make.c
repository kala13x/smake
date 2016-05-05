/*
 *  src/make.h
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#include "stdinc.h"
#include "vector.h"
#include "make.h"
#include "slog.h"

void SMakeMap_Init(SMakeMap *pMap) 
{
    pMap->pFileList = vector_new(3);
    pMap->pObjList = vector_new(3);

    if(!pMap->pFileList || !pMap->pObjList)
    {
        slog(0, SLOG_PANIC, "Can not allocate object vector memory");
        exit(EXIT_FAILURE);
    }

    memset(pMap->sName, 0, sizeof(pMap->sName));
    memset(pMap->sFlags, 0, LINE_MAX);
    memset(pMap->sLibs, 0, LINE_MAX);
}

void SMakeMap_Destroy(SMakeMap *pMap)
{
    int i, nSize = vector_size(pMap->pFileList);
    for (i = 0; i < nSize; i++) 
    {
        char *pFile = (char*)vector_get(pMap->pFileList, i);
        free(pFile);
    }

    nSize = vector_size(pMap->pObjList);
    for (i = 0; i < nSize; i++) 
    {
        char *pObj = (char*)vector_get(pMap->pObjList, i);
        free(pObj);
    }

    vector_free(pMap->pFileList);
    vector_free(pMap->pObjList);
}

int SMakeMap_FillObjects(SMakeMap *pMap)
{
    int nFilesPushed = 0;
    int i, nFiles = vector_size(pMap->pFileList);

    for(i = 0; i < nFiles; i++)
    {
        char *pFile = (char*)vector_get(pMap->pFileList, i);
        if (strstr(pFile, ".c") != NULL) 
        {
            char *pObj = strtok(pFile, ".");
            char *pObjName = (char*)malloc(strlen(pObj));
            sprintf(pObjName, "%s.o", pObj);

            vector_push(pMap->pObjList, (void*)pObjName);
            nFilesPushed++;
        }
    }

    return nFilesPushed;
}
/*
 *  src/makes.h
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#include "stdinc.h"
#include "vector.h"
#include "files.h"
#include "makes.h"
#include "slog.h"

static char *SkipToMain(char *pString)
{
    char sMain[4];
    int i ,x, nFound = 1;

    memset(sMain, 0, sizeof(sMain));
    strcpy(sMain, "main");

    int nLength = strlen(pString);
    int nMain = strlen(sMain);

    for (i = 0; i < nLength; i++)
    {
        if (pString[i] == sMain[0])
        {
            for(x = 1; x < nMain; x++)
                if (pString[i+x] != sMain[x]) nFound = 0;
 
            if (nFound)
            {
                pString += i+x;
                return pString;
            }
        }
    }

    return pString;
}

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
    memset(pMap->sPath, 0, PATH_MAX);
    pMap->nVerbose = 0;
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
            char sFile[PATH_MAX];
            memset(sFile, 0, PATH_MAX);
            strcpy(sFile, pFile);

            char *pObj = strtok(sFile, ".");
            char *pObjName = (char*)malloc(strlen(pObj));
            sprintf(pObjName, "%s.o", pObj);

            vector_push(pMap->pObjList, (void*)pObjName);
            if (pMap->nVerbose) slog(0, SLOG_DEBUG, "Loaded object: %s", pObjName);
            nFilesPushed++;
        }
    }

    if (!nFilesPushed)
    {
        slog(0, SLOG_ERROR, "Unable to find source files at: %s", pMap->sPath);
        return 0;
    }

    return 1;
}

int SMake_FindMain(SMakeMap *pMap)
{
    int i, nFiles = vector_size(pMap->pFileList);
    for (i = 0; i < nFiles; i++)
    {
        char *pFile = (char*)vector_get(pMap->pFileList, i);
        char sFile[PATH_MAX];
        memset(sFile, 0, PATH_MAX);
        sprintf(sFile, "%s/%s", pMap->sPath, pFile);
        int j, nLines = Files_GetLineNumber(sFile);

        for (j = 1; j <= nLines; j++)
        {
            char *pLine = Files_GetLine(sFile, j);
            if (pLine) 
            {
                if (strstr(pLine, " main") != NULL)
                {
                    pLine = SkipToMain(pLine);
                    while(*pLine == ' ') pLine++;

                    if (*pLine == '(')
                    {
                        char sName[128];
                        memset(sName, 0, sizeof(sName));
                        strcpy(sName, pFile);

                        char *pName = strtok(sName, ".");
                        strcpy(pMap->sName, pName);

                        if (pMap->nVerbose)
                            slog(0, SLOG_DEBUG, "Detected main file: %s", sFile);

                        return 1;
                    }
                }
            }
        }
    }

    slog(0, SLOG_WARN, "Can not find main file");
    return 0;
}

int SMake_WriteMake(SMakeMap *pMap)
{
    char sMakefile[PATH_MAX];
    memset(sMakefile, 0, PATH_MAX);
    sprintf(sMakefile, "%s/Makefile", pMap->sPath);

    FILE *pFile = fopen(sMakefile, "w");
    if (!pFile) 
    {
        slog(0, SLOG_ERROR, "Can not open destination file: %s", sMakefile);
        return 0;
    }

    fprintf(pFile, "CFLAGS = %s\n", pMap->sFlags);
    fprintf(pFile, "LIBS = %s\n\n", pMap->sLibs);
    fprintf(pFile, "OBJS = ");

    char sObjects[PATH_MAX];
    memset(sObjects, 0, PATH_MAX);
    int i, nObjs = vector_size(pMap->pObjList);

    for (i = 0; i < nObjs; i++)
    {
        char *pSingleObj = (char*)vector_get(pMap->pObjList, i);
        if (nObjs > 1)
        {
            if (i > 0)
            {
                if (i == (nObjs - 1)) fprintf(pFile, "\t%s\n\n", pSingleObj);
                else fprintf(pFile, "\t%s \\\n", pSingleObj);
            }
            else fprintf(pFile, "%s \\\n", pSingleObj);
        }
        else fprintf(pFile, "%s\n", pSingleObj);
    }

    fprintf(pFile, ".c.o:\n");
    fprintf(pFile, "\t$(CC) $(CFLAGS) -c $< $(LIBS)\n\n");
    fprintf(pFile, "%s: $(OBJS)\n", pMap->sName);
    fprintf(pFile, "\t$(CC) $(CFLAGS) -o %s $(OBJS) $(LIBS)\n\n", pMap->sName);
    fprintf(pFile, ".PHONY: clean\nclean:\n");
    fprintf(pFile, "\t$(RM) %s $(OBJS)\n", pMap->sName);
    fclose(pFile);

    return 1;
}
/*
 *  src/make.h
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#include "stdinc.h"
#include "vector.h"
#include "files.h"
#include "make.h"
#include "slog.h"

static char *SkipToStr(char *pString, const char *pStr)
{
    char sMain[256];
    int i ,x, nFound = 1;

    memset(sMain, 0, sizeof(sMain));
    strcpy(sMain, pStr);

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
    memset(pMap->sInstall, 0, PATH_MAX);
    memset(pMap->sCfgFile, 0, PATH_MAX);
    memset(pMap->sBuild, 0, PATH_MAX);
    memset(pMap->sFlags, 0, LINE_MAX);
    memset(pMap->sLibs, 0, LINE_MAX);
    memset(pMap->sPath, 0, PATH_MAX);
    strcpy(pMap->sCfgFile, "smake.cfg");
    getcwd(pMap->sPath, PATH_MAX);
    pMap->nVerbose = 1;
    pMap->nVPath = 0;
    pMap->nCPP = 0;
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
    char sExt[6];
    int nFilesPushed = 0;
    int i, nFiles = vector_size(pMap->pFileList);

    memset(sExt, 0, sizeof(sExt));
    sprintf(sExt, ".%s", pMap->nCPP ? "cpp" : "c");

    for(i = 0; i < nFiles; i++)
    {
        char *pFile = (char*)vector_get(pMap->pFileList, i);
        if (strstr(pFile, sExt) != NULL)
        {
            char sFile[PATH_MAX];
            memset(sFile, 0, PATH_MAX);
            strcpy(sFile, pFile);

            char *pExt = SkipToStr(sFile, sExt);
            if (*pExt == '\0') 
            {
                char *pObj = strtok(sFile, ".");
                char *pObjName = (char*)malloc(strlen(pObj));
                sprintf(pObjName, "%s.o", pObj);

                vector_push(pMap->pObjList, (void*)pObjName);
                if (pMap->nVerbose > 1) slog(0, SLOG_DEBUG, "Loaded object: %s", pObjName);
                nFilesPushed++;
            }
        }
    }

    if (!nFilesPushed)
    {
        if (!pMap->nCPP)
        {
            pMap->nCPP = 1;
            return SMakeMap_FillObjects(pMap);
        }

        return 0;
    }

    slog(0, SLOG_LIVE, "Object list initialization done");
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
                    pLine = SkipToStr(pLine, " main");
                    while(*pLine == ' ') pLine++;

                    if (*pLine == '(')
                    {
                        char sName[128];
                        memset(sName, 0, sizeof(sName));
                        strcpy(sName, pFile);

                        char *pName = strtok(sName, ".");
                        strcpy(pMap->sName, pName);

                        if (pMap->nVerbose > 1)
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

    if (pMap->nVPath) sprintf(sMakefile, "Makefile");
    else sprintf(sMakefile, "%s/Makefile", pMap->sPath);

    char sCompiler[16], sLinker[16];
    memset(sCompiler, 0, sizeof(sCompiler));
    memset(sLinker, 0, sizeof(sLinker));

    if (pMap->nCPP)
    {
        strcpy(sCompiler, "CXX");
        strcpy(sLinker, "CXXFLAGS");
    }
    else 
    {
        strcpy(sCompiler, "CC");
        strcpy(sLinker, "CFLAGS");
    }

    FILE *pFile = fopen(sMakefile, "w");
    if (!pFile)
    {
        slog(0, SLOG_ERROR, "Can not open destination file: %s", sMakefile);
        return 0;
    }

    fprintf(pFile, "%s = %s\n", sLinker, pMap->sFlags);
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

    int nStatic, nShared;
    nStatic = nShared = 0;
    if (strstr(pMap->sName, ".a") != NULL) nStatic = 1;
    if (strstr(pMap->sName, ".so") != NULL) nShared = 1;

    if (nStatic && nShared)
    {
        slog(0, SLOG_ERROR, "Invalid filename: %s", pMap->sName);
        slog(0, SLOG_INFO, "Only one extension is allowed");
        fclose(pFile);
        remove(sMakefile);
        return 0;
    }

    int nBuild = strlen(pMap->sBuild) > 2 ? 1 : 0;
    int nInstall = strlen(pMap->sInstall) > 2 ? 1 : 0;

    if (nBuild) fprintf(pFile, "BUILD = %s\n", pMap->sBuild);
    if (nInstall) fprintf(pFile, "INSTALL = %s\n", pMap->sInstall);
    if (pMap->nVPath) fprintf(pFile, "VPATH = %s\n", pMap->sPath);
    fprintf(pFile, "\n");

    fprintf(pFile, ".%s.o:\n", pMap->nCPP ? "cpp" : "c");
    fprintf(pFile, "\t$(%s) $(%s) -c $< $(LIBS)\n\n", sCompiler, sLinker);
    fprintf(pFile, "%s: $(OBJS)\n", pMap->sName);

    if (nStatic) fprintf(pFile, "\t$(AR) rcs -o %s $(OBJS)\n\n", pMap->sName);
    else if (nShared) fprintf(pFile, "\t$(%s) -shared -o %s $(OBJS)\n\n", sCompiler, pMap->sName);
    else 
    {
        if (nBuild)
        {
            fprintf(pFile, "\t$(%s) $(%s) -o %s $(OBJS) $(LIBS)\n", sCompiler, sLinker, pMap->sName);
            fprintf(pFile, "\t@test -d $(BUILD) || mkdir $(BUILD)\n");
            fprintf(pFile, "\t@install -m 0755 %s $(BUILD)/\n\n", pMap->sName);
        }
        else fprintf(pFile, "\t$(%s) $(%s) -o %s $(OBJS) $(LIBS)\n\n", sCompiler, sLinker, pMap->sName);
    }

    if (nInstall)
    {
        fprintf(pFile, ".PHONY: install\ninstall:\n");
        fprintf(pFile, "\t@test -d $(INSTALL) || mkdir $(INSTALL)\n");
        fprintf(pFile, "\t@install -m 0755 %s $(INSTALL)/\n\n", pMap->sName);
    }

    fprintf(pFile, ".PHONY: clean\nclean:\n");
    fprintf(pFile, "\t$(RM) %s $(OBJS)\n", pMap->sName);
    fclose(pFile);

    return 1;
}
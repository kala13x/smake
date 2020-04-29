/*
 *  src/make.c
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#include "stdinc.h"
#include "file.h"
#include "xstr.h"
#include "xlog.h"
#include "make.h"

void SMake_ClearCallback(void *pData)
{
    XArrayData *pArrData = (XArrayData*)pData;
    if (pArrData != NULL)
    {
        if (pArrData->pData)
        {
            free(pArrData->pData);
            pArrData->pData = NULL;
            pArrData->nSize = 0;
        }

        free(pArrData);
        pArrData = NULL;
    }
}

SMakeFile* SMake_FileNew(const char *pPath, const char *pName, int nType)
{
    SMakeFile *pFile = malloc(sizeof(SMakeFile));
    if (pFile == NULL)
    {
        XLog_Error(0, "Can not allocate memory for file");
        return NULL;
    }

    memcpy(pFile->sPath, pPath, sizeof(pFile->sPath));
    memcpy(pFile->sName, pName, sizeof(pFile->sName));
    pFile->nType = nType;
    return pFile;
}

void SMake_InitContext(SMakeContext *pCtx) 
{
    if(XArray_Init(&pCtx->fileArr, 3, 0) == NULL)
    {
        XLog_Error(0, "Can not initialize file array");
        exit(EXIT_FAILURE);
    }

    if(XArray_Init(&pCtx->objArr, 3, 0) == NULL)
    {
        XLog_Error(0, "Can not initialize object array");
        XArray_Destroy(&pCtx->fileArr);
        exit(EXIT_FAILURE);
    }

    pCtx->fileArr.clearCb = SMake_ClearCallback;
    pCtx->objArr.clearCb = SMake_ClearCallback;

    memset(pCtx->sInstall, 0, sizeof(pCtx->sInstall));
    memset(pCtx->sConfig, 0, sizeof(pCtx->sConfig));
    memset(pCtx->sBuild, 0, sizeof(pCtx->sBuild));
    memset(pCtx->sFlags, 0, sizeof(pCtx->sFlags));
    memset(pCtx->sVPath, 0, sizeof(pCtx->sVPath));
    memset(pCtx->sName, 0, sizeof(pCtx->sName));
    memset(pCtx->sMain, 0, sizeof(pCtx->sMain));
    memset(pCtx->sLibs, 0, sizeof(pCtx->sLibs));
    pCtx->sPath[0] = pCtx->sOutDir[0] = '.';
    pCtx->sPath[1] = pCtx->sOutDir[1] = 0;
    pCtx->nVerbose = 0;
    pCtx->nVPath = 0;
    pCtx->nCPP = 0;
}

void SMake_ClearContext(SMakeContext *pCtx)
{
    XArray_Destroy(&pCtx->fileArr);
    XArray_Destroy(&pCtx->objArr);
}

int SMake_GetFileType(const char *pPath, int nLen)
{
    if (!strncmp(&pPath[nLen-4], ".cpp", 4)) return SMAKE_FILE_CPP;
    if (!strncmp(&pPath[nLen-4], ".hpp", 4)) return SMAKE_FILE_HPP;
    if (!strncmp(&pPath[nLen-2], ".c", 2)) return SMAKE_FILE_C;
    if (!strncmp(&pPath[nLen-2], ".h", 2)) return SMAKE_FILE_H;
    return SMAKE_FILE_UNF;
}

int SMake_IsExcluded(SMakeContext *pCtx, const char *pPath)
{
    char sExcluded[SMAKE_LINE_MAX];
    strncpy(sExcluded, pCtx->sExcept, sizeof(sExcluded));

    char *pExclude = strtok(sExcluded, ":");
    while(pExclude != NULL)
    {
        if (!strcmp(pExclude, pPath)) return 1;
        pExclude = strtok(NULL, ":");
    }

    return 0;
}

int SMake_LoadFiles(SMakeContext *pCtx, const char *pPath)
{
    if (SMake_IsExcluded(pCtx, pPath))
    {
        XLog_Info(1, "Path is excluded: %s", pPath);
        return 0;
    }

    XDir dir;
    if (!(XDir_Open(&dir, pPath)))
    {
        XLog_Error(0, "Can not open directory: %s", pPath);
        return 0;
    }

    char sFile[SMAKE_NAME_MAX];
    while(XDir_Read(&dir, sFile, sizeof(sFile)) > 0)
    {
        char sFullPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
        int nBytes = snprintf(sFullPath, sizeof(sFullPath), "%s/%s", pPath, sFile);

        if (SMake_IsExcluded(pCtx, sFullPath))
        {
            XLog_Info(1, "Path is excluded: %s", sFullPath);
            memset(sFile, 0, sizeof(sFile));
            continue;
        }

        int nType = SMake_GetFileType(sFullPath, nBytes);
        int nDir = (int)dir.pEntry->d_type == 4 ? 1 : 0;

        if (!nDir && nType != SMAKE_FILE_UNF)
        {
            SMakeFile *pFile = SMake_FileNew(pPath, sFile, nType);
            if (pFile == NULL) return 0;

            XLog_Live(3, "Found file: %s", sFullPath);
            XArray_AddData(&pCtx->fileArr, pFile, 0);
        }

        if (nDir) SMake_LoadFiles(pCtx, sFullPath);
        memset(sFile, 0, sizeof(sFile));
    }

    XDir_Close(&dir);
    return XArray_GetUsedSize(&pCtx->fileArr);
}

int SMake_FindMain(SMakeContext *pCtx, const char *pPath)
{
    XFile srcFile;
	char sLine[SMAKE_LINE_MAX];

    XFile_Open(&srcFile, pPath, "r");
    if (srcFile.nFD < 0)
    {
        XLog_Error(0, "Can not open source file: %s", pPath);
        return 0;
    }

    int nRet = XFile_GetLine(&srcFile, sLine, sizeof(sLine));
    while (nRet == XFILE_SUCCESS)
    {
        char *pLine = strstr(sLine, " main");
        if (pLine != NULL) 
        {
            pLine += 5;
            while(*pLine == ' ') pLine++;
            if (*pLine == '(') 
            {
                int nRetVal = 1;
                XLog_Info(0, "Found main function in file: %s", pPath);

                if (strlen(pCtx->sMain))
                {
                    XLog_Error(0, "Main function already exits: %s", pCtx->sMain);
                    XLog_Info(0, "You can exclude file or directory with argument: -e");
                    nRetVal = 0;
                }

                XFile_Close(&srcFile);
                return nRetVal;
            }
        }

        nRet = XFile_GetLine(&srcFile, sLine, sizeof(sLine));
    }

    XFile_Close(&srcFile);
    return 0;
}

int SMake_ParseProject(SMakeContext *pCtx)
{
    int i, nFiles = XArray_GetUsedSize(&pCtx->fileArr);
    for (i = 0; i < nFiles; i++)
    {
        SMakeFile *pFile = (SMakeFile*)XArray_GetData(&pCtx->fileArr, i);
        if (pFile != NULL)
        {
            char sName[SMAKE_NAME_MAX];
            strncpy(sName, pFile->sName, sizeof(sName));

            int nLength = strlen(sName);
            int nLastBytes = 0;

            if (pFile->nType == SMAKE_FILE_CPP) nLastBytes = 4;
            else if (pFile->nType == SMAKE_FILE_C) nLastBytes = 2;
            else if (pFile->nType == SMAKE_FILE_H || 
                     pFile->nType == SMAKE_FILE_HPP)
            {
                if (strstr(pCtx->sFlags, pFile->sPath) == NULL)
                {
                    strcat(pCtx->sFlags, " -I");
                    strcat(pCtx->sFlags, pFile->sPath);
                }
            }

            if (!nLastBytes) continue;
            memset(&sName[nLength-nLastBytes], 0, nLastBytes);

            char sPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
            snprintf(sPath, sizeof(sPath), "%s/%s", pFile->sPath, pFile->sName);

            if (SMake_FindMain(pCtx, sPath)) strncpy(pCtx->sMain, sName, sizeof(pCtx->sMain));
            strncat(sName, ".$(OBJ)", 8);

            SMakeFile *pObj = SMake_FileNew(pFile->sPath, sName, SMAKE_FILE_OBJ);
            if (pFile != NULL)
            {
                if (strstr(pCtx->sPath, pObj->sPath) == NULL &&
                    strstr(pCtx->sVPath, pObj->sPath) == NULL)
                {
                    strcat(pCtx->sVPath, pObj->sPath);
                    strcat(pCtx->sVPath, ":");
                }

                XLog_Live(3, "Loaded object: %s/%s", pObj->sPath, sName);
                XArray_AddData(&pCtx->objArr, pObj, 0);
            }
        }
    }

    return XArray_GetUsedSize(&pCtx->objArr);
}

int SMake_WriteMake(SMakeContext *pCtx)
{
    char sMakefile[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
    if (pCtx->nVPath) snprintf(sMakefile, sizeof(sMakefile), "Makefile");
    else snprintf(sMakefile, sizeof(sMakefile), "%s/Makefile", pCtx->sPath);

    FILE *pFile = fopen(sMakefile, "w");
    if (pFile == NULL)
    {
        XLog_Error(0, "Can not open destination file");
        return 0;
    }

    fprintf(pFile, "####################################\n");
    fprintf(pFile, "# Automatically generated by SMake #\n");
    fprintf(pFile, "# https://github.com/kala13x/smake #\n");
    fprintf(pFile, "####################################\n\n");

    char *pCompiler = pCtx->nCPP ? "CXX" : "CC";
    char *pLinker = pCtx->nCPP ? "CXXFLAGS" : "CFLAGS";

    int nStatic, nShared;
    nStatic = nShared = 0;

    if (!strlen(pCtx->sName)) strncpy(pCtx->sName, pCtx->sMain, sizeof(pCtx->sName));
    if (strstr(pCtx->sName, ".a") != NULL) nStatic = 1;
    if (strstr(pCtx->sName, ".so") != NULL) nShared = 1;

    fprintf(pFile, "%s = %s\n", pLinker, pCtx->sFlags);
    fprintf(pFile, "LIBS = %s\n", pCtx->sLibs);
    fprintf(pFile, "NAME = %s\n", pCtx->sName);
    fprintf(pFile, "ODIR = %s\n", pCtx->sOutDir);
    fprintf(pFile, "OBJ = o\n\n");
    fprintf(pFile, "OBJS = ");

    int i, nObjs = XArray_GetUsedSize(&pCtx->objArr);
    for (i = 0; i < nObjs; i++)
    {
        SMakeFile *pObj = (SMakeFile*)XArray_GetData(&pCtx->objArr, i);
        if (pObj == NULL) continue;

        if (nObjs < 2) { fprintf(pFile, "%s\n", pObj->sName); break; }
        if (!i) { fprintf(pFile, "%s \\\n", pObj->sName); continue; }
        if (i == (nObjs - 1)) fprintf(pFile, "\t%s\n\n", pObj->sName);
        else fprintf(pFile, "\t%s \\\n", pObj->sName);
    }

    int nBuild = strlen(pCtx->sBuild) ? 1 : 0;
    int nInstall = strlen(pCtx->sInstall) ? 1 : 0;
    int nVPathLen = strlen(pCtx->sVPath);

    fprintf(pFile, "OBJECTS = $(patsubst %%,$(ODIR)/%%,$(OBJS))\n");
    if (nBuild) fprintf(pFile, "BUILD = %s\n", pCtx->sBuild);
    if (nInstall) fprintf(pFile, "INSTALL = %s\n", pCtx->sInstall);

    if (pCtx->sVPath[nVPathLen-1] == ':') pCtx->sVPath[nVPathLen-1] = '\0';
    if (pCtx->nVPath) fprintf(pFile, "VPATH = %s:%s\n", pCtx->sPath, pCtx->sVPath);
    else if (strlen(pCtx->sVPath)) fprintf(pFile, "VPATH = %s\n", pCtx->sVPath);

    fprintf(pFile, "\n.%s.$(OBJ):\n", pCtx->nCPP ? "cpp" : "c");
    fprintf(pFile, "\t@test -d $(ODIR) || mkdir $(ODIR)\n");
    fprintf(pFile, "\t$(%s) $(%s)%s-c -o $(ODIR)/$@ $< $(LIBS)\n\n", 
        pCompiler, pLinker, nShared ? " -fPIC " : " ");
    fprintf(pFile, "$(NAME):$(OBJS)\n");

    if (nStatic) fprintf(pFile, "\t$(AR) rcs -o $(ODIR)/$(NAME) $(OBJECTS)\n");
    else if (nShared) fprintf(pFile, "\t$(%s) -shared -o $(ODIR)/$(NAME) $(OBJECTS)\n", pCompiler);
    else fprintf(pFile, "\t$(%s) $(%s) -o $(ODIR)/$(NAME) $(OBJECTS) $(LIBS)\n", pCompiler, pLinker);

    if (nBuild)
    {
        fprintf(pFile, "\t@test -d $(BUILD) || mkdir $(BUILD)\n");
        fprintf(pFile, "\t@install -m 0755 $(ODIR)/$(NAME) $(BUILD)/\n");
    }

    if (nInstall)
    {
        fprintf(pFile, "\n.PHONY: install\ninstall:\n");
        fprintf(pFile, "\t@test -d $(INSTALL) || mkdir $(INSTALL)\n");
        fprintf(pFile, "\t@install -m 0755 $(ODIR)/$(NAME) $(INSTALL)/\n");
    }

    fprintf(pFile, "\n.PHONY: clean\nclean:\n");
    fprintf(pFile, "\t$(RM) $(ODIR)/$(NAME) $(OBJECTS)\n");

    fclose(pFile);
    return 1;
}
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
#include "slog.h"
#include "make.h"
#include "cfg.h"

void SMake_ClearCallback(xarray_data_t *pArrData)
{
    if (pArrData != NULL)
    {
        if (pArrData->pData)
        {
            free(pArrData->pData);
            pArrData->pData = NULL;
            pArrData->nSize = 0;
        }
    }
}

SMakeFile* SMake_FileNew(const char *pPath, const char *pName, int nType)
{
    SMakeFile *pFile = malloc(sizeof(SMakeFile));
    if (pFile == NULL)
    {
        sloge("Can not allocate memory for file");
        return NULL;
    }

    memcpy(pFile->sPath, pPath, sizeof(pFile->sPath));
    memcpy(pFile->sName, pName, sizeof(pFile->sName));
    pFile->nType = nType;
    return pFile;
}

void SMake_InitContext(SMakeContext *pCtx) 
{
    if (XArray_Init(&pCtx->fileArr, 4, 0) == NULL)
    {
        sloge("Can not initialize file array");
        exit(EXIT_FAILURE);
    }

    if (XArray_Init(&pCtx->hdrArr, 4, 0) == NULL)
    {
        sloge("Can not initialize header array");
        XArray_Destroy(&pCtx->fileArr);
        exit(EXIT_FAILURE);
    }

    if (XArray_Init(&pCtx->objArr, 4, 0) == NULL)
    {
        sloge("Can not initialize object array");
        XArray_Destroy(&pCtx->fileArr);
        XArray_Destroy(&pCtx->hdrArr);
        exit(EXIT_FAILURE);
    }

    pCtx->fileArr.clearCb = SMake_ClearCallback;
    pCtx->objArr.clearCb = SMake_ClearCallback;
    pCtx->hdrArr.clearCb = SMake_ClearCallback;

    pCtx->sPath[0] = pCtx->sOutDir[0] = '.';
    pCtx->sPath[1] = pCtx->sOutDir[1] = XSTRNULL;
    pCtx->sCompiler[0] = XSTRNULL;
    pCtx->sIncludes[0] = XSTRNULL;
    pCtx->sExcept[0] = XSTRNULL;
    pCtx->sConfig[0] = XSTRNULL;
    pCtx->sBinary[0] = XSTRNULL;
    pCtx->sFlags[0] = XSTRNULL;
    pCtx->sVPath[0] = XSTRNULL;
    pCtx->sName[0] = XSTRNULL;
    pCtx->sMain[0] = XSTRNULL;
    pCtx->sLibs[0] = XSTRNULL;
    pCtx->nOverwrite = 0;
    pCtx->nVerbose = 0;
    pCtx->nVPath = 0;
    pCtx->nInit = 0;
    pCtx->nCPP = 0;
}

void SMake_ClearContext(SMakeContext *pCtx)
{
    XArray_Destroy(&pCtx->fileArr);
    XArray_Destroy(&pCtx->objArr);
    XArray_Destroy(&pCtx->hdrArr);
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
    xstrncpy(sExcluded, sizeof(sExcluded), pCtx->sExcept);

    char *pExclude = strtok(sExcluded, ";");
    while(pExclude != NULL)
    {
        if (!strcmp(pExclude, pPath)) return 1;
        pExclude = strtok(NULL, ";");
    }

    return 0;
}

int SMake_LoadFiles(SMakeContext *pCtx, const char *pPath)
{
    if (SMake_IsExcluded(pCtx, pPath))
    {
        slog_info("Path is excluded: %s", pPath);
        return 0;
    }

    XDir dir;
    if (!(XDir_Open(&dir, pPath)))
    {
        sloge("Can not open directory: %s", pPath);
        return 0;
    }

    char sFileName[SMAKE_NAME_MAX];
    while(XDir_Read(&dir, sFileName, sizeof(sFileName)) > 0)
    {
        char sFullPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
        int nBytes = snprintf(sFullPath, sizeof(sFullPath), "%s/%s", pPath, sFileName);
        if (nBytes <= 0) continue;

        if (SMake_IsExcluded(pCtx, sFullPath))
        {
            slog_info("Path is excluded: %s", sFullPath);
            continue;
        }

        int nType = SMake_GetFileType(sFullPath, nBytes);
        int nDir = (int)dir.pEntry->d_type == 4 ? 1 : 0;

        if (!nDir && nType != SMAKE_FILE_UNF)
        {
            SMakeFile *pFile = SMake_FileNew(pPath, sFileName, nType);
            if (pFile == NULL) return 0;

            slog_debug("Found file: %s", sFullPath);
            XArray_AddData(&pCtx->fileArr, pFile, 0);
        }

        if (nDir) SMake_LoadFiles(pCtx, sFullPath);
    }

    XDir_Close(&dir);
    return XArray_GetUsedSize(&pCtx->fileArr);
}

static int SMake_FindMain(SMakeContext *pCtx, const char *pPath)
{
    XFile srcFile;
	char sLine[SMAKE_LINE_MAX];

    XFile_Open(&srcFile, pPath, "r");
    if (srcFile.nFD < 0)
    {
        sloge("Can not open source file: %s", pPath);
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
                slog_info("Found main function in file: %s", pPath);

                if (strlen(pCtx->sMain))
                {
                    sloge("Main function already exists: %s", pCtx->sMain);
                    slog_info("You can exclude file or directory with argument: -e");
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

static void SMake_ProcessHeader(SMakeContext *pCtx, SMakeFile *pFile)
{
    size_t i, nCount = XArray_GetUsedSize(&pCtx->hdrArr);
    for (i = 0; i < nCount; i++)
    {
        const char *pPath = (const char*)XArray_GetData(&pCtx->hdrArr, i);
        if (pPath != NULL && !strcmp(pPath, pFile->sPath)) return;
    }

    XArray_AddData(&pCtx->hdrArr, pFile->sPath, strlen(pFile->sPath) + 1);
}

int SMake_ParseProject(SMakeContext *pCtx)
{
    size_t i, nFiles = XArray_GetUsedSize(&pCtx->fileArr);
    for (i = 0; i < nFiles; i++)
    {
        SMakeFile *pFile = (SMakeFile*)XArray_GetData(&pCtx->fileArr, i);
        if (pFile != NULL)
        {
            char sName[SMAKE_NAME_MAX];
            xstrncpy(sName, sizeof(sName), pFile->sName);

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

                SMake_ProcessHeader(pCtx, pFile);
                continue;
            }

            if (!nLastBytes)
            {
                slog_debug("Skipping file: %s/%s", pFile->sPath, pFile->sName);
                continue;
            }

            nLength -= nLastBytes;
            sName[nLength] = XSTRNULL;

            char sPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
            snprintf(sPath, sizeof(sPath), "%s/%s", pFile->sPath, pFile->sName);
            if (SMake_FindMain(pCtx, sPath)) xstrncpy(pCtx->sMain, sizeof(pCtx->sMain), sName);

            size_t nLeftBytes = sizeof(sName) - nLength;
            strncat(sName, ".$(OBJ)", nLeftBytes);

            SMakeFile *pObj = SMake_FileNew(pFile->sPath, sName, SMAKE_FILE_OBJ);
            if (pObj != NULL)
            {
                if (strstr(pCtx->sPath, pObj->sPath) == NULL &&
                    strstr(pCtx->sVPath, pObj->sPath) == NULL)
                {
                    nLeftBytes = sizeof(pCtx->sVPath) - strlen(pCtx->sVPath);
                    xstrncatf(pCtx->sVPath, nLeftBytes, "%s:", pObj->sPath);
                }

                slog_debug("Loaded object: %s/%s", pObj->sPath, sName);
                XArray_AddData(&pCtx->objArr, pObj, 0);
            }
        }
    }

    return XArray_GetUsedSize(&pCtx->objArr);
}

int SMake_InitProject(SMakeContext *pCtx)
{
    if (!strlen(pCtx->sName))
    {
        char sPwd[SMAKE_NAME_MAX];
        char *pSavePtr = NULL;

        if (getcwd(sPwd, sizeof(sPwd)) == NULL)
        {
            sloge("Filed to read current directory: %s", strerror(errno));
            return 0;
        }

        snprintf(pCtx->sName, sizeof(pCtx->sName), "%s", sPwd);
        char *pTok = strtok_r(sPwd, "/", &pSavePtr);

        while (pTok != NULL)
        {
            snprintf(pCtx->sName, sizeof(pCtx->sName), "%s", pTok);
            pTok = strtok_r(NULL, "/", &pSavePtr);
        }
    }

    char *pPath = pCtx->sPath;
    while (*pPath == '.' || *pPath == '/') pPath++;

    if (strlen(pPath) && !XPath_Exists(pPath) && !XDir_Create(pPath, 0775))
    {
        sloge("Filed to create directory: %s (%s)", pPath, strerror(errno));
        return 0;
    }

    char sSourceFile[SMAKE_PATH_MAX + SMAKE_NAME_MAX + 8];
    snprintf(sSourceFile, sizeof(sSourceFile), "%s/%s.%s",
        pCtx->sPath, pCtx->sName, pCtx->nCPP ? "cpp":"c");

    if (!XPath_Exists(sSourceFile))
    {
        FILE *pFile = fopen(sSourceFile, "w");
        if (pFile == NULL)
        {
            sloge("Can not open source file: %s (%s)",
                sSourceFile, strerror(errno));

            return 0;
        }

        fprintf(pFile, "/*\n"
            "*  Automatically generated by SMake\n"
            "*  https://github.com/kala13x/smake\n"
            "*/\n\n"
            "#include <stdio.h>\n\n"

            "int main(int argc, char *argv[])\n"
            "{\n"
            "    printf(\"Hello, World!\\n\");\n"
            "    return 0;\n"
            "}\n");

        fclose(pFile);
    }

    SMake_WriteConfig(pCtx, "smake.json");
    return 1;
}

int SMake_WriteMake(SMakeContext *pCtx)
{
    char sMakefile[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
    if (pCtx->nVPath) snprintf(sMakefile, sizeof(sMakefile), "Makefile");
    else snprintf(sMakefile, sizeof(sMakefile), "%s/Makefile", pCtx->sPath);

    if (XPath_Exists(sMakefile) && !pCtx->nOverwrite)
    {
        slogw("The Makefile already exists.");
        slogc(SLOG_WARN, 0, "Would you like to owerwrite? (Y/N): ");

        char sAnswer[8];
        sAnswer[0] = '\0';

        fgets(sAnswer, sizeof(sAnswer), stdin);
        if (sAnswer[0] != 'y' && sAnswer[0] != 'Y') return 0;
    }

    FILE *pFile = fopen(sMakefile, "w");
    if (pFile == NULL)
    {
        sloge("Can not open destination file");
        return 0;
    }

    fprintf(pFile, "####################################\n");
    fprintf(pFile, "# Automatically generated by SMake #\n");
    fprintf(pFile, "# https://github.com/kala13x/smake #\n");
    fprintf(pFile, "####################################\n\n");

    const char *pCompiler = pCtx->nCPP ? "CXX" : "CC";
    const char *pLinker = pCtx->nCPP ? "CXXFLAGS" : "CFLAGS";

    int nStatic, nShared;
    nStatic = nShared = 0;

    if (strlen(pCtx->sCompiler)) fprintf(pFile, "%s = %s\n", pCompiler, pCtx->sCompiler);
    if (!strlen(pCtx->sName)) xstrncpy(pCtx->sName, sizeof(pCtx->sName), pCtx->sMain);
    if (strstr(pCtx->sName, ".a") != NULL) nStatic = 1;
    if (strstr(pCtx->sName, ".so") != NULL) nShared = 1;

    fprintf(pFile, "%s = %s\n", pLinker, pCtx->sFlags);
    fprintf(pFile, "LIBS = %s\n", pCtx->sLibs);
    fprintf(pFile, "NAME = %s\n", pCtx->sName);
    fprintf(pFile, "ODIR = %s\n", pCtx->sOutDir);
    fprintf(pFile, "OBJ = o\n\n");
    fprintf(pFile, "OBJS = ");

    size_t i, nObjs = XArray_GetUsedSize(&pCtx->objArr);
    for (i = 0; i < nObjs; i++)
    {
        SMakeFile *pObj = (SMakeFile*)XArray_GetData(&pCtx->objArr, i);
        if (pObj == NULL) continue;

        if (nObjs < 2) { fprintf(pFile, "%s\n", pObj->sName); break; }
        if (!i) { fprintf(pFile, "%s \\\n", pObj->sName); continue; }
        if (i == (nObjs - 1)) fprintf(pFile, "\t%s\n\n", pObj->sName);
        else fprintf(pFile, "\t%s \\\n", pObj->sName);
    }

    int nBinary = strlen(pCtx->sBinary) ? 1 : 0;
    int nIncludes = strlen(pCtx->sIncludes) ? 1 : 0;
    int nVPathLen = strlen(pCtx->sVPath);

    fprintf(pFile, "OBJECTS = $(patsubst %%,$(ODIR)/%%,$(OBJS))\n");
    if (nIncludes) fprintf(pFile, "INSTALL_INC = %s\n", pCtx->sIncludes);
    if (nBinary) fprintf(pFile, "INSTALL_BIN = %s\n", pCtx->sBinary);

    if (nVPathLen && pCtx->sVPath[nVPathLen-1] == ':') pCtx->sVPath[--nVPathLen] = '\0';
    if (pCtx->nVPath) fprintf(pFile, "VPATH = %s:%s\n", pCtx->sPath, pCtx->sVPath);
    else if (nVPathLen) fprintf(pFile, "VPATH = %s\n", pCtx->sVPath);

    fprintf(pFile, "\n.%s.$(OBJ):\n", pCtx->nCPP ? "cpp" : "c");
    fprintf(pFile, "\t@test -d $(ODIR) || mkdir -p $(ODIR)\n");
    fprintf(pFile, "\t$(%s) $(%s)%s-c -o $(ODIR)/$@ $< $(LIBS)\n\n", 
        pCompiler, pLinker, nShared ? " -fPIC " : " ");
    fprintf(pFile, "$(NAME):$(OBJS)\n");

    if (nStatic) fprintf(pFile, "\t$(AR) rcs -o $(ODIR)/$(NAME) $(OBJECTS)\n");
    else if (nShared) fprintf(pFile, "\t$(%s) -shared -o $(ODIR)/$(NAME) $(OBJECTS)\n", pCompiler);
    else fprintf(pFile, "\t$(%s) $(%s) -o $(ODIR)/$(NAME) $(OBJECTS) $(LIBS)\n", pCompiler, pLinker);

    if (nBinary || nIncludes)
    {
        fprintf(pFile, "\n.PHONY: install\ninstall:\n");

        if (nBinary)
        {
            fprintf(pFile, "\t@test -d $(INSTALL_BIN) || mkdir -p $(INSTALL_BIN)\n");
            fprintf(pFile, "\t@install -m 0755 $(ODIR)/$(NAME) $(INSTALL_BIN)/\n");
        }

        if (nIncludes) fprintf(pFile, "\t@test -d $(INSTALL_INC) || mkdir -p $(INSTALL_INC)\n");
        size_t nCount = XArray_GetUsedSize(&pCtx->hdrArr);

        for (i = 0; i < nCount; i++)
        {
            const char *pPath = (const char*)XArray_GetData(&pCtx->hdrArr, i);
            if (pPath != NULL) fprintf(pFile, "\t@cp -r %s/*.h $(INSTALL_INC)/\n", pPath);
        }
    }

    fprintf(pFile, "\n.PHONY: clean\nclean:\n");
    fprintf(pFile, "\t$(RM) $(ODIR)/$(NAME) $(OBJECTS)\n");

    fclose(pFile);
    return 1;
}
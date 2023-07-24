/*!
 *  @file smake/src/make.c
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Analyze project and generate the Makefile.
 */

#include "stdinc.h"
#include "make.h"
#include "cfg.h"

void SMake_ClearCallback(xarray_data_t *pArrData)
{
    XASSERT_VOID_RET(pArrData);
    XASSERT_VOID_RET(pArrData->pData);

    free(pArrData->pData);
    pArrData->pData = NULL;
    pArrData->nSize = 0;
}

SMakeFile* SMake_FileNew(const char *pPath, const char *pName, int nType)
{
    SMakeFile *pFile = malloc(sizeof(SMakeFile));
    if (pFile == NULL)
    {
        xloge("Faild to allocate memory for SmakeFile.");
        return NULL;
    }

    xstrncpy(pFile->sPath, sizeof(pFile->sPath), pPath);
    xstrncpy(pFile->sName, sizeof(pFile->sName), pName);
    pFile->nType = nType;
    return pFile;
}

void SMake_InitContext(smake_ctx_t *pCtx) 
{
    XArray_Init(&pCtx->fileArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->pathArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->flagArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->libArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->hdrArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->objArr, XSTDNON, XFALSE);
    XArray_Init(&pCtx->incArr, XSTDNON, XFALSE);

    pCtx->pathArr.clearCb = SMake_ClearCallback;
    pCtx->fileArr.clearCb = SMake_ClearCallback;
    pCtx->flagArr.clearCb = SMake_ClearCallback;
    pCtx->libArr.clearCb = SMake_ClearCallback;
    pCtx->objArr.clearCb = SMake_ClearCallback;
    pCtx->hdrArr.clearCb = SMake_ClearCallback;
    pCtx->incArr.clearCb = SMake_ClearCallback;

    pCtx->sPath[0] = pCtx->sOutDir[0] = '.';
    pCtx->sPath[1] = pCtx->sOutDir[1] = XSTR_NUL;

    pCtx->sHeaderDst[0] = XSTR_NUL;
    pCtx->sBinaryDst[0] = XSTR_NUL;
    pCtx->sCompiler[0] = XSTR_NUL;
    pCtx->sExcept[0] = XSTR_NUL;
    pCtx->sConfig[0] = XSTR_NUL;
    pCtx->sName[0] = XSTR_NUL;
    pCtx->sMain[0] = XSTR_NUL;

    pCtx->bOverwrite = XFALSE;
    pCtx->bVPath = XFALSE;
    pCtx->bIsCPP = XFALSE;
    pCtx->bIsInit = XFALSE;
    pCtx->nVerbose = XSTDNON;
}

void SMake_ClearContext(smake_ctx_t *pCtx)
{
    XArray_Destroy(&pCtx->fileArr);
    XArray_Destroy(&pCtx->pathArr);
    XArray_Destroy(&pCtx->flagArr);
    XArray_Destroy(&pCtx->libArr);
    XArray_Destroy(&pCtx->objArr);
    XArray_Destroy(&pCtx->hdrArr);
    XArray_Destroy(&pCtx->incArr);
}

int SMake_GetFileType(const char *pPath, int nLen)
{
    if (!strncmp(&pPath[nLen-4], ".cpp", 4)) return SMAKE_FILE_CPP;
    if (!strncmp(&pPath[nLen-4], ".hpp", 4)) return SMAKE_FILE_HPP;
    if (!strncmp(&pPath[nLen-3], ".cc", 3)) return SMAKE_FILE_CPP;
    if (!strncmp(&pPath[nLen-2], ".c", 2)) return SMAKE_FILE_C;
    if (!strncmp(&pPath[nLen-2], ".h", 2)) return SMAKE_FILE_H;
    return SMAKE_FILE_UNF;
}

static xbool_t SMake_IsExcluded(smake_ctx_t *pCtx, const char *pPath)
{
    char sExcluded[SMAKE_LINE_MAX];
    xstrncpy(sExcluded, sizeof(sExcluded), pCtx->sExcept);

    char *pExclude = strtok(sExcluded, ";");
    while(pExclude != NULL)
    {
        if (!strcmp(pExclude, pPath)) return XTRUE;
        pExclude = strtok(NULL, ";");
    }

    return XFALSE;
}

xbool_t SMake_LoadProjectFiles(smake_ctx_t *pCtx, const char *pPath)
{
    if (SMake_IsExcluded(pCtx, pPath))
    {
        xlogi("Path is excluded: %s", pPath);
        return XFALSE;
    }

    xdir_t dir;
    if (XDir_Open(&dir, pPath) < 0)
    {
        xloge("Failed to open directory: %s (%s)", pPath, XSTRERR);
        return XFALSE;
    }

    char sFileName[SMAKE_NAME_MAX];
    while(XDir_Read(&dir, sFileName, sizeof(sFileName)) > 0)
    {
        char sFullPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
        int nBytes = xstrncpyf(sFullPath, sizeof(sFullPath), "%s/%s", pPath, sFileName);
        if (nBytes <= 0) continue;

        if (SMake_IsExcluded(pCtx, sFullPath))
        {
            xlogi("Path is excluded: %s", sFullPath);
            continue;
        }

        int nType = SMake_GetFileType(sFullPath, nBytes);
        xbool_t bIsDir = (int)(dir.pEntry->d_type) == 4 ? XTRUE : XFALSE;

        if (!bIsDir && nType != SMAKE_FILE_UNF)
        {
            SMakeFile *pFile = SMake_FileNew(pPath, sFileName, nType);
            if (pFile == NULL) return XFALSE;

            xlogd("Found project file: %s", sFullPath);
            XArray_AddData(&pCtx->fileArr, pFile, XSTDNON);
        }

        if (bIsDir) SMake_LoadProjectFiles(pCtx, sFullPath);
    }

    XDir_Close(&dir);
    return XTRUE;
}

static xbool_t SMake_FindMain(smake_ctx_t *pCtx, const char *pPath)
{
    xfile_t srcFile;
	char sLine[SMAKE_LINE_MAX];

    XFile_Open(&srcFile, pPath, "r", NULL);
    if (srcFile.nFD < 0)
    {
        xloge("Failed to open source file: %s (%s)", pPath, XSTRERR);
        return XFALSE;
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
                xbool_t bRetVal = XTRUE;
                xlogi("Located main function in the file: %s", pPath);

                if (xstrused(pCtx->sMain))
                {
                    xloge("Main function already exists in \"%s\" object", pCtx->sMain);
                    xlogi("You can exclude file or directory with argument: -e");
                    bRetVal = XFALSE;
                }

                XFile_Close(&srcFile);
                return bRetVal;
            }
        }

        nRet = XFile_GetLine(&srcFile, sLine, sizeof(sLine));
    }

    XFile_Close(&srcFile);
    return XFALSE;
}

xbool_t SMake_ParseProject(smake_ctx_t *pCtx)
{
    size_t i, nFiles = XArray_Used(&pCtx->fileArr);
    if (!nFiles)
    {
        xloge("Input files not found in the project.");
        return XFALSE;
    }

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
                SMake_AddToArray(&pCtx->incArr, "-I%s", pFile->sPath);
                SMake_AddToArray(&pCtx->hdrArr, "%s", pFile->sPath);
                continue;
            }

            if (!nLastBytes)
            {
                xlogd("Skipping file: %s/%s", pFile->sPath, pFile->sName);
                continue;
            }

            nLength -= nLastBytes;
            sName[nLength] = XSTR_NUL;

            char sPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
            xstrncpyf(sPath, sizeof(sPath), "%s/%s", pFile->sPath, pFile->sName);
            if (SMake_FindMain(pCtx, sPath)) xstrncpy(pCtx->sMain, sizeof(pCtx->sMain), sName);

            size_t nLeftBytes = sizeof(sName) - nLength;
            strncat(sName, ".$(OBJ)", nLeftBytes);

            SMakeFile *pObj = SMake_FileNew(pFile->sPath, sName, SMAKE_FILE_OBJ);
            if (pObj != NULL)
            {
                SMake_AddToArray(&pCtx->pathArr, "%s", pObj->sPath);
                XArray_AddData(&pCtx->objArr, pObj, XSTDNON);
                xlogd("Loaded compile object: %s/%s", pObj->sPath, sName);
            }
        }
    }

    if (!XArray_Used(&pCtx->objArr))
    {
        xloge("Object list is empty.");
        return XFALSE;
    }

    return XTRUE;
}

xbool_t SMake_InitProject(smake_ctx_t *pCtx)
{
    if (!xstrused(pCtx->sName))
    {
        char sPwd[SMAKE_NAME_MAX];
        char *pSavePtr = NULL;

        if (getcwd(sPwd, sizeof(sPwd)) == NULL)
        {
            xloge("Filed to read current directory: %s", XSTRERR);
            return XFALSE;
        }

        xstrncpyf(pCtx->sName, sizeof(pCtx->sName), "%s", sPwd);
        char *pTok = strtok_r(sPwd, "/", &pSavePtr);

        while (pTok != NULL)
        {
            xstrncpyf(pCtx->sName, sizeof(pCtx->sName), "%s", pTok);
            pTok = strtok_r(NULL, "/", &pSavePtr);
        }
    }

    char *pPath = pCtx->sPath;
    while (*pPath == '.' || *pPath == '/') pPath++;

    if (xstrused(pPath) && !XPath_Exists(pPath) && !XDir_Create(pPath, 0775))
    {
        xloge("Filed to create directory: %s (%s)", pPath, XSTRERR);
        return XFALSE;
    }

    char sSourceFile[SMAKE_PATH_MAX + SMAKE_NAME_MAX + 8];
    xstrncpyf(sSourceFile, sizeof(sSourceFile), "%s/%s.%s",
        pCtx->sPath, pCtx->sName, pCtx->bIsCPP ? "cpp":"c");

    if (!XPath_Exists(sSourceFile))
    {
        FILE *pFile = fopen(sSourceFile, "w");
        if (pFile == NULL)
        {
            xloge("Can not open source file: %s (%s)",
                sSourceFile, XSTRERR);

            return XFALSE;
        }

        fprintf(pFile, "/*\n"
            "*  Automatically generated by SMake\n"
            "*  https://github.com/kala13x/smake\n"
            "*/\n\n"
            "#include <stdio.h>\n\nint main");

        fprintf(pFile, "(int argc, char *argv[])\n"
            "{\n"
            "    printf(\"Hello, World!\\n\");\n"
            "    return 0;\n"
            "}\n");

        fclose(pFile);
    }

    SMake_WriteConfig(pCtx, "smake.json");
    return XTRUE;
}

int SMake_CompareName(const void *pData1, const void *pData2, void *pCtx)
{
    xarray_data_t *pFirst = (xarray_data_t*)pData1;
    xarray_data_t *pSecond = (xarray_data_t*)pData2;

    SMakeFile *pObj1 = (SMakeFile*)pFirst->pData;
    SMakeFile *pObj2 = (SMakeFile*)pSecond->pData;

    return strcmp(pObj1->sName, pObj2->sName);
}

int SMake_CompareLen(const void *pData1, const void *pData2, void *pCtx)
{
    xarray_data_t *pFirst = (xarray_data_t*)pData1;
    xarray_data_t *pSecond = (xarray_data_t*)pData2;

    const char *pStr1 = (const char*)pFirst->pData;
    const char *pStr2 = (const char*)pSecond->pData;

    return strlen(pStr1) > strlen(pStr2);
}

xbool_t SMake_WriteMake(smake_ctx_t *pCtx)
{
    char sMakefile[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
    xlogd("Starting Makefile generation: %s/Makefile", pCtx->sPath);

    if (pCtx->bVPath) xstrncpyf(sMakefile, sizeof(sMakefile), "Makefile");
    else xstrncpyf(sMakefile, sizeof(sMakefile), "%s/Makefile", pCtx->sPath);

    if (XPath_Exists(sMakefile) && !pCtx->bOverwrite)
    {
        xlogw("The Makefile already exists: %s", sMakefile);

        char sAnswer[8];
        sAnswer[0] = '\0';

        XCLI_GetInput("Would you like to owerwrite? (Y/N): ", sAnswer, sizeof(sAnswer), XTRUE);
        if (!xstrused(sAnswer) || (sAnswer[0] != 'y' && sAnswer[0] != 'Y'))
        {
            xlogn("Stopping Makefile generation.");
            return XFALSE;
        }
    }

    FILE *pFile = fopen(sMakefile, "w");
    if (pFile == NULL)
    {
        xloge("Failed to open destination file: %s (%s)", sMakefile, XSTRERR);
        return XFALSE;
    }

    fprintf(pFile, "####################################\n");
    fprintf(pFile, "# Automatically generated by SMake #\n");
    fprintf(pFile, "# https://github.com/kala13x/smake #\n");
    fprintf(pFile, "####################################\n\n");

    const char *pCompiler = pCtx->bIsCPP ? "CXX" : "CC";
    const char *pLinker = pCtx->bIsCPP ? "CXXFLAGS" : "CFLAGS";

    char sIncludes[SMAKE_LINE_MAX];
    char sFlags[SMAKE_LINE_MAX];
    char sLibs[SMAKE_LINE_MAX];

    sIncludes[0] = XSTR_NUL;
    sFlags[0] = XSTR_NUL;
    sLibs[0] = XSTR_NUL;

    xbool_t bStatic, bShared;
    bStatic = bShared = XFALSE;

    if (xstrused(pCtx->sCompiler)) fprintf(pFile, "%s = %s\n", pCompiler, pCtx->sCompiler);
    if (!xstrused(pCtx->sName)) xstrncpy(pCtx->sName, sizeof(pCtx->sName), pCtx->sMain);

    if (strstr(pCtx->sName, ".a") != NULL) bStatic = XTRUE;
    else if (strstr(pCtx->sName, ".so") != NULL) bShared = XTRUE;

    XArray_Sort(&pCtx->incArr, SMake_CompareLen, NULL);
    SMake_SerializeArray(&pCtx->incArr, XSTR_SPACE, sIncludes, sizeof(sIncludes));
    SMake_SerializeArray(&pCtx->flagArr, XSTR_SPACE, sFlags, sizeof(sFlags));
    SMake_SerializeArray(&pCtx->libArr, XSTR_SPACE, sLibs, sizeof(sLibs));

    fprintf(pFile, "%s = %s\n", pLinker, sFlags);
    if (xstrused(sIncludes))
        fprintf(pFile, "%s += %s\n", pLinker, sIncludes);

    fprintf(pFile, "LIBS = %s\n", sLibs);
    fprintf(pFile, "NAME = %s\n", pCtx->sName);
    fprintf(pFile, "ODIR = %s\n", pCtx->sOutDir);
    fprintf(pFile, "OBJ = o\n\n");
    fprintf(pFile, "OBJS = ");

    xlogi("Compiler flags: %s", sFlags);
    xlogi("Include flags: %s", sIncludes);
    xlogi("Linked libraries: %s", sLibs);
    xlogi("Binary file name: %s", pCtx->sName);
    xlogi("Output Directory: %s", pCtx->sOutDir);
    xlogi("Compiler: %s", strlen(pCtx->sCompiler) ? pCtx->sCompiler : pCompiler);

    XArray_Sort(&pCtx->objArr, SMake_CompareName, NULL);
    size_t i, nObjs = XArray_Used(&pCtx->objArr);

    for (i = 0; i < nObjs; i++)
    {
        SMakeFile *pObj = (SMakeFile*)XArray_GetData(&pCtx->objArr, i);
        if (pObj == NULL) continue;

        if (nObjs < 2) { fprintf(pFile, "%s\n", pObj->sName); break; }
        if (!i) { fprintf(pFile, "%s \\\n", pObj->sName); continue; }

        if (i == (nObjs - 1)) fprintf(pFile, "\t%s\n\n", pObj->sName);
        else fprintf(pFile, "\t%s \\\n", pObj->sName);
        xlogd("Added object to recept: %s", pObj->sName);
    }

    char sVPath[SMAKE_PATH_MAX];
    sVPath[0] = XSTR_NUL;

    if (pCtx->bVPath) SMake_AddToArray(&pCtx->pathArr, "%s", pCtx->sPath);
    XArray_Sort(&pCtx->pathArr, SMake_CompareLen, NULL);
    SMake_SerializeArray(&pCtx->pathArr, ":", sVPath, sizeof(sVPath));

    xbool_t bInstallBinary = xstrused(pCtx->sBinaryDst);
    xbool_t bInstallIncludes = xstrused(pCtx->sHeaderDst);
    int bVPathLen = strlen(sVPath);

    fprintf(pFile, "OBJECTS = $(patsubst %%,$(ODIR)/%%,$(OBJS))\n");
    if (bInstallBinary) fprintf(pFile, "INSTALL_INC = %s\n", pCtx->sHeaderDst);
    if (bInstallIncludes) fprintf(pFile, "INSTALL_BIN = %s\n", pCtx->sBinaryDst);
    if (pCtx->bVPath || bVPathLen) fprintf(pFile, "VPATH = %s\n", sVPath);

    fprintf(pFile, "\n.%s.$(OBJ):\n", pCtx->bIsCPP ? "cpp" : "c");
    fprintf(pFile, "\t@test -d $(ODIR) || mkdir -p $(ODIR)\n");
    fprintf(pFile, "\t$(%s) $(%s)%s-c -o $(ODIR)/$@ $< $(LIBS)\n\n", 
        pCompiler, pLinker, bShared ? " -fPIC " : " ");
    fprintf(pFile, "$(NAME):$(OBJS)\n");

    if (bStatic) fprintf(pFile, "\t$(AR) rcs -o $(ODIR)/$(NAME) $(OBJECTS)\n");
    else if (bShared) fprintf(pFile, "\t$(%s) -shared -o $(ODIR)/$(NAME) $(OBJECTS)\n", pCompiler);
    else fprintf(pFile, "\t$(%s) $(%s) -o $(ODIR)/$(NAME) $(OBJECTS) $(LIBS)\n", pCompiler, pLinker);

    if (bInstallBinary || bInstallIncludes)
    {
        fprintf(pFile, "\n.PHONY: install\ninstall:\n");

        if (bInstallBinary)
        {
            xlogi("Install location for binary: %s -> %s", pCtx->sName, pCtx->sBinaryDst);
            fprintf(pFile, "\t@test -d $(INSTALL_BIN) || mkdir -p $(INSTALL_BIN)\n");
            fprintf(pFile, "\t@install -m 0755 $(ODIR)/$(NAME) $(INSTALL_BIN)/\n");
        }

        if (bInstallIncludes)
        {
            fprintf(pFile, "\t@test -d $(INSTALL_INC) || mkdir -p $(INSTALL_INC)\n");
            size_t nCount = XArray_Used(&pCtx->hdrArr);

            for (i = 0; i < nCount; i++)
            {
                const char *pPath = (const char*)XArray_GetData(&pCtx->hdrArr, i);
                if (pPath != NULL)
                {
                    xlogi("Install location for headers: %s -> %s", pPath, pCtx->sHeaderDst);
                    fprintf(pFile, "\t@cp -r %s/*.h $(INSTALL_INC)/\n", pPath);
                }
            }
        }
    }

    fprintf(pFile, "\n.PHONY: clean\nclean:\n");
    fprintf(pFile, "\t$(RM) $(ODIR)/$(NAME) $(OBJECTS)\n");

    fclose(pFile);
    return XTRUE;
}
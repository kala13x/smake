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

    size_t nLength = xstrncpy(pFile->sPath, sizeof(pFile->sPath), pPath);
    while (pFile->sPath[--nLength] == '/') pFile->sPath[nLength] = '\0';

    xstrncpy(pFile->sName, sizeof(pFile->sName), pName);
    pFile->nType = nType;
    return pFile;
}

void SMake_InitContext(smake_ctx_t *pCtx) 
{
    XArray_Init(&pCtx->includes, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->excludes, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->fileArr, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->pathArr, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->flagArr, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->libArr, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->objArr, NULL, XSTDNON, XFALSE);
    XArray_Init(&pCtx->ldArr, NULL, XSTDNON, XFALSE);

    pCtx->includes.clearCb = SMake_ClearCallback;
    pCtx->excludes.clearCb = SMake_ClearCallback;
    pCtx->pathArr.clearCb = SMake_ClearCallback;
    pCtx->fileArr.clearCb = SMake_ClearCallback;
    pCtx->flagArr.clearCb = SMake_ClearCallback;
    pCtx->libArr.clearCb = SMake_ClearCallback;
    pCtx->objArr.clearCb = SMake_ClearCallback;
    pCtx->ldArr.clearCb = SMake_ClearCallback;

    pCtx->sPath[0] = pCtx->sOutDir[0] = '.';
    pCtx->sPath[1] = pCtx->sOutDir[1] = XSTR_NUL;

    pCtx->sInjectPath[0] = XSTR_NUL;
    pCtx->sHeaderDst[0] = XSTR_NUL;
    pCtx->sBinaryDst[0] = XSTR_NUL;
    pCtx->sCompiler[0] = XSTR_NUL;
    pCtx->sLDFlags[0] = XSTR_NUL;
    pCtx->sConfig[0] = XSTR_NUL;
    pCtx->sName[0] = XSTR_NUL;
    pCtx->sMain[0] = XSTR_NUL;

    pCtx->bSrcFromCfg = XFALSE;
    pCtx->bOverwrite = XFALSE;
    pCtx->bInitProj = XFALSE;
    pCtx->bWriteCfg = XFALSE;
    pCtx->bVPath = XFALSE;
    pCtx->bIsCPP = XFALSE;
    pCtx->nVerbose = XSTDNON;
}

void SMake_ClearContext(smake_ctx_t *pCtx)
{
    XArray_Destroy(&pCtx->includes);
    XArray_Destroy(&pCtx->excludes);
    XArray_Destroy(&pCtx->fileArr);
    XArray_Destroy(&pCtx->pathArr);
    XArray_Destroy(&pCtx->flagArr);
    XArray_Destroy(&pCtx->libArr);
    XArray_Destroy(&pCtx->objArr);
    XArray_Destroy(&pCtx->ldArr);
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

xbool_t SMake_LoadFiles(smake_ctx_t *pCtx, const char *pPath)
{
    const char *pFilePath = pPath ? pPath : pCtx->sPath;

    if (pCtx->bSrcFromCfg)
    {
        size_t nUsed = XArray_Used(&pCtx->fileArr);
        xlogd("Using %zu source files from config.", nUsed);
        return nUsed ? XTRUE : XFALSE;
    }

    if (SMake_IsExcluded(pCtx, pFilePath))
    {
        xlogi("Path is excluded: %s", pFilePath);
        return XFALSE;
    }

    xdir_t dir;
    if (XDir_Open(&dir, pFilePath) < 0)
    {
        xloge("Failed to open directory: %s (%s)", pFilePath, XSTRERR);
        return XFALSE;
    }

    char sFileName[SMAKE_NAME_MAX];
    while(XDir_Read(&dir, sFileName, sizeof(sFileName)) > 0)
    {
        char sFullPath[SMAKE_PATH_MAX + SMAKE_NAME_MAX];
        int nBytes = xstrncpyf(sFullPath, sizeof(sFullPath), "%s/%s", pFilePath, sFileName);
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
            SMakeFile *pFile = SMake_FileNew(pFilePath, sFileName, nType);
            if (pFile == NULL) return XFALSE;

            xlogd("Found project file: %s", sFullPath);
            XArray_AddData(&pCtx->fileArr, pFile, XSTDNON);
        }

        if (bIsDir) SMake_LoadFiles(pCtx, sFullPath);
    }

    XDir_Close(&dir);
    return XTRUE;
}

static xbool_t SMake_FindMain(smake_ctx_t *pCtx, const char *pPath)
{
    xbyte_buffer_t buffer;
    XPath_LoadBuffer(pPath, &buffer);
    XASSERT_RET(buffer.pData, XFALSE);

    char *pBuffer = (char*)buffer.pData;
    int nPosit = xstrsrc(pBuffer, "main");

    if (nPosit <= 0)
    {
        XByteBuffer_Clear(&buffer);
        return XFALSE;
    }

    /* Check empty space before "main" */
    if (pBuffer[nPosit - 1] != '\n' &&
        pBuffer[nPosit - 1] != '\t' &&
        pBuffer[nPosit - 1] != ' ')
    {
        XByteBuffer_Clear(&buffer);
        return XFALSE;
    }

    nPosit += 4; /* Skip "main" */
    xbool_t bRetVal = XFALSE;

    /* Skip empty space */
    while (pBuffer[nPosit] == '\n') nPosit++;
    while (pBuffer[nPosit] == '\t') nPosit++;
    while (pBuffer[nPosit] == ' ') nPosit++;

    /* Check if function is starting */
    if (pBuffer[nPosit] == '(')
    {
        xlogi("Located main function in the file: %s", pPath);
        bRetVal = XTRUE;

        if (xstrused(pCtx->sMain))
        {
            xloge("Main function already exists in \"%s\" object", pCtx->sMain);
            xlogi("You can exclude file or directory with argument: -e");
            bRetVal = XFALSE;
        }
    }

    XByteBuffer_Clear(&buffer);
    return bRetVal;
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
                SMake_AddToArray(&pCtx->includes, "%s", pFile->sPath);
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
    if (!pCtx->bInitProj) return XSTDOK;

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
        xfile_t file;
        if (XFile_Open(&file, sSourceFile, "cwt", NULL) < 0)
        {
            xloge("Can not open source file: %s (%s)",
                sSourceFile, XSTRERR);

            return XFALSE;
        }

        XFile_Print(&file, "/*\n"
            "*  Automatically generated by SMake\n"
            "*  https://github.com/kala13x/smake\n"
            "*/\n\n");

        XFile_Print(&file, "#include <stdio.h>\n\n"
            "int main(int argc, char *argv[])\n"
            "{\n"
            "    printf(\"Hello, World!\\n\");\n"
            "    return 0;\n"
            "}\n");

        XFile_Close(&file);
    }

    return XTRUE;
}

int SMake_CompareName(const void *pData1, const void *pData2, void *pCtx)
{
    xarray_data_t *pFirst = (xarray_data_t*)pData1;
    xarray_data_t *pSecond = (xarray_data_t*)pData2;

    SMakeFile *pObj1 = (SMakeFile*)pFirst->pData;
    SMakeFile *pObj2 = (SMakeFile*)pSecond->pData;

    (void)pCtx;
    return strcmp(pObj1->sName, pObj2->sName);
}

int SMake_CompareLen(const void *pData1, const void *pData2, void *pCtx)
{
    xarray_data_t *pFirst = (xarray_data_t*)pData1;
    xarray_data_t *pSecond = (xarray_data_t*)pData2;

    const char *pStr1 = (const char*)pFirst->pData;
    const char *pStr2 = (const char*)pSecond->pData;

    (void)pCtx;
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

    xfile_t file;
    if (XFile_Open(&file, sMakefile, "cwt", NULL)< 0)
    {
        xloge("Failed to open destination file: %s (%s)", sMakefile, XSTRERR);
        return XFALSE;
    }

    XFile_Print(&file, "####################################\n");
    XFile_Print(&file, "# Automatically generated by SMake #\n");
    XFile_Print(&file, "# https://github.com/kala13x/smake #\n");
    XFile_Print(&file, "####################################\n\n");

    const char *pCompiler = pCtx->bIsCPP ? "CXX" : "CC";
    const char *pCFlags = pCtx->bIsCPP ? "CXXFLAGS" : "CFLAGS";

    char sIncludes[SMAKE_LINE_MAX];
    char sFlags[SMAKE_LINE_MAX];
    char sLibs[SMAKE_LINE_MAX];
    char sLd[SMAKE_LINE_MAX];

    sIncludes[0] = XSTR_NUL;
    sFlags[0] = XSTR_NUL;
    sLibs[0] = XSTR_NUL;
    sLd[0] = XSTR_NUL;

    xbool_t bStatic, bShared;
    bStatic = bShared = XFALSE;

    if (xstrused(pCtx->sCompiler)) XFile_Print(&file, "%s = %s\n", pCompiler, pCtx->sCompiler);
    if (!xstrused(pCtx->sName)) xstrncpy(pCtx->sName, sizeof(pCtx->sName), pCtx->sMain);

    if (strstr(pCtx->sName, ".a") != NULL) bStatic = XTRUE;
    else if (strstr(pCtx->sName, ".so") != NULL) bShared = XTRUE;

    SMake_SerializeIncludes(&pCtx->includes, XSTR_SPACE, sIncludes, sizeof(sIncludes));
    SMake_SerializeArray(&pCtx->flagArr, XSTR_SPACE, sFlags, sizeof(sFlags));
    SMake_SerializeArray(&pCtx->libArr, XSTR_SPACE, sLibs, sizeof(sLibs));
    SMake_SerializeArray(&pCtx->ldArr, XSTR_SPACE, sLd, sizeof(sLd));

    if (xstrused(sFlags)) XFile_Print(&file, "%s = %s\n", pCFlags, sFlags);
    else if (xstrused(sIncludes)) XFile_Print(&file, "%s = %s\n", pCFlags, sIncludes);
    if (xstrused(sFlags) && xstrused(sIncludes)) XFile_Print(&file, "%s += %s\n", pCFlags, sIncludes);

    xbool_t bLDLibs = xstrused(sLd);
    if (xstrused(sLd)) XFile_Print(&file, "LD_LIBS = %s\n", sLd);
    XFile_Print(&file, "LDFLAGS =%s%s\n", xstrused(pCtx->sLDFlags) ? " " : "", pCtx->sLDFlags);

    XFile_Print(&file, "LIBS = %s\n", sLibs);
    XFile_Print(&file, "NAME = %s\n", pCtx->sName);
    XFile_Print(&file, "ODIR = %s\n", pCtx->sOutDir);
    XFile_Print(&file, "OBJ = o\n\n");

    if (xstrused(pCtx->sInjectPath) && XPath_Exists(pCtx->sInjectPath))
    {
        xbyte_buffer_t fileBuffer;
        XPath_LoadBuffer(pCtx->sInjectPath, &fileBuffer);

        if (fileBuffer.pData != NULL)
        {
            XFile_Print(&file, "%s\n\n", (char*)fileBuffer.pData);
            XByteBuffer_Clear(&fileBuffer);
        }
    }

    xlogi("Compiler flags: %s %s", sFlags, sIncludes);
    xlogi("Linked libraries: %s", sLibs);
    xlogi("Custom libraries: %s", sLd);
    xlogi("Binary file name: %s", pCtx->sName);
    xlogi("Output Directory: %s", pCtx->sOutDir);
    xlogi("Inject file: %s", xstrused(pCtx->sInjectPath) ? pCtx->sInjectPath : "None");
    xlogi("Compiler: %s", strlen(pCtx->sCompiler) ? pCtx->sCompiler : pCompiler);

    XArray_Sort(&pCtx->objArr, SMake_CompareName, NULL);
    size_t i, nObjs = XArray_Used(&pCtx->objArr);
    XFile_Print(&file, "OBJS = ");

    for (i = 0; i < nObjs; i++)
    {
        SMakeFile *pObj = (SMakeFile*)XArray_GetData(&pCtx->objArr, i);
        if (pObj == NULL) continue;

        if (nObjs < 2) { XFile_Print(&file, "%s\n", pObj->sName); break; }
        if (!i) { XFile_Print(&file, "%s \\\n", pObj->sName); continue; }

        if (i == (nObjs - 1)) XFile_Print(&file, "\t%s\n\n", pObj->sName);
        else XFile_Print(&file, "\t%s \\\n", pObj->sName);
        xlogd("Added object to recept: %s", pObj->sName);
    }

    char sVPath[SMAKE_PATH_MAX];
    sVPath[0] = XSTR_NUL;

    if (pCtx->bVPath) SMake_AddToArray(&pCtx->pathArr, "%s", pCtx->sPath);
    XArray_Sort(&pCtx->pathArr, SMake_CompareLen, NULL);
    SMake_SerializeArray(&pCtx->pathArr, ":", sVPath, sizeof(sVPath));

    const char *pFPICOption = bShared ? " -fPIC" : XSTR_EMPTY;
    xbool_t bInstallIncludes = xstrused(pCtx->sHeaderDst);
    xbool_t bInstallBinary = xstrused(pCtx->sBinaryDst);
    int bVPathLen = strlen(sVPath);

    XFile_Print(&file, "OBJECTS = $(patsubst %%,$(ODIR)/%%,$(OBJS))\n");
    if (bInstallIncludes) XFile_Print(&file, "INSTALL_INC = %s\n", pCtx->sHeaderDst);
    if (bInstallBinary) XFile_Print(&file, "INSTALL_BIN = %s\n", pCtx->sBinaryDst);
    if (pCtx->bVPath || bVPathLen) XFile_Print(&file, "VPATH = %s\n", sVPath);

    XFile_Print(&file, "\n.%s.$(OBJ):\n", pCtx->bIsCPP ? "cpp" : "c");
    XFile_Print(&file, "\t@test -d $(ODIR) || mkdir -p $(ODIR)\n");
    XFile_Print(&file, "\t$(%s) $(%s)%s -c -o $(ODIR)/$@ $< $(LIBS)\n\n", pCompiler, pCFlags, pFPICOption);
    XFile_Print(&file, "$(NAME):$(OBJS)\n");

    if (bStatic) XFile_Print(&file, "\t$(AR) rcs $(ODIR)/$(NAME) $(OBJECTS)\n");
    else if (bShared) XFile_Print(&file, "\t$(%s) -shared -o $(ODIR)/$(NAME) $(OBJECTS)\n", pCompiler);
    else if (!bLDLibs) XFile_Print(&file, "\t$(%s) $(%s) $(LDFLAGS) -o $(ODIR)/$(NAME) $(OBJECTS) $(LIBS)\n", pCompiler, pCFlags);
    else XFile_Print(&file, "\t$(%s) $(%s) $(LDFLAGS) -o $(ODIR)/$(NAME) $(OBJECTS) $(LD_LIBS) $(LIBS)\n", pCompiler, pCFlags);

    if (bInstallBinary || bInstallIncludes)
    {
        XFile_Print(&file, "\n.PHONY: install\ninstall:\n");

        if (bInstallBinary)
        {
            xlogi("Install location for binary: %s -> %s", pCtx->sName, pCtx->sBinaryDst);
            XFile_Print(&file, "\t@test -d $(INSTALL_BIN) || mkdir -p $(INSTALL_BIN)\n");
            XFile_Print(&file, "\tinstall -m 0755 $(ODIR)/$(NAME) $(INSTALL_BIN)/\n");
        }

        if (bInstallIncludes)
        {
            XFile_Print(&file, "\t@test -d $(INSTALL_INC) || mkdir -p $(INSTALL_INC)\n");
            size_t nCount = XArray_Used(&pCtx->includes);

            for (i = 0; i < nCount; i++)
            {
                const char *pPath = (const char*)XArray_GetData(&pCtx->includes, i);
                if (pPath != NULL)
                {
                    xlogi("Install location for headers: %s -> %s", pPath, pCtx->sHeaderDst);
                    XFile_Print(&file, "\tcp -r %s/*.h $(INSTALL_INC)/\n", pPath);
                }
            }
        }
    }

    XFile_Print(&file, "\n.PHONY: clean\nclean:\n");
    XFile_Print(&file, "\t$(RM) $(ODIR)/$(NAME) $(OBJECTS)\n");

    XFile_Close(&file);
    return XTRUE;
}
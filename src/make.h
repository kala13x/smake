/*!
 *  @file smake/src/make.h
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Analyze project and generate the Makefile.
 */

#ifndef __SMAKE_MAKE_H__
#define __SMAKE_MAKE_H__

#include "stdinc.h"

#define SMAKE_CFG_FILE "smake.json"
#define SMAKE_PATH_MAX 4096
#define SMAKE_LINE_MAX 2048
#define SMAKE_NAME_MAX 128
#define SMAKE_EXT_MAX  6

#define SMAKE_FILE_UNF  0
#define SMAKE_FILE_OBJ  1
#define SMAKE_FILE_CPP  2
#define SMAKE_FILE_HPP  3
#define SMAKE_FILE_C    4
#define SMAKE_FILE_H    5

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char sPath[SMAKE_PATH_MAX];
    char sName[SMAKE_NAME_MAX];
    int nType;
} SMakeFile;

typedef struct SMakeContext {
    /* General info */
    char sCompiler[SMAKE_NAME_MAX];
    char sHeaderDst[SMAKE_PATH_MAX];
    char sBinaryDst[SMAKE_PATH_MAX];
    char sOutDir[SMAKE_PATH_MAX];
    char sConfig[SMAKE_PATH_MAX];
    char sPath[SMAKE_PATH_MAX];
    char sName[SMAKE_NAME_MAX];
    char sMain[SMAKE_NAME_MAX];

    /* Flags */
    xbool_t bSrcFromCfg;
    xbool_t bOverwrite;
    xbool_t bInitProj;
    xbool_t bWriteCfg;
    xbool_t bVPath;
    xbool_t bIsCPP;
    uint8_t nVerbose;

    /* Arrays */
    xarray_t excludes;
    xarray_t fileArr;
    xarray_t pathArr;
    xarray_t flagArr;
    xarray_t libArr;
    xarray_t hdrArr;
    xarray_t objArr;
    xarray_t ldArr;
} smake_ctx_t;

SMakeFile* SMake_FileNew(const char *pPath, const char *pName, int nType);
int SMake_GetFileType(const char *pPath, int nLen);

void SMake_InitContext(smake_ctx_t *pCtx);
void SMake_ClearContext(smake_ctx_t *pCtx);

xbool_t SMake_LoadFiles(smake_ctx_t *pCtx, const char *pPath);
xbool_t SMake_ParseProject(smake_ctx_t *pCtx);
xbool_t SMake_InitProject(smake_ctx_t *pCtx);
xbool_t SMake_WriteMake(smake_ctx_t *pCtx);

#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_MAKE_H__ */
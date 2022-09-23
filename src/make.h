/*
 *  src/make.h
 * 
 *  Copyleft (C) 202-  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#ifndef __SMAKE_MAKE_H__
#define __SMAKE_MAKE_H__

#include "stdinc.h"
#include "xjson.h"

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

typedef struct {
    char sCompiler[SMAKE_NAME_MAX];
    char sIncludes[SMAKE_PATH_MAX];
    char sOutDir[SMAKE_PATH_MAX];
    char sExcept[SMAKE_LINE_MAX];
    char sConfig[SMAKE_PATH_MAX];
    char sBinary[SMAKE_PATH_MAX];
    char sFlags[SMAKE_LINE_MAX];
    char sVPath[SMAKE_PATH_MAX];
    char sLibs[SMAKE_LINE_MAX];
    char sPath[SMAKE_PATH_MAX];
    char sName[SMAKE_NAME_MAX];
    char sMain[SMAKE_NAME_MAX];
    uint8_t nVerbose;
    uint8_t nVPath;
    uint8_t nCPP;
    uint8_t nInit;
    xarray_t fileArr;
    xarray_t objArr;
} SMakeContext;

void SMake_InitContext(SMakeContext *pCtx);
void SMake_ClearContext(SMakeContext *pCtx);

int SMake_LoadFiles(SMakeContext *pCtx, const char *pPath);
int SMake_ParseProject(SMakeContext *pCtx);
int SMake_InitProject(SMakeContext *pCtx);
int SMake_WriteMake(SMakeContext *pCtx);

#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_MAKE_H__ */
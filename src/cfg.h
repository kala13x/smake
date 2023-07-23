/*
 *  src/cfg.h
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#ifndef __SMAKE_CFG_H__
#define __SMAKE_CFG_H__

#include "stdinc.h"
#include "make.h"

#ifdef __cplusplus
extern "C" {
#endif

xbool_t SMake_SerializeArray(xarray_t *pArr, const char *pDlmt, char *pOutput, size_t nSize);
xbool_t SMake_AddToArray(xarray_t *pArr, const char *pFmt, ...);
xbool_t SMake_AddTokens(xarray_t *pArr, const char *pDlmt, const char *pInput);

int SMake_ParseArgs(smake_ctx_t *pCtx, int argc, char *argv[]);
int SMake_ParseConfig(smake_ctx_t *pCtx, const char *pPath);
int SMake_WriteConfig(smake_ctx_t *pCtx, const char *pPath);
int SMake_GetLogFlags(uint8_t nVerbose);

#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_CFG_H__ */
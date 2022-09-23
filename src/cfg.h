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

int SMake_ParseArgs(SMakeContext *pCtx, int argc, char *argv[]);
int SMake_ParseConfig(SMakeContext *pCtx, const char *pPath);
int SMake_WriteConfig(SMakeContext *pCtx, const char *pPath);
int SMake_GetLogFlags(uint8_t nVerbose);

#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_CFG_H__ */
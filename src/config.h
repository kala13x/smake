/*
 *  src/config.h
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Functions for working with config files.
 */

#ifndef __SMAKE_CONFIG_H__
#define __SMAKE_CONFIG_H__

#include "make.h"

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

int ConfigFile_Load(const char *pPath, SMakeMap *pMap);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_CONFIG_H__ */
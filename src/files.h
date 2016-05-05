/*
 *  src/files.h
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Working with files and directories.
 */

#ifndef __SMAKE_FILES_H__
#define __SMAKE_FILES_H__

#include "stdinc.h"
#include "vector.h"

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

int Files_GetList(char *pPath, vector *pFileList);
int Files_PrintDir(char *pPath, int nDepth);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_FILES_H__ */
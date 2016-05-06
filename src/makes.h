/*
 *  src/makes.h
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main works to prepare makefile.
 */

#ifndef __SMAKE_MAKE_H__
#define __SMAKE_MAKE_H__

#include "stdinc.h"
#include "vector.h"

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    vector *pFileList;
    vector *pObjList;
    char sCfgFile[PATH_MAX];
    char sFlags[LINE_MAX];
    char sLibs[LINE_MAX];
    char sPath[PATH_MAX];
    char sName[128];
    int nVerbose;
} SMakeMap;

void SMakeMap_Init(SMakeMap *pMap);
void SMakeMap_Destroy(SMakeMap *pMap);

int SMakeMap_FillObjects(SMakeMap *pMap);
int SMake_FindMain(SMakeMap *pMap);
int SMake_WriteMake(SMakeMap *pMap);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __SMAKE_MAKE_H__ */
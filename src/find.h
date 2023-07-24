/*!
 *  @file smake/src/find.h
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Find dynamic and static libraries in the system.
 */

#ifndef __SMAKE_FIND_H__
#define __SMAKE_FIND_H__

#include "stdinc.h"
#include "make.h"

typedef struct SMakeFind {
    const char *pFindStr;
    const char *pFlags;
    const char *pLibs;
} smake_find_t;

XSTATUS SMake_FindLibs(smake_ctx_t *pCtx, const smake_find_t *pFind);

#endif /* __SMAKE_FIND_H__ */
/*
 *  src/smake.c
 *
 *  Copyleft (C) 2020  Sun Dro (a.k.a. kala13x)
 *
 * Main source file of project
 */

#include "stdinc.h"
#include "make.h"
#include "info.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    xlog_init("smake", XLOG_ERROR | XLOG_WARN, 0);

    SMakeContext smakeCtx;
    SMake_InitContext(&smakeCtx);

    if (SMake_ParseArgs(&smakeCtx, argc, argv))
    {
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    SMake_ParseConfig(&smakeCtx, SMAKE_CFG_FILE);
    int nLogFlags = SMake_GetLogFlags(smakeCtx.nVerbose);
    if (smakeCtx.nVerbose) SMake_Greet(SMAKE_FULL_NAME);

    xlog_cfg_t logCfg;
    xlog_get(&logCfg);
    logCfg.nFlags = nLogFlags;
    logCfg.bIndent = XTRUE;
    xlog_set(&logCfg);

    if (smakeCtx.nInit && !SMake_InitProject(&smakeCtx))
    {
        xloge("Failed to initialize project");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_LoadFiles(&smakeCtx, smakeCtx.sPath))
    {
        xloge("No input files found");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_ParseProject(&smakeCtx))
    {
        xloge("Can not load object list");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_WriteMake(&smakeCtx))
    {
        xloge("Failed to generate Makefile");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }
    
    xlogn("Successfuly generated Makefile");
    SMake_ClearContext(&smakeCtx);
    return 0;
}

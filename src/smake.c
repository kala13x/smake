/*
 *  src/smake.c
 *
 *  Copyleft (C) 2020  Sun Dro (a.k.a. kala13x)
 *
 * Main source file of project
 */

#include "stdinc.h"
#include "slog.h"
#include "make.h"
#include "info.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    slog_init("smake", SLOG_ERROR | SLOG_WARN, 0);

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

    slog_config_t logCfg;
    slog_config_get(&logCfg);
    logCfg.nFlags = nLogFlags;
    logCfg.nIndent = 1;
    slog_config_set(&logCfg);

    if (smakeCtx.nInit && !SMake_InitProject(&smakeCtx))
    {
        sloge("Failed to initialize project");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_LoadFiles(&smakeCtx, smakeCtx.sPath))
    {
        sloge("No input files found");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_ParseProject(&smakeCtx))
    {
        sloge("Can not load object list");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }

    if (!SMake_WriteMake(&smakeCtx))
    {
        sloge("Failed to generate Makefile");
        SMake_ClearContext(&smakeCtx);
        return -1;
    }
    
    slogn("Successfuly generated Makefile");
    SMake_ClearContext(&smakeCtx);
    return 0;
}

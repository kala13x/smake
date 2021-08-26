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
    int nRetVal = 1;
    slog_init("smake", SLOG_ERROR, 0);

    SMakeContext smakeCtx;
    SMake_InitContext(&smakeCtx);

    if (SMake_ParseArgs(&smakeCtx, argc, argv))
    {
        SMake_ClearContext(&smakeCtx);
        return nRetVal;
    }

    SMake_ParseConfig(&smakeCtx, SMAKE_CFG_FILE);
    int nLogFlags = SMake_GetLogFlags(smakeCtx.nVerbose);
    if (smakeCtx.nVerbose) SMake_Greet(SMAKE_FULL_NAME);

    slog_config_t logCfg;
    slog_config_get(&logCfg);
    logCfg.nFlags = nLogFlags;
    slog_config_set(&logCfg);

    if (SMake_LoadFiles(&smakeCtx, smakeCtx.sPath))
    {
        if (SMake_ParseProject(&smakeCtx))
        {
            if (SMake_WriteMake(&smakeCtx))
            {
                slog("Successfuly generated Makefile");
                nRetVal = 0;
            }
        }
        else slog_error("Can not load object list");
    }
    else slog_error("No input files found");
    
    SMake_ClearContext(&smakeCtx);
    return nRetVal;
}

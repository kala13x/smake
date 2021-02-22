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
#include "sver.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    SMake_Greet("Simple-Make");
    slog_init("smake", SLOG_ERROR, 0);

    SMakeContext smakeCtx;
    SMake_InitContext(&smakeCtx);

    if (SMake_ParseArgs(&smakeCtx, argc, argv))
    {
        SMake_ClearContext(&smakeCtx);
        return 0;
    }

    SMake_ParseConfig(&smakeCtx, SMAKE_CFG_FILE);
    int nLogFlags = SMake_GetLogFlags(smakeCtx.nVerbose);

    SLogConfig logCfg;
    slog_config_get(&logCfg);
    logCfg.nFlags = nLogFlags;
    slog_config_set(&logCfg);

    if (SMake_LoadFiles(&smakeCtx, smakeCtx.sPath))
    {
        if (SMake_ParseProject(&smakeCtx))
        {
            if (SMake_WriteMake(&smakeCtx)) 
                slog("Successfuly generated Makefile");
        }
    }

    SMake_ClearContext(&smakeCtx);
    return 0;
}

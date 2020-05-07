/*
 *  src/smake.c
 *
 *  Copyleft (C) 2020  Sun Dro (a.k.a. kala13x)
 *
 * Main source file of project
 */

#include "stdinc.h"
#include "xlog.h"
#include "make.h"
#include "sver.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    SMake_Greet("Simple-Make");
    XLogger_Init("smake", "NULL", 0, 0);

    SMakeContext smakeCtx;
    SMake_InitContext(&smakeCtx);
    if (SMake_ParseArgs(&smakeCtx, argc, argv))
    {
        SMake_ClearContext(&smakeCtx);
        return 0;
    }

    XLoggerConfig logCfg;
    XLogger_ConfigGet(&logCfg);
    logCfg.nLogLevel = smakeCtx.nVerbose;
    XLogger_ConfigSet(&logCfg);

    SMake_ParseConfig(&smakeCtx, SMAKE_CFG_FILE);
    if (SMake_LoadFiles(&smakeCtx, smakeCtx.sPath))
    {
        if (SMake_ParseProject(&smakeCtx))
        {
            if (SMake_WriteMake(&smakeCtx)) 
                XLog_Live(0, "Successfuly generated Makefile");
        }
    }

    SMake_ClearContext(&smakeCtx);
    return 0;
}

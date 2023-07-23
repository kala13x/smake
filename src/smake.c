/*!
 *  @file smake/src/make.h
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Main source file of project.
 */

#include "stdinc.h"
#include "make.h"
#include "info.h"
#include "cfg.h"

int main(int argc, char *argv[])
{
    xlog_defaults();
    xlog_name("smake");
    xlog_setfl(XLOG_ERROR | XLOG_WARN);

    smake_ctx_t smake;
    SMake_InitContext(&smake);

    if (SMake_ParseArgs(&smake, argc, argv))
    {
        SMake_ClearContext(&smake);
        return XSTDERR;
    }

    SMake_ParseConfig(&smake, SMAKE_CFG_FILE);
    int nLogFlags = SMake_GetLogFlags(smake.nVerbose);
    if (smake.nVerbose) SMake_Greet(SMAKE_FULL_NAME);

    xlog_setfl(nLogFlags);
    xlog_indent(XTRUE);

    if ((smake.bIsInit && !SMake_InitProject(&smake)) ||
        !SMake_LoadProjectFiles(&smake, smake.sPath) ||
        !SMake_ParseProject(&smake) ||
        !SMake_WriteMake(&smake))
    {
        SMake_ClearContext(&smake);
        return XSTDERR;
    }
    
    xlogn("Successfuly generated Makefile.");
    SMake_ClearContext(&smake);
    return XSTDNON;
}

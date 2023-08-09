/*!
 *  @file smake/src/smake.c
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
    xlog_indent(XTRUE);
    xlog_name("smake");

    smake_ctx_t smake;
    SMake_InitContext(&smake);

    if (!SMake_ParseArgs(&smake, argc, argv) ||
        !SMake_ParseConfig(&smake) ||
        !SMake_InitProject(&smake) ||
        !SMake_LoadFiles(&smake, NULL) ||
        !SMake_ParseProject(&smake) ||
        !SMake_WriteMake(&smake) ||
        !SMake_WriteConfig(&smake))
    {
        SMake_ClearContext(&smake);
        return XSTDERR;
    }

    xlogn("Successfuly generated Makefile.");
    SMake_ClearContext(&smake);
    return XSTDNON;
}

/*
 *  src/version.c
 *
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Get additional information about project
 */

#include "stdinc.h"
#include "version.h"

const char* GetVersion()
{
    static char sVersion[128];
    sprintf(sVersion, "%d.%d build %d (%s)", 
        VERSION_MAX, VERSION_MIN, BUILD_NUMBER, __DATE__);

    return sVersion;
}

const char* GetVersion_Short()
{
    static char sVersion[128];
    sprintf(sVersion, "%d.%d.%d", 
        VERSION_MAX, VERSION_MIN, BUILD_NUMBER);

    return sVersion;
}

void Greet(const char *pName)
{
    printf("=================================================\n");
    printf("%s Version: %s\n", pName, GetVersion());
    printf("=================================================\n");
}

void Usage(const char *pName)
{
    printf("Usage: %s [-s <src-path>] [-c <config>] [-f <'flags'>] [-l <'libs'>] [-v] [-h]\n", pName);
    printf("Options are:\n");
    printf("  -s <src-path>      # Path of source files\n");
    printf("  -c <config>        # Config file\n");
    printf("  -f <'flags'>       # Compiler flags\n");
    printf("  -l <'libs'>        # Linked libraries\n");
    printf("  -p <name>          # Program name\n");
    printf("  -v                 # Verbose\n");
    printf("  -h                 # Print version and usage\n");
    printf("Example: %s -s src -f '-g -Wall' -l 'lpthread'\n\n", pName);
}
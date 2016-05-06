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

static char *WhiteSpace(const int nLength)
{
    static char sRetVal[128];
    memset(sRetVal, 0, sizeof(sRetVal));
    int i = 0;

    for (i = 0; i < nLength; i++)
        sRetVal[i] = ' ';

    return sRetVal;
}

void Usage(const char *pName)
{
    int nLength = strlen(pName) + 6;
    printf("Usage: %s [-s <src-path>] [-c <config-path>]\n", pName);
    printf(" %s [-f <'flags'>] [-l <'libs'>] [-v] [-h]\n", WhiteSpace(nLength));
    printf("Options are:\n");
    printf("  -s <src-path>       # Path of source files\n");
    printf("  -c <config-path>    # Config file\n");
    printf("  -f <'flags'>        # Compiler flags\n");
    printf("  -l <'libs'>         # Linked libraries\n");
    printf("  -p <name>           # Program name\n");
    printf("  -v                  # Verbose\n");
    printf("  -h                  # Print version and usage\n\n");
    printf("Example: %s -s src -f '-Wall' -l '-lpthread'\n\n", pName);
}
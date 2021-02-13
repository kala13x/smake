/*
 *  src/sver.c
 *
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Get additional information about project
 */

#include "stdinc.h"
#include "sver.h"

const char* SMake_Version()
{
    static char sVersion[128];
    sprintf(sVersion, "%d.%d build %d (%s)", 
        SMAKE_VERSION_MAX, SMAKE_VERSION_MIN, SMAKE_BUILD_NUMBER, __DATE__);

    return sVersion;
}

const char* SMake_VersionShort()
{
    static char sVersion[128];
    sprintf(sVersion, "%d.%d.%d", 
        SMAKE_VERSION_MAX, SMAKE_VERSION_MIN, SMAKE_BUILD_NUMBER);

    return sVersion;
}

void SMake_Greet(const char *pName)
{
    printf("================================================\n");
    printf("%s Version: %s\n", pName, SMake_Version());
    printf("================================================\n");
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

void SMake_Usage(const char *pName)
{
    int nLength = strlen(pName) + 6;
    printf("Usage: %s [-f <'flags'>] [-l <'libs'>] [-b <path>] [-c <path>] [-e <paths>]\n", pName);
    printf(" %s [-i <path>] [-o <path>] [-p <name>] [-s <path>] [-d] [-v] [-x] [-h]\n", WhiteSpace(nLength));
    printf("Options are:\n");
    printf("  -f <'flags'>        # Compiler flags\n");
    printf("  -l <'libs'>         # Linked libraries\n");
    printf("  -b <path>           # Install destination for binary\n");
    printf("  -c <path>           # Path to config file\n");
    printf("  -e <paths>          # Exclude files or directories\n");
    printf("  -i <path>           # Install destination for includes\n");
    printf("  -o <path>           # Object output destination\n");
    printf("  -p <name>           # Program or library name\n");
    printf("  -s <path>           # Path to source files\n");
    printf("  -d                  # Virtual directory\n");
    printf("  -v                  # Verbosity level\n");
    printf("  -x                  # Use CPP compiler\n");
    printf("  -h                  # Print version and usage\n\n");
    printf("Hints:\n1) You can exclude multiple files and directories with \":\" tokenizer\n");
    printf("2) To build static/shared library use parameter: -p <name>.a/<name>.so\n");
    printf("Example: %s -p lib.a -f '-Wall' -l '-lpthread' -e ./dir1:./file1:./file2\n\n", pName);
}
/*!
 *  @file smake/src/info.c
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Get additional information about project.
 */

#include "stdinc.h"
#include "info.h"

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
    printf("=============================================================================\n");
    printf("%s Version: %s (email: s.kalatoz@gmail.com)\n", pName, SMake_Version());
    printf("=============================================================================\n");
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
    SMake_Greet(SMAKE_FULL_NAME);
    int nLength = strlen(pName) + 6;
 
    printf("Usage: %s [-f <'flags'>] [-b <path>] [-i <path>] [-c <path>] [-I]\n", pName);
    printf(" %s [-l <'libs'>] [-e <paths>] [-g <name>] [-o <path>] [-j] \n", WhiteSpace(nLength));
    printf(" %s [-L <'libs'>] [-p <name>] [-s <path>] [-d] [-v] [-w] [-x] [-h]\n", WhiteSpace(nLength));
    printf("Options are:\n");
    printf("  -f <'flags'>        # Compiler flags\n");
    printf("  -l <'libs'>         # Linked libraries\n");
    printf("  -L <'libs'>         # Custom libraries (LD_LIBS)\n");
    printf("  -b <path>           # Install destination for binary\n");
    printf("  -c <path>           # Specify path to config file\n");
    printf("  -e <paths>          # Exclude files or directories\n");
    printf("  -g <name>           # Specify the desired compiler\n");
    printf("  -i <path>           # Install destination for includes\n");
    printf("  -o <path>           # Object output destination\n");
    printf("  -p <name>           # Program or library name\n");
    printf("  -s <path>           # Path to source files\n");
    printf("  -I                  # Initialize project\n");
    printf("  -j                  # Generate smake.json\n");
    printf("  -d                  # Virtual directory\n");
    printf("  -v                  # Verbosity level\n");
    printf("  -w                  # Force overwrite output\n");
    printf("  -x                  # Create Makefile for CPP\n");
    printf("  -h                  # Print version and usage\n\n");
    printf("Hints:\n1) You can exclude multiple files and directories with \";\" tokenizer\n");
    printf("2) To build static/shared library use parameter: -p <name>.a/<name>.so\n");
    printf("3) You can specify the desired compiler like: -g arm-histbv320-linux-gcc\n");
    printf("Example: %s -p lib.a -f '-Wall' -l '-lpthread' -e './dir1;./file1;./file2'\n\n", pName);
}
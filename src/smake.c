/*
 *  src/smake.c
 *
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Main source file of project
 */

#include "stdinc.h"
#include "vector.h"
#include "version.h"
#include "files.h"
#include "slog.h"

int main(int argc, char *argv[])
{
    Greet("Sun-Makefile");
    slog_init("smake", NULL, 2, 2, 0);

    vector *pFileList = vector_new(3);
    int nRetVal = Files_GetList("src", pFileList);
    if (nRetVal > 0)
    {
        int i, nEntryes = vector_size(pFileList);
        for (i = 0; i < nEntryes; i++)
        {
            char *pFile = (char*)vector_get(pFileList, i);
            printf("%s\n", pFile);
        }
    }

    vector_free(pFileList);
    return 0;
}
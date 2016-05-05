/*
 *  src/files.c
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Working with files and directories.
 */

#include "stdinc.h"
#include "vector.h"
#include "files.h"

int Files_GetList(char *pPath, vector *pFileList)
{
    DIR *pDir = opendir(pPath);
    if(pDir == NULL) return 0;

    struct dirent *pEntry;
    int nHaveFiles = -1;
    chdir(pPath);

    while((pEntry = readdir(pDir)) != NULL) 
    {
        /* Found an entry, but ignore . and .. */
        if(strcmp(".", pEntry->d_name) == 0 ||
           strcmp("..", pEntry->d_name) == 0)
           continue;

        char *pFileName = (char*)malloc(strlen(pEntry->d_name));
        strcpy(pFileName, pEntry->d_name);
        vector_push(pFileList, (void*)pFileName);
        nHaveFiles = 1;
    }

    chdir("..");
    closedir(pDir);

    return nHaveFiles;
}

int Files_PrintDir(char *pPath, int nDepth)
{
    DIR *pDir = opendir(pPath);
    if(pDir == NULL) return 0;

    chdir(pPath);
    struct dirent *pEntry;
    struct stat statBuf;

    while((pEntry = readdir(pDir)) != NULL) 
    {
        lstat(pEntry->d_name,&statBuf);
        if(S_ISDIR(statBuf.st_mode)) 
        {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".", pEntry->d_name) == 0 ||
               strcmp("..", pEntry->d_name) == 0)
               continue;

            printf("%*s%s/\n", nDepth, "", pEntry->d_name);
            Files_PrintDir(pEntry->d_name, nDepth + 4);
        }
        else printf("%*s%s\n", nDepth, "", pEntry->d_name);
    }

    chdir("..");
    closedir(pDir);

    return 1;
}

/*
 *  src/files.c
 * 
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Working with files and directories.
 */

#include "stdinc.h"
#include "vector.h"
#include "makes.h"
#include "files.h"
#include "slog.h"

int Files_GetLineNumber(const char *pPath)
{
    /* Open templorary file */
    FILE *fp = fopen(pPath, "r");
    if (fp != NULL) 
    {
        /* Read line-by-line */
        ssize_t read;
        size_t len = 0;
        char *line = NULL;
        unsigned int nLineNumber = 0;
        while ((read = getline(&line, &len, fp)) != -1) nLineNumber++;

        /* Cleanup */
        fclose(fp);
        if(line) free(line);

        return nLineNumber;
    }

    return 0;
}

char* Files_GetLine(const char *pPath, int nLineNumber)
{
    unsigned int nLineNo = 0;
    static char pRetLine[LINE_MAX];

    /* Open templorary file */
    FILE *fp = fopen(pPath, "r");
    if (fp != NULL) 
    {
        /* Read line-by-line */
        ssize_t read;
        size_t len = 0;
        char *line = NULL;
        while ((read = getline(&line, &len, fp)) != -1) 
        {
            nLineNo++;
            if (nLineNumber == nLineNo) 
            {
                strcpy(pRetLine, line);
                break;
            }
        }

        /* Cleanup */
        fclose(fp);
        if(line) free(line);

        return pRetLine;
    }

    return NULL;
}

int Files_GetList(SMakeMap *pMap)
{
    DIR *pDir = opendir(pMap->sPath);
    if(pDir == NULL)
    {
        slog(0, SLOG_ERROR, "Can not open directory: %s", pMap->sPath);
        return 0;
    }

    struct dirent *pEntry;
    int nHaveFiles = -1;
    chdir(pMap->sPath);

    while((pEntry = readdir(pDir)) != NULL) 
    {
        /* Found an entry, but ignore . and .. */
        if(strcmp(".", pEntry->d_name) == 0 ||
           strcmp("..", pEntry->d_name) == 0)
           continue;

        char *pFileName = (char*)malloc(strlen(pEntry->d_name));
        strcpy(pFileName, pEntry->d_name);
        vector_push(pMap->pFileList, (void*)pFileName);
        if (pMap->nVerbose) slog(0, SLOG_DEBUG, "Loaded file: %s", pFileName);
        nHaveFiles = 1;
    }

    chdir("..");
    closedir(pDir);

    if (!nHaveFiles)
    {
        slog(0, SLOG_ERROR, "Directory is emplty: %s", pMap->sPath);
        return 0;
    }

    return 1;
}

int Files_PrintDir(const char *pPath, int nDepth)
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

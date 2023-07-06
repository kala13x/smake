/*
 *  libxutils/src/file.c
 *
 *  This source is part of "libxutils" project
 *  2015-2020  Sun Dro (f4tb0y@protonmail.com)
 * 
 * Implementation of POSIX standart 
 * file and directory functionality.
 */

#include "stdinc.h"
#include "file.h"
#include "xstr.h"

#ifdef PATH_MAX
#define XFILE_BUF_SIZE PATH_MAX
#else
#define XFILE_BUF_SIZE 2048
#endif

int XFile_ParseFlags(const char *pFlags)
{
    int nLen = strlen(pFlags);
    int i, nFlags = 0;

    for (i = 0; i < nLen; i++)
    {
        switch(pFlags[i])
        {
            case 'a': { nFlags |= O_APPEND; break; }
            case 'c': { nFlags |= O_CREAT; break; }
            case 'd': { nFlags |= O_NDELAY; break; }
            case 'e': { nFlags |= O_EXCL; break; }
            case 'n': { nFlags |= O_NONBLOCK; break; }
            case 'r': { nFlags |= O_RDONLY; break; }
            case 't': { nFlags |= O_TRUNC; break; }
            case 's': { nFlags |= O_SYNC; break; }
            case 'w': { nFlags |= O_WRONLY; break; }
            case 'x': { nFlags |= O_RDWR; break; }
            default: break;
        }
    }

    return nFlags;
}

int XFile_Open(XFile *pFile, const char *pPath, char *pFlags)
{
    if (!pFile || !pPath || !pFlags) return XFILE_INVALID;
    pFile->mPerms = S_IRUSR | S_IWUSR | S_IRGRP;
    pFile->mPerms |= S_IWGRP | S_IROTH | S_IWOTH;
    pFile->nFlags = XFile_ParseFlags(pFlags);

    if (XFILE_CHECK_FL(pFile->nFlags, O_RDONLY) &&
        XFILE_CHECK_FL(pFile->nFlags, O_WRONLY))
        {
            pFile->nFlags &= ~O_RDONLY;
            pFile->nFlags &= ~O_WRONLY;
            pFile->nFlags |= O_RDWR;
        }

    pFile->nFD = open(pPath, pFile->nFlags, pFile->mPerms);
    pFile->nPosit = 0;
    return pFile->nFD;
}

void XFile_Close(XFile *pFile)
{
    if (pFile->nFD >= 0)
    {
        close(pFile->nFD);
        pFile->nFD = -1;
    }

    pFile->nFlags = 0;
    pFile->nPosit = 0;
}

int XFile_Seek(XFile *pFile, uint64_t nPosit, int nOffset)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    return lseek(pFile->nFD, nPosit, nOffset);
}

int XFile_Write(XFile *pFile, void *pBuff, size_t nSize)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    return write(pFile->nFD, pBuff, nSize);
}

int XFile_Read(XFile *pFile, void *pBuff, size_t nSize)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    return read(pFile->nFD, pBuff, nSize);
}

size_t XFile_GetSize(XFile *pFile)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    size_t nSize = XFile_Seek(pFile, 0, SEEK_END);
    XFile_Seek(pFile, 0, SEEK_SET);
    return nSize;
}

uint8_t* XFile_ReadBuffer(XFile *pFile, size_t *pSize)
{
    *pSize = 0;

    size_t nSize = XFile_GetSize(pFile);
    if (nSize <= 0) return NULL;

    uint8_t *pBuffer = (uint8_t*)malloc(nSize + 1);
    if (pBuffer == NULL) return NULL;

    int nBytes = XFile_Read(pFile, pBuffer, nSize);
    if (nBytes <= 0)
    {
        free(pBuffer);
        return NULL;
    }

    *pSize = (nBytes > 0) ? nBytes : 0;
    pBuffer[*pSize] = '\0';

    return pBuffer;
}

int XFile_Copy(XFile *pIn, XFile *pOut)
{
    if (pIn->nFD < 0 || pOut->nFD < 0) return XFILE_INVALID;
    char sBuf[XFILE_BUF_SIZE];
    int nSize = 0;

    while ((nSize = XFile_Read(pIn, sBuf, sizeof(sBuf))) > 0)
        if (XFile_Write(pOut, sBuf, nSize) != nSize) break;

    return nSize;
}

int XFile_GetLine(XFile *pFile, char* pLine, size_t nSize)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    char sReadBuf[nSize];
    char cByte;

    int nAvail = nSize;
    int nRead = 0;

    int nBytes = XFile_Read(pFile, &cByte, sizeof(char));
    sReadBuf[nRead] = cByte;
    nAvail -= nBytes;
    nRead += nBytes;

    while (nBytes > 0 && nRead < nAvail)
    {
        if (sReadBuf[nRead-nBytes] == '\n')
        {
            if (pLine != NULL && nSize > 0)
            {
                sReadBuf[nRead-nBytes] = '\0';
                xstrncpy(pLine, nSize, sReadBuf);
            }

            return XFILE_SUCCESS;
        }

        nBytes = XFile_Read(pFile, &cByte, sizeof(char));
        sReadBuf[nRead] = cByte;
        nAvail -= nBytes;
        nRead += nBytes;
    }

    return XFILE_INVALID;
}

int XFile_GetLines(XFile *pFile)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    int nLineNumber = 0;

    size_t nSize = XFile_GetSize(pFile);
    if (!nSize) return XFILE_INVALID;

    int nRet = XFile_GetLine(pFile, NULL, nSize);
    while (nRet == XFILE_SUCCESS)
    {
        nLineNumber++;
        nRet = XFile_GetLine(pFile, NULL, nSize);
    }

    return nLineNumber;
}

int XFile_ReadLine(XFile *pFile, char* pLine, size_t nSize, int nLineNumber)
{
    if (pFile->nFD < 0) return XFILE_INVALID;
    int nReadNumber = 0;

    int nRet = XFile_GetLine(pFile, pLine, nSize);
    while (nRet == XFILE_SUCCESS)
    {
        nReadNumber++;
        if (nLineNumber == nReadNumber) return XFILE_SUCCESS;
        nRet = XFile_GetLine(pFile, pLine, nSize);
    }

    return XFILE_INVALID;
}

int XPath_Exists(const char *pPath)
{
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    if (stat(pPath, &st) == -1) return 0;
    return 1;
}

int XPath_SetPerm(const char *pPath, char mode[])
{
    int i = strtol(mode, 0, 8);
    if (chmod(pPath, i) < 0) return 0;
    return 1;
}

int XPath_CopyFile(const char *pSrc, const char *pDst)
{
    XFile srcFile, dstFile;
    XFile_Open(&srcFile, pSrc, "r");
    if (srcFile.nFD < 0) return XFILE_INVALID;

    XFile_Open(&dstFile, pDst, "cwt");
    if (dstFile.nFD < 0)
    {
        XFile_Close(&srcFile);
        return XFILE_INVALID;
    }

    int nRet = XFile_Copy(&srcFile, &dstFile);
    XFile_Close(&srcFile);
    XFile_Close(&dstFile);

    return nRet;
}

int XPath_ReadFile(const char *pPath, uint8_t *pBuffer, size_t nSize)
{
    pBuffer[0] = '\0';
    XFile fileCtx;

    if (XFile_Open(&fileCtx, pPath, "r") < 0) return 0;
    int nBytes = XFile_Read(&fileCtx, pBuffer, nSize);

    size_t nTermPosit = (nBytes > 0) ? nBytes : 0;
    pBuffer[nTermPosit] = '\0';
    XFile_Close(&fileCtx);

    return nBytes;
}

int XDir_Open(XDir *pDir, const char *pPath)
{
    if (pPath == NULL) return -1;
    pDir->pDirectory = opendir(pPath);
    if(pDir->pDirectory == NULL) return 0;

    pDir->pEntry = NULL;
    pDir->pPath = pPath;
    pDir->nOpen = 1;
    return 1;
}

void XDir_Close(XDir *pDir)
{
    if (pDir->nOpen && pDir->pDirectory)
    {
        closedir(pDir->pDirectory);
        pDir->pDirectory = NULL;
        pDir->nOpen = 0;
    }
}

int XDir_Read(XDir *pDir, char *pFile, size_t nSize)
{
    if (!pDir || !pDir->nOpen) return -1;
    while((pDir->pEntry = readdir(pDir->pDirectory)) != NULL) 
    {
        /* Found an entry, but ignore . and .. */
        if(strcmp(".", pDir->pEntry->d_name) == 0 ||
           strcmp("..", pDir->pEntry->d_name) == 0)
           continue;

        if (pFile != NULL && nSize > 0)
            xstrncpy(pFile, nSize, pDir->pEntry->d_name);

        return 1;
    }

    return 0;
}

int XDir_Valid(const char *pPath)
{
    struct stat statbuf = {0};
    int nStatus = stat(pPath, &statbuf);
    if (nStatus < 0) return nStatus;

    nStatus = S_ISDIR(statbuf.st_mode);
    if (!nStatus) errno = ENOTDIR;

    return nStatus;
}

int XDir_Make(char *pPath, mode_t mode)
{
    if ((XPath_Exists(pPath) == 0) &&
        (mkdir(pPath, mode) < 0) && 
        (errno != EEXIST)) 
        return 0;

    return 1;
}

int XDir_Create(const char *pDir, mode_t nMode)
{
    if (XPath_Exists(pDir)) return 1;
    char sDir[XFILE_BUF_SIZE];
    int nStatus = 0;

    int nLen = xstrncpyf(sDir, sizeof(sDir), "%s", pDir);
    if (nLen <= 0) return nStatus;

    if(sDir[nLen-1] == '/') sDir[nLen-1] = 0;
    char *pOffset = NULL;

    for (pOffset = sDir + 1; *pOffset; pOffset++)
    {
        if(*pOffset == '/')
        {
            *pOffset = 0;
            nStatus = XDir_Make(sDir, nMode);
            if (nStatus <= 0) return nStatus;
            *pOffset = '/';
        }
    }

    return XDir_Make(sDir, nMode);
}

int XDir_Unlink(const char *pPath)
{
    struct stat statbuf;
    if (!stat(pPath, &statbuf))
    {
        return (S_ISDIR(statbuf.st_mode)) ? 
            XDir_Remove(pPath) : 
            unlink(pPath);
    }

    return XFILE_INVALID;
}

int XDir_Remove(const char *pPath)
{
    size_t nLength = strlen(pPath);
    int nStatus = XFILE_INVALID;
    XDir dir;

    if (XDir_Open(&dir, pPath) > 0)
    {
        while (XDir_Read(&dir, NULL, 0) > 0)
        {
            size_t nSize = nLength + strlen(dir.pEntry->d_name) + 2;
            char sPath[nSize];

            xstrncpyf(sPath, nSize, "%s/%s", pPath, dir.pEntry->d_name);
            nStatus = XFILE_INVALID;
            struct stat statbuf;

            if (!stat(pPath, &statbuf))
            {
                nStatus = (S_ISDIR(statbuf.st_mode)) ?
                    XDir_Remove(pPath) : unlink(pPath);
            }
        }

        XDir_Close(&dir);
        rmdir(pPath);
    }

    return nStatus;
}
/*
 *  xutils/file.h
 * 
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * This source includes implementations of POSIX 
 * standart file and directory operations.
 */


#ifndef __XUTILS_XFILE_H__
#define __XUTILS_XFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define XFILE_INVALID -1
#define XFILE_UNSETRC 0
#define XFILE_SUCCESS 1

typedef struct {
    uint64_t nPosit;
    mode_t mPerms;
    int nFlags;
    int nFD;
} XFile;

typedef struct {
    struct dirent *pEntry;
    unsigned short nOpen;
    const char *pPath;
    DIR *pDirectory;
} XDir;

#define XFILE_CHECK_FL(c, f) (((c) & (f)) == (f))

int XFile_Open(XFile *pFile, const char *pPath, char *pFlags);
void XFile_Close(XFile *pFile);

int XFile_Seek(XFile *pFile, uint64_t nPosit, int nOffset);
int XFile_Write(XFile *pFile, void *pBuff, size_t nSize);
int XFile_Read(XFile *pFile, void *pBuff, size_t nSize);

size_t XFile_GetSize(XFile *pFile);
char* XFile_ReadBuffer(XFile *pFile);
int XFile_Copy(XFile *pIn, XFile *pOut);
int XFile_GetLine(XFile *pFile, char* pLine, size_t nSize);
int XFile_GetLines(XFile *pFile);
int XFile_ReadLine(XFile *pFile, char* pLine, size_t nSize, int nLineNumber);

int XPath_Exists(const char *pPath);
int XPath_SetPerm(const char *pPath, char mode[]);
int XPath_CopyFile(const char *pSrc, const char *pDst);

void XDir_Close(XDir *pDir);
int XDir_Open(XDir *pDir, const char *pPath);
int XDir_Read(XDir *pDir, char *pFile, size_t nSize);
int XDir_Create(const char *pDir, mode_t mode);
int XDir_Unlink(const char *pPath);
int XDir_Valid(const char *pPath);
int XDir_Remove(const char *pPath);

#ifdef __cplusplus
}
#endif


#endif /* __XUTILS_XFILE_H__ */
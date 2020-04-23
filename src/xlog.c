/*
 *  xutils/xlog.c
 *
 *  Copyleft (C) Sun Dro (a.k.a. kala13x)
 *
 * Advance logging library for C/C++
 */

#include "stdinc.h"
#include "xlog.h"

/* Max size of string */
#define XLOGGER_MAXMSG 8196
#define XLOGGER_BIGSTR 4098

static XLoggerConfig g_xlogCfg;
static XLoggerTag g_xloggerTags[] =
{
    { 0, "NONE", NULL },
    { 1, "LIVE", CLR_NORMAL },
    { 2, "INFO", CLR_GREEN },
    { 3, "WARN", CLR_YELLOW },
    { 4, "DEBUG", CLR_BLUE },
    { 5, "ERROR", CLR_RED},
    { 6, "FATAL", CLR_RED },
    { 7, "PANIC", CLR_WHITE }
};

void XLogger_Lock()
{
    if (pthread_mutex_lock(&g_xlogCfg.mutex))
    {
        printf("<%s:%d> %s: [ERROR] Can not lock mutex: %d\n", 
            __FILE__, __LINE__, __FUNCTION__, errno);

        exit(EXIT_FAILURE);
    }
}

void XLogger_Unlock()
{
    if (pthread_mutex_unlock(&g_xlogCfg.mutex))
    {
        printf("<%s:%d> %s: [ERROR] Can not unlock mutex: %d\n", 
            __FILE__, __LINE__, __FUNCTION__, errno);
                
        exit(EXIT_FAILURE);
    }
}

uint32_t XLoggerDate_Usec()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void XLoggerDate_ToHstr(const XLoggerDate *pDate, char *pStr) 
{
    sprintf(pStr, "%04d.%02d.%02d-%02d:%02d:%02d.%02d",
        pDate->nYear, pDate->nMonth, pDate->nDay, pDate->nHour, 
        pDate->nMin, pDate->nSec, pDate->nUsec);
}

void XLoggerDate_FromTm(XLoggerDate *pDate, const struct tm *pTm) 
{
    pDate->nYear = pTm->tm_year+1900;
    pDate->nMonth = pTm->tm_mon+1;
    pDate->nDay = pTm->tm_mday;
    pDate->nHour = pTm->tm_hour;
    pDate->nMin = pTm->tm_min;
    pDate->nSec = pTm->tm_sec;
    pDate->nUsec = 0;
}

void XLoggerDate_Get(XLoggerDate *pDate) 
{
    struct tm timeinfo;
    time_t rawtime = time(NULL);
    localtime_r(&rawtime, &timeinfo);
    XLoggerDate_FromTm(pDate, &timeinfo);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    pDate->nUsec = now.tv_nsec / 10000000;
}

const char* XLogger_Version(int nMin)
{
    static char sVersion[128];

    /* Version short */
    if (nMin) sprintf(sVersion, "%d.%d.%d", 
        XLOGGER_VERSION_MAJOR, XLOGGER_VERSION_MINOR, XLOGGER_BUILD_NUM);

    /* Version long */
    else sprintf(sVersion, "%d.%d build %d (%s)", 
        XLOGGER_VERSION_MAJOR, XLOGGER_VERSION_MINOR, XLOGGER_BUILD_NUM, __DATE__);

    return sVersion;
}

void XLogger_CreateOutput(const char* pStr, const XLoggerDate *pDate, int nType, int nColor, char* pOut, int nSize)
{
    char sDate[32];
    XLoggerDate_ToHstr(pDate, sDate);

    /* Walk throu */
    int i;
    for (i = 0;; i++)
    {
        if ((nType == XLOGGER_NONE) && !g_xloggerTags[i].nType && (g_xloggerTags[i].pDesc == NULL))
        {

            snprintf(pOut, nSize, "%s - %s", sDate, pStr);
            return;
        }

        if ((g_xloggerTags[i].nType == nType) && 
            (g_xloggerTags[i].pDesc != NULL) &&
            (nType != XLOGGER_NONE))
        {
            if (nColor)
            {
                snprintf(pOut, nSize, "%s - [%s%s%s] %s", 
                    sDate, g_xloggerTags[i].pColor, 
                    g_xloggerTags[i].pDesc, 
                    CLR_RESET, pStr);
            }
            else
            {
                snprintf(pOut, nSize, "%s - [%s] %s", 
                    sDate, g_xloggerTags[i].pDesc, pStr);
            }

            return;
        }
    }
}

void XLogger_ToFile(char *pStr, const char *pFile, XLoggerDate *pDate)
{
    if (!strlen(g_xlogCfg.sFilePath)) 
        strcpy(g_xlogCfg.sFilePath, ".");

    char sFileName[XLOGGER_FILE_MAX];
    if (g_xlogCfg.nFileStamp)
    {
        snprintf(sFileName, sizeof(sFileName), "%s/%s-%04d-%02d-%02d.log",
            g_xlogCfg.sFilePath, pFile, pDate->nYear, pDate->nMonth, pDate->nDay);
    }
    else snprintf(sFileName, sizeof(sFileName), "%s/%s.log", g_xlogCfg.sFilePath, pFile);

    FILE *fp = fopen(sFileName, "a");
    if (fp == NULL) return;

#ifndef DARWIN
    fprintf(fp, "(%ld) %s\n", syscall(__NR_gettid), pStr);
#else
    fprintf(fp, "%s\n", pStr);
#endif /* DARWIN */

    fclose(fp);
}

int XLogger_ParseConfig(const char *pConfig)
{
    if (pConfig == NULL) return 0;
    FILE *pFile = fopen(pConfig, "r");
    if(pFile == NULL) return 0;

    char sArg[256], sName[32];
    while(fscanf(pFile, "%s %[^\n]\n", sName, sArg) == 2)
    {
        if ((strlen(sName) > 0) && (sName[0] == '#'))
        {
            /* Skip comment */
            continue;
        }
        else if (strcmp(sName, "LOGLEVEL") == 0)
        {
            g_xlogCfg.nLogLevel = atoi(sArg);
        }
        else if (strcmp(sName, "LOGFILELEVEL") == 0)
        {
            g_xlogCfg.nFileLevel = atoi(sArg);
        }
        else if (strcmp(sName, "LOGTOFILE") == 0)
        {
            g_xlogCfg.nToFile = atoi(sArg);
        }
        else if (strcmp(sName, "ERRORLOG") == 0)
        {
            g_xlogCfg.nErrLog = atoi(sArg);
        }
        else if (strcmp(sName, "PRETTYLOG") == 0)
        {
            g_xlogCfg.nPretty = atoi(sArg);
        }
        else if (strcmp(sName, "FILESTAMP") == 0)
        {
            g_xlogCfg.nFileStamp = atoi(sArg);
        }
        else if (strcmp(sName, "LOGPATH") == 0)
        {
            strcpy(g_xlogCfg.sFilePath, sArg);
        }
    }

    fclose(pFile);
    return 1;
}

void XLogger_Log(int nLevel, int nFlag, const char *pMsg, ...)
{
    if (g_xlogCfg.nSilent && 
        (nFlag == XLOGGER_DEBUG || 
        nFlag == XLOGGER_LIVE)) return;

    if (g_xlogCfg.nTdSafe) XLogger_Lock();

    /* Check logging levels */
    if(!nLevel || nLevel <= g_xlogCfg.nLogLevel || nLevel <= g_xlogCfg.nFileLevel)
    {
        char sInput[XLOGGER_MAXMSG];
        va_list args;

        va_start(args, pMsg);
        vsnprintf(sInput, sizeof(sInput), pMsg, args);
        va_end(args);

        XLoggerDate date;
        XLoggerDate_Get(&date);

        char sMessage[XLOGGER_MAXMSG];
        XLogger_CreateOutput(sInput, &date, nFlag, 1, sMessage, sizeof(sMessage));

        if (nLevel <= g_xlogCfg.nLogLevel)
        {
#ifndef DARWIN
            printf("(%ld) %s\n", syscall(__NR_gettid), sMessage);
#else
            printf("%s\n", sMessage);
#endif /* DARWIN */
        }

        /* Save log in the file */
        if ((g_xlogCfg.nToFile && nLevel <= g_xlogCfg.nFileLevel) || 
            (g_xlogCfg.nErrLog && (nFlag == (XLOGGER_ERROR | XLOGGER_PANIC | XLOGGER_FATAL))))
        {
            if (g_xlogCfg.nPretty)
                XLogger_CreateOutput(sInput, &date, nFlag, 0, sMessage, sizeof(sMessage));

            XLogger_ToFile(sMessage, g_xlogCfg.sFileName, &date);
        }
    }

    if (g_xlogCfg.nTdSafe) XLogger_Unlock();
}

void XLogger_Tag(int nLevel, char *pTag, const char *pMsg, ...)
{
    if (g_xlogCfg.nTdSafe) XLogger_Lock();
    char sInput[XLOGGER_MAXMSG];
    va_list args;

    va_start(args, pMsg);
    vsnprintf(sInput, sizeof(sInput), pMsg, args);
    va_end(args);

    if (g_xlogCfg.nTdSafe) XLogger_Unlock();
    XLogger_Log(nLevel, XLOGGER_NONE, "[%s] %s", pTag, sInput);
}

void XLogger_ConfigGet(XLoggerConfig *pCfg)
{
    if (g_xlogCfg.nTdSafe) XLogger_Lock();
    memset(pCfg->sFilePath, 0, sizeof(pCfg->sFilePath));
    memset(pCfg->sFileName, 0, sizeof(pCfg->sFileName));
    strcpy(pCfg->sFilePath, g_xlogCfg.sFilePath);
    strcpy(pCfg->sFileName, g_xlogCfg.sFileName);

    pCfg->nFileStamp = g_xlogCfg.nFileStamp;
    pCfg->nFileLevel = g_xlogCfg.nFileLevel;
    pCfg->nLogLevel = g_xlogCfg.nLogLevel;
    pCfg->nToFile = g_xlogCfg.nToFile;
    pCfg->nPretty = g_xlogCfg.nPretty;
    pCfg->nTdSafe = g_xlogCfg.nTdSafe;
    pCfg->nErrLog = g_xlogCfg.nErrLog;
    pCfg->nSilent = g_xlogCfg.nSilent;
    if (g_xlogCfg.nTdSafe) XLogger_Unlock();
}

void XLogger_ConfigSet(XLoggerConfig *pCfg)
{
    if (g_xlogCfg.nTdSafe) XLogger_Lock();
    memset(g_xlogCfg.sFilePath, 0, sizeof(g_xlogCfg.sFilePath));
    memset(g_xlogCfg.sFileName, 0, sizeof(g_xlogCfg.sFileName));
    strcpy(g_xlogCfg.sFilePath, pCfg->sFilePath);
    strcpy(g_xlogCfg.sFileName, pCfg->sFileName);

    g_xlogCfg.nFileStamp = pCfg->nFileStamp;
    g_xlogCfg.nFileLevel = pCfg->nFileLevel;
    g_xlogCfg.nLogLevel = pCfg->nLogLevel;
    g_xlogCfg.nToFile = pCfg->nToFile;
    g_xlogCfg.nPretty = pCfg->nPretty;
    g_xlogCfg.nErrLog = pCfg->nErrLog;
    g_xlogCfg.nSilent = pCfg->nSilent;
    if (g_xlogCfg.nTdSafe) XLogger_Unlock();
}

void XLogger_Init(const char* pName, const char* pConf, int nLogLevel, int nTdSafe)
{
    /* Set up default values */
    memset(g_xlogCfg.sFilePath, 0, sizeof(g_xlogCfg.sFilePath));
    memset(g_xlogCfg.sFileName, 0, sizeof(g_xlogCfg.sFileName));
    strcpy(g_xlogCfg.sFileName, pName);

    g_xlogCfg.nLogLevel = nLogLevel;
    g_xlogCfg.nFileStamp = 1;
    g_xlogCfg.nFileLevel = 0;
    g_xlogCfg.nTdSafe = 0;
    g_xlogCfg.nErrLog = 0;
    g_xlogCfg.nSilent = 0;
    g_xlogCfg.nToFile = 0;
    g_xlogCfg.nPretty = 0;

    /* Init mutex sync */
    if (nTdSafe)
    {
        /* Init mutex attribute */
        if (pthread_mutexattr_init(&g_xlogCfg.mutexAttr) ||
            pthread_mutexattr_settype(&g_xlogCfg.mutexAttr, PTHREAD_MUTEX_RECURSIVE) ||
            pthread_mutex_init(&g_xlogCfg.mutex, &g_xlogCfg.mutexAttr) ||
            pthread_mutexattr_destroy(&g_xlogCfg.mutexAttr))
        {
            printf("<%s:%d> %s: [ERROR] Can not initialize mutex: %d\n", 
                __FILE__, __LINE__, __FUNCTION__, errno);

            exit(EXIT_FAILURE);
        }
    }

    /* Parse config file */
    if (pConf != NULL)
        XLogger_ParseConfig(pConf);
}

void XLogger_Destroy()
{
    if (g_xlogCfg.nTdSafe && pthread_mutex_destroy(&g_xlogCfg.mutex))
    {
        printf("<%s:%d> %s: [ERROR] Can not deinitialize mutex: %d\n", 
            __FILE__, __LINE__, __FUNCTION__, errno);

        exit(EXIT_FAILURE);
    }

    g_xlogCfg.nTdSafe = 0;
}
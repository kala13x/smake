/*
 * The MIT License (MIT)
 *  
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include "slog.h"

/* Max size of string */
#define MAXMSG 8196

/* Flags */
static SlogFlags slg;
static pthread_mutex_t slog_mutex;

void slog_get_date(SlogDate *pDate)
{
    time_t rawtime;
    struct tm timeinfo;
    struct timespec now;
    rawtime = time(NULL);
    localtime_r(&rawtime, &timeinfo);

    /* Get System Date */
    pDate->year = timeinfo.tm_year+1900;
    pDate->mon = timeinfo.tm_mon+1;
    pDate->day = timeinfo.tm_mday;
    pDate->hour = timeinfo.tm_hour;
    pDate->min = timeinfo.tm_min;
    pDate->sec = timeinfo.tm_sec;

    /* Get micro seconds */
    clock_gettime(CLOCK_MONOTONIC, &now);
    pDate->usec = now.tv_nsec / 10000000;
}

const char* slog_version(int nMin)
{
    static char sVersion[128];

    /* Version short */
    if (nMin) sprintf(sVersion, "%d.%d.%d", 
        SLOGVERSION_MAX, SLOGVERSION_MIN, SLOGBUILD_NUM);

    /* Version long */
    else sprintf(sVersion, "%d.%d build %d (%s)", 
        SLOGVERSION_MAX, SLOGVERSION_MIN, SLOGBUILD_NUM, __DATE__);

    return sVersion;
}

char* slog_clr(const char* pColor, char* pStr, ...) 
{
    static char sOutput[MAXMSG];
    char sVaString[MAXMSG];

    /* Read args */
    va_list args;
    va_start(args, pStr);
    vsprintf(sVaString, pStr, args);
    va_end(args);

    sprintf(sOutput, "%s%s%s", pColor, sVaString, CLR_RESET);
    return sOutput;
}

void slog_to_file(char *out, const char *fname, SlogDate *pDate)
{
    char sFileName[PATH_MAX];
    if (slg.nFileStamp)
    {
        snprintf(sFileName, sizeof(sFileName), "%s-%02d-%02d-%02d.log",
            fname, pDate->year, pDate->mon, pDate->day);
    }
    else snprintf(sFileName, sizeof(sFileName), "%s.log", fname);

    FILE *fp = fopen(sFileName, "a");
    if (fp == NULL) return;

    fprintf(fp, "%s", out);
    fclose(fp);
}

int slog_parse_config(const char *pConfig)
{
    FILE *pFile = fopen(pConfig, "r");
    if(pFile == NULL) return 0;

    ssize_t nRead;
    size_t nLen = 0;
    int nRetVal = 0;
    char *pLine = NULL;

    while ((nRead = getline(&pLine, &nLen, pFile)) != -1)
    {
        if(strstr(pLine, "LOGLEVEL") != NULL)
        {
            slg.nLogLevel = atoi(pLine+8);
            nRetVal = 1;
        }
        if(strstr(pLine, "LOGFILELEVEL") != NULL)
        {
            slg.nFileLevel = atoi(pLine+12);
            nRetVal = 1;
        }
        else if(strstr(pLine, "LOGTOFILE") != NULL)
        {
            slg.nToFile = atoi(pLine+9);
            nRetVal = 1;
        }
        else if(strstr(pLine, "PRETTYLOG") != NULL)
        {
            slg.nPretty = atoi(pLine+9);
            nRetVal = 1;
        }
        else if(strstr(pLine, "FILESTAMP") != NULL)
        {
            slg.nFileStamp = atoi(pLine+9);
            nRetVal = 1;
        }
    }

    /* Cleanup */
    if (pLine) free(pLine);
    fclose(pFile);

    return nRetVal;
}

char* slog_get(SlogDate *pDate, char *pMsg, ...) 
{
    static char sOutput[MAXMSG];
    char sVaString[MAXMSG];

    /* Read args */
    va_list args;
    va_start(args, pMsg);
    vsprintf(sVaString, pMsg, args);
    va_end(args);

    /* Generate output string with date */
    sprintf(sOutput, "%02d.%02d.%02d-%02d:%02d:%02d.%02d - %s", 
        pDate->year, pDate->mon, pDate->day, pDate->hour, 
        pDate->min, pDate->sec, pDate->usec, sVaString);

    /* Return output */
    return sOutput;
}

void slog(int nLevel, int nFlag, const char *pMsg, ...)
{
    /* Lock for safe */
    if (slg.nTdSafe) 
    {
        if (pthread_mutex_lock(&slog_mutex))
        {
            printf("<%s:%d> %s: [ERROR] Can not lock mutex: %d\n", 
                __FILE__, __LINE__, __FUNCTION__, errno);
            exit(EXIT_FAILURE);
        }
    }

    /* Used variables */
    SlogDate date;
    char string[MAXMSG];
    char prints[MAXMSG];
    char color[32], alarm[32];
    char *output;

    slog_get_date(&date);
    bzero(string, sizeof(string));
    bzero(prints, sizeof(prints));
    bzero(color, sizeof(color));
    bzero(alarm, sizeof(alarm));

    /* Read args */
    va_list args;
    va_start(args, pMsg);
    vsprintf(string, pMsg, args);
    va_end(args);

    /* Check logging levels */
    if(!nLevel || nLevel <= slg.nLogLevel || nLevel <= slg.nFileLevel)
    {
        /* Handle flags */
        switch(nFlag) 
        {
            case SLOG_LIVE:
                strncpy(color, CLR_NORMAL, sizeof(color));
                strncpy(alarm, "LIVE", sizeof(alarm));
                break;
            case SLOG_INFO:
                strncpy(color, CLR_GREEN, sizeof(color));
                strncpy(alarm, "INFO", sizeof(alarm));
                break;
            case SLOG_WARN:
                strncpy(color, CLR_YELLOW, sizeof(color));
                strncpy(alarm, "WARN", sizeof(alarm));
                break;
            case SLOG_DEBUG:
                strncpy(color, CLR_BLUE, sizeof(color));
                strncpy(alarm, "DEBUG", sizeof(alarm));
                break;
            case SLOG_ERROR:
                strncpy(color, CLR_RED, sizeof(color));
                strncpy(alarm, "ERROR", sizeof(alarm));
                break;
            case SLOG_FATAL:
                strncpy(color, CLR_RED, sizeof(color));
                strncpy(alarm, "FATAL", sizeof(alarm));
                break;
            case SLOG_PANIC:
                strncpy(color, CLR_WHITE, sizeof(color));
                strncpy(alarm, "PANIC", sizeof(alarm));
                break;
            case SLOG_NONE:
                strncpy(prints, string, sizeof(string));
                break;
            default:
                strncpy(prints, string, sizeof(string));
                nFlag = SLOG_NONE;
                break;
        }

        /* Print output */
        if (nLevel <= slg.nLogLevel || slg.nPretty)
        {
            if (nFlag != SLOG_NONE) sprintf(prints, "[%s] %s", slog_clr(color, alarm), string);
            if (nLevel <= slg.nLogLevel) printf("%s", slog_get(&date, "%s\n", prints));
        }

        /* Save log in file */
        if (slg.nToFile && nLevel <= slg.nFileLevel)
        {
            if (slg.nPretty) output = slog_get(&date, "%s\n", prints);
            else 
            {
                if (nFlag != SLOG_NONE) sprintf(prints, "[%s] %s", alarm, string);
                output = slog_get(&date, "%s\n", prints);
            } 

            /* Add log line to file */
            slog_to_file(output, slg.pFileName, &date);
        }
    }

    /* Done, unlock mutex */
    if (slg.nTdSafe) 
    {
        if (pthread_mutex_unlock(&slog_mutex)) 
        {
            printf("<%s:%d> %s: [ERROR] Can not deinitialize mutex: %d\n", 
                __FILE__, __LINE__, __FUNCTION__, errno);
            exit(EXIT_FAILURE);
        }
    }
}

void slog_init(const char* pName, const char* pConf, int nLevel, int nFileLevel, int nTdSafe)
{
    /* Set up default values */
    slg.nLogLevel = nLevel;
    slg.nFileLevel = nFileLevel;
    slg.nTdSafe = nTdSafe;
    slg.nFileStamp = 1;
    slg.nToFile = 0;
    slg.nPretty = 0;

    /* Init mutex sync */
    if (nTdSafe)
    {
        /* Init mutex attribute */
        pthread_mutexattr_t m_attr;
        if (pthread_mutexattr_init(&m_attr) ||
            pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE) ||
            pthread_mutex_init(&slog_mutex, &m_attr) ||
            pthread_mutexattr_destroy(&m_attr))
        {
            printf("<%s:%d> %s: [ERROR] Can not initialize mutex: %d\n", 
                __FILE__, __LINE__, __FUNCTION__, errno);
            slg.nTdSafe = 0;
        }
    }

    /* Parse config file */
    if (pConf != NULL) 
    {
        slg.pFileName = pName;
        slog_parse_config(pConf);
    }
}

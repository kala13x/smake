/*
 *  libxutils/src/array.c
 *
 *  This source is part of "libxutils" project
 *  2015-2020  Sun Dro (f4tb0y@protonmail.com)
 * 
 * Dynamically allocated data holder with 
 * performance sorting and search features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"

XArrayData *XArray_NewData(void *pData, size_t nSize, uint64_t nKey)
{
    XArrayData *pNewData = (XArrayData*)malloc(sizeof(XArrayData));
    if (pNewData == NULL) return NULL;

    if (pData != NULL && nSize > 0) 
    {
        pNewData->pData = malloc(nSize);
        if (pNewData->pData == NULL)
        {
            free(pNewData);
            return NULL;
        }

        memcpy(pNewData->pData, pData, nSize);
        pNewData->nSize = nSize;
    }
    else
    {
        pNewData->pData = pData;
        pNewData->nSize = 0;
    }

    pNewData->nKey = nKey;
    return pNewData;
}

void XArray_FreeData(XArrayData *pArrData)
{
    if (pArrData != NULL)
    {
        if (pArrData->pData && 
            pArrData->nSize > 0) 
            free(pArrData->pData);

        free(pArrData);
    }
}

void* XArray_Init(XArray *pArr, size_t nSize, uint8_t nFixed)
{
    pArr->pData = (void**)malloc(sizeof(void*) * nSize);
    if (pArr->pData == NULL) return NULL;

    unsigned int i;
    for (i = 0; i < nSize; i++) 
        pArr->pData[i] = NULL;

    pArr->eStatus = XARRAY_STATUS_EMPTY;
    pArr->clearCb = NULL;
    pArr->nLength = nSize;
    pArr->nFixed = nFixed;
    pArr->nUsed = 0;

    return pArr->pData;
}

void XArray_Clear(XArray *pArr)
{
    unsigned int i;
    if (pArr->pData != NULL)
    {
        for (i = 0; i < pArr->nLength; i++) 
        {
            if (pArr->clearCb) pArr->clearCb(pArr->pData[i]);
            else XArray_FreeData(pArr->pData[i]);
            pArr->pData[i] = NULL;
        }
    }

    pArr->eStatus = XARRAY_STATUS_EMPTY;
    pArr->nUsed = 0;
}

void XArray_Destroy(XArray *pArr)
{
    XArray_Clear(pArr);
    if (pArr->pData != NULL)
    {
        free(pArr->pData);
        pArr->pData = NULL;
    }

    pArr->nLength = 0;
    pArr->nFixed = 0;
    pArr->nUsed = 0;
}

int XArray_CheckSpace(XArray *pArr)
{
    pArr->eStatus = (pArr->nUsed >= pArr->nLength)?
            XARRAY_STATUS_FULL : XARRAY_STATUS_OK;
    return !(pArr->eStatus == XARRAY_STATUS_FULL);
}

size_t XArray_Realloc(XArray *pArr)
{
    if (pArr->nFixed) return pArr->nLength;
    void *pData = NULL;
    size_t nLength = 0;

    if(pArr->nUsed == pArr->nLength)
    {
        nLength = pArr->nLength * 2;
        pData = realloc(pArr->pData, sizeof(void*) * nLength);
    }
    else if (pArr->nUsed > 0 && ((float)pArr->nUsed / (float)pArr->nLength) < 0.25)
    {
        nLength = pArr->nLength / 2;
        pData = realloc(pArr->pData, sizeof(void*) * nLength);
    }

    if (pData != NULL)
    {
        pArr->pData = pData;
        pArr->nLength = nLength;

        unsigned int i;
        for (i = pArr->nUsed; i < nLength; i++) 
            pArr->pData[i] = NULL;

        return nLength;
    }

    return pArr->nLength;
}

int XArray_Add(XArray *pArr, XArrayData *pNewData)
{
    if (!XArray_CheckSpace(pArr))
    {
        XArray_FreeData(pNewData);
        return -1;
    }

    if (pArr->pData != NULL)
    {
        pArr->pData[pArr->nUsed] = pNewData;
        pArr->nUsed++;
    }
    else
    {
        pArr->eStatus = XARRAY_STATUS_INVALID;
        XArray_FreeData(pNewData);
        return -2;
    }

    int nVal = XArray_Realloc(pArr);
    return nVal ? pArr->nUsed : -3;
}

int XArray_AddData(XArray *pArr, void *pData, size_t nSize)
{
    XArrayData *pNewData = XArray_NewData(pData, nSize, 0);
    if (pNewData == NULL) 
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return -2;
    }

    int nCurrent = XArray_Add(pArr, pNewData);
    return nCurrent;
}

int XArray_AddDataKey(XArray *pArr, void *pData, size_t nSize, uint64_t nKey)
{
    XArrayData *pNewData = XArray_NewData(pData, nSize, nKey);
    if (pNewData == NULL)
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return -2;
    }

    int nCurrent = XArray_Add(pArr, pNewData);
    return nCurrent;
}

XArrayData* XArray_Get(XArray *pArr, int nIndex)
{
    if (nIndex >= pArr->nLength || nIndex < 0) return NULL;
    return pArr->pData[nIndex];
}

void* XArray_GetData(XArray *pArr, int nIndex)
{
    if (nIndex >= pArr->nLength || nIndex < 0) return NULL;
    XArrayData *pArrData = pArr->pData[nIndex];
    return pArrData ? pArrData->pData : NULL;
}

size_t XArray_GetSize(XArray *pArr, int nIndex)
{
    if (nIndex >= pArr->nLength || nIndex < 0) return 0;
    XArrayData *pArrData = pArr->pData[nIndex];
    return pArrData ? pArrData->nSize : 0;
}

uint64_t XArray_GetKey(XArray *pArr, int nIndex)
{
    if (nIndex >= pArr->nLength || nIndex < 0) return 0;
    XArrayData *pArrData = pArr->pData[nIndex];
    return pArrData ? pArrData->nKey : 0;
}

XArrayData* XArray_Remove(XArray *pArr, int nIndex)
{
    XArrayData *pData = XArray_Get(pArr, nIndex);
    if (pData == NULL) return NULL;

    unsigned int i;
    for (i = nIndex; i < pArr->nUsed; i++)
    {
        if ((i + 1) >= pArr->nUsed) break;
        pArr->pData[i] = pArr->pData[i+1];
    }

    pArr->pData[--pArr->nUsed] = NULL;
    XArray_Realloc(pArr);
    return pData;
}

void XArray_Delete(XArray *pArr, int nIndex)
{
    XArrayData *pData = XArray_Get(pArr, nIndex);
    if (pData != NULL) XArray_FreeData(pData);
}

XArrayData* XArray_Set(XArray *pArr, int nIndex, XArrayData *pNewData)
{
    XArrayData *pOldData = NULL;
    if (nIndex < pArr->nLength || nIndex > 0) 
    {
        pOldData = pArr->pData[nIndex];
        pArr->pData[nIndex] = pNewData;
    }

    return pOldData;
}

XArrayData* XArray_SetData(XArray *pArr, int nIndex, void *pData, size_t nSize)
{
    XArrayData *pNewData = XArray_NewData(pData, nSize, 0);
    if (pNewData == NULL)
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return NULL;
    }

    XArrayData *pOldData = XArray_Set(pArr, nIndex, pNewData);
    return pOldData;
}

XArrayData* XArray_Insert(XArray *pArr, int nIndex,  XArrayData *pData)
{
    if (!XArray_CheckSpace(pArr)) return NULL;

    void *pOldData = XArray_Set(pArr, nIndex, pData);
    if (pOldData == NULL) return NULL;

    unsigned int i, nNextIndex;
    nNextIndex = nIndex + 1;

    for (i = nNextIndex; i < pArr->nUsed; i++)
        pOldData = XArray_Set(pArr, i, pOldData);
    
    XArray_Add(pArr, pOldData);
    return pArr->pData[nNextIndex];
}

XArrayData* XArray_InsertData(XArray *pArr, int nIndex, void *pData, size_t nSize)
{
    if (!XArray_CheckSpace(pArr)) return NULL;

    XArrayData *pNewData = XArray_NewData(pData, nSize, 0);
    if (pNewData == NULL)
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return NULL;
    }

    void *pNextData = XArray_Insert(pArr, nIndex, pNewData);
    return pNextData;
}


void XArray_Swap(XArray *pArr, int nIndex1, int nIndex2)
{
    if ((nIndex1 >= pArr->nUsed || nIndex1 < 0) || 
        (nIndex2 >= pArr->nUsed || nIndex2 < 0)) return;

    XArrayData *pData1 = pArr->pData[nIndex1];
    pArr->pData[nIndex1] = pArr->pData[nIndex2];
    pArr->pData[nIndex2] = pData1;
}

int XArray_CompareSize(const void *pData1, const void *pData2)
{
    XArrayData *pFirst = (XArrayData*)pData1;
    XArrayData *pSecond = (XArrayData*)pData2;
    return pFirst->nSize - pSecond->nSize;
}

int XArray_CompareKey(const void *pData1, const void *pData2)
{
    XArrayData *pFirst = (XArrayData*)pData1;
    XArrayData *pSecond = (XArrayData*)pData2;
    return pFirst->nKey - pSecond->nKey;
}

int XArray_Partitioning(XArray *pArr, int(*compare)(const void*, const void*), int nStart, int nFinish)
{
    int nPivot = nStart;
    while(1)
    {
        while(compare((void*)pArr->pData[nStart], (void*)pArr->pData[nPivot]) < 0) nStart++;
        while(compare((void*)pArr->pData[nFinish], (void*)pArr->pData[nPivot]) > 0) nFinish--;
        if(!compare((void*)pArr->pData[nStart], (void*)pArr->pData[nFinish])) nFinish--;
        if(nStart >= nFinish) return nStart;
        XArray_Swap(pArr, nStart, nFinish);
    }

    return nStart;
}

void XArray_QuickSort(XArray *pArr, int(*compare)(const void*, const void*), int nStart, int nFinish)
{
    if(nStart < nFinish)
    {
        int nPartitioning = XArray_Partitioning(pArr, compare, nStart, nFinish);
        XArray_QuickSort(pArr, compare, nStart, nPartitioning);
        XArray_QuickSort(pArr, compare, nPartitioning+1, nFinish);
    }
}

void XArray_Sort(XArray *pArr, int(*compare)(const void*, const void*))
{
    if (pArr == NULL || pArr->nUsed <= 0) return;
    XArray_QuickSort(pArr, compare, 0, pArr->nUsed-1);
}

void XArray_SortBy(XArray *pArr, int nSortBy)
{
    if (nSortBy == XARRAY_SORTBY_SIZE) 
        XArray_Sort(pArr, XArray_CompareSize);
    else if (nSortBy == XARRAY_SORTBY_KEY)
        XArray_Sort(pArr, XArray_CompareKey);
}

void XArray_BubbleSort(XArray *pArr, int(*compare)(const void*, const void*))
{
    if (pArr == NULL || pArr->nUsed <= 0) return;
    long i, j;

    for (i = 0 ; i < pArr->nUsed-1; i++) 
    {
        for (j = 0 ; j < pArr->nUsed-i-1; j++) 
        {
            if (compare((void*)pArr->pData[j], (void*)pArr->pData[j+1]))
            {
                XArrayData *pData = pArr->pData[j];
                pArr->pData[j] = pArr->pData[j+1];
                pArr->pData[j+1] = pData;
            }
        }
    }
}

int XArray_LinearSearch(XArray *pArr, uint64_t nKey)
{
    if (pArr == NULL || pArr->nUsed <= 0) return -1;
    int i = 0;

    for (i = 0; i < pArr->nUsed; i++)
    {
        XArrayData *pData = pArr->pData[i];
        if (nKey == pData->nKey) return i;
    }

    return -1;
}

int XArray_SentinelSearch(XArray *pArr, uint64_t nKey)
{
    if (pArr == NULL || pArr->nUsed <= 0) return -1;
    int i, nLast = pArr->nUsed - 1;

    XArrayData *pLast = pArr->pData[nLast];
    if (pLast->nKey == nKey) return nLast;

    XArrayData term = {.nKey = nKey, .nSize = 0, .pData = NULL};
    pArr->pData[nLast] = &term;

    for (i = 0;; i++)
    {
        XArrayData *pData = pArr->pData[i];
        if (nKey == pData->nKey)
        {
            pArr->pData[nLast] = pLast;
            return (i < nLast) ? i : -1;
        }
    }

    return -1;
}

int XArray_DoubleSearch(XArray *pArr, uint64_t nKey)
{
    if (pArr == NULL || pArr->nUsed <= 0) return -1;
    int nFront = 0, nBack = pArr->nUsed - 1;

    while (nFront <= nBack)
    {
        XArrayData *pData = pArr->pData[nFront];
        if (nKey == pData->nKey) return nFront;

        pData = pArr->pData[nBack];
        if (nKey == pData->nKey) return nBack;
    
        nFront++;
        nBack--;
    }

    return -1;
}

int XArray_BinarySearch(XArray *pArr, uint64_t nKey)
{
    if (pArr == NULL || pArr->nUsed <= 0) return -1;
    int nLeft = 0, nRight = pArr->nUsed - 1;

    XArrayData *pData = pArr->pData[nLeft];
    if (pData->nKey == nKey) return nLeft;

    while (nLeft <= nRight)
    {
        int nMiddle = nLeft + (nRight - nLeft) / 2;
        pData = pArr->pData[nMiddle];
        if (pData->nKey == nKey) return nMiddle;
        if (pData->nKey < nKey) nLeft = nMiddle + 1;
        else nRight = nMiddle - 1; 
    }
  
    return -1;
}

size_t XArray_GetUsedSize(XArray *pArr)
{
    if (pArr == NULL) return 0;
    return pArr->nUsed;
}

size_t XArray_GetArraySize(XArray *pArr)
{
    if (pArr == NULL) return 0;
    return pArr->nLength;
}

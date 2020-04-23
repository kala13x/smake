/*
 *  xutils/array.c
 *
 *  Copyleft (C) 2019 Sun Dro (a.k.a. kala13x)
 *
 * Dynamically allocated array with performance sorting features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"

XArrayData *XArray_NewData(void *pData, size_t nSize, uint64_t nKey)
{
    XArrayData *pNewData = (XArrayData*)malloc(sizeof(XArrayData));
    if (pNewData == NULL) return NULL;

    if (nSize > 0 && pData != NULL) 
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
        if (pArrData->pData && pArrData->nSize)
        {
            free(pArrData->pData);
            pArrData->pData = NULL;
            pArrData->nSize = 0;
        }

        free(pArrData);
        pArrData = NULL;
    }
}

void* XArray_Init(XArray *pArr, int nSize, int nFixed)
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
    pArr->eStatus = (pArr->nUsed >= pArr->nLength) ?
            XARRAY_STATUS_FULL : XARRAY_STATUS_OK;

    return pArr->eStatus == XARRAY_STATUS_FULL ? 0 : 1;
}

int XArray_Realloc(XArray *pArr)
{
    if (pArr->nFixed) return 0;
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
    else return pArr->nLength;

    if (pData != NULL)
    {
        pArr->pData = pData;
        pArr->nLength = nLength;

        unsigned int i;
        for (i = pArr->nUsed; i < nLength; i++) 
            pArr->pData[i] = NULL;

        return nLength;
    }

    return 0;
}

int XArray_Add(XArray *pArr, XArrayData *pNewData)
{
    if (!XArray_CheckSpace(pArr)) return 0;

    if (pArr->pData && pNewData)
    {
        pArr->pData[pArr->nUsed] = pNewData;
        pArr->nUsed++;
    }
    else
    {
        pArr->eStatus = XARRAY_STATUS_INVALID;
        XArray_FreeData(pNewData);
        return 0;
    }

    int nVal = XArray_Realloc(pArr);
    return nVal ? pArr->nUsed : 0;
}

int XArray_AddData(XArray *pArr, void *pData, size_t nSize)
{
    if (!XArray_CheckSpace(pArr)) return 0;

    XArrayData *pNewData = XArray_NewData(pData, nSize, 0);
    if (pNewData == NULL) 
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return 0;
    }

    int nCurrent = XArray_Add(pArr, pNewData);
    return nCurrent;
}

int XArray_AddDataKey(XArray *pArr, void *pData, size_t nSize, uint64_t nKey)
{
    if (!XArray_CheckSpace(pArr)) return 0;

    XArrayData *pNewData = XArray_NewData(pData, nSize, nKey);
    if (pNewData == NULL)
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return 0;
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

void XArray_Remove(XArray *pArr, int nIndex)
{
    XArrayData *pData = XArray_Get(pArr, nIndex);
    if (pData != NULL)
    {
        XArray_FreeData(pData);
        unsigned int i;

        for (i = nIndex; i < pArr->nUsed; i++)
        {
            if ((i + 1) < pArr->nUsed)
            {
                pData = XArray_Get(pArr, i + 1);
                XArray_Set(pArr, i, pData);
            }
        }

        XArray_Set(pArr, pArr->nUsed-1, NULL);
        pArr->nUsed--;
    }

    XArray_Realloc(pArr);
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
    if (!XArray_CheckSpace(pArr)) return 0;
    
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
    if (!XArray_CheckSpace(pArr)) return 0;

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
    XArray_QuickSort(pArr, compare, 0, pArr->nUsed-1);
}

void XArray_SortBySize(XArray *pArr)
{
    XArray_Sort(pArr, XArray_CompareSize);
}

void XArray_SortByKey(XArray *pArr)
{
    XArray_Sort(pArr, XArray_CompareKey);
}

int XArray_Search(XArray *pArr, uint64_t nKey)
{
    if (nKey < 0 || !pArr || !pArr->nUsed) return -1;
    int i, nUsedSize = XArray_GetUsedSize(pArr);

    for (i = 0; i < nUsedSize; i++)
    {
        int nArrKey = XArray_GetKey(pArr, i);
        if (nKey == nArrKey) return i;
    }

    return -1;
}

int XArray_BinarySearch(XArray *pArr, uint64_t nKey)
{
    if (nKey < 0 || !pArr || !pArr->nUsed) return -1;
    int nRight = pArr->nUsed - 1;
    int nLeft = 0;

    int nArrKey = XArray_GetKey(pArr, nLeft);
    if (nKey == nArrKey) return nLeft;

    while (nLeft <= nRight)
    {
        int nMiddle = nLeft + (nRight - nLeft) / 2;
        int nArrKey = XArray_GetKey(pArr, nMiddle);
        if (nKey ==nArrKey) return nMiddle;
        if (nArrKey < nKey) nLeft = nMiddle + 1;
        else nRight = nMiddle - 1; 
    }
  
    return -1;
}

size_t XArray_GetUsedSize(XArray *pArr)
{
    if (!pArr) return 0;
    return pArr->nUsed;
}

size_t XArray_GetArraySize(XArray *pArr)
{
    if (!pArr) return 0;
    return pArr->nLength;
}

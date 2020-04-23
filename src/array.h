/*
 *  xutils/array.h
 *
 *  Copyleft (C) Sun Dro (a.k.a. kala13x)
 *
 * Dynamically allocated array with performance sorting features
 */

#ifndef __XUTILS_XARRAY_H__
#define __XUTILS_XARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef void(*XArrayClearCb)(void *pArrData);

typedef enum {
    XARRAY_STATUS_OK = 0,
    XARRAY_STATUS_FULL = 1,
    XARRAY_STATUS_EMPTY = 2,
    XARRAY_STATUS_INVALID = 3,
    XARRAY_STATUS_NO_MEMORY = 4
} XARRAY_STATUS_E;

typedef struct {
    uint64_t nKey;
    size_t nSize;
    void* pData;
} XArrayData;

typedef struct {
    XArrayClearCb clearCb;
    XARRAY_STATUS_E eStatus;
    unsigned int nFixed:1;
    size_t nLength;
    size_t nUsed;
    void **pData;
} XArray;

XArrayData *XArray_NewData(void *pData, size_t nSize, uint64_t nKey);
void XArray_FreeData(XArrayData *pArrData);

void* XArray_Init(XArray *pArr, int nSize, int nFixed);
void XArray_Destroy(XArray *pArr);
void XArray_Clear(XArray *pArr);
int XArray_Realloc(XArray *pArr);

int XArray_Add(XArray *pArr, XArrayData *pNewData);
int XArray_AddData(XArray *pArr, void* pData, size_t nSize);
int XArray_AddDataKey(XArray *pArr, void* pData, size_t nSize, uint64_t nKey);
void* XArray_GetData(XArray *pArr, int nIndex);
size_t XArray_GetSize(XArray *pArr, int nIndex);
uint64_t XArray_GetKey(XArray *pArr, int nIndex);

void XArray_Remove(XArray *pArr, int nIndex);
void XArray_Delete(XArray *pArr, int nIndex);
void XArray_Swap(XArray *pArr, int nIndex1, int nIndex2);

void XArray_Sort(XArray *pArr, int(*compare)(const void*, const void*));
void XArray_QuickSort(XArray *pArr, int(*compare)(const void*, const void*), int nStart, int nFinish);
void XArray_SortBySize(XArray *pArr);
void XArray_SortByKey(XArray *pArr);

int XArray_Search(XArray *pArr, uint64_t nKey);
int XArray_BinarySearch(XArray *pArr, uint64_t nKey);

XArrayData* XArray_Get(XArray *pArr, int nIndex);
XArrayData* XArray_Set(XArray *pArr, int nIndex, XArrayData *pNewData);
XArrayData* XArray_SetData(XArray *pArr, int nIndex, void *pData, size_t nSize);
XArrayData* XArray_Insert(XArray *pArr, int nIndex,  XArrayData *pData);
XArrayData* XArray_InsertData(XArray *pArr, int nIndex, void *pData, size_t nSize);

size_t XArray_GetUsedSize(XArray *pArr);
size_t XArray_GetArraySize(XArray *pArr);

#ifdef __cplusplus
}
#endif

#endif /* __XUTILS_XARRAY_H__ */

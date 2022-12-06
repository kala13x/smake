/*
 *  xjson/lib/xjson.h
 *
 *  This source is part of "xjson" project
 *  2019-2021  Sun Dro (f4tb0y@protonmail.com)
 *
 * Implementation of the lexical analyzer and 
 * recursive descent parser with JSON grammar
 *
 * The xJson library is free software. you can redistribute it and / or
 * modify under the terms of The MIT License. See LICENSE file for more
 * information: https://github.com/kala13x/xjson/blob/master/LICENSE
 */

#include <inttypes.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef __XUTILS_JSON_H__
#define __XUTILS_JSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define XJSON_VERSION_MIN       0
#define XJSON_VERSION_MAJ       1
#define XJSON_VERSION_BUILD     32

#define XJSON_SUCCESS           1
#define XJSON_FAILURE           0

#define XMAP_INITIAL_SIZE       16
#define XMAP_CHAIN_LENGTH       32
#define XMAP_MISSING            -5  /* No such element */
#define XMAP_OINV               -4  /* Invalid parameter */
#define XMAP_FULL               -3  /* Hashmap is full */
#define XMAP_OMEM               -2  /* Out of Memory */
#define XMAP_STOP               -1  /* Stop iteration */
#define XMAP_EMPTY              0   /* Map is empty */
#define XMAP_OK                 1   /* Success */

/////////////////////////////////////////////////////////////////////////
// XJSON ARRAY
/////////////////////////////////////////////////////////////////////////

typedef enum {
    XARRAY_STATUS_OK = (uint8_t)0,
    xarray_status_EMPTY,
    XARRAY_STATUS_NO_MEMORY
} xarray_status_t;

typedef struct xarray_data_ {
    size_t nSize;
    void* pData;
} xarray_data_t;

typedef int(*xarray_comparator_t)(const void*, const void*, void*);
typedef void(*xarray_clear_cb_t)(xarray_data_t *pArrData);

typedef struct xarray_ {
    xarray_clear_cb_t clearCb;
    xarray_status_t eStatus;
    xarray_data_t **pData;
    size_t nSize;
    size_t nUsed;
    int nFixed:1;
    int nAlloc:1;
} xarray_t;

void* XArray_Init(xarray_t *pArr, size_t nSize, uint8_t nFixed);
xarray_t* XArray_New(size_t nSize, uint8_t nFixed);
size_t XArray_Realloc(xarray_t *pArr);

void XArray_ClearData(xarray_t *pArr, xarray_data_t *pArrData);
void XArray_FreeData(xarray_data_t *pArrData);
void XArray_Clear(xarray_t *pArr);
void XArray_Destroy(xarray_t *pArr);

xarray_data_t *XArray_NewData(void *pData, size_t nSize);
xarray_data_t* XArray_Remove(xarray_t *pArr, size_t nIndex);
xarray_data_t* XArray_Get(xarray_t *pArr, size_t nIndex);

int XArray_Add(xarray_t *pArr, xarray_data_t *pNewData);
int XArray_AddData(xarray_t *pArr, void *pData, size_t nSize);
void* XArray_GetData(xarray_t *pArr, size_t nIndex);
size_t XArray_GetSize(xarray_t *pArr, size_t nIndex);

int XArray_Partitioning(xarray_t *pArr, xarray_comparator_t compare, void *pCtx, int nStart, int nFinish);
void XArray_QuickSort(xarray_t *pArr, xarray_comparator_t compare, void *pCtx, int nStart, int nFinish);
void XArray_Sort(xarray_t *pArr, xarray_comparator_t compare, void *pCtx);
void XArray_Swap(xarray_t *pArr, size_t nIndex1, size_t nIndex2);

size_t XArray_GetUsedSize(xarray_t *pArr);
size_t XArray_GetArraySize(xarray_t *pArr);

/////////////////////////////////////////////////////////////////////////
// XJSON HASHMAP
/////////////////////////////////////////////////////////////////////////

typedef struct xmap_pair_ {
    const char *pKey;
    void *pData;
    int nUsed:1;
} xmap_pair_t;

typedef void(*xmap_clear_cb_t)(xmap_pair_t*);
typedef int(*xmap_it_t)(xmap_pair_t*, void*);

typedef struct xmap_ {
    xmap_clear_cb_t clearCb;
    xmap_pair_t *pPairs;
    size_t nTableSize;
    size_t nUsed;
    int nAlloc:1;
} xmap_t;

int XMap_Init(xmap_t *pMap, size_t nSize);
xmap_t *XMap_New(size_t nSize);
void XMap_Destroy(xmap_t *pMap);

int XMap_UsedSize(xmap_t *pMap);
int XMap_Realloc(xmap_t *pMap);

uint32_t XMap_CRC32B(const uint8_t *pInput, size_t nLength);
int XMap_GetIndex(xmap_t *pMap, const char* pKey);
int XMap_Hash(xmap_t *pMap, const char *pStr);

int XMap_Iterate(xmap_t *pMap, xmap_it_t itfunc, void *pCtx);
int XMap_ClearIt(xmap_pair_t *pPair, void *pCtx);

int XMap_Put(xmap_t *pMap, const char* pKey, void *pValue);
void* XMap_Get(xmap_t *pMap, const char* pKey);
int XMap_Remove(xmap_t *pMap, const char* pKey);

/////////////////////////////////////////////////////////////////////////
// XJSON PARSER/ASSEMBLER
/////////////////////////////////////////////////////////////////////////

typedef enum {
    XJSON_TOKEN_INVALID = (uint8_t)0,
    XJSON_TOKEN_COMMA,
    XJSON_TOKEN_COLON,
    XJSON_TOKEN_QUOTE,
    XJSON_TOKEN_LCURLY,
    XJSON_TOKEN_RCURLY,
    XJSON_TOKEN_LPAREN,
    XJSON_TOKEN_RPAREN,
    XJSON_TOKEN_LSQUARE,
    XJSON_TOKEN_RSQUARE,
    XJSON_TOKEN_INTEGER,
    XJSON_TOKEN_FLOAT,
    XJSON_TOKEN_BOOL,
    XJSON_TOKEN_NULL,
    XJSON_TOKEN_EOF
} xjson_token_type_t;

typedef struct xjson_token_ {
    xjson_token_type_t nType;
    const char *pData;
    size_t nLength;
} xjson_token_t;

typedef enum {
    XJSON_TYPE_INVALID = (uint8_t)0,
    XJSON_TYPE_OBJECT,
    XJSON_TYPE_ARRAY,
    XJSON_TYPE_BOOLEAN,
    XJSON_TYPE_STRING,
    XJSON_TYPE_NUMBER,
    XJSON_TYPE_FLOAT,
    XJSON_TYPE_NULL,
} xjson_type_t;

typedef struct xjson_obj_ {
    xjson_type_t nType;
    void *pData;
    char *pName;
    int nAlloc:1;
} xjson_obj_t;

typedef enum {
    XJSON_ERR_NONE = (uint8_t)0,
    XJSON_ERR_UNEXPECTED,
    XJSON_ERR_INVALID,
    XJSON_ERR_BOUNDS,
    XJSON_ERR_EXITS,
    XJSON_ERR_ALLOC
} xjson_error_t;

xjson_error_t XJSON_AddObject(xjson_obj_t *pDst, xjson_obj_t *pSrc);
xjson_obj_t* XJSON_CreateObject(const char *pName, void *pValue, xjson_type_t nType);
xjson_obj_t* XJSON_NewObject(const char *pName);
xjson_obj_t* XJSON_NewArray(const char *pName);
xjson_obj_t* XJSON_NewU64(const char *pName, uint64_t nValue);
xjson_obj_t* XJSON_NewInt(const char *pName, int nValue);
xjson_obj_t* XJSON_NewFloat(const char *pName, double fValue);
xjson_obj_t* XJSON_NewString(const char *pName, const char *pValue);
xjson_obj_t* XJSON_NewBool(const char *pName, int nValue);
xjson_obj_t* XJSON_NewNull(const char *pName);
void XJSON_FreeObject(xjson_obj_t *pObj);

typedef struct xjson_ {
    xjson_token_t lastToken;
    xjson_error_t nError;
    xjson_obj_t *pRootObj;
    const char *pData;
    size_t nDataSize;
    size_t nOffset;
} xjson_t;

int XJSON_GetErrorStr(xjson_t *pJson, char *pOutput, size_t nSize);

int XJSON_Parse(xjson_t *pJson, const char *pData, size_t nSize);
void XJSON_Destroy(xjson_t *pJson);

size_t XJSON_GetArrayLength(xjson_obj_t *pObj);
xjson_obj_t* XJSON_GetObject(xjson_obj_t *pObj, const char *pName);
xjson_obj_t* XJSON_GetArrayItem(xjson_obj_t *pObj, size_t nIndex);

const char* XJSON_GetString(xjson_obj_t *pObj);
uint32_t XJSON_GetU32(xjson_obj_t *pObj);
uint64_t XJSON_GetU64(xjson_obj_t *pObj);
uint8_t XJSON_GetBool(xjson_obj_t *pObj);
double XJSON_GetFloat(xjson_obj_t *pObj);
int XJSON_GetInt(xjson_obj_t *pObj);

typedef struct xjson_writer_ {
    size_t nTabSize;
    size_t nIdents;
    size_t nLength;
    size_t nAvail;
    size_t nSize;
    char *pData;
    int nAlloc:1;
} xjson_writer_t;

void XJSON_DestroyWriter(xjson_writer_t *pWriter);
int XJSON_InitWriter(xjson_writer_t *pWriter, char *pOutput, size_t nSize);
int XJSON_WriteObject(xjson_obj_t *pObj, xjson_writer_t *pWriter);
int XJSON_Write(xjson_t *pJson, char *pOutput, size_t nSize);

#ifdef __cplusplus
}
#endif

#endif /* __XUTILS_JSON_H__ */
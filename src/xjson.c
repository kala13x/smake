/*
 *  xjson/lib/xjson.c
 *
 *  This source is part of "xjson" project
 *  2019-2021 (c) Sun Dro (f4tb0y@protonmail.com)
 *
 * Implementation of the lexical analyzer and 
 * recursive descent parser with JSON grammar
 *
 * The xJson library is free software. you can redistribute it and / or
 * modify under the terms of The MIT License. See LICENSE file for more
 * information: https://github.com/kala13x/xjson/blob/master/LICENSE
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "xjson.h"

#define XJSON_ASSERT(condition) \
    if (!condition) return XJSON_FAILURE

#define XOBJ_INITIAL_SIZE   2
#define XJSON_IDENT_INC     1
#define XJSON_IDENT_DEC     0

#define XJSON_NUMBER_MAX    64
#define XJSON_BOOL_MAX      6
#define XJSON_NULL_MAX      5

#define XARRAY_SUCCESS      0
#define XARRAY_FAILURE      -1

typedef struct xjson_iterator_ {
    xjson_writer_t *pWriter;
    size_t nCurrent;
    size_t nUsed;
} xjson_iterator_t;

int XJSON_GetErrorStr(xjson_t *pJson, char *pOutput, size_t nSize)
{
    switch (pJson->nError)
    {
        case XJSON_ERR_INVALID: 
            return snprintf(pOutput, nSize, "Invalid item at posit(%zu)", pJson->nOffset);
        case XJSON_ERR_EXITS: 
            return snprintf(pOutput, nSize, "Duplicate Key at posit(%zu)", pJson->nOffset);
        case XJSON_ERR_BOUNDS: 
            return snprintf(pOutput, nSize, "Unexpected EOF at posit(%zu)", pJson->nOffset);
        case XJSON_ERR_ALLOC: 
            return snprintf(pOutput, nSize, "Can not allocate memory for object at posit(%zu)", pJson->nOffset);
        case XJSON_ERR_UNEXPECTED: 
            return snprintf(pOutput, nSize, "Unexpected symbol '%c' at posit(%zu)", pJson->pData[pJson->nOffset], pJson->nOffset);
        default:
            break;
    }

    return snprintf(pOutput, nSize, "Undeclared error");
}

/////////////////////////////////////////////////////////////////////////
// XJSON ARRAY
/////////////////////////////////////////////////////////////////////////

xarray_data_t *XArray_NewData(void *pData, size_t nSize)
{
    xarray_data_t *pNewData = (xarray_data_t*)malloc(sizeof(xarray_data_t));
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

    return pNewData;
}

void XArray_FreeData(xarray_data_t *pArrData)
{
    if (pArrData != NULL)
    {
        if (pArrData->pData && 
            pArrData->nSize > 0) 
            free(pArrData->pData);

        free(pArrData);
    }
}

void XArray_ClearData(xarray_t *pArr, xarray_data_t *pArrData)
{
    if (pArr != NULL && 
        pArrData != NULL && 
        pArr->clearCb != NULL) 
    {
        pArr->clearCb(pArrData);
        pArrData->pData = NULL;
        pArrData->nSize = 0;
    }

    XArray_FreeData(pArrData);
}

void* XArray_Init(xarray_t *pArr, size_t nSize, uint8_t nFixed)
{
    pArr->pData = (xarray_data_t**)malloc(sizeof(xarray_data_t*) * nSize);
    if (pArr->pData == NULL) return NULL;

    size_t i;
    for (i = 0; i < nSize; i++) 
        pArr->pData[i] = NULL;

    pArr->eStatus = xarray_status_EMPTY;
    pArr->clearCb = NULL;
    pArr->nSize = nSize;
    pArr->nFixed = nFixed;
    pArr->nAlloc = 0;
    pArr->nUsed = 0;

    return pArr->pData;
}

xarray_t* XArray_New(size_t nSize, uint8_t nFixed)
{
    xarray_t *pArr = (xarray_t*)malloc(sizeof(xarray_t));
    if (pArr == NULL) return NULL;

    if (XArray_Init(pArr, nSize, nFixed) == NULL)
    {
        free(pArr);
        return NULL;
    }

    pArr->nAlloc = 1;
    return pArr;
}

void XArray_Clear(xarray_t *pArr)
{
    if (pArr->pData != NULL)
    {
        size_t i;
        for (i = 0; i < pArr->nSize; i++) 
        {
            XArray_ClearData(pArr, pArr->pData[i]);
            pArr->pData[i] = NULL;
        }
    }

    pArr->eStatus = xarray_status_EMPTY;
    pArr->nUsed = 0;
}

void XArray_Destroy(xarray_t *pArr)
{
    XArray_Clear(pArr);
    if (pArr->pData != NULL)
    {
        free(pArr->pData);
        pArr->pData = NULL;
    }

    pArr->nSize = 0;
    pArr->nFixed = 0;

    if (pArr->nAlloc) 
        free(pArr);
}

size_t XArray_Realloc(xarray_t *pArr)
{
    if (pArr->nFixed) return pArr->nSize;
    size_t nSize = 0, nUsed = pArr->nUsed;
    float fQuotient = (float)nUsed / (float)pArr->nSize;

    if (nUsed && nUsed == pArr->nSize) nSize = pArr->nSize * 2;
    else if (nUsed && fQuotient < 0.25) nSize = pArr->nSize / 2;

    if (nSize)
    {
        xarray_data_t **pData = (xarray_data_t**)malloc(sizeof(xarray_data_t*) * nSize);
        if (pData == NULL)
        {
            pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
            return 0;
        }

        size_t nCopySize = sizeof(xarray_data_t*) * pArr->nSize;
        memcpy(pData, pArr->pData, nCopySize);

        free(pArr->pData);
        pArr->pData = pData;
        pArr->nSize = nSize;

        size_t i;
        for (i = nUsed; i < nSize; i++) 
            pArr->pData[i] = NULL;
    }

    return pArr->nSize;
}

int XArray_CheckSpace(xarray_t *pArr)
{
    if (pArr && pArr->pData && 
        pArr->nUsed >= pArr->nSize)
        return XArray_Realloc(pArr);

    return 1;
}

int XArray_Add(xarray_t *pArr, xarray_data_t *pNewData)
{
    if (!XArray_CheckSpace(pArr))
    {
        XArray_ClearData(pArr, pNewData);
        return XARRAY_FAILURE;
    }

    pArr->pData[pArr->nUsed++] = pNewData;
    return pArr->nUsed;
}

int XArray_AddData(xarray_t *pArr, void *pData, size_t nSize)
{
    xarray_data_t *pNewData = XArray_NewData(pData, nSize);

    if (pNewData == NULL)
    {
        pArr->eStatus = XARRAY_STATUS_NO_MEMORY;
        return XARRAY_FAILURE;
    }

    return XArray_Add(pArr, pNewData);
}

void* XArray_GetData(xarray_t *pArr, size_t nIndex)
{
    if (nIndex >= pArr->nSize) return NULL;
    xarray_data_t *pArrData = pArr->pData[nIndex];
    return pArrData ? pArrData->pData : NULL;
}

size_t XArray_GetSize(xarray_t *pArr, size_t nIndex)
{
    if (nIndex >= pArr->nSize) return 0;
    xarray_data_t *pArrData = pArr->pData[nIndex];
    return pArrData ? pArrData->nSize : 0;
}

xarray_data_t* XArray_Remove(xarray_t *pArr, size_t nIndex)
{
    if (nIndex >= pArr->nSize) return NULL;
    size_t i;

    xarray_data_t *pData = pArr->pData[nIndex];
    if (pData == NULL) return NULL;

    for (i = nIndex; i < pArr->nUsed; i++)
    {
        if ((i + 1) >= pArr->nUsed) break;
        pArr->pData[i] = pArr->pData[i+1];
    }

    pArr->pData[--pArr->nUsed] = NULL;
    XArray_Realloc(pArr);
    return pData;
}

size_t XArray_GetUsedSize(xarray_t *pArr)
{
    if (pArr == NULL) return 0;
    return pArr->nUsed;
}

size_t XArray_GetArraySize(xarray_t *pArr)
{
    if (pArr == NULL) return 0;
    return pArr->nSize;
}

/////////////////////////////////////////////////////////////////////////
// XJSON MAP
/////////////////////////////////////////////////////////////////////////

int XMap_Init(xmap_t *pMap, size_t nSize)
{
    size_t nInitialSize = nSize ? nSize : XMAP_INITIAL_SIZE;
    pMap->pPairs = (xmap_pair_t*)calloc(nInitialSize, sizeof(xmap_pair_t));
    if (pMap->pPairs == NULL) return XMAP_OMEM;

    pMap->nTableSize = nInitialSize;
    pMap->clearCb = NULL;
    pMap->nAlloc = 0;
    pMap->nUsed = 0;
    return XMAP_OK;
}

xmap_t *XMap_New(size_t nSize)
{
    xmap_t *pMap = (xmap_t*)malloc(sizeof(xmap_t));
    if(pMap == NULL) return NULL;

    if (XMap_Init(pMap, nSize) < 0)
    {
        free(pMap);
        return NULL;
    }

    pMap->nAlloc = 1;
    return pMap;
}

void XMap_Free(xmap_t *pMap)
{
    if (pMap != NULL)
    {
        if (pMap->pPairs) free(pMap->pPairs);
        if (pMap->nAlloc) free(pMap);
    }
}

int XMap_UsedSize(xmap_t *pMap)
{
    if(pMap == NULL) return XMAP_OINV;
    return pMap->nUsed;
}

int XMap_Iterate(xmap_t *pMap, xmap_it_t itfunc, void *pCtx) 
{
    if (!XMap_UsedSize(pMap)) return XMAP_EMPTY;
    size_t i;

    for (i = 0; i < pMap->nTableSize; i++)
    {
        if (pMap->pPairs[i].nUsed)
        {
            int nStatus = itfunc(&pMap->pPairs[i], pCtx);
            if (nStatus != XMAP_OK) return nStatus;
        }
    }

    return XMAP_OK;
}

int XMap_ClearIt(xmap_pair_t *pPair, void *pCtx)
{
    xmap_t *pMap = (xmap_t*)pCtx;
    if (pMap->clearCb != NULL)
        pMap->clearCb(pPair);
    return XMAP_OK;
}

void XMap_Destroy(xmap_t *pMap)
{
    XMap_Iterate(pMap, XMap_ClearIt, pMap);
    XMap_Free(pMap);
}

uint32_t XMap_CRC32B(const uint8_t *pInput, size_t nLength)
{
    uint32_t nCRC = 0xFFFFFFFF;
    unsigned int i;

    for (i = 0;  i < nLength; i++)
    {
        uint32_t nByte = pInput[i];
        nCRC = nCRC ^ nByte;
        int j;

        for (j = 7; j >= 0; j--)
        {
            uint32_t nMask = -(nCRC & 1);
            nCRC = (nCRC >> 1) ^ (0xEDB88320 & nMask);
        }
    }

    return ~nCRC;
}

int XMap_Hash(xmap_t *pMap, const char *pStr)
{
    uint32_t pKey = XMap_CRC32B((unsigned char*)(pStr), strlen(pStr));

    /* Robert Jenkins' 32 bit Mix Function */
    pKey += (pKey << 12);
    pKey ^= (pKey >> 22);
    pKey += (pKey << 4);
    pKey ^= (pKey >> 9);
    pKey += (pKey << 10);
    pKey ^= (pKey >> 2);
    pKey += (pKey << 7);
    pKey ^= (pKey >> 12);

    /* Knuth's Multiplicative Method */
    pKey = (pKey >> 3) * 2654435761;
    return pKey % pMap->nTableSize;
}

int XMap_GetIndex(xmap_t *pMap, const char* pKey)
{
    if (pMap->nUsed >= pMap->nTableSize) return XMAP_FULL;
    int i, nIndex = XMap_Hash(pMap, pKey);

    for (i = 0; i < XMAP_CHAIN_LENGTH; i++)
    {
        if (!pMap->pPairs[nIndex].nUsed) return nIndex;
        else if (!strcmp(pMap->pPairs[nIndex].pKey, pKey)) return nIndex;
        nIndex = (nIndex + 1) % pMap->nTableSize;
    }

    return XMAP_FULL;
}

int XMap_Realloc(xmap_t *pMap)
{
    size_t nNewSize = pMap->nTableSize * 2;
    xmap_pair_t *pPairs = (xmap_pair_t*)calloc(nNewSize, sizeof(xmap_pair_t));
    if (pPairs == NULL) return XMAP_OMEM;

    xmap_pair_t* pOldPairs = pMap->pPairs;
    size_t nOldSize = pMap->nTableSize;
    size_t i, nUsed = pMap->nUsed;

    pMap->nTableSize = nNewSize;
    pMap->pPairs = pPairs;
    pMap->nUsed = 0;
    int nStatus = XMAP_OK;

    for(i = 0; i < nOldSize; i++)
    {
        if (!pOldPairs[i].nUsed) continue;
        nStatus = XMap_Put(pMap, pOldPairs[i].pKey, pOldPairs[i].pData);

        if (nStatus != XMAP_OK)
        {
            pMap->nTableSize = nOldSize;
            pMap->pPairs = pOldPairs;
            pMap->nUsed = nUsed;

            free(pPairs);
            return nStatus;
        }
    }

    free(pOldPairs);
    return nStatus;
}

int XMap_Put(xmap_t *pMap, const char* pKey, void *pValue)
{
    if (pMap == NULL || pKey == NULL) return XMAP_OINV;
    int nIndex = XMap_GetIndex(pMap, pKey);

    while (nIndex == XMAP_FULL)
    {
        int nStatus = XMap_Realloc(pMap);
        if (nStatus < 0) return nStatus;
        nIndex = XMap_GetIndex(pMap, pKey);
    }

    /* Set the data */
    pMap->pPairs[nIndex].pData = pValue;
    pMap->pPairs[nIndex].pKey = pKey;
    pMap->pPairs[nIndex].nUsed = 1;
    pMap->nUsed++; 
    return XMAP_OK;
}

void* XMap_Get(xmap_t *pMap, const char* pKey)
{
    if (pMap == NULL || pKey == NULL) return NULL;
    int i, nIndex = XMap_Hash(pMap, pKey);

    for (i = 0; i < XMAP_CHAIN_LENGTH; i++)
    {
        if (pMap->pPairs[nIndex].nUsed)
        {
            if (!strcmp(pMap->pPairs[nIndex].pKey, pKey))
                return pMap->pPairs[nIndex].pData;
        }

        nIndex = (nIndex + 1) % pMap->nTableSize;
    }

    return NULL;
}

int XMap_Remove(xmap_t *pMap, const char* pKey)
{
    if (pMap == NULL || pKey == NULL) return XMAP_OINV;
    int i, nIndex = XMap_Hash(pMap, pKey);

    for (i = 0; i < XMAP_CHAIN_LENGTH; i++)
    {
        if (pMap->pPairs[nIndex].nUsed)
        {
            if (!strcmp(pMap->pPairs[nIndex].pKey, pKey))
            {
                xmap_pair_t *pPair = &pMap->pPairs[nIndex];
                if (pMap->nUsed) pMap->nUsed--;

                if (pMap->clearCb != NULL) 
                    pMap->clearCb(pPair);

                pPair->nUsed = 0;
                pPair->pData = NULL;
                pPair->pKey = NULL;

                return XMAP_OK;
            }
        }

        nIndex = (nIndex + 1) % pMap->nTableSize;
    }

    return XMAP_MISSING;
}

/////////////////////////////////////////////////////////////////////////
// LEXICAL ANALYZER
/////////////////////////////////////////////////////////////////////////

static int XJSON_UnexpectedToken(xjson_t *pJson)
{
    xjson_token_t *pToken = &pJson->lastToken;
    pJson->nError = XJSON_ERR_UNEXPECTED;
    pJson->nOffset -= pToken->nLength;

    if (pToken->nType == XJSON_TOKEN_QUOTE)
        pJson->nOffset -= 2;

    return XJSON_FAILURE;
}

static int XJSON_UndoLastToken(xjson_t *pJson)
{
    pJson->nOffset -= pJson->lastToken.nLength;
    return XJSON_SUCCESS;
}

static int XJSON_CheckBounds(xjson_t *pJson)
{
    if (pJson->nOffset >= pJson->nDataSize)
    {
        pJson->nError = XJSON_ERR_BOUNDS;
        return XJSON_FAILURE;
    }

    return XJSON_SUCCESS;
}

static int XJSON_NextChar(xjson_t *pJson, char *pChar)
{
    XJSON_ASSERT(XJSON_CheckBounds(pJson));
    char nCharacter = pJson->pData[pJson->nOffset++];

    /* Skip space and new line characters */
    while (nCharacter == ' ' || 
           nCharacter == '\n' || 
           nCharacter == '\t')
    {
        XJSON_ASSERT(XJSON_CheckBounds(pJson));
        nCharacter = pJson->pData[pJson->nOffset++];
    }

    *pChar = nCharacter;
    return XJSON_SUCCESS;
}

static int XJSON_ParseDigit(xjson_t *pJson, char nCharacter)
{
    xjson_token_t *pToken = &pJson->lastToken;
    pToken->nType = XJSON_TOKEN_INVALID;
    size_t nPosition = pJson->nOffset;
    uint8_t nPoint = 0;

    if ((nCharacter == '-')) nCharacter = pJson->pData[pJson->nOffset];

    while (isdigit(nCharacter) || (nPoint < 2 && nCharacter == '.'))
    {
        XJSON_ASSERT(XJSON_CheckBounds(pJson));
        nCharacter = pJson->pData[pJson->nOffset++];

        if (nCharacter == '.' && ++nPoint == 2)
        {
            pToken->nLength = pJson->nOffset - nPosition;
            return XJSON_UnexpectedToken(pJson);
        }
    }

    pToken->nLength = pJson->nOffset - nPosition;
    pToken->nType = nPoint ? XJSON_TOKEN_FLOAT : XJSON_TOKEN_INTEGER;
    pToken->pData = &pJson->pData[nPosition - 1];
    pJson->nOffset--;

    return XJSON_SUCCESS;
}

static int XJSON_ParseQuote(xjson_t *pJson)
{
    xjson_token_t *pToken = &pJson->lastToken;
    pToken->nType = XJSON_TOKEN_INVALID;
    XJSON_ASSERT(XJSON_CheckBounds(pJson));

    size_t nStart = pJson->nOffset;
    char nCurr = 0, nPrev = 0;

    for (;;)
    {
        if (nCurr == '"' && nPrev != '\\') break;
        XJSON_ASSERT(XJSON_CheckBounds(pJson));

        nPrev = pJson->pData[pJson->nOffset-1];
        nCurr = pJson->pData[pJson->nOffset++];
    }

    pToken->nLength = pJson->nOffset - nStart - 1;
    pToken->pData = &pJson->pData[nStart];
    pToken->nType = XJSON_TOKEN_QUOTE;

    return XJSON_SUCCESS;
}

static int XJSON_IsAlphabet(char nChar)
{
    return ((nChar >= 'a' && nChar <= 'z') ||
            (nChar >= 'A' && nChar <= 'Z')) ?
                XJSON_SUCCESS : XJSON_FAILURE;
}

static int XJSON_ParseAlphabet(xjson_t *pJson, char nCharacter)
{
    xjson_token_t *pToken = &pJson->lastToken;
    pToken->nType = XJSON_TOKEN_INVALID;
    size_t nPosition = pJson->nOffset;

    while (XJSON_IsAlphabet(nCharacter))
    {
        XJSON_ASSERT(XJSON_CheckBounds(pJson));
        nCharacter = pJson->pData[pJson->nOffset++];
    }

    pToken->nLength = pJson->nOffset - nPosition;
    pToken->pData = &pJson->pData[nPosition - 1];
    pJson->nOffset--;

    if (!strncmp((char*)pToken->pData, "null", 4))
    {
        pToken->nType = XJSON_TOKEN_NULL;
        return XJSON_SUCCESS;
    }

    if (!strncmp((char*)pToken->pData, "true", 4) ||
        !strncmp((char*)pToken->pData, "false", 5))
    {
        pToken->nType = XJSON_TOKEN_BOOL;
        return XJSON_SUCCESS;
    }

    return XJSON_UnexpectedToken(pJson);
}

static int XJSON_GetNextToken(xjson_t *pJson)
{
    xjson_token_t *pToken = &pJson->lastToken;
    pToken->nType = XJSON_TOKEN_INVALID;
    pToken->pData = NULL;
    pToken->nLength = 0;
    char nChar = 0; 

    if (!XJSON_NextChar(pJson, &nChar))
    {
        pToken->nType = XJSON_TOKEN_EOF;
        return XJSON_FAILURE; // End of input
    }

    if (nChar == '-' || isdigit(nChar)) return XJSON_ParseDigit(pJson, nChar);
    else if (XJSON_IsAlphabet(nChar)) return XJSON_ParseAlphabet(pJson, nChar);
    else if (nChar == '"') return XJSON_ParseQuote(pJson);

    pToken->pData = &pJson->pData[pJson->nOffset-1];
    pToken->nLength = 1;

    switch (nChar)
    {
        case '\0': case EOF:
            pToken->nType = XJSON_TOKEN_EOF;
            pToken->pData = NULL;
            pToken->nLength = 0;
            break;
        case '{':
            pToken->nType = XJSON_TOKEN_LCURLY;
            break;
        case '}':
            pToken->nType = XJSON_TOKEN_RCURLY;
            break;
        case '[':
            pToken->nType = XJSON_TOKEN_LSQUARE;
            break;
        case ']':
            pToken->nType = XJSON_TOKEN_RSQUARE;
            break;
        case ':':
            pToken->nType = XJSON_TOKEN_COLON;
            break;
        case ',':
            pToken->nType = XJSON_TOKEN_COMMA;
            break;
        default:
            return XJSON_UnexpectedToken(pJson);
    }

    return XJSON_SUCCESS;
}

static int XJSON_Expect(xjson_t *pJson, xjson_token_type_t nType)
{
    XJSON_ASSERT(XJSON_GetNextToken(pJson));
    xjson_token_t *pToken = &pJson->lastToken;

    if (pToken->nType == nType) return XJSON_SUCCESS;
    return XJSON_UnexpectedToken(pJson);
}

/////////////////////////////////////////////////////////////////////////
// RECURSIVE PARSER/ASSEMBLER
/////////////////////////////////////////////////////////////////////////

static xjson_type_t XJSON_GetItemType(xjson_token_type_t eToken)
{
    switch(eToken) 
    {
        case XJSON_TOKEN_INTEGER: return XJSON_TYPE_NUMBER;
        case XJSON_TOKEN_QUOTE: return XJSON_TYPE_STRING;
        case XJSON_TOKEN_FLOAT: return XJSON_TYPE_FLOAT;
        case XJSON_TOKEN_BOOL: return XJSON_TYPE_BOOLEAN;
        case XJSON_TOKEN_NULL: return XJSON_TYPE_NULL;
        default: break;
    }

    return XJSON_TYPE_INVALID;
}

void XJSON_FreeObject(xjson_obj_t *pObj)
{
    if (pObj != NULL)
    {
        if (pObj->pData != NULL)
        {
            if (pObj->nType == XJSON_TYPE_OBJECT)
                XMap_Destroy((xmap_t*)pObj->pData);
            else if (pObj->nType == XJSON_TYPE_ARRAY)
                XArray_Destroy((xarray_t*)pObj->pData);
            else free(pObj->pData);
        }

        if (pObj->pName != NULL) free(pObj->pName);
        if (pObj->nAlloc) free(pObj);
    }
}

static void XJSON_ObjectClearCb(xmap_pair_t *pPair)
{
    if (pPair == NULL) return;
    XJSON_FreeObject((xjson_obj_t*)pPair->pData);
}

static void XJSON_ArrayClearCb(xarray_data_t *pItem)
{
    if (pItem == NULL) return;
    XJSON_FreeObject((xjson_obj_t*)pItem->pData);
}

xjson_error_t XJSON_AddObject(xjson_obj_t *pDst, xjson_obj_t *pSrc)
{
    if (pDst == NULL || pSrc == NULL) return XJSON_ERR_INVALID;
    else if (pDst->nType == XJSON_TYPE_OBJECT)
    {
        xmap_t *pMap = (xmap_t*)pDst->pData;
        if (XMap_Get(pMap, pSrc->pName)) return XJSON_ERR_EXITS;

        int nStatus = XMap_Put(pMap, pSrc->pName, (void*)pSrc);
        return nStatus < 0 ? XJSON_ERR_ALLOC : XJSON_ERR_NONE;
    }
    else if (pDst->nType == XJSON_TYPE_ARRAY)
    {
        xarray_t *pArray = (xarray_t*)pDst->pData;
        int nStatus = XArray_AddData(pArray, (void*)pSrc, 0);
        return nStatus < 0 ? XJSON_ERR_ALLOC : XJSON_ERR_NONE;
    }

    return XJSON_ERR_INVALID;
}

xjson_obj_t* XJSON_CreateObject(const char *pName, void *pValue, xjson_type_t nType)
{
    xjson_obj_t *pObj = (xjson_obj_t*)malloc(sizeof(xjson_obj_t));
    if (pObj == NULL) return NULL;

    pObj->pName = NULL;
    pObj->pData = pValue;
    pObj->nType = nType;
    pObj->nAlloc = 1;

    if (pName == NULL) return pObj;
    size_t nLength = strlen(pName);

    if (nLength > 0)
    {
        pObj->pName = (char*)malloc(nLength + 1);
        if (pObj->pName == NULL)
        {
            free(pObj);
            return NULL;
        }

        memcpy(pObj->pName, pName, nLength);
        pObj->pName[nLength] = '\0';
    }

    return pObj;
}

xjson_obj_t* XJSON_NewObject(const char *pName)
{
    xmap_t *pMap = XMap_New(XOBJ_INITIAL_SIZE);
    if (pMap == NULL) return NULL;

    pMap->clearCb = XJSON_ObjectClearCb;
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pMap, XJSON_TYPE_OBJECT);

    if (pObj == NULL)
    {
        XMap_Destroy(pMap);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewArray(const char *pName)
{
    xarray_t *pArray = XArray_New(XOBJ_INITIAL_SIZE, 0);
    if (pArray == NULL) return NULL;

    pArray->clearCb = XJSON_ArrayClearCb;
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pArray, XJSON_TYPE_ARRAY);

    if (pObj == NULL)
    {
        XArray_Destroy(pArray);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewU64(const char *pName, uint64_t nValue)
{
    char *pValue = (char*)malloc(XJSON_NUMBER_MAX);
    if (pValue == NULL) return NULL;

    snprintf(pValue, XJSON_NUMBER_MAX, "%"PRIu64, nValue);
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pValue, XJSON_TYPE_NUMBER);

    if (pObj == NULL)
    {
        free(pValue);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewInt(const char *pName, int nValue)
{
    char *pValue = (char*)malloc(XJSON_NUMBER_MAX);
    if (pValue == NULL) return NULL;

    snprintf(pValue, XJSON_NUMBER_MAX, "%d", nValue);
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pValue, XJSON_TYPE_NUMBER);

    if (pObj == NULL)
    {
        free(pValue);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewFloat(const char *pName, double fValue)
{
    char *pValue = (char*)malloc(XJSON_NUMBER_MAX);
    if (pValue == NULL) return NULL;

    snprintf(pValue, XJSON_NUMBER_MAX, "%lf", fValue);
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pValue, XJSON_TYPE_FLOAT);

    if (pObj == NULL)
    {
        free(pValue);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewString(const char *pName, const char *pValue)
{
    char *pSaveValue = strdup(pValue);
    if (pSaveValue == NULL) return NULL;

    xjson_obj_t *pObj = XJSON_CreateObject(pName, pSaveValue, XJSON_TYPE_STRING);

    if (pObj == NULL)
    {
        free(pSaveValue);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewBool(const char *pName, int nValue)
{
    char *pValue = (char*)malloc(XJSON_BOOL_MAX);
    if (pValue == NULL) return NULL;

    snprintf(pValue, XJSON_BOOL_MAX, "%s", nValue ? "true" : "false");
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pValue, XJSON_TYPE_BOOLEAN);

    if (pObj == NULL)
    {
        free(pValue);
        return NULL;
    }

    return pObj;
}

xjson_obj_t* XJSON_NewNull(const char *pName)
{
    char *pValue = (char*)malloc(XJSON_NULL_MAX);
    if (pValue == NULL) return NULL;

    snprintf(pValue, XJSON_NULL_MAX, "%s", "null");
    xjson_obj_t *pObj = XJSON_CreateObject(pName, pValue, XJSON_TYPE_NULL);

    if (pObj == NULL)
    {
        free(pValue);
        return NULL;
    }

    return pObj;
}

/* Forward declararions */
int XJSON_ParseObject(xjson_t *pJson, xjson_obj_t *pObj);
int XJSON_ParseArray(xjson_t *pJson, xjson_obj_t *pObj);

static int XJSON_ParseNewObject(xjson_t *pJson, xjson_obj_t *pObj, const char *pName)
{
    xjson_obj_t *pNewObj = XJSON_NewObject(pName);
    if (pNewObj == NULL)
    {
        pJson->nError = XJSON_ERR_ALLOC;
        return XJSON_FAILURE;
    }

    if (!XJSON_ParseObject(pJson, pNewObj))
    {
        XJSON_FreeObject(pNewObj);
        return XJSON_FAILURE;
    }

    pJson->nError = XJSON_AddObject(pObj, pNewObj);
    if (pJson->nError != XJSON_ERR_NONE)
    {
        XJSON_FreeObject(pNewObj);
        return XJSON_FAILURE;
    }

    return XJSON_Expect(pJson, XJSON_TOKEN_RCURLY);
}

static int XJSON_ParseNewArray(xjson_t *pJson, xjson_obj_t *pObj, const char *pName)
{
    xjson_obj_t *pNewObj = XJSON_NewArray(pName);
    if (pNewObj == NULL)
    {
        pJson->nError = XJSON_ERR_ALLOC;
        return XJSON_FAILURE;
    }

    if (!XJSON_ParseArray(pJson, pNewObj))
    {
        XJSON_FreeObject(pNewObj);
        return XJSON_FAILURE;
    }

    pJson->nError = XJSON_AddObject(pObj, pNewObj);
    if (pJson->nError != XJSON_ERR_NONE)
    {
        XJSON_FreeObject(pNewObj);
        return XJSON_FAILURE;
    }

    return XJSON_Expect(pJson, XJSON_TOKEN_RSQUARE);
}

static int XJSON_CheckObject(xjson_obj_t *pObj, xjson_type_t nType)
{
    return (pObj != NULL &&
            pObj->pData != NULL &&
            pObj->nType == nType) ? 
                XJSON_SUCCESS : XJSON_FAILURE; 
}

static int XJSON_TokenIsItem(xjson_token_t *pToken)
{
    return (pToken->nType == XJSON_TOKEN_QUOTE ||
            pToken->nType == XJSON_TOKEN_FLOAT ||
            pToken->nType == XJSON_TOKEN_BOOL ||
            pToken->nType == XJSON_TOKEN_NULL ||
            pToken->nType == XJSON_TOKEN_INTEGER) ?
                XJSON_SUCCESS : XJSON_FAILURE;
}

static char* JSON_LastTokenValue(xjson_t *pJson)
{
    xjson_token_t *pToken = &pJson->lastToken;
    char *pValue = malloc(pToken->nLength + 1);

    if (pValue == NULL)
    {
        pJson->nError = XJSON_ERR_ALLOC;
        return NULL;
    }

    memcpy(pValue, pToken->pData, pToken->nLength);
    pValue[pToken->nLength] = '\0';

    return pValue;
}

static int XJSON_PutItem(xjson_t *pJson, xjson_obj_t *pObj, const char *pName)
{
    xjson_token_t *pToken = &pJson->lastToken;
    xjson_type_t nType = XJSON_GetItemType(pToken->nType);

    if (nType == XJSON_TYPE_INVALID)
    {
        pJson->nError = XJSON_ERR_INVALID;
        return XJSON_FAILURE;
    }

    char *pValue = JSON_LastTokenValue(pJson);
    if (pValue == NULL) return XJSON_FAILURE;

    xjson_obj_t *pNewObj = XJSON_CreateObject(pName, pValue, nType);
    if (pNewObj == NULL)
    {
        free(pValue);
        pJson->nError = XJSON_ERR_ALLOC;
        return XJSON_FAILURE;
    }

    pJson->nError = XJSON_AddObject(pObj, pNewObj);
    if (pJson->nError != XJSON_ERR_NONE)
    {
        XJSON_FreeObject(pNewObj);
        return XJSON_FAILURE;
    }

    return XJSON_SUCCESS;
}

int XJSON_ParseArray(xjson_t *pJson, xjson_obj_t *pObj)
{
    xjson_token_t *pToken = &pJson->lastToken;
    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (pToken->nType == XJSON_TOKEN_RSQUARE)
        return XJSON_UndoLastToken(pJson);
    else if (XJSON_TokenIsItem(pToken))
        { XJSON_ASSERT(XJSON_PutItem(pJson, pObj, NULL)); }
    else if (pToken->nType == XJSON_TOKEN_LCURLY)
        { XJSON_ASSERT(XJSON_ParseNewObject(pJson, pObj, NULL)); }
    else if (pToken->nType == XJSON_TOKEN_LSQUARE)
        { XJSON_ASSERT(XJSON_ParseNewArray(pJson, pObj, NULL)); }
    else return XJSON_UnexpectedToken(pJson);

    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (pToken->nType == XJSON_TOKEN_COMMA)
        return XJSON_ParseArray(pJson, pObj);
    else if (pToken->nType != XJSON_TOKEN_RSQUARE)
        return XJSON_UnexpectedToken(pJson);

    return XJSON_UndoLastToken(pJson);
}

static int XJSON_ParsePair(xjson_t *pJson, xjson_obj_t *pObj)
{
    xjson_token_t *pToken = &pJson->lastToken;
    char sPairName[pToken->nLength + 1];

    strncpy(sPairName, pToken->pData, pToken->nLength);
    sPairName[pToken->nLength] = 0;

    XJSON_ASSERT(XJSON_Expect(pJson, XJSON_TOKEN_COLON));
    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (XJSON_TokenIsItem(pToken))
        { XJSON_ASSERT(XJSON_PutItem(pJson, pObj, sPairName)); }
    else if (pToken->nType == XJSON_TOKEN_LCURLY)
        { XJSON_ASSERT(XJSON_ParseNewObject(pJson, pObj, sPairName)); }
    else if (pToken->nType == XJSON_TOKEN_LSQUARE)
        { XJSON_ASSERT(XJSON_ParseNewArray(pJson, pObj, sPairName)); }
    else return XJSON_UnexpectedToken(pJson);

    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (pToken->nType == XJSON_TOKEN_COMMA)
        return XJSON_ParseObject(pJson, pObj);
    else if (pToken->nType != XJSON_TOKEN_RCURLY)
        return XJSON_UnexpectedToken(pJson);

    return XJSON_UndoLastToken(pJson);
}

int XJSON_ParseObject(xjson_t *pJson, xjson_obj_t *pObj)
{
    xjson_token_t *pToken = &pJson->lastToken;
    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (pToken->nType == XJSON_TOKEN_RCURLY) return XJSON_UndoLastToken(pJson);
    else if (pToken->nType == XJSON_TOKEN_QUOTE) return XJSON_ParsePair(pJson, pObj);
    else if (pToken->nType == XJSON_TOKEN_COMMA) return XJSON_ParseObject(pJson, pObj);
    else if (pToken->nType == XJSON_TOKEN_EOF) return XJSON_FAILURE;

    return XJSON_UnexpectedToken(pJson);
}

int XJSON_Parse(xjson_t *pJson, const char *pData, size_t nSize)
{
    pJson->nError = XJSON_ERR_NONE;
    pJson->nDataSize = nSize;
    pJson->pData = pData;
    pJson->nOffset = 0;

    xjson_token_t *pToken = &pJson->lastToken;
    XJSON_ASSERT(XJSON_GetNextToken(pJson));

    if (pToken->nType == XJSON_TOKEN_LCURLY)
    {
        pJson->pRootObj = XJSON_NewObject(NULL);
        if (pJson->pRootObj == NULL)
        {
            pJson->nError = XJSON_ERR_ALLOC;
            return XJSON_FAILURE;
        }

        XJSON_ASSERT(XJSON_ParseObject(pJson, pJson->pRootObj));
        return XJSON_Expect(pJson, XJSON_TOKEN_RCURLY);
    }
    else if (pToken->nType == XJSON_TOKEN_LSQUARE)
    {
        pJson->pRootObj = XJSON_NewArray(NULL);
        if (pJson->pRootObj == NULL)
        {
            pJson->nError = XJSON_ERR_ALLOC;
            return XJSON_FAILURE;
        }

        XJSON_ASSERT(XJSON_ParseArray(pJson, pJson->pRootObj));
        return XJSON_Expect(pJson, XJSON_TOKEN_RSQUARE);
    }

    return XJSON_UnexpectedToken(pJson);
}

void XJSON_Destroy(xjson_t *pJson)
{
    XJSON_FreeObject(pJson->pRootObj);
    pJson->nDataSize = 0;
    pJson->nOffset = 0;
    pJson->pData = NULL;
}

xjson_obj_t* XJSON_GetObject(xjson_obj_t *pObj, const char *pName)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_OBJECT)) return NULL;
    return XMap_Get((xmap_t*)pObj->pData, pName);
}

xjson_obj_t* XJSON_GetArrayItem(xjson_obj_t *pObj, size_t nIndex)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_ARRAY)) return NULL;
    return XArray_GetData((xarray_t*)pObj->pData, nIndex);
}

size_t XJSON_GetArrayLength(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_ARRAY)) return 0;
    return XArray_GetUsedSize((xarray_t*)pObj->pData);
}

int XJSON_GetInt(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_NUMBER)) return 0;
    return atoi((const char*)pObj->pData);
}

double XJSON_GetFloat(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_FLOAT)) return 0.;
    return atof((const char*)pObj->pData);
}

uint32_t XJSON_GetU32(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_NUMBER)) return 0;
    return atol((const char*)pObj->pData);
}

uint64_t XJSON_GetU64(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_NUMBER)) return 0;
    return strtoull((const char*)pObj->pData, NULL, 0);
}

uint8_t XJSON_GetBool(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_BOOLEAN)) return 0;
    return !strncmp((const char*)pObj->pData, "true", 4);
}

const char* XJSON_GetString(xjson_obj_t *pObj)
{
    if (!XJSON_CheckObject(pObj, XJSON_TYPE_STRING)) return "";
    return (const char*)pObj->pData;
}

static int XJSON_Realloc(xjson_writer_t *pWriter, size_t nSize)
{
    if (nSize < pWriter->nAvail) return XJSON_SUCCESS;
    else if (!pWriter->nAlloc) return XJSON_FAILURE;

    size_t nNewSize = pWriter->nSize + nSize;
    pWriter->pData = realloc(pWriter->pData, nNewSize);
    if (pWriter->pData == NULL) return XJSON_FAILURE;

    pWriter->nAvail += nSize;
    pWriter->nSize = nNewSize;
    return XJSON_SUCCESS;
} 

static int XJSON_AppedSpaces(xjson_writer_t *pWriter)
{
    if (!pWriter->nTabSize) return XJSON_SUCCESS;
    char sSpaces[pWriter->nIdents + 1];
    size_t nLenght = 0;

    while (nLenght < pWriter->nIdents)
        sSpaces[nLenght++] = ' ';

    sSpaces[nLenght] = '\0';
    XJSON_ASSERT(XJSON_Realloc(pWriter, nLenght));

    char *pOffset = &pWriter->pData[pWriter->nLength];
    nLenght = snprintf(pOffset, pWriter->nAvail, "%s", sSpaces);

    pWriter->nLength += nLenght;
    pWriter->nAvail -= nLenght;
    return XJSON_SUCCESS;
}

static int XJSON_WriteString(xjson_writer_t *pWriter, int nIdent, const char *pStr, ...)
{
    if (nIdent) XJSON_ASSERT(XJSON_AppedSpaces(pWriter));

    char *pBuffer = NULL;
    size_t nBytes = 0;

    va_list args;
    va_start(args, pStr);
    nBytes += vasprintf(&pBuffer, pStr, args);
    va_end(args);

    if (!nBytes || !XJSON_Realloc(pWriter, nBytes))
    {
        free(pBuffer);
        return XJSON_FAILURE;
    }

    strncpy(&pWriter->pData[pWriter->nLength], pBuffer, nBytes);
    pWriter->nLength += nBytes;
    pWriter->nAvail -= nBytes;
    pWriter->pData[pWriter->nLength] = '\0';

    free(pBuffer);
    return XJSON_SUCCESS;
}

static int XJSON_WriteName(xjson_obj_t *pObj, xjson_writer_t *pWriter)
{
    if (pObj->pName == NULL) return XJSON_SUCCESS;
    return XJSON_WriteString(pWriter, 1, "\"%s\":%s", 
        pObj->pName, pWriter->nTabSize ? " " : "");
}

static int XJSON_WriteItem(xjson_obj_t *pObj, xjson_writer_t *pWriter)
{  
    if (pObj->nType == XJSON_TYPE_INVALID ||
        pObj->nType == XJSON_TYPE_OBJECT ||
        pObj->nType == XJSON_TYPE_ARRAY)
            return XJSON_FAILURE;

    XJSON_ASSERT(pObj->pData);
    XJSON_ASSERT(XJSON_WriteName(pObj, pWriter));
    int nIdent = pObj->pName == NULL ? 1 : 0;

    return (pObj->nType == XJSON_TYPE_STRING) ? 
        XJSON_WriteString(pWriter, nIdent, "\"%s\"", (const char*)pObj->pData):
        XJSON_WriteString(pWriter, nIdent, "%s", (const char*)pObj->pData);
}

static int XJSON_MapIt(xmap_pair_t *pPair, void *pContext)
{
    xjson_iterator_t *pIt = (xjson_iterator_t*)pContext;
    xjson_obj_t *pItem = (xjson_obj_t *)pPair->pData;

    return (!XJSON_WriteObject(pItem, pIt->pWriter) ||
            (++pIt->nCurrent < pIt->nUsed &&
            !XJSON_WriteString(pIt->pWriter, 0, ",")) ||
            (pIt->pWriter->nTabSize &&
            !XJSON_WriteString(pIt->pWriter, 0, "\n"))) ?
                XMAP_STOP : XMAP_OK;
}

static int XJSON_Ident(xjson_writer_t *pWriter, int nIncrease)
{
    if (pWriter->nTabSize)
    {
        if (nIncrease) pWriter->nIdents += pWriter->nTabSize;
        else if (pWriter->nTabSize <= pWriter->nIdents) 
            pWriter->nIdents -= pWriter->nTabSize;
        else return XJSON_FAILURE;
    }

    return XJSON_SUCCESS;
}

static int XJSON_WriteHashmap(xjson_obj_t *pObj, xjson_writer_t *pWriter)
{
    XJSON_ASSERT(XJSON_CheckObject(pObj, XJSON_TYPE_OBJECT));
    XJSON_ASSERT(XJSON_WriteName(pObj, pWriter));
    int nIdent = pObj->pName == NULL ? 1 : 0;
    xmap_t *pMap = (xmap_t*)pObj->pData;

    XJSON_ASSERT(XJSON_WriteString(pWriter, nIdent, "{"));
    nIdent = (pWriter->nTabSize && pMap->nUsed) ? 1 : 0;

    if (nIdent)
    {
        XJSON_ASSERT(XJSON_WriteString(pWriter, 0, "\n"));
        XJSON_ASSERT(XJSON_Ident(pWriter, XJSON_IDENT_INC));
    }

    if (pMap->nUsed)
    {
        xjson_iterator_t *pIterator = (xjson_iterator_t*)malloc(sizeof(xjson_iterator_t));
        if (pIterator == NULL) return XJSON_FAILURE;

        pIterator->nCurrent = 0;
        pIterator->pWriter = pWriter;
        pIterator->nUsed = pMap->nUsed;

        if (XMap_Iterate(pMap, XJSON_MapIt, pIterator) != XMAP_OK) 
        {
            free(pIterator);
            return XJSON_FAILURE;
        }

        free(pIterator);
    }

    if (nIdent) XJSON_ASSERT(XJSON_Ident(pWriter, XJSON_IDENT_DEC));
    return XJSON_WriteString(pWriter, nIdent, "}");
}

static int XJSON_WriteArray(xjson_obj_t *pObj, xjson_writer_t *pWriter)
{
    XJSON_ASSERT(XJSON_CheckObject(pObj, XJSON_TYPE_ARRAY));
    XJSON_ASSERT(XJSON_WriteName(pObj, pWriter));
    int nIdent = pObj->pName == NULL ? 1 : 0;

    xarray_t* pArray = (xarray_t*)pObj->pData;
    XJSON_ASSERT(XJSON_WriteString(pWriter, nIdent, "["));

    size_t i, nUsed = XArray_GetUsedSize(pArray);
    nIdent = (pWriter->nTabSize && nUsed) ? 1 : 0;

    if (nIdent)
    {
        XJSON_ASSERT(XJSON_WriteString(pWriter, 0, "\n"));
        XJSON_ASSERT(XJSON_Ident(pWriter, XJSON_IDENT_INC));
    }

    for (i = 0; i < nUsed; i++)
    {
        xjson_obj_t *pItem = XArray_GetData(pArray, i);
        XJSON_ASSERT(XJSON_WriteObject(pItem, pWriter));
        if ((i + 1) < nUsed) XJSON_ASSERT(XJSON_WriteString(pWriter, 0, ","));
        if (pWriter->nTabSize) XJSON_ASSERT(XJSON_WriteString(pWriter, 0, "\n"));
    }

    if (nIdent) XJSON_ASSERT(XJSON_Ident(pWriter, XJSON_IDENT_DEC));
    return XJSON_WriteString(pWriter, nIdent, "]");
}

int XJSON_WriteObject(xjson_obj_t *pObj, xjson_writer_t *pWriter)
{
    if (pObj == NULL || pWriter == NULL) return XJSON_FAILURE;

    switch (pObj->nType)
    {
        case XJSON_TYPE_ARRAY: return XJSON_WriteArray(pObj, pWriter);
        case XJSON_TYPE_OBJECT: return XJSON_WriteHashmap(pObj, pWriter);
        case XJSON_TYPE_INVALID: return XJSON_FAILURE;
        default: break;
    }

    return XJSON_WriteItem(pObj, pWriter);
}

int XJSON_InitWriter(xjson_writer_t *pWriter, char *pOutput, size_t nSize)
{
    pWriter->nAvail = nSize;
    pWriter->pData = pOutput;
    pWriter->nSize = nSize;
    pWriter->nAlloc = 0;

    if (pWriter->pData == NULL && pWriter->nSize)
    {
        pWriter->pData = malloc(pWriter->nSize);
        if (pWriter->pData == NULL) return 0;
        pWriter->nAlloc = 1;
    }

    pWriter->pData[0] = '\0';
    pWriter->nTabSize = 0;
    pWriter->nIdents = 0;
    pWriter->nLength = 0;

    return 1;
}

void XJSON_DestroyWriter(xjson_writer_t *pWriter)
{
    if (pWriter && pWriter->nAvail)
    {
        free(pWriter->pData);
        pWriter->pData = NULL;
        pWriter->nAlloc = 0;
    }
}

int XJSON_Write(xjson_t *pJson, char *pOutput, size_t nSize)
{
    if (pJson == NULL || 
        pOutput == NULL || 
        !nSize) return XJSON_FAILURE;

    xjson_writer_t writer;
    XJSON_InitWriter(&writer, pOutput, nSize);
    return XJSON_WriteObject(pJson->pRootObj, &writer);;
}

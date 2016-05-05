/*
 *  src/vector.c
 *
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * This source includes functions to work with vectors
 * See the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

vector *vector_new(int size) 
{
    vector *pv = malloc(sizeof(vector));
    if (!pv) return NULL;

    pv->length = size;
    pv->current = 0;
    pv->storage = calloc(size, sizeof(void *));

    return pv;
}

void vector_free(vector *pv)
{
    if (!pv) return;
    pv->length = 0;
    pv->current = 0;
    free(pv->storage);
    free(pv);
}

size_t vector_size(vector *pv) 
{
    if (!pv) return 0;
    return pv->current;
}

int vector_empty(vector *pv)
{
    if (!pv) return 1;
    return pv->current ? 0 : 1;
}

void vector_resize(vector *pv)
{
    if (!pv) return;
    void *new_storage;
    int new_length;

    if (pv->current == pv->length) 
    {
        new_length = pv->length * 2;
        new_storage = realloc(pv->storage, sizeof(void *) *pv->length * 2);
    } 
    else if ((float)pv->current/(float)pv->length < 0.25) 
    {
        new_length = pv->length / 2;
        new_storage = realloc(pv->storage, sizeof(void *) *pv->length / 2);
    } 
    else return;

    /* 
     * Could not allocate more memory for the new storage of vector. This 
     * may cause trivial errors in our ptogram, so we must free and exit.
     */
    if (new_storage == NULL) 
    {
        vector_free(pv);
        printf("<%s:%d> %s: [ERROR] Could not allocate more memory for vector\n", 
            __FILE__, __LINE__, __FUNCTION__);

        exit(EXIT_FAILURE);
    }

    pv->storage = new_storage;
    pv->length = new_length;
}

int vector_clear(vector *pv) 
{
    if (!pv) return 0;
    pv->current = 0;
    return 1;
}

void vector_erase(vector *pv, int index)
{
    if (index >= pv->current) return;
    
    int i, j;
    pv->storage[index] = NULL;

    void **new_storage = (void**)malloc(sizeof(void*) * pv->current * 2);
    for (i = 0, j = 0; i < pv->current; i++) {
        if (pv->storage[i] != NULL) {
            new_storage[j] = pv->storage[i];
            j++;
        }       
    }

    free(pv->storage);

    pv->storage = new_storage;
    pv->current--;
}

void *vector_get(vector *pv, int index) 
{
    if (!pv) return NULL;
    if (index >= pv->current || index < 0)
        return NULL;

    return pv->storage[index];
}

void *vector_set(vector *pv, int index, void *elem) 
{
    if (!pv) return NULL;
    void *old_elem = NULL;

    if (index < pv->current && index >= 0) 
    {
        old_elem = pv->storage[index];
        pv->storage[index] = elem;
    }

    return old_elem;
}

void vector_push(vector *pv, void *elem) 
{
    if (!pv) return;
    pv->storage[pv->current++] = elem;
    vector_resize(pv);
}

void *vector_pop(vector *pv) 
{
    if (!pv) return NULL;
    if (!pv->current)
        return NULL;

    void *elem = pv->storage[--pv->current];
    vector_resize(pv);
    return elem;
}

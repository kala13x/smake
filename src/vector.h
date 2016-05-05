/*
 *  src/vector.h
 *
 *  Copyleft (C) 2015  Sun Dro (a.k.a. kala13x)
 *
 * This header includes functions to work with vectors.
 * See the LICENSE file.
 */

#ifndef __SUTILS_VECTORS_H__
#define __SUTILS_VECTORS_H__

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

struct XVector {
  size_t length;
  size_t current;
  void **storage;
};

typedef struct XVector vector;

vector *vector_new(int size);
size_t vector_size(vector *pv);
int vector_empty(vector *pv);
int vector_clear(vector *pv);

void vector_free(vector *pv);
void vector_resize(vector *pv);
void vector_erase(vector *pv, int index);
void vector_push(vector *pv, void *elem);

void *vector_get(vector *pv, int index);
void *vector_set(vector *pv, int index, void *elem);
void *vector_pop(vector *pv);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* __SUTILS_VECTORS_H__ */

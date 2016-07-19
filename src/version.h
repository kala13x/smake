/*
 *  src/version.h
 *
 *  Copyleft (C) 2016  Sun Dro (a.k.a. kala13x)
 *
 * Get additional information about project
 */

#define VERSION_MAX     0
#define VERSION_MIN     1
#define BUILD_NUMBER    7

#ifndef _SMAKE_VERSION_H_
#define _SMAKE_VERSION_H_

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

const char* GetVersion();
const char* GetVersion_Short();

void Greet(const char *pName);
void Usage(const char *pName);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* _SMAKE_VERSION_H_ */

/*
 *  src/sver.h
 *
 *  Copyleft (C) 2020  Sun Dro (a.k.a. kala13x)
 *
 * Get additional information about project
 */

#define SMAKE_VERSION_MAX     1
#define SMAKE_VERSION_MIN     0
#define SMAKE_BUILD_NUMBER    20

#ifndef _SMAKE_VERSION_H_
#define _SMAKE_VERSION_H_

/* For include header in CPP code */
#ifdef __cplusplus
extern "C" {
#endif

const char* SMake_Version();
const char* SMake_VersionShort();

void SMake_Greet(const char *pName);
void SMake_Usage(const char *pName);

/* For include header in CPP code */
#ifdef __cplusplus
}
#endif

#endif /* _SMAKE_VERSION_H_ */

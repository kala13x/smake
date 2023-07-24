/*!
 *  @file smake/src/info.h
 *
 *  This source is part of "smake" project
 *  2020-2023  Sun Dro (s.kalatoz@gmail.com)
 * 
 * @brief Get additional information about project.
 */

#define SMAKE_FULL_NAME       "Simple-Make"

#define SMAKE_VERSION_MAX     1
#define SMAKE_VERSION_MIN     1
#define SMAKE_BUILD_NUMBER    5

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

#ifndef _CG_UTIL_CMUTEX_H_
#define _CG_UTIL_CMUTEX_H_

#include <net/typedef.h>
#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************
 * Data Types
 ****************************************/

/**
 * \brief The generic wrapper struct for CyberLinkC's mutexes.
 *
 * This wrapper has been created to enable 100% code
 * compatibility for different platforms (Linux, Win32 etc..)
 */
typedef struct _CgMutex {
	/** The mutex entity */
	pthread_mutex_t mutexID;
} CgMutex;

/****************************************
 * Functions
 ****************************************/

/** 
 * Create a new mutex
 */
CgMutex *cg_mutex_new();

/** 
 * Destroy a mutex
 *
 * \param mutex The mutex to destroy
 */
BOOL cg_mutex_delete(CgMutex *mutex);

/** 
 * Acquire a mutex lock
 *
 * \param mutex Mutex to lock
 */
	BOOL cg_mutex_lock(CgMutex *mutex);
/** 
 * Release a locked mutex
 *
 * \param mutex Mutex to unlock
 */
	BOOL cg_mutex_unlock(CgMutex *mutex);
#ifdef  __cplusplus

} /* extern "C" */

#endif

#endif

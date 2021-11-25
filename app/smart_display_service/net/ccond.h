#ifndef _CG_UTIL_CCOND_H_
#define _CG_UTIL_CCOND_H_

#include <net/typedef.h>
#include <net/cmutex.h>

#include <pthread.h>

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************
 * Data Types
 ****************************************/

/**
 * \brief The generic wrapper struct for CyberLinkC's conds.
 *
 * This wrapper has been created to enable 100% code
 * compatibility for different platforms (Linux, Win32 etc..)
 */
typedef struct _CgCond {
	/** The cond entity */
	pthread_cond_t condID;
} CgCond;

/****************************************
 * Functions
 ****************************************/

/** 
 * Create a new condition variable
 */
CgCond *cg_cond_new();

/** 
 * Destroy a condition variable
 *
 * \param cond The cond to destroy
 */
BOOL cg_cond_delete(CgCond *cond);

/** 
 * Wait for condition variable to be signalled.
 *
 * \param cond Cond to be waited
 * \param mutex Mutex used for synchronization
 * \param timeout Maximum time in seconds to wait, 0 to wait forever
 */
int cg_cond_wait(CgCond *cond, CgMutex *mutex, unsigned long timeout);

/** 
 * Signal a condition variable
 *
 * \param cond Cond to be signalled
 */
BOOL cg_cond_signal(CgCond *cond);

/** 
 * Broadcast a condition variable
 *
 * \param cond Cond to be signalled
 */

BOOL cg_cond_broadcast(CgCond *cond);

#ifdef  __cplusplus

} /* extern "C" */

#endif

#endif

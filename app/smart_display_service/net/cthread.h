#ifndef _CG_UTIL_CTHREAD_H_
#define _CG_UTIL_CTHREAD_H_

#include <net/typedef.h>
#include <net/ccond.h>

#include <pthread.h>
#include <signal.h>

#ifdef  __cplusplus
extern "C" {
#endif

/****************************************
 * Define
 ****************************************/

#define CG_THREAD_MIN_SLEEP 1000
/* ADD END Fabrice Fontaine Orange 24/04/2007 */ 

/****************************************
 * Data Type
 ****************************************/

/**
 * \brief The generic wrapper struct for CyberLinkC's threads.
 *
 * This wrapper has been created to enable 100% code
 * compatibility between different platforms (Linux, Win32 etc..)
 */
typedef struct _CgThread {
	BOOL headFlag;
	struct _CgThread *prev;
	struct _CgThread *next;
		
	/** Indicates whether this thread is ready to run */
	BOOL runnableFlag;

#if defined DEBUG
	char friendlyName[32];
#endif

	/** The POSIX thread handle */
	pthread_t pThread;

	int priority;
	/** Thread's worker function */
	void (*action)(struct _CgThread *);

	/** Arbitrary data pointer */
	void *userData;
} CgThread, CgThreadList;

/**
 * Prototype for the threads' worker functions 
 */
typedef void (*CG_THREAD_FUNC)(CgThread *);

/****************************************
* Function
****************************************/

/**
 * Create a new thread
 */
CgThread *cg_thread_new();

/**
 * Get a self reference to thread.
 */

CgThread *cg_thread_self();

/**
 * Stop and destroy a thread.
 *
 * \param thread Thread to destroy
 */
BOOL cg_thread_delete(CgThread *thread);

/**
 * Start a thread (must be created first with ch_thread_new())
 *
 * \param thread Thread to start
 */
BOOL cg_thread_start(CgThread *thread);

/**
 * Stop a running thread.
 *
 * \param thread Thread to stop
 */
BOOL cg_thread_stop(CgThread *thread);

/**
 * Stop the running thread and signal the given CGCond.
 */
BOOL cg_thread_stop_with_cond(CgThread *thread, CgCond *cond);

/**
 * Restart a thread. Essentially calls cg_thread_stop() and cg_thread_start()
 *
 * \param thread Thread to restart
 */
BOOL cg_thread_restart(CgThread *thread);

/**
 * Check if a thread has been started
 *
 * \param thread Thread to check
 */
BOOL cg_thread_isrunnable(CgThread *thread);

/**
 * Set the thread's worker function.
 *
 * \param thread Thread struct
 * \param actionFunc Function pointer to set as the worker function
 */
void cg_thread_setaction(CgThread *thread, CG_THREAD_FUNC actionFunc);

/**
 * Set the user data pointer
 *
 * \param thread Thread struct
 * \param data Pointer to user data
 */
void cg_thread_setuserdata(CgThread *thread, void *data);

int cg_thread_setname(CgThread *thread,const char *name);

/**
 * Get the user data pointer
 *
 * \param thread Thread from which to get the pointer
 */
void *cg_thread_getuserdata(CgThread *thread);

#ifdef  __cplusplus

} /* extern "C" */

#endif

#endif

#include <net/cmutex.h>

#include <errno.h>

#if !defined(CG_MUTEX_LOG_ENABLED)
#undef cg_log_debug_l4
#define cg_log_debug_l4(msg)
#endif

/****************************************
* cg_mutex_new
****************************************/

CgMutex *cg_mutex_new()
{
	CgMutex *mutex;

	mutex = (CgMutex *)malloc(sizeof(CgMutex));

	if ( NULL != mutex )
	{
		pthread_mutex_init(&mutex->mutexID, NULL);
	}
	return mutex;
}

/****************************************
* cg_mutex_delete
****************************************/

BOOL cg_mutex_delete(CgMutex *mutex)
{
	if (!mutex)
		return FALSE;

	pthread_mutex_destroy(&mutex->mutexID);
	free(mutex);

	return TRUE;
}

BOOL cg_mutex_lock(CgMutex *mutex)
{
	if (!mutex)
		return FALSE;

	pthread_mutex_lock(&mutex->mutexID);

	return TRUE;
}

/****************************************
* cg_mutex_unlock
****************************************/

BOOL cg_mutex_unlock(CgMutex *mutex)
{
	if (!mutex)
		return FALSE;

	pthread_mutex_unlock(&mutex->mutexID);

	return TRUE;
}


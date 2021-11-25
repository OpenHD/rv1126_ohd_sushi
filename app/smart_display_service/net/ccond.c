/******************************************************************
*
*	CyberUtil for C
*
*	Copyright (C) 2006 Nokia Corporation
*
*       Copyright (C) 2006 Nokia Corporation. All rights reserved.
*
*       This is licensed under BSD-style license,
*       see file COPYING.
*
*	File: ccond.c
*
*	Revision:
*
*	16-Jan-06
*		- first revision
*	09-May-08
*		- Changed cg_cond_signal() using SetEvent() instead of WaitForSingleObject for WIN32 platform.
*
******************************************************************/

#include <net/ccond.h>
#include <sys/time.h>

/****************************************
* cg_cond_new
****************************************/

CgCond *cg_cond_new()
{
	CgCond *cond;

	cond = (CgCond *)malloc(sizeof(CgCond));

	if ( NULL != cond )
	{
		pthread_cond_init(&cond->condID, NULL);
	}

	return cond;
}

/****************************************
* cg_cond_delete
****************************************/

BOOL cg_cond_delete(CgCond *cond)
{
	pthread_cond_destroy(&cond->condID);
	free(cond);

	return TRUE;
}

/****************************************
* cg_cond_lock
****************************************/

int cg_cond_wait(CgCond *cond, CgMutex *mutex, unsigned long timeout)
{

	struct timeval  now;
	struct timespec timeout_s;

       int wait_result =0;

	gettimeofday(&now, NULL);

	if (timeout < 1)
	{
		pthread_cond_wait(&cond->condID, &mutex->mutexID);
	} else {
		timeout_s.tv_sec = now.tv_sec + timeout;
		timeout_s.tv_nsec = now.tv_usec * 1000;
		wait_result = pthread_cond_timedwait(&cond->condID, &mutex->mutexID, &timeout_s);
	}

	return wait_result;
}

/****************************************
* cg_cond_unlock
****************************************/

BOOL cg_cond_signal(CgCond *cond)
{
	BOOL success = FALSE;

	if (!cond) return success;
	success = (pthread_cond_signal(&cond->condID) == 0);

	return success;
}

/****************************************
* cg_cond_broadcast
****************************************/

BOOL cg_cond_broadcast(CgCond *cond)
{
	BOOL success = FALSE;

	if (!cond) return success;
	success = (pthread_cond_broadcast(&cond->condID) == 0);

	return success;
}


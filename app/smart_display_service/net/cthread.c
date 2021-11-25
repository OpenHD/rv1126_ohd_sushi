#include <signal.h>

#include <net/cthread.h>


#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Private function prototypes */
static void sig_handler(int sign);

/****************************************
 * Thread Function
 ****************************************/

/* Key used to store self reference in (p)thread global storage */
static pthread_key_t cg_thread_self_ref;
static pthread_once_t cg_thread_mykeycreated = PTHREAD_ONCE_INIT;

static void cg_thread_createkey()
{
    pthread_key_create(&cg_thread_self_ref, NULL);
}

CgThread *cg_thread_self()
{
    return (CgThread *)pthread_getspecific(cg_thread_self_ref);
}

static void *PosixThreadProc(void *param)
{

    sigset_t set;
    struct sigaction actions;
    CgThread *thread = (CgThread *)param;

    /* SIGQUIT is used in thread deletion routine
     * to force accept and recvmsg to return during thread
     * termination process. */
    sigfillset(&set);
    sigdelset(&set, SIGQUIT);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = sig_handler;
    sigaction(SIGQUIT, &actions, NULL);

    pthread_once(&cg_thread_mykeycreated, cg_thread_createkey);
    pthread_setspecific(cg_thread_self_ref, param);

    if (thread->action != NULL)
        thread->action(thread);

    return 0;
}

/****************************************
 * cg_thread_new
 ****************************************/

CgThread *cg_thread_new()
{
    CgThread *thread;


    thread = (CgThread *)malloc(sizeof(CgThread));

    if ( NULL != thread )
    {

        thread->runnableFlag = FALSE;
        thread->action = NULL;
        thread->userData = NULL;
        thread->priority = 0;
    }

    return thread;
}

/****************************************
 * cg_thread_delete
 ****************************************/

BOOL cg_thread_delete(CgThread *thread)
{
    if (!thread) return TRUE;

    if (thread->runnableFlag == TRUE)
        cg_thread_stop(thread);

    free(thread);

    return TRUE;
}

/****************************************
 * cg_thread_start
 ****************************************/

BOOL cg_thread_start(CgThread *thread)
{
    /**** Thanks for Visa Smolander (09/11/2005) ****/
    thread->runnableFlag = TRUE;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    /* MODIFICATION Fabrice Fontaine Orange 24/04/2007
       pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE); */
    /* Bug correction : Threads used to answer UPnP requests are created */
    /* in joinable state but the main thread doesn't call pthread_join on them. */
    /* So, they are kept alive until the end of the program. By creating them */
    /* in detached state, they are correctly clean up */
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
#ifdef STACK_SIZE
    /* Optimization : not we can set STACK_SIZE attribute at compilation time */
    /* to specify thread stack size */
    pthread_attr_setstacksize (&thread_attr,STACK_SIZE);
#endif
    if (thread->priority > 0) {
        struct sched_param param;
        pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        param.sched_priority = thread->priority;
        pthread_attr_setschedparam(&thread_attr, &param);
    }
    /* MODIFICATION END Fabrice Fontaine Orange 24/04/2007 */
    if (pthread_create(&thread->pThread, &thread_attr, PosixThreadProc, thread) != 0) {
        thread->runnableFlag = FALSE;
        pthread_attr_destroy(&thread_attr);
        return FALSE;
    }
    pthread_attr_destroy(&thread_attr);
    return TRUE;
}

/****************************************
 * cg_thread_stop
 ****************************************/

BOOL cg_thread_stop(CgThread *thread)
{
    return cg_thread_stop_with_cond(thread, NULL);
}

BOOL cg_thread_stop_with_cond(CgThread *thread, CgCond *cond)
{
    if (thread->runnableFlag == TRUE) {
        thread->runnableFlag = FALSE;
        if (cond != NULL) {
            cg_cond_signal(cond);
        }
        //printf("Killing thread %p ret=%d EBUSY=%d\n", thread, ret, EBUSY);
        if (thread->pThread > 0) {
            printf("thread %p is runing, kill it\n", thread);
            pthread_kill(thread->pThread, 0/*SIGQUIT*/);
        } else {
            printf("thread %p already  detached\n", thread);
        }
        /* MODIFICATION Fabrice Fontaine Orange 24/04/2007
           pthread_join(thread->pThread, NULL);*/
        /* Now we wait one second for thread termination instead of using pthread_join */
        usleep(CG_THREAD_MIN_SLEEP*5);
    }
    return TRUE;
}

/****************************************
 * cg_thread_restart
 ****************************************/

BOOL cg_thread_restart(CgThread *thread)
{
    cg_thread_stop(thread);
    cg_thread_start(thread);
    return TRUE;
}

/****************************************
 * cg_thread_isrunnable
 ****************************************/

BOOL cg_thread_isrunnable(CgThread *thread)
{
    return thread->runnableFlag;
}

/****************************************
 * cg_thread_setaction
 ****************************************/

void cg_thread_setaction(CgThread *thread, CG_THREAD_FUNC func)
{
    thread->action = func;
}

/****************************************
 * cg_thread_setuserdata
 ****************************************/

void cg_thread_setuserdata(CgThread *thread, void *value)
{	
    thread->userData = value;
}

/****************************************
 * cg_thread_getuserdata
 ****************************************/

void *cg_thread_getuserdata(CgThread *thread)
{
    return thread->userData;
}

int cg_thread_setname(CgThread *thread,const char *name){
    pthread_setname_np(thread->pThread,name);
}

/* Private helper functions */

static void sig_handler(int sign)
{
    printf("Got signal %d.\n", sign);
    return;
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <net/tcp_server.h>
#include <net/tcp_control.h>
#include <net/tcp_response.h>
#include "uvc_connect.pb.h"
#include <event/event_handle.h>
#include "utils/log.h"

extern tcp_client *tcp_client_cmd;
extern tcp_client *tcp_client_data;
extern int isOut;
static rndis_device* mRndisDev=NULL;
CgMutex* mIdMutex;
int netmode=0;
rndis_device* rndis_dev_new()
{
    rndis_device* dev = (rndis_device*)calloc(1, sizeof(rndis_device));
    dev->mConnector = NULL;
    /*init tcp threads: conn,recv,heartbeat*/
    dev->mTcpThreads = (tcp_thread*)calloc(1, sizeof(tcp_thread));
    dev->mTcpThreads->mEventRvThread = NULL;
    dev->mTcpThreads_data = (tcp_thread*)calloc(1, sizeof(tcp_thread));
    dev->mTcpThreads_data->mEventRvThread = NULL;
    dev->retrycnt = 0;
    return dev;
}

int rndis_do_reconnect()
{
    if (!mRndisDev)
        return -1;
    LOG_INFO("rndis_do_reconnect *************************\n");
    netmode = 0;
    cg_cond_signal(mRndisDev->mConnector->mCond);
    return 0;
}

void rndis_start_recv_thread(CgThread *thread, rndis_device* mdev)
{
    cg_thread_setaction(thread, rndis_do_tcp_recv);
    thread->priority = 0;
    cg_thread_setuserdata(thread, (void*)mdev);
    cg_thread_start(thread);
}

void rndis_start_recv_thread_data(CgThread *thread, rndis_device* mdev)
{
    cg_thread_setaction(thread, rndis_do_tcp_recv_data);
    thread->priority = 1;
    cg_thread_setuserdata(thread, (void*)mdev);
    cg_thread_start(thread);
}


void rndis_connector_thread(CgThread * thread)
{
    rndis_device* dev = (rndis_device*)cg_thread_getuserdata(thread);
    int timeout=0, ret=false;
    prctl(PR_SET_NAME, "rndis_connector_thread");
    while (cg_thread_isrunnable(thread) == TRUE)
    {
        cg_mutex_lock(dev->mConnector->mMutex);
        cg_cond_wait(dev->mConnector->mCond, dev->mConnector->mMutex, timeout);
        cg_mutex_unlock(dev->mConnector->mMutex);

        rndis_stop_tcp_threads(dev);
        usleep(100*1000);
        rndis_start_tcp_threads(dev);
        LOG_INFO("rndis_connector_thread************3\n");
    }
}

void rndis_stop_tcp_threads(rndis_device * dev)
{
    if (!dev) return;
    dev->mTcpThreads->isconnect = 0;
    dev->mTcpThreads_data->isconnect = 0;
    LOG_INFO("%s:%d dev->mTcpThreads=%p,%p\n", __FUNCTION__, __LINE__, dev->mTcpThreads,dev->mTcpThreads->mEventRvThread);
    LOG_INFO("%s:%d dev->mTcpThreads_data=%p,%p\n", __FUNCTION__, __LINE__, dev->mTcpThreads_data,dev->mTcpThreads_data->mEventRvThread);
    if (dev->mTcpThreads->mEventRvThread)
    {
        cg_thread_delete(dev->mTcpThreads->mEventRvThread);
        dev->mTcpThreads->mEventRvThread = NULL;
    }
    if (dev->mTcpThreads_data->mEventRvThread)
    {
        cg_thread_delete(dev->mTcpThreads_data->mEventRvThread);
        dev->mTcpThreads_data->mEventRvThread = NULL;
    }
    tcp_socket_delete(0);
    dev->retrycnt = 0;
}

void rndis_start_tcp_threads(rndis_device * dev)
{
    LOG_INFO("rndis_start_tcp_threads mEventRvThread enter\n");
    /*init cmcc recv thread*/
    if (dev->mTcpThreads->mEventRvThread == NULL)
    {
        dev->mTcpThreads->mEventRvThread = cg_thread_new();
        rndis_start_recv_thread(dev->mTcpThreads->mEventRvThread, dev);
    }
    if (dev->mTcpThreads_data->mEventRvThread == NULL)
    {
        dev->mTcpThreads_data->mEventRvThread = cg_thread_new();
        rndis_start_recv_thread_data(dev->mTcpThreads_data->mEventRvThread, dev);
    }

    dev->mTcpThreads->isconnect = tcp_socket_new();
    LOG_INFO("rndis_start_tcp_threads tcp_socket_new done\n");
    LOG_INFO("rndis_start_tcp_threads mEventRvThread=%p\n", dev->mTcpThreads->mEventRvThread);
    if(!dev->mTcpThreads->isconnect)
        isOut = 1;
}

void rndis_initialize_connector(rndis_device* dev)
{
    if (!dev) return;
    dev->mConnector = (uvc_connector*)malloc(sizeof(uvc_connector));
    dev->mConnector->mCond = cg_cond_new();
    dev->mConnector->mMutex = cg_mutex_new();
    dev->mConnector->mConnectorThread = cg_thread_new();

    /*start thread for tcp/http connect, if want to reconnect
      just do rndis_do_reconnect.
      */
    cg_thread_setaction(dev->mConnector->mConnectorThread, rndis_connector_thread);
    cg_thread_setuserdata(dev->mConnector->mConnectorThread, dev);
    cg_thread_start(dev->mConnector->mConnectorThread);
    usleep(50*1000);
}

int rndis_initialize_tcp(rndis_device* mdev)
{
    int mFd = 0;
    char* mAddr;
    mRndisDev = mdev;
    mIdMutex = cg_mutex_new();
    rndis_start_tcp_threads(mdev);

    //rndis_initialize_connector(mdev);
    usleep(500*1000);
    //if (netmode == LOCAL_CONFIG)
    //rndis_do_reconnect();

    LOG_INFO("----rndis_initialize_tcp done---\n");
    return mFd;
}

void rndis_deinit_connector(rndis_device* dev)
{
    if (!dev) return;
    if (dev->mConnector->mConnectorThread)
    {
        cg_thread_stop_with_cond(dev->mConnector->mConnectorThread, dev->mConnector->mCond);
        cg_thread_delete(dev->mConnector->mConnectorThread);
    }
    cg_mutex_delete(dev->mConnector->mMutex);
    cg_cond_delete(dev->mConnector->mCond);
}

void rndis_deinit(rndis_device* dev)
{
    if (!dev) return;

    rndis_stop_tcp_threads(dev);
    rndis_deinit_connector(dev);
}
void rndis_do_tcp_recv(CgThread * thread)//should in a thread
{
    //CgThread* mTcpClientThread = NULL;
    rndis_device* mDev = (rndis_device*)cg_thread_getuserdata(thread);
    prctl(PR_SET_NAME, "rndis_do_tcp_recv");
    while (cg_thread_isrunnable(thread) == TRUE)
    {
        int len = 0;
        mRndisTcpStream* mTcpStream = NULL;
        if (uvc_get_socket() <= 0)
        {
            usleep(50*1000);
            continue;
        }
        mTcpStream = (mRndisTcpStream*)malloc(sizeof(mRndisTcpStream));
        memset(mTcpStream, 0, sizeof(mRndisTcpStream));
        len = tcp_data_accept(mTcpStream,tcp_client_cmd);
        if (len > 0)
        {
            /*mTcpClientThread = cg_thread_new();
              cg_thread_setaction(mTcpClientThread, uvc_tcp_client_process_thread);
              cg_thread_setuserdata(mTcpClientThread, (void*)mTcpStream);
              cg_thread_start(mTcpClientThread);*/
            UvcConnectData mUvcConnectData;
            mUvcConnectData.ParseFromArray((const void*)(mTcpStream->mContent), len);
            //if(!mUvcConnectData)
            //{
            //cmd_receive(mUvcConnectData,  NULL);
            cmd_receive(handle_callback, mUvcConnectData,  NULL);
            //uvc_connect_data__free_unpacked(mUvcConnectData, NULL);
            //}
            rndis_tcp_stream_free(mTcpStream);
            mDev->retrycnt = 0;
        }
        else
        {
            rndis_tcp_stream_free(mTcpStream);

            usleep(10*1000);
        }
    }
    thread->pThread = 0;
    mDev->retrycnt = 0;
    LOG_INFO("rndis_do_tcp_recv end********** thread=%p\n", thread);
}
void rndis_do_tcp_recv_data(CgThread * thread)//should in a thread
{
    //CgThread* mTcpClientThread = NULL;
    rndis_device* mDev = (rndis_device*)cg_thread_getuserdata(thread);
    prctl(PR_SET_NAME, "rndis_do_tcp_recv_data");
    while (cg_thread_isrunnable(thread) == TRUE)
    {
        int len = 0;
        mRndisTcpStream* mTcpStream = NULL;
        if (uvc_get_socket_data() <= 0)
        {
            usleep(50*1000);
            continue;
        }
        mTcpStream = (mRndisTcpStream*)malloc(sizeof(mRndisTcpStream));
        memset(mTcpStream, 0, sizeof(mRndisTcpStream));
        len = tcp_data_accept(mTcpStream,tcp_client_data);
        if (len > 0)
        {
            /*mTcpClientThread = cg_thread_new();
              cg_thread_setaction(mTcpClientThread, uvc_tcp_client_process_thread);
              cg_thread_setuserdata(mTcpClientThread, (void*)mTcpStream);
              cg_thread_start(mTcpClientThread);*/
            UvcConnectData mUvcConnectData;
            mUvcConnectData.ParseFromArray((const void*)(mTcpStream->mContent), len);
            //if(!mUvcConnectData)
            //{
            //cmd_receive(mUvcConnectData,  NULL);
            cmd_receive(handle_callback, mUvcConnectData,  NULL);
            //uvc_connect_data__free_unpacked(mUvcConnectData, NULL);
            //}
            rndis_tcp_stream_free(mTcpStream);
            mDev->retrycnt = 0;
        }
        else
        {
            rndis_tcp_stream_free(mTcpStream);

            usleep(10*1000);
        }
    }
    thread->pThread = 0;
    mDev->retrycnt = 0;
    LOG_INFO("rndis_do_tcp_recv_data end********** thread=%p\n", thread);
}
void rndis_tcp_stream_free(mRndisTcpStream* mStream)
{
    if (NULL == mStream)
        return;
    if (mStream->mContent != NULL)
        free(mStream->mContent);
    free(mStream);
}

rndis_device* rndis_get_dev()
{
    if (mRndisDev == NULL)
    {
        mRndisDev = rndis_dev_new();
    }
    return mRndisDev;
}

int uvc_gen_request_id(bool autoplus)
{
    cg_mutex_lock(mIdMutex);
    static long unique_id = 1;
    EventId eventId = unique_id;
    if(autoplus)
    {
        eventId = ++unique_id;
    }
    else
    {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        eventId = tv.tv_sec;
    }
    cg_mutex_unlock(mIdMutex);
    return eventId;
}

int rndis_tcp_cmd_send(int command, char* msg)
{
    return response_send(command, msg, 0);
}

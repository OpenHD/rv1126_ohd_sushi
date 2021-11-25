#ifndef TCP_CONTROL_H
#define TCP_CONTROL_H
#include <sys/types.h>
#include <net/cthread.h>
#include <net/cmutex.h>
#include <net/tcp_server.h>
#include <net/cthread.h>
#include <net/typedef.h>

#ifdef  __cplusplus
extern "C" {
#endif
typedef long EventId;

typedef struct {
	CgThread* mEventRvThread;
    int isconnect;
}tcp_thread;

typedef struct {
    CgThread* mConnectorThread;
    CgCond* mCond;
	CgMutex* mMutex;
}uvc_connector;


typedef struct _rndis_device{
	//struct event_base * pWorkBase;
    int retrycnt;
    tcp_thread* mTcpThreads;
    tcp_thread* mTcpThreads_data;
    uvc_connector* mConnector;
}rndis_device;

rndis_device* rndis_dev_new();
int rndis_initialize_tcp(rndis_device* mdev);
void rndis_deinit(rndis_device* dev);
void rndis_start_tcp_threads(rndis_device * dev);
void rndis_stop_tcp_threads(rndis_device * dev);
void rndis_do_tcp_recv(CgThread* thread);
void rndis_do_tcp_recv_data(CgThread* thread);
int rndis_do_reconnect();

void rndis_tcp_stream_free(mRndisTcpStream* mStream);

int uvc_gen_request_id(bool autoplus);
rndis_device* rndis_get_dev();


int rndis_tcp_cmd_send(int command, char* msg);

#ifdef  __cplusplus
}
#endif
#endif

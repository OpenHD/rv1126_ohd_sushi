#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <event.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/bufferevent_ssl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/prctl.h>

#ifdef  __cplusplus
extern "C" {
#endif
#define TCP_SERVER_PORT 8090
#define TCP_SERVER_OTA_PORT 8093
typedef int SOCKET_FD;

typedef enum _TCP_ERROR_TYPE{
	TCP_REMOTE_SERVER_CLOSE  =-1,  
	TCP_CONNECT_ERROR = -2,
	TCP_NETWORK_ERROR = -3,
	TCP_READ_NULL = -4,
}TCP_ERROR_TYPE;

/**
 * A struct for client specific data, also includes pointer to create
 * a list of clients.
 */
typedef struct _tcp_client
{
	/* The clients socket. */
	int fd;
	/* The bufferedevent for this client. */
	struct bufferevent *buf_ev;
    /* The output buffer for this client. */
    struct evbuffer *output_buffer;
}tcp_client;

typedef struct _mRndisTcpStream{
	int header;
	int len;
	char* mContent;
}mRndisTcpStream;

int uvc_get_socket();
int uvc_get_socket_data();
int tcp_socket_new();
void tcp_event_dispatch();
void closeClient(tcp_client *client);
int tcp_socket_delete(SOCKET_FD fd);
void killServer(void);

int tcp_data_accept(mRndisTcpStream* mTcpStream,tcp_client* tcp_client_);
int tcp_socket_send(uint8_t* mSendData, int mContentLen);

#ifdef  __cplusplus
}
#endif
#endif

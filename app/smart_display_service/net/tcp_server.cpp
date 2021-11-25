/*
 * libevent echo server example using buffered events.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Required by event.h. */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <signal.h>
/* Libevent. */
#include <event.h>
#include <event2/thread.h>
#include "utils/log.h"
#include <pthread.h>

#include <net/tcp_server.h>

#define MAX_WATERMARKS 20*1024*1024

tcp_client *tcp_client_cmd = NULL;
tcp_client *tcp_client_data = NULL;
static struct event_base *evbase_accept;
extern int isOut;
static bool log_printed = 0;
static SOCKET_FD m_socket_fd= -1;
static SOCKET_FD m_data_socket_fd= -1;
pthread_mutex_t tcpMutex;
int cmcc_mutext_init()
{
    if (pthread_mutex_init(&tcpMutex, NULL) != 0)
    {
        LOG_ERROR("pthread_mutex_init fail--\n");
        return -1;
    }
    return 0;
}
int uvc_get_socket()
{
    return tcp_client_cmd != NULL ? 1 : 0;
}
int uvc_get_socket_data()
{
    return tcp_client_data != NULL ? 1 : 0;
}
/**
 * Set a socket to non-blocking mode.
 */
//用于设置非阻塞
int setnonblock(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;
    return 0;
}

/**
 * Called by libevent when there is data to read.
 */
void buffered_on_read(struct bufferevent *bev, void *arg)
{
    /* Write back the read buffer. It is important to note that
    * bufferevent_write_buffer will drain the incoming data so it
    * is effectively gone after we call it. */

    struct evbuffer *input;
    input = bufferevent_get_input(bev);
}
int tcp_data_accept(mRndisTcpStream* mTcpStream,tcp_client* tcp_client_)
{
    char mRvData[16];
    int readCnt = 0;

    if(tcp_client_->fd < 0)
    {
        //LOG_ERROR("--uvc_data_accept,network error--\n");
        return TCP_READ_NULL;
    }
    memset(mRvData, 0, 16);
    memset(mTcpStream, 0, sizeof(mRndisTcpStream));
    int nRead = bufferevent_read(tcp_client_->buf_ev, mRvData, 4);
    if (nRead <= 0)
    {
        //LOG_ERROR("********%s mContentLen is 0\n",__FUNCTION__);
        return TCP_READ_NULL;
    }
    int headerCode = 0, mContentLen=0;
    mContentLen |= (mRvData[0] << 24);
    mContentLen |= (mRvData[1] << 16);
    mContentLen |= (mRvData[2] << 8);
    mContentLen |= mRvData[3];
    LOG_DEBUG("recv the client msg: len = %d \n", mContentLen);

    mTcpStream->mContent = (char*)malloc(mContentLen);
    mTcpStream->header = headerCode;
    mTcpStream->len = mContentLen;
    if (mTcpStream->mContent == NULL)
        return -1;

    nRead = bufferevent_read(tcp_client_->buf_ev, mTcpStream->mContent, mContentLen);
    LOG_DEBUG("recv the client msg: nRead = %d \n", nRead);

    readCnt = nRead;
    while(readCnt < mContentLen)
    {
        nRead = bufferevent_read(tcp_client_->buf_ev, mTcpStream->mContent+readCnt, mContentLen-readCnt);
        readCnt += nRead;
    }
    LOG_DEBUG("recv the client msg: readCnt = %d \n", readCnt);
    return readCnt;
}

/**
 * Called by libevent when the write buffer reaches 0. We only
 * provide this because libevent expects it, but we don't use it.
 */
//当写缓冲区达到低水位时触发调用，我们这边不用
void buffered_on_write(struct bufferevent *bev, void *arg)
{
    //printf("buffered_on_write =============\n");
}

int tcp_socket_send(uint8_t* mSendData, int mContentLen)
{
    char* mOutpuData = NULL;
    int memSize=0, length=0;
    int headLen = 4;

    if (mContentLen == 0 || tcp_client_cmd == NULL)
    {
        //LOG_ERROR("********%s mContentLen is 0\n",__FUNCTION__);
        return 0;
    }
    if (pthread_mutex_lock(&tcpMutex) != 0)
    {
        LOG_ERROR("--tcp_socket_send lock error!\n");
        return -1;
    }
    if(tcp_client_cmd->fd < 0 || isOut == 1)
    {
        LOG_ERROR("--tcp_socket_send,network error--\n");
        return -1;
    }

    memSize= mContentLen + headLen + 1;
    length = mContentLen + headLen;
    mOutpuData = (uint8_t*) malloc(memSize);
    if (mOutpuData == NULL)
    {
        LOG_ERROR("tcp_socket_send malloc  error!\n");
        pthread_mutex_unlock(&tcpMutex);
        return 0;
    }
    mOutpuData[0] = (mContentLen >> 24) & 0xff;
    mOutpuData[1] = (mContentLen >> 16) & 0xff;
    mOutpuData[2] = (mContentLen >> 8) & 0xff;
    mOutpuData[3] = mContentLen & 0xff;
    memcpy(mOutpuData + 4, mSendData, mContentLen);

    /* Send the results to the client.  This actually only queues the results
        * for sending. Sending will occur asynchronously, handled by libevent. */
    struct evbuffer *output_ev = bufferevent_get_output(tcp_client_cmd->buf_ev);
    size_t bev_output_buf_len = evbuffer_get_length(output_ev);
    if (bev_output_buf_len <= MAX_WATERMARKS) {
        evbuffer_add(tcp_client_cmd->output_buffer, mOutpuData, length);
        if (bufferevent_write_buffer(tcp_client_cmd->buf_ev, tcp_client_cmd->output_buffer))
        {
            LOG_ERROR("Error sending data to client on fd %d\n", tcp_client_cmd->fd);
        }
    } else
    {
        if (!log_printed) {
            log_printed = true;
            LOG_WARN("WARNING, out put buffer len = %d, larger than MAX_WATERMARKS = %d\n",
                bev_output_buf_len, MAX_WATERMARKS);
        }

    }

    //Log::debug("---tcp_socket_send success nSendCnt=%d length=%d\n", nSendCnt, length);
    if (mOutpuData != NULL)
        free(mOutpuData);
    pthread_mutex_unlock(&tcpMutex);
    return 0;
}

/**
 * Called by libevent when there is an error on the underlying socket
 * descriptor.
 */
void buffered_on_error(struct bufferevent *bev, short what, void *arg)
{
    if (what & EVBUFFER_EOF)
    {
        /* Client disconnected, remove the read event and the
        * free the client structure. */
        LOG_ERROR("Client disconnected.\n");
    }
    else
    {
        LOG_ERROR("Client socket error, disconnecting.\n");
    }
    isOut = 1;
    tcp_client *client = (tcp_client *)arg;
    closeClient(client);
    if (event_base_loopbreak(evbase_accept))
    {
        LOG_ERROR("Error shutting down server");
    }
    if (event_base_loopexit(evbase_accept,NULL))
    {
        LOG_ERROR("Error shutting down server");
    }
}

/**
* This function will be called by libevent when there is a connection
* ready to be accepted.
*/
void on_accept(int fd, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    tcp_client *client;

    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        LOG_ERROR("accept failed");
        return;
    }
    /* Set the client socket to non-blocking mode. */
    if (setnonblock(client_fd) < 0)
        LOG_ERROR("failed to set client socket non-blocking");
    /*if (evutil_make_socket_nonblocking(client_fd) < 0) {
        warn("failed to set client socket to non-blocking");
        return;
    }*/
    /* We've accepted a new client, create a client object. */
    client = calloc(1, sizeof(*client));
    if (client == NULL)
        LOG_ERROR("malloc failed");
    client->fd = client_fd;

    if ((client->output_buffer = evbuffer_new()) == NULL)
    {
        LOG_ERROR("client output buffer allocation failed");
        return;
    }

    /* Create the buffered event.
     *
     * The first argument is the file descriptor that will trigger
     * the events, in this case the clients socket.
     *
     * The second argument is the callback that will be called
     * when data has been read from the socket and is available to
     * the application.
     *
     * The third argument is a callback to a function that will be
     * called when the write buffer has reached a low watermark.
     * That usually means that when the write buffer is 0 length,
     * this callback will be called. It must be defined, but you
     * don't actually have to do anything in this callback.
     *
     * The fourth argument is a callback that will be called when
     * there is a socket error. This is where you will detect
     * that the client disconnected or other socket errors.
     *
     * The fifth and final argument is to store an argument in
     * that will be passed to the callbacks. We store the client
     * object here.
     */
    /*client->buf_ev = bufferevent_new(client_fd, buffered_on_read,
                                         buffered_on_write, buffered_on_error, client);
    */
    client->buf_ev = bufferevent_socket_new(evbase_accept, client_fd,
                                            BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    bufferevent_setcb(client->buf_ev, buffered_on_read, buffered_on_write,
                      buffered_on_error, client);
    //client->buf_ev->wm_read.high = 1024 * 1024;
    //client->buf_ev->wm_read.low = 0;
    //bufferevent_setwatermark(client->buf_ev, EV_READ, 0, 1024 * 1024 * 10);
    /* We have to enable it before our callbacks will be
     * called. */
    //设置优先级0------base->nactivequeues / 2(默认优先级)，越小优先级越高
    bufferevent_priority_set(client->buf_ev,0);
    bufferevent_enable(client->buf_ev, EV_READ);
    tcp_client_cmd = client;
    LOG_INFO("Accepted connection from %s\n",
           inet_ntoa(client_addr.sin_addr));
}

void on_accept_data(int fd, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    tcp_client *client;

    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        LOG_ERROR("accept failed");
        return;
    }
    if (setnonblock(client_fd) < 0)
        LOG_ERROR("failed to set client socket non-blocking");
    client = calloc(1, sizeof(*client));
    if (client == NULL)
        LOG_ERROR("malloc failed");
    client->fd = client_fd;

    if ((client->output_buffer = evbuffer_new()) == NULL)
    {
        LOG_ERROR("client output buffer allocation failed");
        return;
    }
    client->buf_ev = bufferevent_socket_new(evbase_accept, client_fd,
                                            BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    bufferevent_setcb(client->buf_ev, buffered_on_read, buffered_on_write,
                      buffered_on_error, client);
    bufferevent_enable(client->buf_ev, EV_READ);
    tcp_client_data = client;
    LOG_INFO("Accepted connection_data from %s\n",
           inet_ntoa(client_addr.sin_addr));
}

int tcp_socket_new()
{
    int listen_fd;
    int listen_fd_data;
    struct sockaddr_in listen_addr;
    struct sockaddr_in listen_addr_data;
    struct event* ev_accept;
    struct event* ev_accept_data;
    int reuseaddr_on;
    int reuseaddr_on_data;

    /* Initialize libevent. */
    event_init();
    /* Create our listening socket. */
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("listen failed");
        return 0;
    }
    m_socket_fd = listen_fd;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(TCP_SERVER_PORT);
    reuseaddr_on = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,sizeof(reuseaddr_on));
    if (bind(listen_fd, (struct sockaddr *)&listen_addr,
             sizeof(listen_addr)) < 0) {
        LOG_ERROR("bind failed");
        close(listen_fd);
        return 0;
     }
    if (listen(listen_fd, 5) < 0) {
        LOG_ERROR("listen failed");
        close(listen_fd);
        return 0;
    }
    listen_fd_data = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_data < 0) {
        LOG_ERROR("listen_data failed");
        return 0;
    }
    m_data_socket_fd = listen_fd_data;
    memset(&listen_addr_data, 0, sizeof(listen_addr_data));
    listen_addr_data.sin_family = AF_INET;
    listen_addr_data.sin_addr.s_addr = INADDR_ANY;
    listen_addr_data.sin_port = htons(TCP_SERVER_OTA_PORT);
    reuseaddr_on_data = 1;
    setsockopt(listen_fd_data, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on_data,sizeof(reuseaddr_on_data));
    if (bind(listen_fd_data, (struct sockaddr *)&listen_addr_data,
             sizeof(listen_addr_data)) < 0) {
        LOG_ERROR("bind_data failed");
        close(listen_fd_data);
        return 0;
    }
    if (listen(listen_fd_data, 5) < 0) {
        LOG_ERROR("listen_data failed");
        close(listen_fd_data);
        return 0;
     }
    /* Set the socket to non-blocking, this is essential in event
     * based programming with libevent. */
    if (setnonblock(listen_fd) < 0)
        LOG_ERROR("failed to set server socket to non-blocking");
    if (setnonblock(listen_fd_data) < 0)
        LOG_ERROR("failed to set server socket to non-blocking");
    evthread_use_pthreads();

    if ((evbase_accept = event_base_new()) == NULL) {
        LOG_ERROR("Unable to create socket accept event base");
        close(listen_fd);
        return 0;
    }

    ev_accept = event_new(evbase_accept, listen_fd, EV_READ|EV_PERSIST,
                          on_accept,NULL);
    ev_accept_data = event_new(evbase_accept, listen_fd_data, EV_READ|EV_PERSIST,
                          on_accept_data,NULL);
    evthread_make_base_notifiable(evbase_accept);
    event_add(ev_accept, NULL);
    event_add(ev_accept_data, NULL);
    /* Start the event loop. */	event_base_dispatch(evbase_accept);
    event_free(ev_accept);
    ev_accept = NULL;
    event_base_free(evbase_accept);
    evbase_accept = NULL;
    close(listen_fd);
    close(listen_fd_data);

    /* We now have a listening socket, we create a read event to
     * be notified when a client connects. */
    /*event_set(&ev_accept, listen_fd, EV_READ | EV_PERSIST, on_accept, NULL);
    event_add(&ev_accept, NULL);
    */
    /* Start the event loop. */
//	event_dispatch();
    LOG_INFO("tcp_socket_new event_dispatch");

    return listen_fd;
}
void tcp_event_dispatch()
{
    event_dispatch();
}
void closeClient(tcp_client *client)
{
    if(client == NULL) return;
    //先关闭套接字，避免再去读取数据到buf中,导致崩溃
    if (client->fd >= 0)
    {
        close(client->fd);
        client->fd = -1;
    }
    if (client->buf_ev != NULL)
    {
        bufferevent_free(client->buf_ev);
        client->buf_ev = NULL;
    }
    if (client->output_buffer != NULL)
    {
        evbuffer_free(client->output_buffer);
        client->output_buffer = NULL;
    }

}
int tcp_socket_delete(SOCKET_FD fd)
{
    closeClient(tcp_client_cmd);
    if(m_socket_fd > 0)
        close(m_socket_fd);
    closeClient(tcp_client_data);
    if(m_data_socket_fd > 0)
        close(m_data_socket_fd);
    return 0;
}
/**
 * Kill the server.  This function can be called from another thread to kill
 * the server, causing runServer() to return.
 */
void killServer(void) {
    fprintf(stdout, "Stopping socket listener event loop.\n");
    if (event_base_loopexit(evbase_accept, NULL)) {
        LOG_ERROR("Error shutting down server");
    }
    fprintf(stdout, "Stopping workers.\n");
}


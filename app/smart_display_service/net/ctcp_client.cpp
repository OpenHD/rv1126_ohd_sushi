#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <net/typedef.h>
#include "uvc_connect.pb.h"

#include<event.h>
#include "shmc/shm_queue.h"
#include "shmc/shm_array.h"

#define SERVER_PORT 8090

namespace {

constexpr const char* kShmKey = "0x10007";
constexpr size_t kQueueBufSize = 1024*1024*1;
}  // namespace
using namespace shmc;
using namespace std;
shmc::ShmQueue<SVIPC> queue_r_;

//服务端信息
struct server {
    /* The server socket. */
    int fd;
    /* The bufferedevent for this server. */
    struct bufferevent *buf_ev;
    /* The output buffer for this client. */
    struct evbuffer *output_buffer;
};

//全局server数据
struct server *serv;


typedef struct {
    int64_t picture_timestamp;     // the time stamp of picture
    int64_t npu_outputs_timestamp; // the time stamp of npu outputs
    uint32_t npu_output_type;      // RK_NN_OUTPUT_TYPE
    uint8_t model_identifier[64];
} __attribute__((packed)) jpeg_data;


//设置文件状态标记
int setclientnonblock(int fd)
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
int client_socket_send(uint8_t* mSendData, int mContentLen,struct bufferevent* bev )
{
    char* mOutpuData = NULL;
    int memSize=0, length=0;
    int headLen = 4;

    if (mContentLen == 0)
    {
        //printf("********%s mContentLen is 0\n",__FUNCTION__);
        return 0;
    }

    memSize= mContentLen + headLen + 1;
    length = mContentLen + headLen;
    mOutpuData = (uint8_t*) malloc(memSize);
    if (mOutpuData == NULL)
    {
        printf("tcp_socket_send malloc  error!\n");
        return 0;
    }
    mOutpuData[0] = (mContentLen >> 24) & 0xff;
    mOutpuData[1] = (mContentLen >> 16) & 0xff;
    mOutpuData[2] = (mContentLen >> 8) & 0xff;
    mOutpuData[3] = mContentLen & 0xff;
    memcpy(mOutpuData + 4, mSendData, mContentLen);


    if (bufferevent_write(bev, mOutpuData, length))
    {
        printf("Error sending data to server on mContentLen %d\n", mContentLen);
    }

    //Log::debug("---tcp_socket_send success nSendCnt=%d length=%d\n", nSendCnt, length);
    if (mOutpuData != NULL)
        free(mOutpuData);
    return 0;
}

int client_send(int command, void* mCmdData,struct bufferevent* bev )
{
    UvcConnectData* mUvcConnectData;
    size_t msgLen=0;
    int ret = 0;
    uint8_t* outputBuffer=NULL;

    mUvcConnectData = (UvcConnectData*)malloc(sizeof(UvcConnectData));
    uvc_connect_data__init(mUvcConnectData);
    mUvcConnectData->id = 100;
    mUvcConnectData->code = command;

    mUvcConnectData->jsonbody_data = (char*)mCmdData;//cmcc_get_response_body_by_cmd(command, mCmdData);

    msgLen = uvc_connect_data__get_packed_size(mUvcConnectData);
    outputBuffer = (uint8_t*)malloc(msgLen);
    msgLen = uvc_connect_data__pack(mUvcConnectData, outputBuffer);
    ret = client_socket_send(outputBuffer, msgLen,bev);

    printf("---cmcc_commadn_send command=%d len:%d-%s--\n",mUvcConnectData->code, msgLen,mUvcConnectData->jsonbody_data);
    if (outputBuffer)
        free(outputBuffer);
    return ret;
}

//键盘事件
void cmd_msg_cb(int fd, short events, void* arg)
{
    printf("cmd_msg_cb\n");
    char msg[1024];
    char car[] = "0123456789ABCDEF";
    int ret = read(fd, msg, sizeof(msg));
    if (ret < 0)
    {
        perror("read fail ");
        exit(1);
    }
    struct bufferevent* bev = (struct bufferevent*)arg;
    //把终端的消息发送给服务器端
    //bufferevent_write(bev, msg, ret);
    for (int i=0; i<1; i++) {
        //usleep(1000*1000);
        sprintf(msg, "%s-%d", car,i);
        client_send(1,msg,bev);
    }
    std::string out;
    for (int i = 0; i < 100; i++) {
        assert(queue_r_.Pop(&out));
        printf("test pop %s\n",out.c_str());
        //ASSERT_EQ(src, out);
    }
}
int data_accept(struct bufferevent* bev)
{
    char mRvData[16];
    int readCnt = 0;
    UvcConnectData* mUvcConnectData = NULL;


    memset(mRvData, 0, 16);
    int nRead = bufferevent_read(bev, mRvData, 4);
    if (nRead <= 0)
    {
        //printf("%s:%d errno=%s(%d)\n", __FUNCTION__, __LINE__, strerror(errno), errno);
        return -1;
    }

    int headerCode = 0, mContentLen=0;
    mContentLen |= (mRvData[0] << 24);
    mContentLen |= (mRvData[1] << 16);
    mContentLen |= (mRvData[2] << 8);
    mContentLen |= mRvData[3];
    printf("recv the client msg: len = %d \n", mContentLen);

    char*  mContent = (char*)malloc(mContentLen);


    nRead = bufferevent_read(bev, mContent, mContentLen);
    printf("recv the client msg: nRead = %d \n", nRead);

    if (nRead <= 0)
        return -1;
    readCnt = nRead;
    while(readCnt < mContentLen)
    {
        nRead = bufferevent_read(bev, mContent+readCnt, mContentLen-readCnt);
        readCnt += nRead;
        if (nRead <= 0)
            break;
    }

    mUvcConnectData = uvc_connect_data__unpack(NULL, mContentLen, mContent);

    int len = mUvcConnectData->bitmap_data.len;
    printf("---cmd_receive code %d, len=%d -%s--\n",mUvcConnectData->code, len,mUvcConnectData->jsonbody_data);
    /*for(int i = 0;i< len;i++){
     printf("---cmd_receive bitmap_data %d",mUvcConnectData->bitmap_data.data[i]);
    }*/
    jpeg_data* data = (jpeg_data*)mUvcConnectData->bitmap_data.data;
    printf("---cmd_receive bitmap_data npu_outputs_timestamp: %lld ,picture_timestamp:%lld,npu_output_type:%d --\n",data->npu_outputs_timestamp,data->picture_timestamp,data->npu_output_type);
    for(int i = 0; i< 6; i++) {
        printf("---cmd_receive bitmap_data %x --\n",data->model_identifier[i]);
    }

    return readCnt;
}

//读服务端发来的数据
void read_msg_cb(struct bufferevent* bev, void* arg)
{
    printf("read_msg_cb\n");
    char msg[1024];

    //size_t len = bufferevent_read(bev, msg, sizeof(msg));
    //msg[len] = '\0';
    //printf("recv %s from server", msg);
    data_accept(bev);
}

//连接断开或者出错回调
void event_error(struct bufferevent *bev, short event, void *arg)
{
    printf("event_error\n");
    if (event & EVBUFFER_EOF)
        printf("connection closed\n");
    else if (event & EVBUFFER_ERROR)
        printf("some other error\n");
    struct event *ev = (struct event*)arg;
    //因为socket已经没有，所以这个event也没有存在的必要了
    free(ev);
//当发生错误退出事件循环
    event_loopexit(0);
    bufferevent_free(bev);
}

//连接到server
typedef struct sockaddr SA;
int tcp_connect_server(const char* server_ip, int port)
{
    int sockfd, status, save_errno;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    status = inet_aton(server_ip, &server_addr.sin_addr);

    if (status == 0) //the server_ip is not valid value
    {
        errno = EINVAL;
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        return sockfd;
    status = connect(sockfd, (SA*)&server_addr, sizeof(server_addr));

    if (status == -1)
    {
        save_errno = errno;
        close(sockfd);
        errno = save_errno; //the close may be error
        return -1;
    }

    setclientnonblock(sockfd);

    return sockfd;
}
int main(int argc, char** argv)
{
    event_init();
    //测试用直接连接本地server
    int sockfd = tcp_connect_server("127.0.0.1", SERVER_PORT);
    if (sockfd == -1)
    {
        perror("tcp_connect error ");
        return -1;
    }
    queue_r_.InitForRead(kShmKey);

    printf("connect to server successful\n");
    serv = calloc(1, sizeof(*serv));
    if (serv == NULL)
        err(1, "malloc failed");
    serv->fd = sockfd;
    serv->buf_ev = bufferevent_new(sockfd, read_msg_cb,
                                   NULL, NULL, (void *)serv);
    //监听终端输入事件
    struct event *ev_cmd = calloc(1,sizeof(*ev_cmd));
    event_set(ev_cmd, STDIN_FILENO,EV_READ | EV_PERSIST, cmd_msg_cb,
              (void*)serv->buf_ev);
    event_add(ev_cmd, NULL);
    //设置下read和发生错误的回调函数。（当socket关闭时会用到回调参数，删除键盘事件）
    bufferevent_setcb(serv->buf_ev, read_msg_cb, NULL, event_error, (void*)ev_cmd);
    bufferevent_enable(serv->buf_ev, EV_READ| EV_PERSIST);
    event_dispatch();
    printf("finished \n");
    return 0;
}

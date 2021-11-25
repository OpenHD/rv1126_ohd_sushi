#include "socket_server.h"
#include "xcam_obj_debug.h"
#include <fcntl.h>
#include <sys/time.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "socket_server.cpp"
#define UNIX_DOMAIN "/tmp/UNIX.domain"
#define UNIX_DOMAIN1 "/tmp/UNIX_1.domain"
SocketServer::~SocketServer() {
}


void SocketServer::SaveEixt() {
    LOGD("socket in aiq uit");
    quit_ = 1;
    if (_stop_fds[1] != -1) {
        char buf = 0xf;  // random value to write to flush fd.
        unsigned int size = write(_stop_fds[1], &buf, sizeof(char));
        if (size != sizeof(char))
            LOGW("Flush write not completed");
    }
}

void hexdump2(char* buf, const int num)
{
    int i;
    for(i = 0; i < num; i++) {
        LOGE("%02X ", buf[i]);
        if((i + 1) % 32 == 0) {
        }
    }
    return;
}

unsigned int MurMurHash(const void* key, int len)
{
    const unsigned int m = 0x5bd1e995;
    const int r = 24;
    const int seed = 97;
    unsigned int h = seed ^ len;
    // Mix 4 bytes at a time into the hash
    const unsigned char* data = (const unsigned char*)key;
    while(len >= 4)
    {
        unsigned int k = *(unsigned int*)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    // Handle the last few bytes of the input array
    switch(len)
    {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= m;
    };
    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

int ProcessText(int client_socket, rk_aiq_sys_ctx_t* ctx, char * data, int size) {
    char * tmpArray = data;
    int packetSize = (tmpArray[2] & 0xff) | ((tmpArray[3] & 0xff) << 8) | ((tmpArray[4] & 0xff) << 16) | ((tmpArray[5] & 0xff) << 24);
    int  ret;

    if( size != packetSize) {
        LOGE("packetSize check failed,returnsize is %d, pack is %d",size, packetSize);
        return -1;
    }

    char* receivedPacket = (char*)malloc(packetSize);
    memset(receivedPacket, 0, packetSize);
    memcpy(receivedPacket, tmpArray, packetSize);

    //parse data
    RkAiqSocketData receivedData;
    int offset = 0;
    offset += 2;

    //packetSize
    memcpy(receivedData.packetSize, receivedPacket+offset, 4);
    offset += 4;
    //command id
    memcpy((void*) & (receivedData.commandID), receivedPacket + offset, sizeof(int));
    offset += sizeof(int);
    //command id
    memcpy((void*) & (receivedData.commandResult), receivedPacket + offset, sizeof(int));
    offset += sizeof(int);

    //data size
    memcpy((void*) & (receivedData.dataSize), receivedPacket + offset, sizeof(unsigned int));
    //LOGE("receivedData.dataSize:%d", receivedData.dataSize);

    offset += sizeof(unsigned int);
    //data
    receivedData.data = (char*)malloc(receivedData.dataSize);
    memcpy(receivedData.data, receivedPacket + offset, receivedData.dataSize);
    offset += receivedData.dataSize;
    //data hash
    memcpy((void*) & (receivedData.dataHash), receivedPacket + offset, sizeof(unsigned int));
    free(receivedPacket);
    receivedPacket = NULL;

    //hash check
    unsigned int dataHash = MurMurHash(receivedData.data, receivedData.dataSize);
    //LOGE("receivedData.dataSize:%d", receivedData.dataSize);
    //LOGE("dataHash calculated:%x", dataHash);
    //LOGE("receivedData.dataHash:%x", receivedData.dataHash);

    if(dataHash != receivedData.dataHash) {
        LOGE("data hash not match.return");
        free(receivedData.data);
        receivedData.data = NULL;
        return -1;
    }
    //else {
        //LOGE("data hash check pass.");
    //}

    RkAiqSocketData dataReply;
    ret = ProcessCommand(ctx, &receivedData, &dataReply);
    free(receivedData.data);
    receivedData.data = NULL;

    if (ret != -1) {
        unsigned int packetSize = sizeof(RkAiqSocketData) + dataReply.dataSize - sizeof(char*);
        memcpy(dataReply.packetSize, &packetSize, 4);
        char* dataToSend = (char*)malloc(packetSize);
        int offset = 0;
        char magic[2] = {'R','K'};
        memset(dataToSend, 0, packetSize);
        memcpy(dataToSend, magic, 2);
        offset += 2;
        memcpy(dataToSend + offset, dataReply.packetSize, 4);
        offset += 4;
        memcpy(dataToSend + offset, (void*)&dataReply.commandID, sizeof(dataReply.commandID));
        offset += sizeof(dataReply.commandID);
        memcpy(dataToSend + offset, (void*)&dataReply.commandResult, sizeof(dataReply.commandResult));
        offset += sizeof(dataReply.commandResult);
        memcpy(dataToSend + offset, (void*)&dataReply.dataSize, sizeof(dataReply.dataSize));
        offset += sizeof(dataReply.dataSize);
        memcpy(dataToSend + offset, dataReply.data, dataReply.dataSize);
        offset += dataReply.dataSize;
        //LOGE("offset is %d,packetsize is %d",offset,packetSize);
        memcpy(dataToSend + offset, (void*)&dataReply.dataHash, sizeof(dataReply.dataHash));
        send(client_socket, dataToSend, packetSize, 0);
        if (dataReply.data != NULL){
            free(dataReply.data);
            dataReply.data = NULL;
            }
        free(dataToSend);
        dataToSend = NULL;
    }
    else {
        return -1;
    }
    return 0;
}

int SocketServer::Send(int cilent_socket, char *buff, int size) {
  return send(cilent_socket, buff, size, 0);
}

int SocketServer::Recvieve() {
  char buffer[MAXPACKETSIZE];
  char* receivedpacket;
  //int size = sizeof(buffer);
  int size = 6, offset =0;
  int packetsize = 0;
  struct timeval interval = {3, 0};
  struct RkAiqSocketData AiqData;
  socklen_t sosize = sizeof(clientAddress);
  setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,
               sizeof(struct timeval));
  while (!quit_) {
    int length = 0;
    if (size < 17000){
        length = recv(client_socket, buffer, size, 0);
        //LOGE("aiq socket length is %d,%d", length,sns_idx);
    }
    else {
        length = recv(client_socket, buffer, 17000, 0);
    }

    if (length <= 0) {
        if (length == 0) {
            break;
            //close(client_socket);
            //client_socket = accept(sockfd, (struct sockaddr *)&clientAddress, &sosize);
            //setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,
            //sizeof(struct timeval));
        }
        size = 6;
        offset = 0;
      //break;
    } else {
      if (offset == 0) {
        if (buffer[0] =='R' && buffer [1] =='K') {
            packetsize = (buffer[2] & 0xff) | ((buffer[3] & 0xff) << 8) | ((buffer[4] & 0xff) << 16) | ((buffer[5] & 0xff) << 24);
            receivedpacket = (char*)malloc(packetsize);
            memset(receivedpacket, 0, packetsize);
            memcpy(receivedpacket, buffer, 6);
            size = packetsize - 6;
            offset += length;
            memset(buffer,0x00,sizeof(buffer));
        } else {
            recv(client_socket, buffer, 8192, 0);
            memset(buffer,0x00,sizeof(buffer));
        }
      } else {
        memcpy(receivedpacket+offset, buffer, length);
        offset += length;
        size = packetsize - offset;
        if (size <= 0){
            //LOGE("%x,%x,%x,%x,pro",receivedpacket[2],receivedpacket[3],receivedpacket[4],receivedpacket[5]);
            ProcessText(client_socket, aiq_ctx, receivedpacket, packetsize);
            size = 6;
            free(receivedpacket);
            receivedpacket = NULL;
            offset = 0;
        }
      }
    }
    if (callback_) {
      callback_(client_socket, buffer, length);
    }
  }
  return 0;
}

#define POLL_STOP_RET (3)

int
SocketServer::poll_event (int timeout_msec, int fds[])
{
    int num_fds = fds[1] == -1 ? 1 : 2;
    struct pollfd poll_fds[num_fds];
    int ret = 0;

    memset(poll_fds, 0, sizeof(poll_fds));
    poll_fds[0].fd = fds[0];
    poll_fds[0].events = (POLLPRI | POLLIN | POLLOUT | POLLERR | POLLNVAL | POLLHUP);

    if (fds[1] != -1) {
        poll_fds[1].fd = fds[1];
        poll_fds[1].events = POLLPRI | POLLIN | POLLOUT;
        poll_fds[1].revents = 0;
    }

    ret = poll (poll_fds, num_fds, timeout_msec);
    if (fds[1] != -1) {
        if ((poll_fds[1].revents & POLLIN) || (poll_fds[1].revents & POLLPRI)) {
            LOGD("%s: Poll returning from flush", __FUNCTION__);
            return POLL_STOP_RET;
        }
    }

    if (ret > 0 && (poll_fds[0].revents & (POLLERR | POLLNVAL | POLLHUP))) {
        LOGE("polled error");
        return -1;
    }

    return ret;
}

void SocketServer::Accepted() {
  struct timeval interval = {3, 0};
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,sizeof(struct timeval));
  while (!quit_) {
    //int client_socket;
    socklen_t sosize = sizeof(clientAddress);
    //LOGE("socket accept ip");
    int fds[2] = { sockfd, _stop_fds[0] };
    int poll_ret = poll_event(-1, fds);
    if (poll_ret == POLL_STOP_RET) {
        LOG1("poll socket stop success !");
        break;
    } else if (poll_ret <= 0) {
        LOGW("poll socket got error(0x%x) but continue\n");
        ::usleep (10000); // 10ms
        continue;
    }
    client_socket = accept(sockfd, (struct sockaddr *)&clientAddress, &sosize);
    if (client_socket < 0) {
      if (errno != EAGAIN)
        LOGE("Error socket accept failed %d %d\n", client_socket, errno);
      continue;
    }
    //LOGE("socket accept ip %s\n", serverAddress);

    std::shared_ptr<std::thread> recv_thread;
    //recv_thread = make_shared<thread>(&SocketServer::Recvieve, this, client_socket);
    //recv_thread->join();
    //recv_thread = nullptr;
    quit_ = 0;
    this->Recvieve();
    close(client_socket);
    LOG1("socket accept close\n");
  }
  LOG1("socket accept exit\n");
}

int SocketServer::Process(rk_aiq_sys_ctx_t* ctx, const char* sns_ent_name) {
  LOGW("SocketServer::Process\n");
  int opt = 1;
  quit_ = 0;
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  aiq_ctx = ctx;
  memset(&serverAddress, 0, sizeof(serverAddress));

  serverAddress.sun_family = AF_UNIX;
  rk_aiq_static_info_t static_info;
  rk_aiq_uapi_sysctl_enumStaticMetas(0, &static_info);
  if (strcmp(static_info.sensor_info.sensor_name,sns_ent_name) == 0) {
    LOGD("the net name is %s,sns ent name is %s\n,idx0",static_info.sensor_info.sensor_name, sns_ent_name);
    strncpy(serverAddress.sun_path,UNIX_DOMAIN,sizeof(serverAddress.sun_path)-1);
    sns_idx = 0;
  } else {
    LOGD("the net name is %s,sns ent name is %s\n,idx1",static_info.sensor_info.sensor_name, sns_ent_name);
    strncpy(serverAddress.sun_path,UNIX_DOMAIN1,sizeof(serverAddress.sun_path)-1);
    sns_idx = 1;
  }

  if (sns_idx == 0) {
    unlink(UNIX_DOMAIN);
  } else {
    unlink(UNIX_DOMAIN1);
  }
  if ((::bind(sockfd, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress))) < 0) {
    LOGE("Error bind\n");
    exit(EXIT_FAILURE);
  }
  if (listen(sockfd, 5) < 0) {
    LOGE("Error listen\n");
    exit(EXIT_FAILURE);
  }

    if (pipe(_stop_fds) < 0) {
        LOGE("poll stop pipe error: %d", strerror(errno));
    } else {
        if (fcntl(_stop_fds[0], F_SETFL, O_NONBLOCK)) {
            LOGE("Fail to set stop pipe flag: %s", strerror(errno));
        }
    }

  std::shared_ptr<std::thread> accept_thread;
  this->accept_threads_ = make_shared<thread>(&SocketServer::Accepted, this);

  return 0;
}

void SocketServer::Deinit(){
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;

    this->SaveEixt();
    //setsockopt(client_socket,SOL_SOCKET,SO_LINGER,&so_linger,sizeof(so_linger));
    struct timeval interval = {0, 0};
    //setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,sizeof(struct timeval));

    if (client_socket >= 0)
        shutdown(client_socket, SHUT_RDWR);

    if (this->accept_threads_) {
        this->accept_threads_->join();
        this->accept_threads_ = nullptr;
    }

    //close(client_socket); //client_socket has been closed in SocketServer::Accepted()
    if (sockfd >= 0) {
        close(sockfd);
    }

    if (sns_idx == 0) {
        unlink(UNIX_DOMAIN);
    } else {
        unlink(UNIX_DOMAIN1);
    }

    if (_stop_fds[0] != -1)
      close(_stop_fds[0]);
    if (_stop_fds[1] != -1)
      close(_stop_fds[1]);
    LOGD("socekt stop in aiq");
}

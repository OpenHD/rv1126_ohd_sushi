#include <cstring>
#include <fstream>
#include <iostream>
#include <errno.h>
#include <err.h>
#include <openssl/md5.h>

#include <net/tcp_response.h>
#include <net/tcp_control.h>
#include <event/event_handle.h>
#include "utils/log.h"
#include "dbus/dbus_manage.h"

#define TAG "EventHandle"
using namespace std;

// 自定义数据结构
typedef struct SockInfo
{
    int fd;
    struct sockaddr_in addr;
    pthread_t id;
    ofstream *out;
}SockInfo;

#define MAX_RECEIVE 1024*1024*5    //5242880
#ifdef TMP_UPDATE_LOCATION
#define OTA_FILE "/tmp/update.img"
#else
#define OTA_FILE "/userdata/update.img"
#endif
bool checkdata(const char *dest_path, unsigned char *out_md5sum, long long offset, long long checkSize){
    MD5_CTX ctx;
    unsigned char md5sum[16];
    char buffer[512];
    int len = 0;

    FILE *fp = fopen(dest_path, "rb");

    if(fp == NULL){
        LOG_ERROR("open file failed %s", dest_path);
        return false;
    }

    fseek(fp, offset, SEEK_SET);

    MD5_Init(&ctx);

    long long readSize = 0;
    int step = 512;
    while(checkSize > 0){
        readSize = checkSize > step ? step: checkSize;
        if(fread(buffer, 1, readSize, fp) != readSize){
            LOG_ERROR("fread error.\n");
            return false;
        }
        checkSize = checkSize - readSize;
        MD5_Update(&ctx, buffer, readSize);
        memset(buffer, 0, sizeof(buffer));
    }
    MD5_Final(md5sum, &ctx);
    fclose(fp);
    LOG_INFO("\n");
    LOG_INFO("new md5:");
    for(int i = 0; i < 16; i++){
        LOG_INFO("%02x", md5sum[i]);
    }
    if(out_md5sum != NULL){
        memset(out_md5sum, 0, 16);
        memcpy(out_md5sum, md5sum, 16);
    }
    return true;
}

bool compareMd5sum(const char *dest_path, unsigned char *source_md5sum, long long offset, long long checkSize){
    unsigned char md5sum[16];

    checkdata(dest_path, md5sum, offset, checkSize);

    unsigned char tmp[16][2] = {
        0x30, 0x00,
        0x31, 0x01,
        0x32, 0x02,
        0x33, 0x03,
        0x34, 0x04,
        0x35, 0x05,
        0x36, 0x06,
        0x37, 0x07,
        0x38, 0x08,
        0x39, 0x09,
        0x61, 0x0a,
        0x62, 0x0b,
        0x63, 0x0c,
        0x64, 0x0d,
        0x65, 0x0e,
        0x66, 0x0f,
    };
    for(int i = 0; i < 32; i = i+2){
        for(int j = 0; j < 16; j++){
            if(tmp[j][1] == (md5sum[i/2] >> 4)){
                if(source_md5sum[i] != tmp[j][0]){
                    LOG_ERROR("MD5Check is error of %s", dest_path);
                    return false;
                }
            }
            if(tmp[j][1] == (md5sum[i/2] & 0x0f)){
                if(source_md5sum[i+1] != tmp[j][0]){
                    LOG_ERROR("MD5Check is error of %s", dest_path);
                    return false;
                }
            }
        }
    }
    LOG_INFO("\nMD5Check is ok of %s\n", dest_path);
    return true;
}

/**
 * Set a socket to non-blocking mode.
 */
//用于设置非阻塞
int setnonblockota(int fd)
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

/* 从socketfd描述字中读取"size"个字节. */
size_t readn(int fd, void *buffer, size_t size,ofstream &out) {
    char *buffer_pointer = (char *)buffer;
    int length = size;
    while (length > 0) {
        int result = read(fd, buffer_pointer, length);
        LOG_INFO("result is %d\n",result);
        if (result < 0) {
            if (errno == EINTR)
                continue;     /* 考虑非阻塞的情况，这里需要再次调用read */
            else
                return (-1);
        } else if (result == 0)
            break;                /* EOF(End of File)表示套接字关闭 */
        length -= result;
        buffer_pointer += result; 
    }
    return (size - length);        /* 返回的是实际读取的字节数*/
}

int read_data(int sockfd,ofstream &out) {
    ssize_t n;
    int time = 0;
    char *buf=(char*)malloc(sizeof(char) * MAX_RECEIVE);//动态申请空间
    for (;;) {
        LOG_INFO("block in read\n");
        if ((n = readn(sockfd, buf, MAX_RECEIVE,out)) == 0)
        {
            if(buf != NULL)
            {
                free(buf);
                buf = NULL;
            }
            return -1;
        }
        LOG_INFO("readn is %ld\n",n);
        out.write(buf,n);
        time++;
        LOG_INFO("5M read for %d \n", time);
        sleep(1);
    }
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
    return 0;
}

void handle_callback(UvcConnectData mEvent)
{
    int32_t flag = mEvent.code();

    if(flag == RNDIS_REC_H265_OPEN) {
       if(mEvent.bitmap_data() == "1") {
          system("touch /tmp/use_encodec_h265");
          LOG_ERROR("RNDIS_REC_H265_OPEN: select h265!!\n");
       } else {
          system("rm /tmp/use_encodec_h265");
          LOG_ERROR("RNDIS_REC_SWITCH_CAM: select h264!!\n");
       }
    }
    if(flag == RNDIS_REC_VERSION) {
        Version ver;
        ver.receive();
    }
    if(flag == RNDIS_REC_HEARTBEAT) {
        HeartBeat heart;
        heart.receive();
    }
    if(flag == RNDIS_REC_SWITCH_CAM) {
       if(mEvent.bitmap_data() == "1") {
          system("touch /tmp/use_cam_aux");
          LOG_ERROR("RNDIS_REC_SWITCH_CAM: select camerea aux!!\n");
       } else {
          system("rm /tmp/use_cam_aux");
          LOG_ERROR("RNDIS_REC_SWITCH_CAM: select camerea main!!\n");
       }
    }
    if(flag == RNDIS_REC_UPGRADE) {
        int len = mEvent.id();
        int len1 = mEvent.bitmap_data().length();
        //printf("RNDIS_REC_UPGRADE len %d , len1 %d \n", len, len1);
        if (mEvent.bitmap_data() == "DOWNLOAD")
        {
            if (access(OTA_FILE, F_OK) == 0) {
                char cmd[80];
                sprintf(cmd, "rm %s", OTA_FILE);
                LOG_INFO("cmd = %s", cmd);
                system(cmd);
            }
            return ;
        }
        char* content = new char[len];
        memset(content,0,len);
        memcpy(content,mEvent.bitmap_data().c_str(),len);
        FILE* fp = fopen(OTA_FILE, "a+");
        if (fp==0)
        {
            delete[] content;
            fclose(fp);
            LOG_ERROR("can't open log file\n");
            return ;
        }
        fwrite(content, len, 1, fp);//将msg对应的字符串append到文件末尾
        delete[] content;
        fclose(fp);
        std::string heart_event = "update receive";
        response_send(RNDIS_SEND_UPGRADE_OK,heart_event.c_str(),heart_event.length());
    }
    if(flag == RNDIS_REC_RESET) {
        //重启
        system("/oem/RkLunch-stop.sh && sleep 1 && reboot");
    }

    if(flag == RNDIS_REC_UPGRADESTART) {
        int len = mEvent.id();
        int len1 = mEvent.bitmap_data().length();
        LOG_DEBUG("RNDIS_REC_UPGRADESTART mEvent.bitmap_data() is %s, len %d , len1 %d \n", mEvent.bitmap_data().c_str(),len, len1);
        char* content = new char[len];
        memset(content,0,len);
        memcpy(content,mEvent.bitmap_data().c_str(),len);
        unsigned int fileSize = 0;
        int m_signMd5Size;
        unsigned char m_md5[32];
        int fd = open(OTA_FILE, O_RDONLY);
        if (fd < 0) {
            delete[] content;
            LOG_ERROR("Can't open update.img\n");
            close(fd);
            return ;
        }
        fileSize = lseek(fd, 0L, SEEK_END);
        if(lseek(fd, -32, SEEK_END) == -1){
            LOG_ERROR("lseek failed.\n");
        }
        if (!compareMd5sum((char*)OTA_FILE, content, 0, fileSize)){
            LOG_INFO("Md5Check update.img fwSize:%ld\n", fileSize);
        }
        else
        {
            //system("updateEngine --image_url=/userdata/update.img --update --savepath=/userdata/update.img --reboot &");
            Update update;
            update.receive();
        }
        delete[] content;
        close(fd);
        std::string heart_event = "update start";
        response_send(RNDIS_SEND_UPGRADE_OK,heart_event.c_str(),heart_event.length());
    }
    
    if(flag == RNDIS_REC_EPTZ) {
        if (mEvent.bitmap_data() == "1")
        {
            aiserver_start_eptz(1);
        } else {
            aiserver_start_eptz(0);
        }
    }

    if(RNDIS_REC_NN_REQ == flag){
      if(mEvent.bitmap_data() == "1") {
        aiserver_start_nn("app_nn", 1);
      } else if (mEvent.bitmap_data() == "0"){
        aiserver_start_nn("app_nn", 0);
      }
    }

    if(RNDIS_REC_MODEL_SELECT == flag) {
        if(mEvent.bitmap_data() == "1") {
            aiserver_set_nnstatus("face_landmark", 0);
            aiserver_set_nnstatus("face", 1);
            aiserver_set_nnstatus("body", 0);
        } else if (mEvent.bitmap_data() == "2") {
            aiserver_set_nnstatus("face_landmark", 0);
            aiserver_set_nnstatus("face", 0);
            aiserver_set_nnstatus("body", 1);
        } else if (mEvent.bitmap_data() == "3") {
            aiserver_set_nnstatus("face_landmark", 1);
            aiserver_set_nnstatus("face", 0);
            aiserver_set_nnstatus("body", 0);
        } else if (mEvent.bitmap_data() == "4") {
            aiserver_set_nnstatus("face_landmark", 1);
            aiserver_set_nnstatus("face", 1);
            aiserver_set_nnstatus("body", 1);
        }
    }

    if(flag == RNDIS_REC_FACEDETECT){
      std::string enable = mEvent.bitmap_data();
      if(enable == "1"){
          aiserver_set_nnstatus("face", 1);
      }else{
          aiserver_set_nnstatus("face", 0);
      }
    }
    if(flag == RNDIS_REC_FACELANDMARK){
      std::string enable = mEvent.bitmap_data();
      if(enable == "1"){
          aiserver_set_nnstatus("face_landmark", 1);
      }else{
          aiserver_set_nnstatus("face_landmark", 0);
      }
    }
    if(flag == RNDIS_REC_POSEBODY){
      std::string enable = mEvent.bitmap_data();
      if(enable == "1"){
          aiserver_set_nnstatus("body", 1);
      }else{
          aiserver_set_nnstatus("body", 0);
      }
    }
}

void Version:: receive()
{
    //获取版本信息
    std::ifstream readFile;
    readFile.open(SW_VERSION);
    std::string camera_version;
    if(readFile.is_open()) {
        getline(readFile, camera_version);
        if(camera_version.length() == 0) {
            camera_version = "AI Camera V1.0.0";
        }
    } else {
        camera_version = "AI Camera V1.0.2";
    }
    response_send(RNDIS_SEND_VERSION,camera_version.c_str(),camera_version.length());
}

void HeartBeat:: receive()
{
    //心跳
    std::string heart_event = "heart_event receive";
    response_send(RNDIS_SEND_HEARTBEAT,heart_event.c_str(),heart_event.length());
}

void Update:: receive()
{
    //下载固件,执行下载
    char cmd[100];
    sprintf(cmd, "updateEngine --image_url=%s --update --savepath=%s --reboot &", OTA_FILE, OTA_FILE);
    LOG_INFO("cmd = %s", cmd);
    system(cmd);
    std::string heart_event = "update receive";
    response_send(RNDIS_SEND_UPGRADE_OK,heart_event.c_str(),heart_event.length());
}


#ifndef TCP_RESPONSE_H
#define TCP_RESPONSE_H
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uvc_connect.pb.h"

#include <net/tcp_server.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define SW_VERSION "/etc/SW_VERSION"

typedef void (*RK_handle_callback)(UvcConnectData mEvent);

typedef struct {
  int64_t picture_timestamp;     // the time stamp of picture
  int64_t npu_outputs_timestamp; // the time stamp of npu outputs
  uint32_t npu_output_type;      // RK_NN_OUTPUT_TYPE
  uint8_t model_identifier[64];
} __attribute__((packed)) jpeg_data;

void cmd_receive(RK_handle_callback cb, UvcConnectData mEvent, void* userdata);

int response_send(int command, const void* mCmdData, int size);

UvcConnectData* parse_tcp_packet(mRndisTcpStream* mTcpStream);

enum RNDIS_EVENT_SEND{
    RNDIS_SEND_CONNECT        =  1000,
    RNDIS_SEND_DISCONNECT     =  1001,
    RNDIS_SEND_TIMEOUT        =  1002,
    RNDIS_SEND_ERROR          =  1003,
    RNDIS_SEND_NN_RESULT_SSD  =  1004,
    RNDIS_SEND_NN_RESULT_FACE =  1005,
    RNDIS_SEND_UPGRADE_OK     =  1006,
    RNDIS_SEND_UPGRADING      =  1007,
    RNDIS_SEND_UPGRAD_FAIL    =  1008,
    RNDIS_SEND_HEARTBEAT      =  1009,
    RNDIS_SEND_VERSION        =  1010,
};

enum RNDIS_EVENT_RECEIVE{
    RNDIS_REC_VERSION        =  1,
    RNDIS_REC_RESET          =  2,
    RNDIS_REC_UPGRADE        =  3,
    RNDIS_REC_AI_ON          =  4,
    RNDIS_REC_AI_OFF         =  5,
    RNDIS_REC_NN_REQ         =  6,
    RNDIS_REC_H265_OPEN      =  7,
    RNDIS_REC_FACTORY        =  8,
    RNDIS_REC_HEARTBEAT      =  9,
    RNDIS_REC_EPTZ      =  10,
    RNDIS_REC_MODEL_SELECT      =  11,
    RNDIS_REC_UPGRADESTART        =  12,
    RNDIS_REC_FACEDETECT        =  13,
    RNDIS_REC_FACELANDMARK        =  14,
    RNDIS_REC_POSEBODY        =  15,
    RNDIS_REC_SWITCH_CAM        =  16,
};

enum RNDIS_NN_MODEL{
    RNDIS_NN_ROCKX_FACE_RECT        =  1,
    RNDIS_NN_ROCKX_FACE_AGE          =  2,
    RNDIS_NN_POSEBODY        =  3,
};

#ifdef  __cplusplus
}
#endif
#endif


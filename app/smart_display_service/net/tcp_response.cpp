#include <cstring>
#include <fstream>
#include <iostream>

#include <net/tcp_response.h>
#include <net/tcp_control.h>
#include <event/event_handle.h>
#include "utils/log.h"

void cmd_receive(RK_handle_callback cb,UvcConnectData mEvent, void* userdata)
{
    int32_t flag = mEvent.code();
    LOG_DEBUG("---cmd_receive code %d, id=%d from zw\n", flag, mEvent.id());
    cb(mEvent);
}
int response_send(int command, const void* mCmdData, int size)
{
    LOG_DEBUG("---response_send code %d --- \n",command);
    UvcConnectData mUvcConnectData;
    size_t msgLen=0;
    int ret = 0;

    int len = size ;
    mUvcConnectData.set_id(uvc_gen_request_id(true));
    mUvcConnectData.set_code(command);
    mUvcConnectData.set_bitmap_data(mCmdData, len);
    std::string sendbuf;
    mUvcConnectData.SerializeToString(&sendbuf);
    LOG_DEBUG("---tcp_socket_send begin --- \n");
    ret = tcp_socket_send(sendbuf.c_str(), sendbuf.length());
    LOG_DEBUG("%s %d---command_send command=%d , pb_len=%d , bitmap_len=%d \n",__FUNCTION__,__LINE__,
           command, sendbuf.length(), len);
    return ret;
}

UvcConnectData* parse_tcp_packet(mRndisTcpStream* mTcpStream)
{
#if 0
    UvcConnectData* mUvcConnectData = NULL;

    if (mTcpStream == NULL)
        return NULL;
    mUvcConnectData = uvc_connect_data__unpack(NULL, mTcpStream->len, mTcpStream->mContent);

    return mUvcConnectData;
#endif
}


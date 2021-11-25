// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <sys/prctl.h>

#include <net/cmutex.h>
#include <net/typedef.h>
#include <net/shm_control.h>
#include <net/tcp_response.h>
#include "utils/log.h"
extern int enable_minilog;
extern int smart_display_service_log_level;
namespace rndis_server
{
ShmControl::ShmControl()
{
    char command[50] = "ps -e | grep -c aiserver";
    FILE* fp = NULL;
    int BUFSZ = 100;
    char buf[BUFSZ];
    int count = 1;
    if((fp = popen(command,"r")) == NULL)
    {
        LOG_INFO("popen err\r\n");
    }
    if((fgets(buf,BUFSZ,fp))!= NULL)
    {
        count = atoi(buf);
    }
    pclose(fp);
    fp=NULL;
    //LOG_INFO("count is %d\n",count);
    if (count <= 0) {
        LOG_INFO("read shm fail\n");
    } else {
        if(!queue_r_.InitForRead(kShmKey))
        {
            for (size_t i = 0; i < 8; i++)
            {
                if (queue_r_.InitForRead(kShmKey));
                {
                    LOG_INFO("read success\n");
                    break;
                }
		LOG_INFO("retry %d\n",i+1);
                usleep(500000);
            }
            LOG_INFO("read shm fail\n");
        }
        else
            LOG_INFO("read shm success\n");
    }
    shmc::SetLogHandler(shmc::kDebug, [](shmc::LogLevel lv, const char* s)
    {
        LOG_INFO("[%d] %s", lv, s);
    });
}
static void send_process_loop(void *arg);

void send_process_loop(void *arg)
{
    LOG_INFO("---send_process_loop \n");
    LOG_DEBUG("enable_minilog is %d, smart_display_service_log_level is %d\n",enable_minilog,smart_display_service_log_level);
    prctl(PR_SET_NAME, "uvc_tcp_client_process_thread");
    bool ret;
    ShmControl* control = ( ShmControl*)arg;
    int cnt = 0;
    while (true)
    {
        if(control->isStopUvcEngine())
        {
            usleep(10*1000);
            continue;
        }
        std::string out;
        ret = control->queue_r_.Pop(&out);
        if (ret)
        {
            cnt++;
            if(cnt == 5)
            {
                //control->StopUvcEngine(true);
                LOG_DEBUG("---send_process_loop out = %s ,len = %d \n",out.c_str(), out.length());
                if(cnt == 1000)
                    cnt = 0;
            }
            //printf("---send_process_loop out = %s \n",out.c_str());
            response_send(RNDIS_SEND_NN_RESULT_FACE,out.c_str(),out.length());
        }
        else
        {
            usleep(10*1000);
        }
    }
    char tname[32];
    prctl(PR_GET_NAME, tname);
    LOG_INFO("---uvc_tcp_client_process_thread  %s\n",tname);
}
void ShmControl::StopUvcEngine(bool stop)
{
    std::string out;
    int ret = 0;
    int length = 0;
    stop_uvc_engine = stop;
    std::lock_guard<std::mutex> lock(queue_mtx);
    do {
        ret = queue_r_.Pop(&out);
        length = out.length();
        LOG_INFO("---StopUvcEngine ret = %d ,length = %d\n",ret,length);
        out.clear();
    } while(ret && length > 0);
    //queue_r_.Reset();
    //queue_r_.InitForRead(kShmKey);
}

void ShmControl::StartSendThread()
{
    send_process = new std::thread(send_process_loop, this);
}
bool ShmControl::Stop()
{
    if (send_process)
    {
        send_process->join();
        delete send_process;
        send_process = nullptr;
    }
    return true;
}

}  // namespace rndis_server


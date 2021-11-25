/*
 *  Copyright (c) 2021 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include "Isp20PollThread.h"
#include "Isp20SpThread.h"
#include "motion_detect.h"
#include "rk_aiq_types_af_algo.h"

namespace RkCam {
#define DEBUG_TIMESTAMP                 1
#define RATIO_PP_FLG                    0
#define WRITE_FLG 						0
#define WRITE_FLG_OTHER 				1
int write_frame_num     = 2;
int frame_write_st      = -1;
char name_wr_flg[100] = "/tmp/motion_detection_wr_flg";
char name_wr_other_flg[100] = "/tmp/motion_detection_wr_other_flg";

static int thread_bind_cpu(int target_cpu)
{
 cpu_set_t mask;
 int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
 int i;

 if (target_cpu >= cpu_num)
  return -1;

 CPU_ZERO(&mask);
 CPU_SET(target_cpu, &mask);

 if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
  LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "pthread_setaffinity_np");

 if (pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
  LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM,"pthread_getaffinity_np");

 LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM,"Thread bound to cpu:");
 for (i = 0; i < CPU_SETSIZE; i++) {
 if (CPU_ISSET(i, &mask))
  LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM," %d", i);
 }

 return i >= cpu_num ? -1 : i;
}

XCamReturn
Isp20SpThread::select_motion_params(RKAnr_Mt_Params_Select_t *stmtParamsSelected, uint32_t frameid)
{
    SmartPtr<SensorHw> snsSubdev = _sensor_dev.dynamic_cast_ptr<SensorHw>();
    SmartPtr<RkAiqExpParamsProxy> expParams = nullptr;
    int iso = 50;

    snsSubdev->getEffectiveExpParams(expParams, frameid);
    if (expParams.ptr()) {
        RKAiqAecExpInfo_t* exp_tbl = &expParams->data()->aecExpInfo;

        if(_working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            iso = exp_tbl->LinearExp.exp_real_params.analog_gain * 50;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(_working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
            iso = exp_tbl->HdrExp[1].exp_real_params.analog_gain * 50;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(_working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
            iso = exp_tbl->HdrExp[2].exp_real_params.analog_gain * 50;
        }
    }

    float gain                              = iso / 50;
    float gain_f                            = log2(gain);
    uint8_t gain_l                          = ceil(gain_f);
    uint8_t gain_r                          = floor(gain_f);
    float ratio                             = (float)(gain_f - gain_r);

    SmartLock locker (_motion_param_mutex);

    stmtParamsSelected->enable              = _motion_params.stMotion.enable;
    stmtParamsSelected->sigmaHScale         = (_motion_params.stMotion.sigmaHScale       [gain_l] * ratio + _motion_params.stMotion.sigmaHScale       [gain_r] * (1 - ratio));
    stmtParamsSelected->sigmaLScale         = (_motion_params.stMotion.sigmaLScale       [gain_l] * ratio + _motion_params.stMotion.sigmaLScale       [gain_r] * (1 - ratio));
    stmtParamsSelected->light_clp           = (_motion_params.stMotion.lightClp          [gain_l] * ratio + _motion_params.stMotion.lightClp          [gain_r] * (1 - ratio));
    stmtParamsSelected->uv_weight           = (_motion_params.stMotion.uvWeight          [gain_l] * ratio + _motion_params.stMotion.uvWeight          [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[0] = (_motion_params.stMotion.yuvnrGainScale0   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale0   [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[1] = (_motion_params.stMotion.yuvnrGainScale1   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale1   [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[2] = (_motion_params.stMotion.yuvnrGainScale2   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale2   [gain_r] * (1 - ratio));
    stmtParamsSelected->frame_limit_y       = (_motion_params.stMotion.frame_limit_y     [gain_l] * ratio + _motion_params.stMotion.frame_limit_y     [gain_r] * (1 - ratio));
    stmtParamsSelected->frame_limit_uv      = (_motion_params.stMotion.frame_limit_uv    [gain_l] * ratio + _motion_params.stMotion.frame_limit_uv    [gain_r] * (1 - ratio));
    stmtParamsSelected->mfnr_sigma_scale    = (_motion_params.stMotion.mfnrSigmaScale    [gain_l] * ratio + _motion_params.stMotion.mfnrSigmaScale    [gain_r] * (1 - ratio));

    stmtParamsSelected->gain_scale_l_y      = (_motion_params.stMotion.reserved7         [gain_l] * ratio + _motion_params.stMotion.reserved7          [gain_r] * (1 - ratio));
    stmtParamsSelected->gain_scale_l_uv     = (_motion_params.stMotion.reserved6         [gain_l] * ratio + _motion_params.stMotion.reserved6          [gain_r] * (1 - ratio));
    stmtParamsSelected->gain_scale_h_y      = (_motion_params.stMotion.reserved5         [gain_l] * ratio + _motion_params.stMotion.reserved5          [gain_r] * (1 - ratio));
    stmtParamsSelected->gain_scale_h_uv     = (_motion_params.stMotion.reserved4         [gain_l] * ratio + _motion_params.stMotion.reserved4          [gain_r] * (1 - ratio));
    stmtParamsSelected->motion_dn_str	    = (_motion_params.stMotion.reserved3         [gain_l] * ratio + _motion_params.stMotion.reserved3          [gain_r] * (1 - ratio));
    if(stmtParamsSelected->mfnr_sigma_scale > 0)
        static_ratio_r_bit = static_ratio_l_bit - ceil(log2(stmtParamsSelected->mfnr_sigma_scale)) - ceil(log2(stmtParamsSelected->motion_dn_str));
    else
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "stmtParamsSelected->mfnr_sigma_scale %d is out of range\n", stmtParamsSelected->mfnr_sigma_scale);

    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "selected:gain_r %d gain_l:%d iso %d ratio %f, %f,%f,%f,%f, %f,%f,%f, %f %f %6f %6f  %6f %6f %6f %6f r_bit %d\n",gain_r,gain_l,iso,
        ratio, stmtParamsSelected->sigmaHScale, stmtParamsSelected->sigmaLScale ,
        stmtParamsSelected->light_clp, stmtParamsSelected->uv_weight,stmtParamsSelected->mfnr_sigma_scale,
        stmtParamsSelected->yuvnr_gain_scale[0],stmtParamsSelected->yuvnr_gain_scale[1],stmtParamsSelected->yuvnr_gain_scale[2], mtParamsSelect.frame_limit_y,
        mtParamsSelect.frame_limit_uv,  stmtParamsSelected->gain_scale_l_y, stmtParamsSelected->gain_scale_l_uv, stmtParamsSelected->gain_scale_h_y, stmtParamsSelected->gain_scale_h_uv,
        stmtParamsSelected->motion_dn_str, static_ratio_r_bit);
    stmtParamsSelected->gain_ratio          = _motion_params.gain_ratio;

    return XCAM_RETURN_NO_ERROR;
}


int get_wr_flg_func(int framenum, int pp_flg)
{

    if(!pp_flg && framenum == 0)
    {
        if(WRITE_FLG)
        {
            write_frame_num         = 2;
            frame_write_st          = 300;
            printf("%s WRITE_FLG 1\n", __func__);
        }
        else
        {
            write_frame_num         = 0;
            frame_write_st          = -1;
        }
    }

    if(!pp_flg && frame_write_st == -1)
    {
        int fp = -1;
        int write_flg = 0;
        const char *delim   = " ";
        char buffer[16]     = {0};
        char *name          = name_wr_flg;
        if (access(name,F_OK)==0) {
            printf("%s WRITE_FLG 21\n", __func__);
            fp = open(name, O_RDONLY | O_SYNC);
            printf("%s access ! framenum %d\n", __func__, framenum);
            if (read(fp, buffer, sizeof(buffer)) <= 0) {
                printf("%s read %s fail! empty\n", __func__, name);
                write_frame_num             = 0;
                write_flg                   = 0;
                remove(name);
            } else {
                char *p = NULL;
                p = strtok(buffer, delim);
                if (p != NULL) {
                    int value = atoi(p);
                    if(value < 0 || value > 30)
                    {
                        printf("%s read framenum %d failed! framenum %d \n", __func__, value, framenum);
                        write_flg           = 0;
                        write_frame_num     = 0;
                        remove(name);
                    }
                    else
                    {
                        printf("%s read  success value %d !framenum %d\n", __func__, value, framenum);
                        write_flg           = 1;
                        write_frame_num     = value;
                    }
                }
            }
            close(fp);
        }
        else
        {
            write_flg                       = 0;
            write_frame_num                 = 0;
        }
        if(write_flg)
            frame_write_st                  = framenum + 2;
    }
    int write_flg_cur;
    if((frame_write_st != -1) && ((framenum >= frame_write_st) && (framenum < frame_write_st + write_frame_num)))
        write_flg_cur                       = 1;
    else
        write_flg_cur                       = 0;


    return write_flg_cur;
}
void set_wr_flg_func(int framenum)
{
    char *name = name_wr_flg;
    if((frame_write_st != -1) && (framenum > frame_write_st + write_frame_num))
    {
        if (access(name,F_OK)==0)
        {
            printf("%s remove /tmp/motion_detection_wr_flg name %s frame_write_st %d write_frame_num %d framenum %d\n", __func__, name, frame_write_st, write_frame_num,framenum);
            remove(name);
            frame_write_st              = -1;
            write_frame_num             = 0;
        }

    }
}


int get_wr_other_flg_func()
{




    int write_other_flg = 0;
    char *name          = name_wr_other_flg;
    if (access(name,F_OK)==0)
    {

        write_other_flg                 = 1;
    }
    else
    {
        write_other_flg                 = 0;
    }

    return write_other_flg;
}
Isp20SpThread::Isp20SpThread ()
    : Thread("motionDetectThread")
{
    mKgThread = new KgProcThread(this);
    mWrThread = new WrProcThread(this);
    mWrThread2 = new WrProcThread2(this);
    _img_width = 0;
    _img_height = 0;
    _working_mode = RK_AIQ_WORKING_MODE_NORMAL;
}

Isp20SpThread::~Isp20SpThread()
{
}

void
Isp20SpThread::set_calibDb(const CamCalibDbContext_t* calib) {
    _calibDb = calib;
}

void
Isp20SpThread::start()
{
    SmartPtr<LensHw> lensHw = _focus_dev.dynamic_cast_ptr<LensHw>();

	LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s", RK_AIQ_MOTION_DETECTION_VERSION);
    init();
    subscrible_ispgain_event(true);
    if (create_stop_fds_ispsp()) {
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM,  "create ispsp stop fds failed !");
        return;
    }

    xcam_mem_clear(_lens_des);
    if (lensHw.ptr())
        lensHw->getLensModeData(_lens_des);

	pthread_attr_t &attr = get_pthread_attr();
	pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	struct sched_param  param;
	param.sched_priority = 99;
	pthread_attr_setschedparam(&attr, &param);

    mKgThread->start();
    mWrThread->start();
    mWrThread2->start();
    Thread::start();
    struct rkispp_trigger_mode tnr_trigger;
    tnr_trigger.module = ISPP_MODULE_TNR;
    tnr_trigger.on = 1;
    int ret = _ispp_dev->io_control(RKISPP_CMD_TRIGGER_MODE, &tnr_trigger);
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "start tnr process,ret=%d",ret);
}

void
Isp20SpThread::stop()
{
    struct rkispp_trigger_mode tnr_trigger;
    tnr_trigger.module = ISPP_MODULE_TNR;
    tnr_trigger.on = 0;
    int ret = _ispp_dev->io_control(RKISPP_CMD_TRIGGER_MODE, &tnr_trigger);
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "stop tnr process,ret=%d",ret);
    notify_stop_fds_exit();
    Thread::stop();
    mKgThread->stop();
    notify_wr_thread_exit();
    mWrThread->stop();
    notify_wr2_thread_exit();
    mWrThread2->stop();
    destroy_stop_fds_ispsp();
    subscrible_ispgain_event(false);
    deinit();
    for (int i=0; i<_isp_buf_num; i++)
        ::close(_isp_fd_array[i]);

    for (int i=0; i<_ispp_buf_num; i++)
        ::close(_ispp_fd_array[i]);
}

void
Isp20SpThread::pause()
{
}

void
Isp20SpThread::resume()
{
}

int
Isp20SpThread::subscrible_ispgain_event(bool on) {
  struct v4l2_event_subscription sub;
  int ret = 0;

  memset(&sub, 0, sizeof(sub));
  sub.type = RKISPP_V4L2_EVENT_TNR_COMPLETE;
  if (on)
      ret = _ispp_dev->io_control(VIDIOC_SUBSCRIBE_EVENT, &sub);
  else
      ret = _ispp_dev->io_control(VIDIOC_UNSUBSCRIBE_EVENT, &sub);
  if (ret) {
      LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "can't subscribe TNR complete event!\n");
  }
  return 0;
}

int
Isp20SpThread::wait_ispgain_event(unsigned int event_type, struct v4l2_event *event) {
  int ret;

  memset(event, 0, sizeof(*event));
  do {
    /*
     * xioctl instead of poll.
     * Since poll() cannot wait for input before stream on,
     * it will return an error directly. So, use ioctl to
     * dequeue event and block until sucess.
     */
    ret = _ispp_dev->io_control(VIDIOC_DQEVENT, event);
    if (ret == 0 && event->type == event_type) {
        return 0;
    }
  } while (true);

  return -1;
}

bool Isp20SpThread::init_fbcbuf_fd()
{
    struct isp2x_buf_idxfd ispbuf_fd;
    int res = -1;

    memset(&ispbuf_fd, 0, sizeof(ispbuf_fd));
    res = _isp_dev->io_control(RKISP_CMD_GET_FBCBUF_FD , &ispbuf_fd);
    if (res)
        return false;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "ispbuf_num=%d",ispbuf_fd.buf_num);
    for (uint32_t i=0; i<ispbuf_fd.buf_num; i++) {
        if (ispbuf_fd.dmafd[i] < 0) {
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "fbcbuf_fd[%u]:%d is illegal!",ispbuf_fd.index[i],ispbuf_fd.dmafd[i]);
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }
        _isp_fd_array[i] = ispbuf_fd.dmafd[i];
        _isp_idx_array[i] = ispbuf_fd.index[i];
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "fbcbuf_fd[%u]:%d",ispbuf_fd.index[i],ispbuf_fd.dmafd[i]);
    }
    _isp_buf_num = ispbuf_fd.buf_num;
    return true;
}

bool Isp20SpThread::init_tnrbuf_fd()
{
    struct rkispp_buf_idxfd isppbuf_fd;
    int res = -1;

    memset(&isppbuf_fd, 0, sizeof(isppbuf_fd));
    res = _ispp_dev->io_control(RKISPP_CMD_GET_TNRBUF_FD , &isppbuf_fd);
    if (res)
        return false;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "isppbuf_num=%d",isppbuf_fd.buf_num);
    for (uint32_t i=0; i<isppbuf_fd.buf_num; i++) {
        if (isppbuf_fd.dmafd[i] < 0) {
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "tnrbuf_fd[%u]:%d is illegal!",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }
        _ispp_fd_array[i] = isppbuf_fd.dmafd[i];
        _ispp_idx_array[i] = isppbuf_fd.index[i];
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "tnrbuf_fd[%u]:%d",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
    }
    _ispp_buf_num = isppbuf_fd.buf_num;
    return true;
}

XCamReturn
Isp20SpThread::kg_proc_loop ()
{
    int poll_ret = 0;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    poll_ret = _ispp_dev->poll_event(1000, ispp_stop_fds[0]);
    if (poll_ret == POLL_STOP_RET) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM, "poll ispp stop sucess !");
        // stop success, return error to stop the poll thread
        return XCAM_RETURN_ERROR_UNKNOWN;
    }
    if (poll_ret <= 0) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM,  "ispp poll buffer event got error(%d) but continue\n", poll_ret);
        return XCAM_RETURN_ERROR_TIMEOUT;
    }

    wait_ispgain_event(RKISPP_V4L2_EVENT_TNR_COMPLETE, &event);

	struct rkispp_tnr_inf *tnr_inf = (struct rkispp_tnr_inf *)&event.u.data;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "kg_loop frame_num_pp %d flg %d\n", frame_num_pp, frame_detect_flg[static_ratio_idx_out]);
    int kg_fd = -1, wr_fd = -1;

    if(frame_detect_flg[static_ratio_idx_out] && _calibDb->mfnr.enable && _calibDb->mfnr.motion_detect_en)
    {

        for (int i=0; i<_ispp_buf_num; i++) {
            if (tnr_inf->gainkg_idx == _ispp_idx_array[i]) {
                kg_fd = _ispp_fd_array[i];
            }
            if (tnr_inf->gainwr_idx == _ispp_idx_array[i]) {
               wr_fd = _ispp_fd_array[i];
            }
        }
        {
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "send MSG_CMD_WR_START,frameid=%u", tnr_inf->frame_id);
            SmartPtr<sp_msg_t> msg = new sp_msg_t();
            msg->cmd = MSG_CMD_WR_START;
            msg->sync = false;
            msg->arg1 = static_ratio_idx_out;
            msg->arg2 = tnr_inf;
            msg->arg3 = wr_fd;
            notify_yg_cmd(msg);
        }

    }else {
        _ispp_dev->io_control(RKISPP_CMD_TRIGGER_YNRRUN, tnr_inf);
    }

    do {
        _buf_list_mutex.lock();
        if (_isp_buf_list.empty()) {
            _buf_list_mutex.unlock();
            usleep(1000);
            if (!is_running())
                break;
        } else {
            _buf_list_mutex.unlock();
            if (frame_num_isp <= frame_num_pp) {
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "frame_num_isp(%d) should be greater than frame_num_pp(%d)!", frame_num_isp, frame_num_pp);
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                assert(0);
            }

            if(frame_detect_flg[static_ratio_idx_out])
            {
                int static_ratio_idx_out_plus1  = (static_ratio_idx_out + 1) % static_ratio_num;
                uint8_t* ratio                  = static_ratio[static_ratio_idx_out];
                uint8_t* ratio_next             = static_ratio[static_ratio_idx_out_plus1];

                #if DEBUG_TIMESTAMP
                struct timeval tv0, tv1, tv2, tv3;
                gettimeofday(&tv0, NULL);
                #endif
                void *gainkg_addr = mmap(NULL, tnr_inf->gainkg_size, PROT_READ | PROT_WRITE, MAP_SHARED, kg_fd, 0);
                if (MAP_FAILED == gainkg_addr) {
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gainkg_fd failed!!!(errno=%d),fd: %d, idx:%u, size: %d", errno, kg_fd, tnr_inf->gainkg_idx, tnr_inf->gainkg_size);
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                    assert(0);
                }

                #if DEBUG_TIMESTAMP
                gettimeofday(&tv1, NULL);
                #endif
                set_gainkg(gainkg_addr,     ratio, ratio_next);
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv2, NULL);
                #endif
                munmap(gainkg_addr, tnr_inf->gainkg_size);
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv3, NULL);
                #endif
                #if DEBUG_TIMESTAMP
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_kg idx %d %d fid %u %8ld %8ld %8ld %8ld  delta %8ld %8ld %8ld  \n", static_ratio_idx_out, static_ratio_idx_out_plus1, tnr_inf->frame_id,
                    tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec,  tv1.tv_usec - tv0.tv_usec,
                    tv2.tv_usec - tv1.tv_usec, tv3.tv_usec - tv2.tv_usec  );
                #endif
            }
            set_wr_flg_func(frame_num_pp);
            static_ratio_idx_out++;
            static_ratio_idx_out    %=static_ratio_num;
            frame_id_pp_upt         = tnr_inf->frame_id;
            frame_num_pp++;

            _buf_list_mutex.lock();
            LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "v4l2buf index %d pop list\n", _isp_buf_list.front()->get_v4l2_buf_index());
            _isp_buf_list.pop_front();//feed new frame to tnr
            _buf_list_mutex.unlock();
            break;
        }
    } while (1);

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

bool
Isp20SpThread::wr_proc_loop ()
{
    SmartPtr<sp_msg_t> msg;
    void *gainwr_addr = NULL;
    uint32_t ratio_idx;
    struct rkispp_tnr_inf *tnr_info = NULL;
    struct timeval tv0, tv1, tv2, tv3, tv4, tv5, tv6;
    bool loop_live = true;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    while (loop_live) {
        msg = _notifyYgCmdQ.pop(1000);
        if (!msg.ptr())
            continue;
        switch(msg->cmd)
        {
            case MSG_CMD_WR_START:
            {
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "MSG_CMD_WR_START received");
                ratio_idx = msg->arg1;
                tnr_info = (struct rkispp_tnr_inf *)msg->arg2;
                int wr_fd = msg->arg3;
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv0, NULL);
                #endif
                gainwr_addr = mmap(NULL, tnr_info->gainwr_size, PROT_READ | PROT_WRITE, MAP_SHARED, wr_fd, 0);
                if (MAP_FAILED == gainwr_addr) {
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gainwr_fd failed!!!(errno=%d),fd: %d, size: %d", errno, wr_fd, tnr_info->gainwr_size);
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                    assert(0);
                }

                #if DEBUG_TIMESTAMP
                gettimeofday(&tv1, NULL);
                #endif
                if (static_ratio[ratio_idx] == NULL) {
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "ratio_idx=%d",ratio_idx);
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                    assert(0);
                }



                int wr_flg                          = get_wr_flg_func(frame_num_pp, 1);
                int wr_other_flg                    = get_wr_other_flg_func();
                wr_flg &= wr_other_flg;

                {
                    static FILE *fd_gain_yuvnr_wr   = NULL;
                    if(wr_flg)
                    {
                        if(fd_gain_yuvnr_wr == NULL)
                            fd_gain_yuvnr_wr            = fopen("/tmp/gain_pp_out.yuv", "wb");
                        if(fd_gain_yuvnr_wr)
                        {
                            fwrite(gainwr_addr, gain_blk_ispp_stride * gain_blk_ispp_h * 2, 1, fd_gain_yuvnr_wr);
                            fflush(fd_gain_yuvnr_wr);
                        }

                    }
                    else
                    {
                        fd_gain_yuvnr_wr                = NULL;
                    }
                }



                {
                    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "send MSG_CMD_WR_START2");
                    SmartPtr<sp_msg_t> msg = new sp_msg_t();
                    msg->cmd = MSG_CMD_WR_START;
                    msg->sync = false;
                    msg->arg1 = ratio_idx;
                    msg->arg2 = gainwr_addr;
                    notify_yg2_cmd(msg);
                }

                uint8_t *gain_isp_buf_cur                   = gain_isp_buf_bak[static_ratio_idx_out];
                uint8_t* ratio                              = static_ratio[ratio_idx];


                set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, 0,                    gain_blk_ispp_h / 2);
                //set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, gain_blk_ispp_h / 2,  gain_blk_ispp_h);

                {
                    static FILE *fd_gain_yuvnr_up_wr        = NULL;

                    if(wr_flg)
                    {

                        if(fd_gain_yuvnr_up_wr==NULL)
                            fd_gain_yuvnr_up_wr                 = fopen("/tmp/gain_pp_up_out.yuv", "wb");
                        if(fd_gain_yuvnr_up_wr)
                        {
                            fwrite(gainwr_addr, gain_blk_ispp_stride * gain_blk_ispp_h * 2, 1, fd_gain_yuvnr_up_wr);
                            fflush(fd_gain_yuvnr_up_wr);
                        }

                    }
                    else
                    {
                        fd_gain_yuvnr_up_wr        = NULL;
                    }
                }


                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM,"set_gain_wr top done");
                char ch;
                read(sync_pipe_fd[0], &ch, 1);//blocked
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv2, NULL);
                #endif
                munmap(gainwr_addr, tnr_info->gainwr_size);
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv3, NULL);
                #endif
                _ispp_dev->io_control(RKISPP_CMD_TRIGGER_YNRRUN, tnr_info);
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv4, NULL);
                #endif
                #if DEBUG_TIMESTAMP
        	    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr fid %u %8ld %8ld %8ld %8ld %8ld delta %8ld %8ld %8ld %8ld \n", tnr_info->frame_id,
        	        tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec, tv4.tv_usec, tv1.tv_usec - tv0.tv_usec,
        	        tv2.tv_usec - tv1.tv_usec, tv3.tv_usec - tv2.tv_usec, tv4.tv_usec - tv3.tv_usec  );
                #endif
                break;
            }
            case MSG_CMD_WR_EXIT:
            {
                if (msg->sync) {
                    msg->mutex->lock();
                    msg->cond->broadcast ();
                    msg->mutex->unlock();
                }
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s: wr_proc_loop exit", __FUNCTION__);
                loop_live = false;
                break;
            }
        }
    }
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return false;
}

int Isp20SpThread::get_lowpass_fv(uint32_t sequence, SmartPtr<V4l2BufferProxy> buf_proxy)
{
    SmartPtr<LensHw> lensHw = _focus_dev.dynamic_cast_ptr<LensHw>();
    uint8_t *image_buf = (uint8_t *)buf_proxy->get_v4l2_planar_userptr(0);
    rk_aiq_af_algo_meas_t meas_param;

    _afmeas_param_mutex.lock();
    meas_param = _af_meas_params;
    _afmeas_param_mutex.unlock();

    if (meas_param.sp_meas.enable) {
        get_lpfv(sequence, image_buf, _img_width, _img_height,
            _img_width_align, _img_height_align, pAfTmp, sub_shp4_4,
            sub_shp8_8, high_light, high_light2, &meas_param);

        lensHw->setLowPassFv(sub_shp4_4, sub_shp8_8, high_light, high_light2, sequence);
    }

    return 0;
}


bool
Isp20SpThread::wr_proc_loop2 ()
{
    SmartPtr<sp_msg_t> msg;
    void *gainwr_addr = NULL;
    uint32_t ratio_idx;
    struct timeval tv0, tv1;
    bool loop_live = true;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    while (loop_live) {
        msg = _notifyYgCmdQ2.pop(500);
        if (!msg.ptr())
            continue;
        switch(msg->cmd)
        {
            case MSG_CMD_WR_START:
            {
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "MSG_CMD_WR_START2 received");
                ratio_idx = msg->arg1;
                gainwr_addr = msg->arg2;
                uint8_t *gain_isp_buf_cur                   = gain_isp_buf_bak[static_ratio_idx_out];
                uint8_t* ratio                              = static_ratio[ratio_idx];
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv0, NULL);
                #endif
                //set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, 0,                    gain_blk_ispp_h / 2);
                set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, gain_blk_ispp_h / 2,  gain_blk_ispp_h);
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM,"set_gain_wr bottom done");
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv1, NULL);
                #endif
                #if DEBUG_TIMESTAMP
        	    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr2 %8ld \n", tv1.tv_usec - tv0.tv_usec);
                #endif
                char ch = 0x1;//whatever
                write(sync_pipe_fd[1], &ch, 1);//nonblock
                break;
            }
            case MSG_CMD_WR_EXIT:
            {
                if (msg->sync) {
                    msg->mutex->lock();
                    msg->cond->broadcast ();
                    msg->mutex->unlock();
                }
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s: wr_proc_loop2 exit", __FUNCTION__);
                loop_live = false;
                break;
            }
        }
    }
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return false;
}


bool
Isp20SpThread::loop () {
    SmartPtr<V4l2Buffer> buf;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    int poll_ret;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    if (_fd_init_flag) {
        if (!_isp_buf_num && !init_fbcbuf_fd()) {
            usleep(1000);
            return true;
        }
        if (!_ispp_buf_num && !init_tnrbuf_fd()) {
            usleep(1000);
            return true;
        }
        thread_bind_cpu(3);
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "isp&ispp buf fd init success!");
        _fd_init_flag = false;
    }

    poll_ret = _isp_sp_dev->poll_event(1000, ispsp_stop_fds[0]);
    if (poll_ret == POLL_STOP_RET) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM, "poll isp sp stop sucess !");
        // stop success, return error to stop the poll thread
        return false;
    }

    if (poll_ret <= 0) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM,  "isp sp poll buffer event got error(%d) but continue\n", poll_ret);
        return true;
    }

    ret = _isp_sp_dev->dequeue_buffer (buf);
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "dequeue isp_sp_dev buffer failed");
        return false;
    }

    SmartPtr<V4l2BufferProxy> buf_proxy         = new V4l2BufferProxy(buf, _isp_sp_dev);
    uint8_t *image_buf                          = (uint8_t *)buf_proxy->get_v4l2_planar_userptr(0);
	unsigned long long image_ts, ispgain_ts, mfbc_ts;
	image_ts                                    = *(unsigned long long*)(image_buf + buf_proxy->get_v4l2_buf_planar_length(0) - 8);
	struct isp2x_ispgain_buf *ispgain           = (struct isp2x_ispgain_buf *)buf_proxy->get_v4l2_planar_userptr(1);

    select_motion_params(&mtParamsSelect, ispgain->frame_id);

    int detect_flg                      = mtParamsSelect.enable;
    int detect_flg_last                 = frame_detect_flg[(static_ratio_idx_in + static_ratio_num - 1) % static_ratio_num];

    while(frame_id_isp_upt != -1 && frame_id_isp_upt != frame_id_pp_upt && static_ratio_idx_out == static_ratio_idx_in)
    {
        if (!is_running())
            break;
        usleep(1000);
    }

    if (_calibDb->af.ldg_param.enable)
        get_lowpass_fv(ispgain->frame_id, buf_proxy);

    uint8_t *static_ratio_cur                   = static_ratio[static_ratio_idx_in];
    if(detect_flg && _calibDb->mfnr.enable && _calibDb->mfnr.motion_detect_en)
    {
        int wr_flg = get_wr_flg_func(frame_num_isp, 0);
        int wr_other_flg    = get_wr_other_flg_func();
        if(1)
        {
            struct timeval tv0, tv1, tv2, tv3, tv4, tv5, tv6,tva,tvb;
            gettimeofday(&tv0, NULL);
            int gain_fd = -1, mfbc_fd = -1;

            for (int i=0; i<_isp_buf_num; i++) {
                if (ispgain->gain_dmaidx == _isp_idx_array[i]) {
                    gain_fd = _isp_fd_array[i];
                }
                if (ispgain->mfbc_dmaidx == _isp_idx_array[i]) {
                    mfbc_fd = _isp_fd_array[i];
                }
            }
            LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "loop gain_dmaidx %u gain_fd %d mfbc_dmaidx %u mfbc_fd %d\n", ispgain->gain_dmaidx, gain_fd, ispgain->mfbc_dmaidx, mfbc_fd);
            void *gain_addr                             = mmap(NULL, ispgain->gain_size, PROT_READ | PROT_WRITE, MAP_SHARED, gain_fd, 0);
            if (MAP_FAILED == gain_addr) {
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gain_dmafd failed!!!(errno:%d),fd: %d, idx: %u, size: %d", errno, gain_fd, ispgain->gain_dmaidx, ispgain->gain_size);
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                assert(0);
            }

            uint8_t *pCurIn                     = pImgbuf[static_ratio_idx_in];
            uint8_t *pPreIn                     = pImgbuf[(static_ratio_idx_in - 1 + static_ratio_num) % static_ratio_num];
#if DEBUG_TIMESTAMP
gettimeofday(&tva, NULL);
#endif
            //memcpy(pCurIn, image_buf, img_buf_size + img_buf_size_uv);
	        memcpy(pCurIn, image_buf, img_buf_size);
	        memcpy(pCurIn+img_buf_size, image_buf+ALIGN_UP(img_buf_size, 64), img_buf_size_uv);
#if DEBUG_TIMESTAMP
gettimeofday(&tvb, NULL);
#endif
            {
                static    FILE *fd_ds_wr                = NULL;
                static    FILE *fd_ratio_iir_out        = NULL;
                static    FILE *fd_ds_last_wr           = NULL;
                static    FILE *fd_param_out            = NULL;
                if(wr_flg)
                {
                    if(fd_ds_wr == NULL)
                        fd_ds_wr                        = fopen("/tmp/ds_out.yuv", "wb");
                    if(fd_ds_wr)
                    {
                        fwrite(pCurIn, img_buf_size + img_buf_size_uv,        1,   fd_ds_wr);
                        fflush(fd_ds_wr);
                    }
                    if(fd_ratio_iir_out == NULL)
                        fd_ratio_iir_out                = fopen("/tmp/ratio_iir_out.yuv", "wb");
                    if(fd_ratio_iir_out)
                    {
                        fwrite(pPreAlpha, ratio_stride * gain_kg_tile_h_align,        1,   fd_ratio_iir_out);
                        fflush(fd_ratio_iir_out);
                    }
                    if(fd_ds_last_wr == NULL)
                        fd_ds_last_wr                   = fopen("/tmp/ds_out_last.yuv", "wb");
                    if(fd_ds_last_wr)
                    {
                        fwrite(pPreIn, img_buf_size + img_buf_size_uv,        1,   fd_ds_last_wr);
                        fflush(fd_ds_last_wr);
                    }

                    if(fd_param_out == NULL)
                        fd_param_out                        = fopen("/tmp/param_out.yuv", "wb");
                    if(fd_param_out)
                    {
                        fwrite(&mtParamsSelect,     sizeof(mtParamsSelect)-sizeof(float), 1,   fd_param_out);
                        float gain_ratio_cur    = mtParamsSelect.gain_ratio;
                        fwrite(&gain_ratio_cur,    sizeof(float),          1,   fd_param_out);
                        float gain_ratio_last   = (*(mtParamsSelect_list[(static_ratio_idx_in + static_ratio_num - 1) % static_ratio_num])).gain_ratio;
                        fwrite(&gain_ratio_last,    sizeof(float),          1,   fd_param_out);
                        fflush(fd_param_out);
                    }


                }
                else
                {
                    fd_ds_wr                            = NULL;
                    fd_ratio_iir_out                    = NULL;
                    fd_ds_last_wr                       = NULL;
                    fd_param_out                            = NULL;
                }

                static    FILE *fd_gain_isp_wr          = NULL;


                if(wr_flg && wr_other_flg)
                {
                    if(fd_gain_isp_wr == NULL )
                        fd_gain_isp_wr                  = fopen("/tmp/gain_isp_out.yuv", "wb");
                    if(fd_gain_isp_wr > 0)
                    {
                        fwrite(gain_addr, gain_blk_isp_mem_size, 1,    fd_gain_isp_wr);
                        fflush(fd_gain_isp_wr);
                    }


                }
                else
                {
                    fd_gain_isp_wr                      = NULL;


                }
            }

	        if(detect_flg_last == 1)
            {
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv1, NULL);
                #endif
                static int wr_flg_last = 0;

                uint8_t *src = (uint8_t*)gain_addr;
                uint8_t *gain_isp_buf_cur           = gain_isp_buf_bak[static_ratio_idx_in];
                memcpy(gain_isp_buf_cur, src, gain_blk_isp_stride * gain_blk_isp_h);
                #if 1
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv2, NULL);
                #endif
                motion_detect(pCurIn, pPreIn, pTmpBuf, static_ratio_cur, pPreAlpha, (uint8_t*)src, _img_height_align, _img_width_align, _img_height, _img_width,
                        gain_blk_isp_stride, mtParamsSelect.sigmaHScale, mtParamsSelect.sigmaLScale,
                        mtParamsSelect.uv_weight, static_ratio_r_bit, wr_flg && wr_other_flg, wr_flg_last);
                #if DEBUG_TIMESTAMP
                gettimeofday(&tv3, NULL);
                #endif

                wr_flg_last         = wr_flg && wr_other_flg;
                #if DEBUG_TIMESTAMP
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_isp fid %u frame_write_st %d time %8ld %8ld %8ld %8ld delta %8ld %8ld %8ld %8ld %8ld %d %x\n",ispgain->frame_id,frame_write_st,
                          tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec, tv1.tv_usec - tv0.tv_usec, tv2.tv_usec - tv1.tv_usec,
                          tv3.tv_usec - tv2.tv_usec, tv3.tv_usec - tv0.tv_usec, tvb.tv_usec - tva.tv_usec, static_ratio_cur[0], ratio_stride * gain_kg_tile_h_align);
                #endif
               // memset(static_ratio_cur, 1<<7, gain_kg_tile_h_align * ratio_stride);

                #else
                if(frame_num_isp & 1)
                    memset(static_ratio_cur, 1<<7, gain_kg_tile_h_align * ratio_stride);
                else
                    memset(static_ratio_cur, 1<<7, gain_kg_tile_h_align * ratio_stride);

                    for(int i = 0 ;i < gain_blk_isp_stride * gain_blk_isp_h*4/5; i++)
                        src[i] = ROUND_INT((uint16_t)src[i] * static_ratio_cur[i], static_ratio_l_bit) ;
                #endif
            }
            else
                memset(static_ratio_cur, static_ratio_l, gain_kg_tile_h_align * ratio_stride);
            {
                static FILE *fd_ratio_wr        = NULL;
                if(wr_flg)
                {
                    if(fd_ratio_wr == NULL)
                        fd_ratio_wr             = fopen("/tmp/ratio_out.yuv", "wb");
                    if(fd_ratio_wr)
                    {
                        fwrite(static_ratio_cur, ratio_stride * gain_kg_tile_h_align, 1,    fd_ratio_wr);
                        fflush(fd_ratio_wr);
                    }
                }
                else
                {
                    fd_ratio_wr                 = NULL;
                }
                static FILE *fd_ratio_iir_out_out       = NULL;
                if(wr_flg && wr_other_flg)
                {
                    if(fd_ratio_iir_out_out == NULL)
                        fd_ratio_iir_out_out                = fopen("/tmp/ratio_iir_out_out.yuv", "wb");
                    if(fd_ratio_iir_out_out)
                    {
                        fwrite(pPreAlpha, ratio_stride * gain_kg_tile_h_align,        1,   fd_ratio_iir_out_out);
                        fflush(fd_ratio_iir_out_out);
                    }

                }
                else
                {
                    if(fd_ratio_iir_out_out)
                        fclose(fd_ratio_iir_out_out);
                    fd_ratio_iir_out_out = NULL;
                }
                static    FILE *fd_ds_wr    = NULL;
                static    FILE *fd_gain_isp_up_out = NULL;

                if(wr_flg && wr_other_flg)
                {
                    if(fd_ds_wr == NULL)
                        fd_ds_wr                = fopen("/tmp/ds_out_iir.yuv", "wb");
                    if(fd_ds_wr)
                    {
                        fwrite(pCurIn, img_buf_size + img_buf_size_uv,        1,   fd_ds_wr);
                        fflush(fd_ds_wr);
                    }

                    if(fd_gain_isp_up_out == NULL)
                        fd_gain_isp_up_out      = fopen("/tmp/gain_isp_up_out.yuv", "wb");
                    if(fd_gain_isp_up_out)
                    {
                        fwrite(gain_addr, gain_blk_isp_mem_size, 1,    fd_gain_isp_up_out);
                        fflush(fd_gain_isp_up_out);
                    }
                }
                else
                {
                    fd_ds_wr                    = NULL;
                    fd_gain_isp_up_out          = NULL;
                }

            }

	        munmap(gain_addr, ispgain->gain_size);
	    }
    }

    frame_detect_flg[static_ratio_idx_in]       = detect_flg;
    *(mtParamsSelect_list[static_ratio_idx_in]) = mtParamsSelect;
    frame_num_isp++;
    static_ratio_idx_in++;
	static_ratio_idx_in     %= static_ratio_num;
    frame_id_isp_upt        = ispgain->frame_id;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "loop frame_num_isp %d fid %u \n", frame_num_isp, ispgain->frame_id);

    if(_first_frame) {
        _first_frame = false;
        LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "first frame\n");
    }
    else {
        _buf_list_mutex.lock();
        LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "v4l2buf index %d push list\n", buf_proxy->get_v4l2_buf_index());
        _isp_buf_list.push_back(buf_proxy);
        _buf_list_mutex.unlock();
    }

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);

    return true;
}

void
Isp20SpThread::set_sp_dev(SmartPtr<V4l2SubDevice> ispdev, SmartPtr<V4l2Device> ispspdev, SmartPtr<V4l2SubDevice> isppdev, SmartPtr<V4l2SubDevice> snsdev, SmartPtr<V4l2SubDevice> lensdev)
{
    _isp_dev = ispdev;
    _isp_sp_dev = ispspdev;
    _ispp_dev = isppdev;
    _sensor_dev = snsdev;
    _focus_dev = lensdev;
}

void
Isp20SpThread::set_sp_img_size(int w, int h, int w_align, int h_align)
{
    _img_width          = w;
    _img_height         = h;
    _img_width_align    = w_align;
    _img_height_align   = h_align;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "_img_height %d %d _img_width %d %d\n", h, h_align, w, w_align);
    //assert(((w == (w_align - 1)) || (w == w_align)) && ((h == (h_align - 1)) || (h == h_align )));
}

void
Isp20SpThread::set_gain_isp(void *buf, uint8_t* ratio)
{}


void
Isp20SpThread::set_gain_wr(void *buf, uint8_t* ratio, uint8_t* gain_isp_buf_cur, uint16_t h_st, uint16_t h_end)
{
    RKAnr_Mt_Params_Select_t mtParamsSelect_cur     = *(mtParamsSelect_list[static_ratio_idx_out]);
	uint16_t yuvnr_gain_scale_fix[3];
    float yuvnr_gain_scale[3];
    uint8_t *src                            = (uint8_t*)buf;

	yuvnr_gain_scale[0]                     = mtParamsSelect_cur.yuvnr_gain_scale[0];
	yuvnr_gain_scale[1]                     = mtParamsSelect_cur.yuvnr_gain_scale[1];
	yuvnr_gain_scale[2]                     = mtParamsSelect_cur.yuvnr_gain_scale[2];


	yuvnr_gain_scale_fix[0]                 = ROUND_F(yuvnr_gain_scale[0]       * (1 << YUV_SCALE_FIX_BITS));
	yuvnr_gain_scale_fix[1]                 = ROUND_F(yuvnr_gain_scale[1]       * (1 << YUV_SCALE_FIX_BITS));
	yuvnr_gain_scale_fix[2]                 = ROUND_F(2 * yuvnr_gain_scale[2]   * (1 << YUV_SCALE_FIX_BITS));
    float coeff                             = yuvnr_gain_scale[2] * 2.0f;
    uint16_t ratio_static                   = (1 << static_ratio_l_bit) - 20;

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr frame_num_pp %d frame_write_st %d  write_frame_num %d\n ",frame_num_pp, frame_write_st, write_frame_num);
 //   printf("ratio_shf_bit %d mtParamsSelect.yuvnr_gain_scale %f %f %f\n", ratio_shf_bit, mtParamsSelect.yuvnr_gain_scale[0], mtParamsSelect.yuvnr_gain_scale[1], mtParamsSelect.yuvnr_gain_scale[2]);
#ifndef ENABLE_NEON
    for(int i = h_st; i < h_end; i++)
        for(int j = 0; j < gain_blk_ispp_stride; j++)
        {
            int idx_isp                     = i * gain_blk_isp_stride + j * 2;
            int idx_gain                    = (i * gain_blk_ispp_stride + j) * 2;
            int idx_ispp                    = (i * gain_blk_ispp_stride + j);
            int idx_ratio                   = i * ratio_stride + j * 2;
            uint16_t ratio_cur              = ratio[idx_ratio];
            uint16_t ratio_cur1             = ratio[idx_ratio + 1];
#if !RATIO_PP_FLG
            ratio_cur                       = MAX(ratio_cur, ratio_cur1);
#endif
            int gain_isp_cur                = MAX(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);


                uint16_t tmp0;
    			uint16_t tmp1;
                float rr[2];

				if(idx_gain==16)
					idx_gain=idx_gain;
        	    if(ratio_cur >  (1 << static_ratio_l_bit) - 20)
        	    {

    					if ((1.0f*src[idx_gain+1])/gain_isp_cur  > 1.3f/4)
    					{
    	        	        rr[0] 							= coeff;
    	        	        rr[1] 							= coeff;
    					}
    					else
    					{
    	        	        rr[0] 							= 1;
    	        	        rr[1] 							= 1;
    					}
        	    }
        	    else
        	    {
    	        rr[0] 							= yuvnr_gain_scale[0];
    	        rr[1] 							= yuvnr_gain_scale[1];



        	    }

				tmp0 							= (src[idx_gain]		* rr[0] * 256);
				tmp1 							= (src[idx_gain+1]	    * rr[1]* 256);

				tmp0 							= ROUND_INT(ROUND_F((tmp0 << static_ratio_l_bit)/ratio_cur), 8);
				tmp1 							= ROUND_INT(ROUND_F((tmp1 << static_ratio_l_bit)/ratio_cur), 8);

			src[idx_gain]     				= MIN(255, tmp0);
				src[idx_gain + 1] 				= MIN(255, tmp1);


            }
#else


    uint32_t test_fpscr;
//    __asm__("mov %[output], %[input]\n" : [output] "=r"(test_fpscr) : [input]  "r" (test_fpscr));
    __asm__("vmrs %[output], fpscr\n" : [output] "=r"(test_fpscr) );
    static int num = 0;
   // if((num%30)==0)
   //     printf("test_fpscr %x\n",test_fpscr);
    num++;
	int offsetX_last    = gain_blk_ispp_w - 8;
    int prefetch_num    = (ratio_stride  + 255) / 256;
  //  for(int k = 0;k<20;k++)

	for(int i = h_st; i < h_end; i++)
		{
			int offsetX = 0;
			int idx_isp             = 0;
			int idx_gain		    = 0;
			int idx_ratio		    = 0;
			uint8_t	*pGainIsp00		= gain_isp_buf_cur  + i * gain_blk_isp_stride;
			uint8_t	*pSrc00		    = src               + i * gain_blk_ispp_stride * 2;
			uint8_t	*pRatio00	    = ratio             + i * ratio_stride;
			uint8x8x2_t				vSrc00;
			uint8x8x2_t				vGainIsp00;
			uint8x8x2_t				vRatio_u8;


			for(int j = 0; j < gain_blk_ispp_stride ; j += 8)
			{
				offsetX			        = j;

				if(j + 8 > gain_blk_ispp_stride)
				{
					offsetX 	        = offsetX_last;
				}




				uint16x8x2_t			vSrc00_u16;




				uint16x8_t			    vsrc_cmp_l;
				uint16x8_t  			vsrc_cmp_r;
				uint16x8_t				vFlag00_u16, vFlag01_u16;
				uint16x8x2_t			vRR00;
				uint16x4x2_t			vRR00_0;
				uint16x4x2_t			vRR00_1;



    			vSrc00				    = vld2_u8(pSrc00        + idx_isp);
    			vGainIsp00				= vld2_u8(pGainIsp00    + idx_gain);
    			vRatio_u8			    = vld2_u8(pRatio00      + idx_ratio);
				vSrc00_u16.val[0]		= vmovl_u8(vSrc00.val[0]);
				vSrc00_u16.val[1]		= vmovl_u8(vSrc00.val[1]);

				vGainIsp00.val[0]		= vmax_u8(vGainIsp00.val[0],            vGainIsp00.val[1]);
				vRatio_u8.val[0]		= vmax_u8(vRatio_u8.val[0],             vRatio_u8.val[1]);





				vsrc_cmp_l	            = vmulq_n_u16(vmovl_u8(vSrc00.val[1]),        40);
				vsrc_cmp_r      	    = vmulq_n_u16(vmovl_u8(vGainIsp00.val[0]),    13);
				// (1.0f*src[idx_gain+1])/gain_isp_cur  > 13.0/40
				vFlag00_u16				= vcgtq_u16(vsrc_cmp_r,                 vsrc_cmp_l);
				// ratio_cur >  (1 << static_ratio_l_bit) - 20
				vFlag01_u16			    = vcgtq_u16(vmovl_u8(vRatio_u8.val[0]), vdupq_n_u16(ratio_static));
				vRR00.val[1]			= vbslq_u16(vFlag00_u16,                vdupq_n_u16(1 << YUV_SCALE_FIX_BITS),   vdupq_n_u16(yuvnr_gain_scale_fix[2]));

				vRR00.val[0]			= vbslq_u16(vFlag01_u16,                vRR00.val[1],                           vdupq_n_u16(yuvnr_gain_scale_fix[0]));
				vRR00.val[1]			= vbslq_u16(vFlag01_u16,                vRR00.val[1],                           vdupq_n_u16(yuvnr_gain_scale_fix[1]));


				// tmp0 				= (src[idx_gain]     * rr) << static_ratio_l_bit;
				vSrc00_u16.val[0]		= vmulq_u16(vSrc00_u16.val[0],          vRR00.val[0]);
				vSrc00_u16.val[1]		= vmulq_u16(vSrc00_u16.val[1],          vRR00.val[1]);


                float32x4x2_t			vSrc_l_f32, vSrc_h_f32;
                //0 1 low 2 3 high
                vSrc_l_f32.val[0]       = vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vSrc00_u16.val[0])));
                vSrc_l_f32.val[1]       = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vSrc00_u16.val[0])));
                vSrc_h_f32.val[0]       = vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vSrc00_u16.val[1])));
                vSrc_h_f32.val[1]       = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vSrc00_u16.val[1])));

				///////////////////////////////////////////////////////////////////////////////////////////////
				// tmp0 				= (tmp0 << static_ratio_l_bit)/ratio_cur;
				// reciprocal
				float32x4x2_t				vRatio_f32_4;
				vRatio_f32_4.val[0]		= vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vmovl_u8(vRatio_u8.val[0]))));
				vRatio_f32_4.val[1]	    = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vmovl_u8(vRatio_u8.val[0]))));



				float32x4x2_t				reciprocal_vRatio;
				reciprocal_vRatio.val[0]= vrecpeq_f32(vRatio_f32_4.val[0]);
				reciprocal_vRatio.val[1]= vrecpeq_f32(vRatio_f32_4.val[1]);

				reciprocal_vRatio.val[0]= vmulq_f32(vrecpsq_f32(vRatio_f32_4.val[0], reciprocal_vRatio.val[0]), reciprocal_vRatio.val[0]);
				reciprocal_vRatio.val[1]= vmulq_f32(vrecpsq_f32(vRatio_f32_4.val[1], reciprocal_vRatio.val[1]), reciprocal_vRatio.val[1]);



				// multiply ,1 for rounding
				uint32x4_t				vOut00_lo, vOut00_hi, vOut01_lo, vOut01_hi;
				vOut00_lo	            = vcvtq_n_u32_f32(vmulq_f32(vSrc_l_f32.val[0], reciprocal_vRatio.val[0]), 1);
				vOut00_hi	            = vcvtq_n_u32_f32(vmulq_f32(vSrc_h_f32.val[0], reciprocal_vRatio.val[0]), 1);

				vOut01_lo	            = vcvtq_n_u32_f32(vmulq_f32(vSrc_l_f32.val[1], reciprocal_vRatio.val[1]), 1);
				vOut01_hi	            = vcvtq_n_u32_f32(vmulq_f32(vSrc_h_f32.val[1], reciprocal_vRatio.val[1]), 1);

    			uint16x8_t              vOut_lo, vOut_hi;
				uint8x8x2_t				vOut00;
                vOut_lo                 = vcombine_u16(vmovn_u32(vOut00_lo), vmovn_u32(vOut01_lo));
                vOut_hi                 = vcombine_u16(vmovn_u32(vOut00_hi), vmovn_u32(vOut01_hi));
				vOut00.val[0]		    = vqrshrn_n_u16(vOut_lo, YUV_SCALE_FIX_BITS - RATIO_BITS_NUM+ 1);
				vOut00.val[1]			= vqrshrn_n_u16(vOut_hi, YUV_SCALE_FIX_BITS - RATIO_BITS_NUM+ 1);

				vst2_u8(pSrc00 + idx_isp, vOut00);

				idx_isp     +=16;
				idx_gain    +=16;
				idx_ratio   +=16;



		}
		}

#endif

     //   printf("buf[0] %d\n",src[0]);






}






void
Isp20SpThread::set_gainkg(void *buf, uint8_t* ratio, uint8_t* ratio_next)
{


    uint8_t *src                            = (uint8_t *)buf;
    int tile_size                           = (gain_tile_ispp_y * gain_tile_ispp_x * 8);
    int dst_stride                          = 2;

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_kg frame_num_pp %d frame_write_st %d  write_frame_num %d\n ",frame_num_pp, frame_write_st, write_frame_num);

    int wr_flg                              = get_wr_flg_func(frame_num_pp, 1);
    int wr_other_flg                        = get_wr_other_flg_func();
    wr_flg &= wr_other_flg;
    uint8_t *src_ori = NULL;
    uint8_t *src_mid = NULL;
    uint8_t *test_buff[4];
    uint8_t *test_buff_mid[4];
    uint8_t *test_buff_mid1[4];
    uint8_t *test_buff_ori[4];
    uint8_t * ratio_in = NULL;
    uint8_t * ratio_out = NULL;
    uint8_t *gain_isp_buf_cur                       = gain_isp_buf_bak[static_ratio_idx_out];
    RKAnr_Mt_Params_Select_t mtParamsSelect_cur     = *(mtParamsSelect_list[static_ratio_idx_out]);
    RKAnr_Mt_Params_Select_t mtParamsSelect_last    = *(mtParamsSelect_list[(static_ratio_idx_out + static_ratio_num - 1) % static_ratio_num]);
	float gain_ratio_cur                            = mtParamsSelect_cur.gain_ratio;
	float gain_ratio_last                           = mtParamsSelect_last.gain_ratio;
    uint8_t ratio_shf_bit                           = static_ratio_l_bit;
    uint8_t ratio_shf_bit1                          = static_ratio_l_bit - 1;

    int8_t gain_ratio_shf_bits                     = (int8_t)(log2(gain_ratio_cur / gain_ratio_last) / 2);
    uint8_t gain_ratio_shf_bits_abs                 = abs(gain_ratio_shf_bits);
    #if 0
    static int framenum1=0;

    if(wr_flg)
    {
        if(framenum1 ==0)
        {
            gain_ratio_cur = 1;
            gain_ratio_last = 1;
        }
        else if(framenum1 ==1)
        {

            gain_ratio_cur = 1;
            gain_ratio_last = 16;
        }
        else if(framenum1 ==2)
        {

            gain_ratio_cur = 1;
            gain_ratio_last = 1.0/16;
        }
        else if(framenum1 ==3)
        {

            gain_ratio_cur = 16;
            gain_ratio_last = 1;
        }
        else if(framenum1 ==4)
        {

            gain_ratio_cur = 16;
            gain_ratio_last = 1.0/16;
        }
        else if(framenum1 ==5)
        {

            gain_ratio_cur = 1.0/16;
            gain_ratio_last = 1;
        }
        else if(framenum1 ==6)
        {

            gain_ratio_cur = 1.0/16;
            gain_ratio_last = 16;
        }
        framenum1++;

    }
    else
        framenum1 = 0;
    #endif

    if(gain_ratio_last == -1)
    {
        gain_ratio_shf_bits                         = 0;
        gain_ratio_shf_bits_abs                     = 0;
    }
    else
    {
        gain_ratio_shf_bits                         = (int8_t)(log2(gain_ratio_cur / gain_ratio_last) / 2);
        gain_ratio_shf_bits_abs                     = abs(gain_ratio_shf_bits);
    }
    if(wr_flg)
    {
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_kg frame_num_pp %d frame_write_st %d  write_frame_num %d gain_ratio %f %f gain_ratio_shf_bits %d %f\n ",frame_num_pp, frame_write_st, write_frame_num,gain_ratio_cur, gain_ratio_last, gain_ratio_shf_bits,log2(gain_ratio_cur / gain_ratio_last) / 2);

        ratio_in                = (uint8_t*)malloc(ratio_stride      * gain_kg_tile_h_align      *    sizeof(ratio_in[0]));
        ratio_out               = (uint8_t*)malloc(ratio_stride      * gain_kg_tile_h_align      *    sizeof(ratio_in[0]));
        for(int i = 0; i < 4; i++)
        {
            test_buff[i]        = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff[0][0]) );
            test_buff_mid[i]    = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_mid[0][0]) );
            test_buff_mid1[i]   = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_mid1[0][0]) );
            test_buff_ori[i]    = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_ori[0][0]) );
        }
        src_mid                 = (uint8_t*)malloc(gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]) );
        memset(src_mid, 0, gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]));
        src_ori                 = (uint8_t*)malloc(gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]) );
        memcpy(src_ori, src, gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]));
    }
    float gain_scale_l_y            = mtParamsSelect_cur.gain_scale_l_y;
    int frame_limit_div_y           = 256 / sqrt(mtParamsSelect_cur.frame_limit_y);
    int frame_limit_div_uv          = 256 / sqrt(mtParamsSelect_cur.frame_limit_uv);
	uint8_t gain_min_val 			= 1;
    uint16_t ratio_r[4];
    ratio_r[0] =  (1 << 8) * gain_scale_l_y;
    ratio_r[1] =  (1 << 8) * gain_scale_l_y;
    ratio_r[2] =  (1 << 8) * gain_scale_l_y;
    ratio_r[3] =  (1 << 8) * gain_scale_l_y;
    for(int i = 0; i < gain_tile_gainkg_h; i++)

    {




		uint8_t block_h_cur = MIN(gain_blk_ispp_h - i * gain_tile_ispp_y, gain_tile_ispp_y);
        for(int j = 0; j < gain_tile_gainkg_w; j+=gainkg_tile_num)
        {
         //   for(int tile_idx = 0; tile_idx < gainkg_tile_num; tile_idx++)
            {
                int tile_off        = i * gain_tile_gainkg_stride + j * gain_tile_gainkg_size;
                int tile_i_ispp     = i * gain_tile_ispp_y;
                int tile_j_ispp     = j * gain_tile_ispp_x;
                if((j%2)==1)//i != 0)
                {
                    for(int len = 0; len < 2 ; len++)
                    {
                    //    prefetch_4x(src      + tile_off + gain_tile_gainkg_size + len * 256);
                    }

                }
#ifndef ENABLE_NEON
                for(uint16_t y = 0; y < gain_tile_ispp_y; y++)
				{
                    for(int x = 0; x < gain_tile_ispp_x; x++)
                    {

                        int i_act                   = tile_i_ispp + y;
                        int j_act                   = tile_j_ispp + x;
                        int idx_isp;
                        int idx_gain;
                        int idx_ratio;
                        int idx_ispp;
                        uint8_t ratio_cur;
                        uint8_t ratio_nxt;
                        uint16_t ratio_nxt_scale[4];
                        int gain_isp_cur;
                        int gain_isp_cur_y;
                        int gain_isp_cur_uv;
                        idx_isp                     = i_act * gain_blk_isp_stride   + j_act * 2;
                        idx_gain                    = tile_off + y * gain_tile_ispp_x * gainkg_unit + x;
                        idx_ratio                   = i_act * ratio_stride + j_act * 2;
                        idx_ispp                    = i_act * gain_blk_ispp_stride + j_act;


                        gain_isp_cur                = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                        gain_isp_cur_y              = MAX((gain_isp_cur * frame_limit_div_y + 128) >> 8, 1);
                        gain_isp_cur_uv             = MAX((gain_isp_cur * frame_limit_div_uv + 128) >> 8, 1);

                        ratio_cur                   = MAX(ratio[idx_ratio], ratio[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);


                        if(i_act > gain_blk_ispp_h - 1 || j_act > gain_blk_ispp_w - 1)
                            continue;
                        if(wr_flg)
                        {
                            test_buff_ori[0][idx_ispp]  = src[idx_gain + 0];
                            test_buff_ori[1][idx_ispp]  = src[idx_gain + 2];
                            test_buff_ori[2][idx_ispp]  = src[idx_gain + 4];
                            test_buff_ori[3][idx_ispp]  = src[idx_gain + 6];
                        }
						if(idx_ispp==0x1c200)
							idx_gain=idx_gain;
                            src[idx_gain + 0]               = MIN(255,ROUND_F((float)(src[idx_gain + 0]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 2]               = MIN(255,ROUND_F((float)(src[idx_gain + 2]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 4]               = MIN(255,ROUND_F((float)(src[idx_gain + 4]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 6]               = MIN(255,ROUND_F((float)(src[idx_gain + 6]    << ratio_shf_bit) / ratio_cur));
#if 1

                        src[idx_gain + 0]               = MAX(gain_isp_cur_y,   src[idx_gain + 0]);
                        src[idx_gain + 2]               = MAX(gain_isp_cur_uv,  src[idx_gain + 2]);
                        src[idx_gain + 4]               = MAX(gain_isp_cur_y,   src[idx_gain + 4]);
                        src[idx_gain + 6]               = MAX(gain_isp_cur_uv,  src[idx_gain + 6]);
#endif

                        ratio_nxt                       = MAX(ratio_next[idx_ratio], ratio_next[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);

                        if(wr_flg)
                        {
                            test_buff_mid[0][idx_ispp]  = src[idx_gain + 0];
                            test_buff_mid[1][idx_ispp]  = src[idx_gain + 2];
                            test_buff_mid[2][idx_ispp]  = src[idx_gain + 4];
                            test_buff_mid[3][idx_ispp]  = src[idx_gain + 6];
                        }



                            for(int idx = 0; idx < 4; idx++)
                            {
                                if(ratio_nxt > 120)
                                    ratio_nxt_scale[idx]    = ratio_nxt;
                                else
                                    ratio_nxt_scale[idx]    = ((uint32_t)ratio_nxt * ratio_r[idx] + (1 << 7)) >> 8;

                            }

                            src[idx_gain + 0]               = (src[idx_gain + 0] * ratio_nxt_scale[0] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 2]               = (src[idx_gain + 2] * ratio_nxt_scale[1] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 4]               = (src[idx_gain + 4] * ratio_nxt_scale[2] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 6]               = (src[idx_gain + 6] * ratio_nxt_scale[3] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;

                        src[idx_gain + 0]               = MAX(gain_min_val,    src[idx_gain + 0]);
                        src[idx_gain + 2]               = MAX(gain_min_val,    src[idx_gain + 2]);
                        src[idx_gain + 4]               = MAX(gain_min_val,    src[idx_gain + 4]);
                        src[idx_gain + 6]               = MAX(gain_min_val,    src[idx_gain + 6]);
                        if(wr_flg)
                        {
                            src_mid[idx_gain + 0]       = src[idx_gain + 0];
                            src_mid[idx_gain + 2]       = src[idx_gain + 2];
                            src_mid[idx_gain + 4]       = src[idx_gain + 4];
                            src_mid[idx_gain + 6]       = src[idx_gain + 6];
                            test_buff[0][idx_ispp]      = src[idx_gain + 0];
                            test_buff[1][idx_ispp]      = src[idx_gain + 2];
                            test_buff[2][idx_ispp]      = src[idx_gain + 4];
                            test_buff[3][idx_ispp]      = src[idx_gain + 6];

                        }

                        if(i_act > gain_blk_ispp_h - 1 || j_act + 1 > gain_blk_ispp_w - 1)
                            continue;
                        idx_gain                        = tile_off +  y * gain_tile_ispp_x * gainkg_unit  + gainkg_unit + x;
                        idx_ratio                       = i_act * ratio_stride          + (j_act + dst_stride) * 2;
                        idx_ispp                        = i_act * gain_blk_ispp_stride  + j_act + dst_stride;
                        idx_isp                         = i_act * gain_blk_isp_stride   + (j_act + dst_stride) * 2;


                        gain_isp_cur                    = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                        gain_isp_cur_y                  = MAX((gain_isp_cur * frame_limit_div_y + 128) >> 8, 1);
                        gain_isp_cur_uv                 = MAX((gain_isp_cur * frame_limit_div_uv + 128) >> 8, 1);

                        ratio_cur                       = MAX(ratio[idx_ratio], ratio[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);

                        if(wr_flg)
                        {
                            test_buff_ori[0][idx_ispp]  = src[idx_gain + 0];
                            test_buff_ori[1][idx_ispp]  = src[idx_gain + 2];
                            test_buff_ori[2][idx_ispp]  = src[idx_gain + 4];
                                test_buff_ori[3][idx_ispp]  = src[idx_gain + 6];
                            }
                            src[idx_gain + 0]               = MIN(255,ROUND_F((float)(src[idx_gain + 0]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 2]               = MIN(255,ROUND_F((float)(src[idx_gain + 2]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 4]               = MIN(255,ROUND_F((float)(src[idx_gain + 4]    << ratio_shf_bit) / ratio_cur));
                            src[idx_gain + 6]               = MIN(255,ROUND_F((float)(src[idx_gain + 6]    << ratio_shf_bit) / ratio_cur));
#if 1
                        src[idx_gain + 0]               = MAX(gain_isp_cur_y,   src[idx_gain + 0]);
                        src[idx_gain + 2]               = MAX(gain_isp_cur_uv,  src[idx_gain + 2]);
                        src[idx_gain + 4]               = MAX(gain_isp_cur_y,   src[idx_gain + 4]);
                        src[idx_gain + 6]               = MAX(gain_isp_cur_uv,  src[idx_gain + 6]);
#endif

                        ratio_nxt                       = MAX(ratio_next[idx_ratio], ratio_next[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                        if(wr_flg)
                        {
                            src_mid[idx_gain + 0]       = src[idx_gain + 0];
                            src_mid[idx_gain + 2]       = src[idx_gain + 2];
                            src_mid[idx_gain + 4]       = src[idx_gain + 4];
                            src_mid[idx_gain + 6]       = src[idx_gain + 6];

                            test_buff_mid[0][idx_ispp]  = src[idx_gain + 0];
                            test_buff_mid[1][idx_ispp]  = src[idx_gain + 2];
                            test_buff_mid[2][idx_ispp]  = src[idx_gain + 4];
                            test_buff_mid[3][idx_ispp]  = src[idx_gain + 6];
                            }


                            for(int idx = 0; idx < 4; idx++)
                            {
                                if(ratio_nxt > 120)
                                    ratio_nxt_scale[idx]    = ratio_nxt;
                                else
                                    ratio_nxt_scale[idx]    = ((uint32_t)ratio_nxt * ratio_r[idx] + (1 << 7)) >> 8;

                            }

                            src[idx_gain + 0]               = (src[idx_gain + 0] * ratio_nxt_scale[0] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 2]               = (src[idx_gain + 2] * ratio_nxt_scale[1] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 4]               = (src[idx_gain + 4] * ratio_nxt_scale[2] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                            src[idx_gain + 6]               = (src[idx_gain + 6] * ratio_nxt_scale[3] + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;


                        src[idx_gain + 0]               = MAX(gain_min_val,    src[idx_gain + 0]);
                        src[idx_gain + 2]               = MAX(gain_min_val,    src[idx_gain + 2]);
                        src[idx_gain + 4]               = MAX(gain_min_val,    src[idx_gain + 4]);
                        src[idx_gain + 6]               = MAX(gain_min_val,    src[idx_gain + 6]);
                        if(wr_flg)
                        {
                            test_buff[0][idx_ispp]      = src[idx_gain + 0];
                            test_buff[1][idx_ispp]      = src[idx_gain + 2];
                            test_buff[2][idx_ispp]      = src[idx_gain + 4];
                            test_buff[3][idx_ispp]      = src[idx_gain + 6];
                        }


                    }
                }
#else
                int idx_ratio0              = tile_i_ispp * ratio_stride          + tile_j_ispp * 2;

                uint8_t *pSrc00             = src               + tile_off;
                uint8_t *pSrc00_st          = src               + tile_off;
                uint8_t *ratio_addr         = ratio             + idx_ratio0;
                uint8_t *ratio_next_addr    = ratio_next        + idx_ratio0;
                uint8_t *pGainIsp00         = gain_isp_buf_cur  + tile_i_ispp * gain_blk_isp_stride   + tile_j_ispp * 2;



                uint16_t                    ratio0, ratio1, ratio2, ratio3;
				uint16_t 					ratio_nxt0, ratio_nxt1, ratio_nxt2, ratio_nxt3;

                uint8x8x2_t                 ratio_u8, ratio_nxt_u8;
                    uint16x4_t                  ratio_u16;
                    uint16x4_t                  ratio_r_u16;
                    uint16x4_t                  ratio_nxt_u16;
                    uint16x4_t                  ratio_nxt1_u16;
                    uint16x8_t                  ratio_nxt_u16x8;
                    uint32x4_t                  ratio_nxt_mul0_u32;
                    uint16x4_t                  ratio_nxt0_u16;
                    uint16x4_t                  ratio_nxt_flg_u16;
                    uint8x8x2_t                 vSrc0, vSrc;
                    uint8x8x2_t                 vGainIsp00;
                    uint16x8_t                  frame_limit_reg = vcombine_u16(vdup_n_u16(frame_limit_div_y), vdup_n_u16(frame_limit_div_uv));
					//	int16x8_t frame_limit_reg1=frame_limit_reg;

//frame_limit_reg=                    vqrdmulhq_s16(frame_limit_reg, frame_limit_reg1);

	                ratio_r_u16         	        = vld1_u16(ratio_r);

                for(uint16_t y = 0; y < gain_tile_ispp_y; y++)
				{



                    vGainIsp00                  = vld2_u8(pGainIsp00);

	                ratio_u8				    = vld2_u8(ratio_addr);
	                ratio_nxt_u8        	    = vld2_u8(ratio_next_addr);
                    vSrc0                       = vld2_u8(pSrc00);


		            ratio_u8.val[0]		        = vmax_u8(ratio_u8.val[0],      ratio_u8.val[1]);
                    ratio_u16                   = vget_low_u16(vmovl_u8(ratio_u8.val[0]));

		            ratio_nxt_u8.val[0]		    = vmax_u8(ratio_nxt_u8.val[0],  ratio_nxt_u8.val[1]);
                        ratio_nxt_u16               = vget_low_u16(vmovl_u8(ratio_nxt_u8.val[0]));

                    float32x4_t				    ratio_f32;
    				float32x4_t				    reciprocal_ratio;

		            ratio_f32			        = vcvtq_f32_u32(vmovl_u16(ratio_u16));
    				reciprocal_ratio	        = vrecpeq_f32(ratio_f32);
    				reciprocal_ratio	        = vmulq_f32(vrecpsq_f32(ratio_f32, reciprocal_ratio), reciprocal_ratio);


                    uint16x8x2_t                vSrc_u16;
                    float32x4x4_t               vSrc_f32;
                    uint32x4x4_t                vSrc_u32;


                    vSrc_u16.val[0]             = vmovl_u8(vSrc0.val[0]);
                    vSrc_u16.val[1]             = vmovl_u8(vSrc0.val[1]);

                    //+1 is for float rounding
					// 2 bblock is a tile, vSrc_u16 val 0 is block 0 of each tile, 1 is block of each tile
                    vSrc_f32.val[0]             = vcvtq_f32_u32(vshll_n_u16(vget_low_u16   (vSrc_u16.val[0]), RATIO_BITS_NUM + 1));
                    vSrc_f32.val[1]             = vcvtq_f32_u32(vshll_n_u16(vget_high_u16  (vSrc_u16.val[0]), RATIO_BITS_NUM + 1));
                    vSrc_f32.val[2]             = vcvtq_f32_u32(vshll_n_u16(vget_low_u16   (vSrc_u16.val[1]), RATIO_BITS_NUM + 1));
                    vSrc_f32.val[3]             = vcvtq_f32_u32(vshll_n_u16(vget_high_u16  (vSrc_u16.val[1]), RATIO_BITS_NUM + 1));

                    // one ratio for two low & high data
    				vSrc_u32.val[0]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[0], vgetq_lane_f32(reciprocal_ratio, 0)));
    				vSrc_u32.val[1]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[1], vgetq_lane_f32(reciprocal_ratio, 2)));
    				vSrc_u32.val[2]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[2], vgetq_lane_f32(reciprocal_ratio, 1)));
    				vSrc_u32.val[3]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[3], vgetq_lane_f32(reciprocal_ratio, 3)));

                    //+1 is for float rounding
                    vSrc_u16.val[0]             = vcombine_u16(vmovn_u32(vSrc_u32.val[0]), vmovn_u32(vSrc_u32.val[1]));
                    vSrc_u16.val[1]             = vcombine_u16(vmovn_u32(vSrc_u32.val[2]), vmovn_u32(vSrc_u32.val[3]));
    				vSrc.val[0]		            = vqrshrn_n_u16(vSrc_u16.val[0],  1);
    				vSrc.val[1]			        = vqrshrn_n_u16(vSrc_u16.val[1],  1);


                    //
                    uint32x4x2_t                tmpVacc00, tmpVacc01;
                    uint16x8_t                  tmpVacc00_u16, tmpVacc01_u16;
                    uint8x8x2_t                 tmpVacc00_u8;
                    uint16x8_t                  vMaxGainIsp00;


                    // gain_isp_cur                = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                    vGainIsp00.val[0]           = vmin_u8(vGainIsp00.val[0], vGainIsp00.val[1]);
                    uint8x8_t flag_h;
		            flag_h			            = vcgt_u8(vdup_n_u8(block_h_cur), vdup_n_u8(y));
		            vGainIsp00.val[0]			= vbsl_u8(flag_h,                   vGainIsp00.val[0],        vdup_n_u8(GAIN_MIN_VAL));


                    vMaxGainIsp00               = vmovl_u8(vGainIsp00.val[0]);
                    uint16x4_t                  tmpGain, tmpGain1;
                    // gain 0 1 2 3 to 0 2 1 3 ,y and uv is y 0 2 1 3 uv 4 6 5 7
                    tmpGain                     = vget_low_u16(vMaxGainIsp00);
                    tmpGain                     = vset_lane_u16(vgetq_lane_u16(vMaxGainIsp00, 2), tmpGain, 1);
                    tmpGain                     = vset_lane_u16(vgetq_lane_u16(vMaxGainIsp00, 1), tmpGain, 2);

                    vMaxGainIsp00               = vcombine_u16(tmpGain, tmpGain);


                    // y0, y1, y2, y3 uv0, uv1, uv2, uv3
                    tmpVacc00_u16               = vmulq_u16(vMaxGainIsp00,		frame_limit_reg);
                    tmpVacc00_u8.val[0]         = vqrshrn_n_u16(tmpVacc00_u16,  8);
                    uint16x4_t                  vGain_isp_cur_y, vGain_isp_cur_uv;
                    tmpVacc00_u8.val[0]         = vmax_u8(tmpVacc00_u8.val[0], vdup_n_u8(GAIN_MIN_VAL));
                    //  0 2 1 3 uv 4 6 5 7 to  00 44 22 66 11 55 33 77

                    tmpVacc00_u8                = vzip_u8(tmpVacc00_u8.val[0], tmpVacc00_u8.val[0]);
                    //  0 4 0 4 2 6 2 6 1 5 1 5 3 7 3 7
                    tmpVacc00_u8                = vzip_u8(tmpVacc00_u8.val[0], tmpVacc00_u8.val[1]);
                    // vSrc 0 y uv y uv y uv y uv equal to 0 4 0 4 2 6 2 6 1 5 1 5 3 7 3 7

                    vSrc.val[0]                 = vmax_u8(vSrc.val[0],							tmpVacc00_u8.val[0]);
                    vSrc.val[1]                 = vmax_u8(vSrc.val[1],							tmpVacc00_u8.val[1]);

                        ratio_nxt1_u16              = vshl_n_u16(ratio_nxt_u16, 4);

                        ratio_nxt_flg_u16           = vcge_u16(ratio_nxt_u16, vdup_n_u16(121));
                        ratio_nxt_mul0_u32          = vmull_lane_u16(ratio_nxt_u16, ratio_r_u16, 0);
                        ratio_nxt1_u16              = vrshrn_n_u32(ratio_nxt_mul0_u32, 8);
                        ratio_nxt_u16               = vbsl_u16(ratio_nxt_flg_u16, ratio_nxt_u16, ratio_nxt1_u16);
						tmpVacc00_u16               = vmulq_u16(vmovl_u8(vSrc.val[0]), vcombine_u16(vdup_n_u16(vget_lane_u16(ratio_nxt_u16, 0)), vdup_n_u16(vget_lane_u16(ratio_nxt_u16, 2))));
                        tmpVacc01_u16               = vmulq_u16(vmovl_u8(vSrc.val[1]), vcombine_u16(vdup_n_u16(vget_lane_u16(ratio_nxt_u16, 1)), vdup_n_u16(vget_lane_u16(ratio_nxt_u16, 3))));



					vSrc.val[0]					= vrshrn_n_u16(tmpVacc00_u16, RATIO_BITS_NUM);
                    vSrc.val[1]					= vrshrn_n_u16(tmpVacc01_u16, RATIO_BITS_NUM);
                    vSrc.val[0]                 = vmax_u8(vSrc.val[0], vdup_n_u8(GAIN_MIN_VAL));
                    vSrc.val[1]                 = vmax_u8(vSrc.val[1], vdup_n_u8(GAIN_MIN_VAL));

                    ratio_addr                  += ratio_stride;
                    ratio_next_addr             += ratio_stride;
                    pSrc00                      += gain_tile_ispp_x * gainkg_unit;
                    pGainIsp00                  += gain_blk_isp_stride;

		            vst2_u8(pSrc00_st, vSrc);
                    pSrc00_st                   += gain_tile_ispp_x * gainkg_unit;
                }


#endif
            }
        }
    }





    {
        static FILE *fd_gainkg_out              = NULL;
        static FILE *fd_ratio_nxt_wr            = NULL;
        static FILE *fd_gainkg_mid_out          = NULL;
        static FILE *fd_gainkg_up_out           = NULL;
        static FILE *fd_gainkg_up_sp_out        = NULL;
        static FILE *fd_gainkg_mid_sp_out       = NULL;
        static FILE *fd_gainkg_mid1_sp_out      = NULL;
        static FILE *fd_gainkg_sp_out           = NULL;
        static FILE *fd_gainkg_ratio_in_out     = NULL;
        static FILE *fd_gainkg_ratio_out_out    = NULL;
        if(wr_flg)
        {


            if(fd_gainkg_ratio_in_out == NULL)
                fd_gainkg_ratio_in_out            = fopen("/tmp/ratio_in_out.yuv", "wb");
            if(fd_gainkg_ratio_in_out)
            {
                fwrite(ratio_in, ratio_stride * gain_kg_tile_h_align, 1,    fd_gainkg_ratio_in_out);
                fflush(fd_gainkg_ratio_in_out);
            }

            if(fd_gainkg_ratio_out_out == NULL)
                fd_gainkg_ratio_out_out            = fopen("/tmp/ratio_out_out.yuv", "wb");
            if(fd_gainkg_ratio_out_out)
            {
                fwrite(ratio_out, ratio_stride * gain_kg_tile_h_align, 1,    fd_gainkg_ratio_out_out);
                fflush(fd_gainkg_ratio_out_out);
            }



             if(fd_ratio_nxt_wr == NULL)
                 fd_ratio_nxt_wr            = fopen("/tmp/ratio_nxt_out.yuv", "wb");
             if(fd_ratio_nxt_wr)
             {
                 fwrite(ratio_next, ratio_stride * gain_kg_tile_h_align, 1,    fd_ratio_nxt_wr);
                 fflush(fd_ratio_nxt_wr);
             }

            if(fd_gainkg_out == NULL)
                fd_gainkg_out               = fopen("/tmp/gainkg_out.yuv", "wb");
            if(fd_gainkg_out)
            {
                fwrite(src_ori, gain_tile_gainkg_stride * gain_tile_gainkg_h,  1, fd_gainkg_out);
                fflush(fd_gainkg_out);
            }

            if(fd_gainkg_mid_out == NULL)
                fd_gainkg_mid_out           = fopen("/tmp/gainkg_mid_out.yuv",      "wb");
            if(fd_gainkg_mid_out)
            {
                fwrite(src_mid, gain_tile_gainkg_stride * gain_tile_gainkg_h, 1, fd_gainkg_mid_out);
                fflush(fd_gainkg_mid_out);
            }

            if(fd_gainkg_up_out == NULL)
                fd_gainkg_up_out            = fopen("/tmp/gainkg_up_out.yuv",       "wb");
            if(fd_gainkg_up_out)
            {
                fwrite(src,     gain_tile_gainkg_stride * gain_tile_gainkg_h, 1, fd_gainkg_up_out);
                fflush(fd_gainkg_up_out);
            }

            if (1)
            {
                if(fd_gainkg_up_sp_out == NULL)
                    fd_gainkg_up_sp_out     = fopen("/tmp/gainkg_up_sp_out.yuv",    "wb");

                if(fd_gainkg_mid_sp_out == NULL)
                    fd_gainkg_mid_sp_out    = fopen("/tmp/gainkg_mid_sp_out.yuv",   "wb");
                if(fd_gainkg_mid1_sp_out == NULL)
                    fd_gainkg_mid1_sp_out    = fopen("/tmp/gainkg_mid1_sp_out.yuv",   "wb");


                if(fd_gainkg_sp_out == NULL)
                    fd_gainkg_sp_out        = fopen("/tmp/gainkg_sp_out.yuv",       "wb");
                for(int i = 0; i < 4; i++)
                {
                    if(fd_gainkg_up_sp_out)
                    {
                        fwrite(test_buff[i],       gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_up_sp_out);
                        fflush(fd_gainkg_up_sp_out);
                    }
                    if(fd_gainkg_mid_sp_out)
                    {
                        fwrite(test_buff_mid[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_mid_sp_out);
                        fflush(fd_gainkg_mid_sp_out);
                    }
                    if(fd_gainkg_mid1_sp_out)
                    {
                        fwrite(test_buff_mid1[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_mid1_sp_out);
                        fflush(fd_gainkg_mid1_sp_out);
                    }
                    if(fd_gainkg_sp_out)
                    {
                        fwrite(test_buff_ori[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_sp_out);
                        fflush(fd_gainkg_sp_out);
                    }
                }
            }

            free(ratio_in);
            free(ratio_out);
            for(int i = 0; i < 4; i++)
            {
                if(test_buff[i])
                    free(test_buff[i]);
                if(test_buff_mid[i])
                    free(test_buff_mid[i]);
                if(test_buff_mid1[i])
                    free(test_buff_mid1[i]);
                if(test_buff_ori[i])
                    free(test_buff_ori[i]);
            }
            if(src_mid)
                free(src_mid);
            if(src_ori)
                free(src_ori);
        }
        else
        {
            fd_gainkg_out          = NULL;
            fd_gainkg_mid_out      = NULL;
            fd_gainkg_up_out       = NULL;
            fd_gainkg_up_sp_out    = NULL;
            fd_gainkg_mid_sp_out   = NULL;
            fd_gainkg_mid1_sp_out  = NULL;
            fd_gainkg_sp_out       = NULL;
            fd_ratio_nxt_wr        = NULL;
            fd_gainkg_ratio_in_out       = NULL;
            fd_gainkg_ratio_out_out        = NULL;
        }
    }
}



void
Isp20SpThread::init()
{
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    ispsp_stop_fds[0]                       = -1;
    ispsp_stop_fds[1]                       = -1;
    ispp_stop_fds[0]                        = -1;
    ispp_stop_fds[1]                        = -1;
    sync_pipe_fd[0]                         = -1;
    sync_pipe_fd[1]                         = -1;
    _isp_buf_num = 0;
    _ispp_buf_num = 0;
    _fd_init_flag = true;

    _first_frame = true;
    _motion_params.stMotion = _calibDb->mfnr.mode_cell[0].motion;
    _motion_params.gain_ratio = 16.0f;
    /*----------------- motion detect algorithm parameters init---------------------*/
    max_list_num                        = 4;
    static_ratio_num                    = max_list_num + 0;

    frame_id_pp_upt                     = -1;
    frame_id_isp_upt                    = -1;
    frame_num_pp                        = 0;
    frame_num_isp                       = 0;

    gain_buf_size                       = _img_height * _img_width;
    img_buf_size                        = _img_height_align * _img_width_align;
    img_buf_size_uv                     = img_buf_size / 2;


    gain_tile_ispp_w                    = (_img_width + 3)      / 4;
    gain_tile_ispp_h                    = (_img_height + 15)    / 16;
    gain_tile_ispp_x                    = 2;
    gain_tile_ispp_y                    = 16;

    gain_tile_gainkg_w                  = (gain_tile_ispp_w + 1) & 0xfffe;
    gain_tile_gainkg_h                  = gain_tile_ispp_h;
    gainkg_unit                         = 8;
    gain_tile_gainkg_size               = (gain_tile_ispp_x * gain_tile_ispp_y * gainkg_unit);
    gain_tile_gainkg_stride             = gain_tile_gainkg_w * gain_tile_gainkg_size + gain_tile_gainkg_w * gain_tile_gainkg_size / 2;;
    gainkg_tile_num                     = 2;


    gain_blk_isp_w                      = _img_width;
    gain_blk_isp_stride                 = ((gain_blk_isp_w + 15) / 16) * 16;
    gain_blk_isp_h                      = _img_height;
    gain_blk_isp_mem_size               = gain_blk_isp_stride * gain_blk_isp_h;


    gain_blk_ispp_w                     = (gain_blk_isp_w + 1 ) / 2;
    gain_blk_ispp_stride                = ((gain_blk_ispp_w + 7) / 8) * 8;
    gain_blk_ispp_h                     = _img_height;
    gain_blk_ispp_mem_size              = gain_blk_ispp_stride * gain_blk_ispp_h;


    ratio_stride                        = ((gain_blk_isp_w + 7) / 8) * 8;

    gain_kg_tile_w_align                = ((gain_blk_isp_w + 7) / 8) * 8;
    gain_kg_tile_h_align                = (gain_blk_isp_h + 15) & 0xfff0;

    static_ratio_l_bit                  = RATIO_BITS_NUM;
    static_ratio_r_bit                  = RATIO_BITS_R_NUM;
    static_ratio_l                      = 1 << static_ratio_l_bit;



    static_ratio_idx_in                 = 0;
    static_ratio_idx_out                = 0;
    static_ratio                        = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    pImgbuf                             = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    gain_isp_buf_bak                    = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    mtParamsSelect_list                 = (RKAnr_Mt_Params_Select_t**)  malloc(static_ratio_num * sizeof(RKAnr_Mt_Params_Select_t*));
    for(int i = 0; i < static_ratio_num; i++)
    {
        static_ratio[i]                 = (uint8_t*)malloc(ratio_stride      * (gain_kg_tile_h_align + 16)      *    sizeof(static_ratio[i][0]));
        memset(static_ratio[i], static_ratio_l, ratio_stride     * gain_kg_tile_h_align);
    }


    for(int i = 0; i < static_ratio_num; i++)
        pImgbuf[i]                      = (uint8_t*)malloc((img_buf_size  + img_buf_size_uv + 16)    *   sizeof(pImgbuf[i][0]));

    for(int i = 0; i < static_ratio_num; i++)
        gain_isp_buf_bak[i]             = (uint8_t*)malloc(gain_blk_isp_stride  * (gain_kg_tile_h_align + 16) * sizeof(gain_isp_buf_bak[i][0]));
    for(int i = 0; i < static_ratio_num; i++)
    {
        mtParamsSelect_list[i]          = (RKAnr_Mt_Params_Select_t *)malloc(static_ratio_num   *   sizeof(mtParamsSelect_list[0][0]));
        (*(mtParamsSelect_list[i])).gain_ratio = -1;
    }

	pPreAlpha							= (uint8_t*)malloc(ratio_stride         * (gain_kg_tile_h_align + 16)      * sizeof(pPreAlpha[0]));
    memset(pPreAlpha, 0, ratio_stride         * gain_kg_tile_h_align      * sizeof(pPreAlpha[0]));


    pTmpBuf                             = (int16_t*)malloc(gain_blk_isp_w       * gain_blk_isp_h * 6        *    sizeof(pTmpBuf[0]));

    frame_detect_flg                    = (uint8_t*)malloc(static_ratio_num * sizeof(frame_detect_flg[0]));
    for(int i = 0; i < static_ratio_num; i++)
    {
        frame_detect_flg[i]             = -1;
    }

    pAfTmp                              = (uint8_t*)malloc(img_buf_size * sizeof(pAfTmp[0]) * 3 / 2);
    _af_meas_params.sp_meas.ldg_xl      = _calibDb->af.ldg_param.ldg_xl;
    _af_meas_params.sp_meas.ldg_yl      = _calibDb->af.ldg_param.ldg_yl;
    _af_meas_params.sp_meas.ldg_kl      = _calibDb->af.ldg_param.ldg_kl;
    _af_meas_params.sp_meas.ldg_xh      = _calibDb->af.ldg_param.ldg_xh;
    _af_meas_params.sp_meas.ldg_yh      = _calibDb->af.ldg_param.ldg_yh;
    _af_meas_params.sp_meas.ldg_kh      = _calibDb->af.ldg_param.ldg_kh;
    _af_meas_params.sp_meas.highlight_th  = _calibDb->af.highlight.ther0;
    _af_meas_params.sp_meas.highlight2_th = _calibDb->af.highlight.ther1;

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "Isp20SpThread::%s exit w %d h %d\n", __FUNCTION__, gain_blk_isp_w, gain_blk_isp_h);
}

void
Isp20SpThread::deinit()
{
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    SmartLock locker (_buf_list_mutex);
    _isp_buf_list.clear();
    _notifyYgCmdQ.clear();
    /*------- motion detect algorithm parameters deinit--------*/
    if(frame_detect_flg)
        free(frame_detect_flg);
    for(int i = 0; i < static_ratio_num; i++)
    {
        if(static_ratio[i])
            free(static_ratio[i]);
        if(pImgbuf[i])
            free(pImgbuf[i]);
        if(gain_isp_buf_bak[i])
            free(gain_isp_buf_bak[i]);
        if(mtParamsSelect_list[i])
            free(mtParamsSelect_list[i]);
    }
    if(static_ratio)
        free(static_ratio);
    if(pImgbuf)
        free(pImgbuf);
    if(gain_isp_buf_bak)
        free(gain_isp_buf_bak);
    if(mtParamsSelect_list)
        free(mtParamsSelect_list);
    if(pTmpBuf)
        free(pTmpBuf);
	if(pPreAlpha)
		free(pPreAlpha);

    if(pAfTmp)
        free(pAfTmp);

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
}

void
Isp20SpThread::set_working_mode(int mode)
{
    _working_mode = mode;
}

bool Isp20SpThread::notify_yg_cmd(SmartPtr<sp_msg_t> msg)
{
    bool ret = true;
    if (msg->sync) {
        msg->mutex = new Mutex();
        msg->cond = new XCam::Cond();
        SmartLock lock (*msg->mutex.ptr());
        ret = _notifyYgCmdQ.push(msg);
        msg->cond->wait(*msg->mutex.ptr());
    } else {
        ret = _notifyYgCmdQ.push(msg);
    }

    return ret;
}

bool Isp20SpThread::notify_yg2_cmd(SmartPtr<sp_msg_t> msg)
{
    bool ret = true;
    if (msg->sync) {
        msg->mutex = new Mutex();
        msg->cond = new XCam::Cond();
        SmartLock lock (*msg->mutex.ptr());
        ret = _notifyYgCmdQ2.push(msg);
        msg->cond->wait(*msg->mutex.ptr());
    } else {
        ret = _notifyYgCmdQ2.push(msg);
    }

    return ret;
}

void Isp20SpThread::notify_stop_fds_exit()
{
    if (ispp_stop_fds[1] != -1) {
        char buf = 0xf;  // random value to write to flush fd.
        unsigned int size = write(ispp_stop_fds[1], &buf, sizeof(char));
        if (size != sizeof(char))
            LOGW_CAMHW_SUBM(ISP20POLL_SUBM, "Flush write not completed");
    }

    if (ispsp_stop_fds[1] != -1) {
        char buf = 0xf;  // random value to write to flush fd.
        unsigned int size = write(ispsp_stop_fds[1], &buf, sizeof(char));
        if (size != sizeof(char))
            LOGW_CAMHW_SUBM(ISP20POLL_SUBM, "Flush write not completed");
    }
}

void Isp20SpThread::notify_wr_thread_exit()
{
    SmartPtr<sp_msg_t> msg = new sp_msg_t();
    msg->cmd = MSG_CMD_WR_EXIT;
    msg->sync = true;
    notify_yg_cmd(msg);
}

void Isp20SpThread::notify_wr2_thread_exit()
{
    SmartPtr<sp_msg_t> msg = new sp_msg_t();
    msg->cmd = MSG_CMD_WR_EXIT;
    msg->sync = true;
    notify_yg2_cmd(msg);
}

void Isp20SpThread::destroy_stop_fds_ispsp () {

    if (ispsp_stop_fds[0] != -1 || ispsp_stop_fds[1] != -1) {
        close(ispsp_stop_fds[0]);
        close(ispsp_stop_fds[1]);
        ispsp_stop_fds[0] = -1;
        ispsp_stop_fds[1] = -1;
    }

    if (ispp_stop_fds[0] != -1 || ispp_stop_fds[1] != -1) {
        close(ispp_stop_fds[0]);
        close(ispp_stop_fds[1]);
        ispp_stop_fds[0] = -1;
        ispp_stop_fds[1] = -1;
    }

    if (sync_pipe_fd[0] != -1 || sync_pipe_fd[1] != -1) {
        close(sync_pipe_fd[0]);
        close(sync_pipe_fd[1]);
        sync_pipe_fd[0] = -1;
        sync_pipe_fd[1] = -1;
    }
}

XCamReturn
Isp20SpThread::create_stop_fds_ispsp () {
    int i, status = 0;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    destroy_stop_fds_ispsp();

    status = pipe(ispsp_stop_fds);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispsp poll stop pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(ispsp_stop_fds[0], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispsp stop pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    status = pipe(ispp_stop_fds);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispp poll stop pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(ispp_stop_fds[0], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispp stop pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    status = pipe(sync_pipe_fd);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispp poll sync pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(sync_pipe_fd[1], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispp sync pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    return XCAM_RETURN_NO_ERROR;
exit_error:
    destroy_stop_fds_ispsp();
    return ret;
}

void Isp20SpThread::update_motion_detection_params(ANRMotionParam_t *motion)
{
    SmartLock locker (_motion_param_mutex);
    if (motion && (0 != memcmp(motion, &_motion_params, sizeof(ANRMotionParam_t)))) {
        _motion_params = *motion;
    }
}

void Isp20SpThread::update_af_meas_params(rk_aiq_af_algo_meas_t *af_meas)
{
    SmartLock locker (_afmeas_param_mutex);
    if (af_meas && (0 != memcmp(af_meas, &_af_meas_params, sizeof(rk_aiq_af_algo_meas_t)))) {
        _af_meas_params = *af_meas;
    }
}

}; //namspace RkCam

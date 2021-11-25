#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "../c_api/rkmedia_buffer_impl.h"
#include "../socket/socket.h"
#include "rkmedia_api.h"

#include "utils.h"

static int listenfd = 0;
static pthread_t RkMediaServerTid = 0;
static int RkMediaServerRun = 0;

typedef enum rkCB_TYPE { CB_TYPE_EVENT = 0, CB_TYPE_OUT } CB_TYPE_E;

struct FunMap {
  char *fun_name;
  int (*fun)(int);
};

typedef struct CB_LIST {
  int id;
  int fd;
  CB_TYPE_E type;
  MPP_CHN_S stChn;
  struct CB_LIST *next;
} CB_LIST_S;

static CB_LIST_S *event_list = NULL;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

#define CONST_STRING_NUM 256
const char *const_string[CONST_STRING_NUM] = {NULL};

int ser_RK_MPI_SYS_Init(int fd);
int ser_RK_MPI_SYS_Bind(int fd);
int ser_RK_MPI_SYS_UnBind(int fd);
int ser_RK_MPI_SYS_SendMediaBuffer(int fd);
int ser_RK_MPI_SYS_GetMediaBuffer(int fd);

int ser_RK_MPI_VI_SetChnAttr(int fd);
int ser_RK_MPI_VI_EnableChn(int fd);
int ser_RK_MPI_VI_DisableChn(int fd);
int ser_RK_MPI_VI_StartRegionLuma(int fd);
int ser_RK_MPI_VI_StopRegionLuma(int fd);
int ser_RK_MPI_VI_GetChnRegionLuma(int fd);
int ser_RK_MPI_VI_RGN_SetCover(int fd);
int ser_RK_MPI_VI_StartStream(int fd);

int ser_RK_MPI_RGA_CreateChn(int fd);
int ser_RK_MPI_RGA_DestroyChn(int fd);
int ser_RK_MPI_RGA_GetChnAttr(int fd);
int ser_RK_MPI_RGA_SetChnAttr(int fd);

int ser_RK_MPI_VO_CreateChn(int fd);
int ser_RK_MPI_VO_GetChnAttr(int fd);
int ser_RK_MPI_VO_SetChnAttr(int fd);
int ser_RK_MPI_VO_DestroyChn(int fd);

int ser_RK_MPI_ALGO_MD_CreateChn(int fd);
int ser_RK_MPI_ALGO_MD_DestroyChn(int fd);
int ser_RK_MPI_ALGO_MD_EnableSwitch(int fd);

int ser_RK_MPI_ALGO_OD_CreateChn(int fd);
int ser_RK_MPI_ALGO_OD_DestroyChn(int fd);
int ser_RK_MPI_ALGO_OD_EnableSwitch(int fd);

int ser_RK_MPI_ADEC_CreateChn(int fd);
int ser_RK_MPI_ADEC_DestroyChn(int fd);

int ser_RK_MPI_MB_ReleaseBuffer(int fd);
int ser_RK_MPI_MB_GetPtr(int fd);
int ser_RK_MPI_MB_GetFD(int fd);
int ser_RK_MPI_MB_GetImageInfo(int fd);
int ser_RK_MPI_MB_GetSize(int fd);
int ser_RK_MPI_MB_GetModeID(int fd);
int ser_RK_MPI_MB_GetChannelID(int fd);
int ser_RK_MPI_MB_GetTimestamp(int fd);
int ser_RK_MPI_MB_SetTimestamp(int fd);
int ser_RK_MPI_MB_SetSize(int fd);
int ser_RK_MPI_MB_CreateBuffer(int fd);
int ser_RK_MPI_MB_CreateImageBuffer(int fd);
int ser_RK_MPI_MB_CreateAudioBuffer(int fd);
int ser_RK_MPI_MB_POOL_Create(int fd);
int ser_RK_MPI_MB_POOL_Destroy(int fd);
int ser_RK_MPI_MB_POOL_GetBuffer(int fd);
int ser_RK_MPI_MB_ConvertToImgBuffer(int fd);
int ser_RK_MPI_MB_GetFlag(int fd);
int ser_RK_MPI_MB_GetTsvcLevel(int fd);
int ser_RK_MPI_MB_IsViFrame(int fd);
int ser_RK_MPI_MB_TsNodeDump(int fd);

int ser_RK_MPI_VDEC_CreateChn(int fd);
int ser_RK_MPI_VDEC_DestroyChn(int fd);

int ser_RK_MPI_VENC_CreateChn(int fd);
int ser_RK_MPI_VENC_DestroyChn(int fd);
int ser_RK_MPI_VENC_CreateJpegLightChn(int fd);
int ser_RK_MPI_VENC_GetVencChnAttr(int fd);
int ser_RK_MPI_VENC_SetVencChnAttr(int fd);
int ser_RK_MPI_VENC_GetRcParam(int fd);
int ser_RK_MPI_VENC_SetRcParam(int fd);
int ser_RK_MPI_VENC_SetJpegParam(int fd);
int ser_RK_MPI_VENC_RGN_Init(int fd);
int ser_RK_MPI_VENC_RGN_SetBitMap(int fd);
int ser_RK_MPI_VENC_StartRecvFrame(int fd);
int ser_RK_MPI_VENC_RGN_SetCover(int fd);
int ser_RK_MPI_VENC_RGN_SetCoverEx(int fd);
int ser_RK_MPI_VENC_SetGopMode(int fd);
int ser_RK_MPI_VENC_GetRoiAttr(int fd);
int ser_RK_MPI_VENC_SetRoiAttr(int fd);
int ser_RK_MPI_VENC_SetResolution(int fd);

int ser_RK_MPI_VMIX_CreateDev(int fd);
int ser_RK_MPI_VMIX_DestroyDev(int fd);
int ser_RK_MPI_VMIX_EnableChn(int fd);
int ser_RK_MPI_VMIX_DisableChn(int fd);
int ser_RK_MPI_VMIX_SetLineInfo(int fd);
int ser_RK_MPI_VMIX_ShowChn(int fd);
int ser_RK_MPI_VMIX_HideChn(int fd);
int ser_RK_MPI_VMIX_GetChnRegionLuma(int fd);
int ser_RK_MPI_VMIX_RGN_SetCover(int fd);

int ser_RK_MPI_AO_SetChnAttr(int fd);
int ser_RK_MPI_AO_EnableChn(int fd);
int ser_RK_MPI_AO_DisableChn(int fd);
int ser_RK_MPI_AO_SetVolume(int fd);
int ser_RK_MPI_AO_GetVolume(int fd);
int ser_RK_MPI_AO_SetVqeAttr(int fd);
int ser_RK_MPI_AO_GetVqeAttr(int fd);
int ser_RK_MPI_AO_EnableVqe(int fd);
int ser_RK_MPI_AO_DisableVqe(int fd);
int ser_RK_MPI_AO_QueryChnStat(int fd);
int ser_RK_MPI_AO_ClearChnBuf(int fd);

int ser_RK_MPI_AENC_CreateChn(int fd);
int ser_RK_MPI_AENC_DestroyChn(int fd);

int ser_RK_MPI_AI_SetChnAttr(int fd);
int ser_RK_MPI_AI_EnableChn(int fd);
int ser_RK_MPI_AI_DisableChn(int fd);
int ser_RK_MPI_AI_SetVolume(int fd);
int ser_RK_MPI_AI_GetVolume(int fd);
int ser_RK_MPI_AI_SetTalkVqeAttr(int fd);
int ser_RK_MPI_AI_GetTalkVqeAttr(int fd);
int ser_RK_MPI_AI_SetRecordVqeAttr(int fd);
int ser_RK_MPI_AI_GetRecordVqeAttr(int fd);
int ser_RK_MPI_AI_EnableVqe(int fd);
int ser_RK_MPI_AI_StartStream(int fd);
int ser_RK_MPI_AI_DisableVqe(int fd);

int ser_out_server_run(int fd);
int ser_event_server_run(int fd);
int ser_example_0(int fd);
int ser_example_1(int fd);

static const struct FunMap map[] = {
    {(char *)"RK_MPI_SYS_Init", &ser_RK_MPI_SYS_Init},
    {(char *)"RK_MPI_SYS_Bind", &ser_RK_MPI_SYS_Bind},
    {(char *)"RK_MPI_SYS_UnBind", &ser_RK_MPI_SYS_UnBind},
    {(char *)"RK_MPI_SYS_SendMediaBuffer", &ser_RK_MPI_SYS_SendMediaBuffer},
    {(char *)"RK_MPI_SYS_GetMediaBuffer", &ser_RK_MPI_SYS_GetMediaBuffer},

    {(char *)"RK_MPI_VI_SetChnAttr", &ser_RK_MPI_VI_SetChnAttr},
    {(char *)"RK_MPI_VI_EnableChn", &ser_RK_MPI_VI_EnableChn},
    {(char *)"RK_MPI_VI_DisableChn", &ser_RK_MPI_VI_DisableChn},
    {(char *)"RK_MPI_VI_StartRegionLuma", &ser_RK_MPI_VI_StartRegionLuma},
    {(char *)"RK_MPI_VI_StopRegionLuma", &ser_RK_MPI_VI_StopRegionLuma},
    {(char *)"RK_MPI_VI_GetChnRegionLuma", &ser_RK_MPI_VI_GetChnRegionLuma},
    {(char *)"RK_MPI_VI_RGN_SetCover", &ser_RK_MPI_VI_RGN_SetCover},
    {(char *)"RK_MPI_VI_StartStream", &ser_RK_MPI_VI_StartStream},

    {(char *)"RK_MPI_RGA_CreateChn", &ser_RK_MPI_RGA_CreateChn},
    {(char *)"RK_MPI_RGA_DestroyChn", &ser_RK_MPI_RGA_DestroyChn},
    {(char *)"RK_MPI_RGA_GetChnAttr", &ser_RK_MPI_RGA_GetChnAttr},
    {(char *)"RK_MPI_RGA_SetChnAttr", &ser_RK_MPI_RGA_SetChnAttr},

    {(char *)"RK_MPI_VO_CreateChn", &ser_RK_MPI_VO_CreateChn},
    {(char *)"RK_MPI_VO_GetChnAttr", &ser_RK_MPI_VO_GetChnAttr},
    {(char *)"RK_MPI_VO_SetChnAttr", &ser_RK_MPI_VO_SetChnAttr},
    {(char *)"RK_MPI_VO_DestroyChn", &ser_RK_MPI_VO_DestroyChn},

    {(char *)"RK_MPI_ALGO_MD_CreateChn", &ser_RK_MPI_ALGO_MD_CreateChn},
    {(char *)"RK_MPI_ALGO_MD_DestroyChn", &ser_RK_MPI_ALGO_MD_DestroyChn},
    {(char *)"RK_MPI_ALGO_MD_EnableSwitch", &ser_RK_MPI_ALGO_MD_EnableSwitch},

    {(char *)"RK_MPI_ALGO_OD_CreateChn", &ser_RK_MPI_ALGO_OD_CreateChn},
    {(char *)"RK_MPI_ALGO_OD_DestroyChn", &ser_RK_MPI_ALGO_OD_DestroyChn},
    {(char *)"RK_MPI_ALGO_OD_EnableSwitch", &ser_RK_MPI_ALGO_OD_EnableSwitch},

    {(char *)"RK_MPI_ADEC_CreateChn", &ser_RK_MPI_ADEC_CreateChn},
    {(char *)"RK_MPI_ADEC_DestroyChn", &ser_RK_MPI_ADEC_DestroyChn},

    {(char *)"RK_MPI_MB_ReleaseBuffer", &ser_RK_MPI_MB_ReleaseBuffer},
    {(char *)"RK_MPI_MB_GetPtr", &ser_RK_MPI_MB_GetPtr},
    {(char *)"RK_MPI_MB_GetFD", &ser_RK_MPI_MB_GetFD},
    {(char *)"RK_MPI_MB_GetImageInfo", &ser_RK_MPI_MB_GetImageInfo},
    {(char *)"RK_MPI_MB_GetSize", &ser_RK_MPI_MB_GetSize},
    {(char *)"RK_MPI_MB_GetModeID", &ser_RK_MPI_MB_GetModeID},
    {(char *)"RK_MPI_MB_GetChannelID", &ser_RK_MPI_MB_GetChannelID},
    {(char *)"RK_MPI_MB_GetTimestamp", &ser_RK_MPI_MB_GetTimestamp},
    {(char *)"RK_MPI_MB_SetTimestamp", &ser_RK_MPI_MB_SetTimestamp},
    {(char *)"RK_MPI_MB_SetSize", &ser_RK_MPI_MB_SetSize},
    {(char *)"RK_MPI_MB_CreateBuffer", &ser_RK_MPI_MB_CreateBuffer},
    {(char *)"RK_MPI_MB_CreateImageBuffer", &ser_RK_MPI_MB_CreateImageBuffer},
    {(char *)"RK_MPI_MB_CreateAudioBuffer", &ser_RK_MPI_MB_CreateAudioBuffer},
    {(char *)"RK_MPI_MB_POOL_Create", &ser_RK_MPI_MB_POOL_Create},
    {(char *)"RK_MPI_MB_POOL_Destroy", &ser_RK_MPI_MB_POOL_Destroy},
    {(char *)"RK_MPI_MB_POOL_GetBuffer", &ser_RK_MPI_MB_POOL_GetBuffer},
    {(char *)"RK_MPI_MB_ConvertToImgBuffer", &ser_RK_MPI_MB_ConvertToImgBuffer},
    {(char *)"RK_MPI_MB_GetFlag", &ser_RK_MPI_MB_GetFlag},
    {(char *)"RK_MPI_MB_GetTsvcLevel", &ser_RK_MPI_MB_GetTsvcLevel},
    {(char *)"RK_MPI_MB_IsViFrame", &ser_RK_MPI_MB_IsViFrame},
    {(char *)"RK_MPI_MB_TsNodeDump", &ser_RK_MPI_MB_TsNodeDump},

    {(char *)"RK_MPI_VDEC_CreateChn", &ser_RK_MPI_VDEC_CreateChn},
    {(char *)"RK_MPI_VDEC_DestroyChn", &ser_RK_MPI_VDEC_DestroyChn},

    {(char *)"RK_MPI_VENC_CreateChn", &ser_RK_MPI_VENC_CreateChn},
    {(char *)"RK_MPI_VENC_DestroyChn", &ser_RK_MPI_VENC_DestroyChn},
    {(char *)"RK_MPI_VENC_CreateJpegLightChn",
     &ser_RK_MPI_VENC_CreateJpegLightChn},
    {(char *)"RK_MPI_VENC_GetVencChnAttr", &ser_RK_MPI_VENC_GetVencChnAttr},
    {(char *)"RK_MPI_VENC_SetVencChnAttr", &ser_RK_MPI_VENC_SetVencChnAttr},
    {(char *)"RK_MPI_VENC_GetRcParam", &ser_RK_MPI_VENC_GetRcParam},
    {(char *)"RK_MPI_VENC_SetRcParam", &ser_RK_MPI_VENC_SetRcParam},
    {(char *)"RK_MPI_VENC_SetJpegParam", &ser_RK_MPI_VENC_SetJpegParam},
    {(char *)"RK_MPI_VENC_RGN_Init", &ser_RK_MPI_VENC_RGN_Init},
    {(char *)"RK_MPI_VENC_RGN_SetBitMap", &ser_RK_MPI_VENC_RGN_SetBitMap},
    {(char *)"RK_MPI_VENC_StartRecvFrame", &ser_RK_MPI_VENC_StartRecvFrame},
    {(char *)"RK_MPI_VENC_RGN_SetCover", &ser_RK_MPI_VENC_RGN_SetCover},
    {(char *)"RK_MPI_VENC_RGN_SetCoverEx", &ser_RK_MPI_VENC_RGN_SetCoverEx},
    {(char *)"RK_MPI_VENC_SetGopMode", &ser_RK_MPI_VENC_SetGopMode},
    {(char *)"RK_MPI_VENC_GetRoiAttr", &ser_RK_MPI_VENC_GetRoiAttr},
    {(char *)"RK_MPI_VENC_SetRoiAttr", &ser_RK_MPI_VENC_SetRoiAttr},
    {(char *)"RK_MPI_VENC_SetResolution", &ser_RK_MPI_VENC_SetResolution},

    {(char *)"RK_MPI_VMIX_CreateDev", &ser_RK_MPI_VMIX_CreateDev},
    {(char *)"RK_MPI_VMIX_DestroyDev", &ser_RK_MPI_VMIX_DestroyDev},
    {(char *)"RK_MPI_VMIX_EnableChn", &ser_RK_MPI_VMIX_EnableChn},
    {(char *)"RK_MPI_VMIX_DisableChn", &ser_RK_MPI_VMIX_DisableChn},
    {(char *)"RK_MPI_VMIX_SetLineInfo", &ser_RK_MPI_VMIX_SetLineInfo},
    {(char *)"RK_MPI_VMIX_ShowChn", &ser_RK_MPI_VMIX_ShowChn},
    {(char *)"RK_MPI_VMIX_HideChn", &ser_RK_MPI_VMIX_HideChn},
    {(char *)"RK_MPI_VMIX_GetChnRegionLuma", &ser_RK_MPI_VMIX_GetChnRegionLuma},
    {(char *)"RK_MPI_VMIX_RGN_SetCover", &ser_RK_MPI_VMIX_RGN_SetCover},

    {(char *)"RK_MPI_AO_SetChnAttr", &ser_RK_MPI_AO_SetChnAttr},
    {(char *)"RK_MPI_AO_EnableChn", &ser_RK_MPI_AO_EnableChn},
    {(char *)"RK_MPI_AO_DisableChn", &ser_RK_MPI_AO_DisableChn},
    {(char *)"RK_MPI_AO_SetVolume", &ser_RK_MPI_AO_SetVolume},
    {(char *)"RK_MPI_AO_GetVolume", &ser_RK_MPI_AO_GetVolume},
    {(char *)"RK_MPI_AO_SetVqeAttr", &ser_RK_MPI_AO_SetVqeAttr},
    {(char *)"RK_MPI_AO_GetVqeAttr", &ser_RK_MPI_AO_GetVqeAttr},
    {(char *)"RK_MPI_AO_EnableVqe", &ser_RK_MPI_AO_EnableVqe},
    {(char *)"RK_MPI_AO_DisableVqe", &ser_RK_MPI_AO_DisableVqe},
    {(char *)"RK_MPI_AO_QueryChnStat", &ser_RK_MPI_AO_QueryChnStat},
    {(char *)"RK_MPI_AO_ClearChnBuf", &ser_RK_MPI_AO_ClearChnBuf},

    {(char *)"RK_MPI_AENC_CreateChn", &ser_RK_MPI_AENC_CreateChn},
    {(char *)"RK_MPI_AENC_DestroyChn", &ser_RK_MPI_AENC_DestroyChn},

    {(char *)"RK_MPI_AI_SetChnAttr", &ser_RK_MPI_AI_SetChnAttr},
    {(char *)"RK_MPI_AI_EnableChn", &ser_RK_MPI_AI_EnableChn},
    {(char *)"RK_MPI_AI_DisableChn", &ser_RK_MPI_AI_DisableChn},
    {(char *)"RK_MPI_AI_SetVolume", &ser_RK_MPI_AI_SetVolume},
    {(char *)"RK_MPI_AI_GetVolume", &ser_RK_MPI_AI_GetVolume},
    {(char *)"RK_MPI_AI_SetTalkVqeAttr", &ser_RK_MPI_AI_SetTalkVqeAttr},
    {(char *)"RK_MPI_AI_GetTalkVqeAttr", &ser_RK_MPI_AI_GetTalkVqeAttr},
    {(char *)"RK_MPI_AI_SetRecordVqeAttr", &ser_RK_MPI_AI_SetRecordVqeAttr},
    {(char *)"RK_MPI_AI_GetRecordVqeAttr", &ser_RK_MPI_AI_GetRecordVqeAttr},
    {(char *)"RK_MPI_AI_EnableVqe", &ser_RK_MPI_AI_EnableVqe},
    {(char *)"RK_MPI_AI_StartStream", &ser_RK_MPI_AI_StartStream},
    {(char *)"RK_MPI_AI_DisableVqe", &ser_RK_MPI_AI_DisableVqe},

    {(char *)"event_server_run", &ser_event_server_run},
    {(char *)"example_0", &ser_example_0},
    {(char *)"example_1", &ser_example_1}};

CB_LIST_S *ser_event_list_add(MPP_CHN_S *pstChn, int fd, CB_TYPE_E type) {
  CB_LIST_S *ret = NULL;
  CB_LIST_S *tmp = NULL;
  int id = (pstChn->enModId << 16) + pstChn->s32ChnId;

  pthread_mutex_lock(&event_mutex);
  if (event_list) {
    tmp = event_list;
    while (tmp) {
      if ((tmp->id == id) && (tmp->fd == fd) && (tmp->type == type)) {
        if (type != CB_TYPE_OUT)
          ret = tmp;
        pthread_mutex_unlock(&event_mutex);
        return ret;
      }
      tmp = tmp->next;
    }
  }
  if (ret == NULL) {
    tmp = (CB_LIST_S *)malloc(sizeof(CB_LIST_S));
    tmp->next = event_list;
    memcpy(&tmp->stChn, pstChn, sizeof(MPP_CHN_S));
    tmp->id = id;
    tmp->fd = fd;
    tmp->type = type;
    event_list = tmp;
    ret = event_list;
  }

  pthread_mutex_unlock(&event_mutex);

  return ret;
}

void ser_event_list_del_fd(int fd) {
  pthread_mutex_lock(&event_mutex);
again:
  if (event_list) {
    CB_LIST_S *tmp = event_list;
    CB_LIST_S *next = tmp->next;
    if (tmp->fd == fd) {
      event_list = tmp->next;
      free(tmp);
      goto again;
    }
    while (next) {
      if (next->fd == fd) {
        tmp->next = next->next;
        free(next);
      }
      tmp = next;
      next = tmp->next;
    }
  }
  pthread_mutex_unlock(&event_mutex);
}

int ser_evet_list_get(int id, int *fd, CB_TYPE_E type) {
  int ret = 0;

  pthread_mutex_lock(&event_mutex);
  if (event_list) {
    CB_LIST_S *tmp = event_list;
    while (tmp) {
      if ((tmp->id == id) && (tmp->type == type)) {
        ret = 1;
        *fd = tmp->fd;
        break;
      }
      tmp = tmp->next;
    }
  }
  pthread_mutex_unlock(&event_mutex);

  return ret;
}

void ser_out_cb(MEDIA_BUFFER mb) {
  pthread_mutex_lock(&event_mutex);
  CB_LIST_S *tmp = event_list;
  MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
  int id = (mb_impl->mode_id << 16) + mb_impl->chn_id;
  while (tmp) {
    if ((tmp->id == id) && (tmp->type == CB_TYPE_OUT)) {
      if (sock_write(tmp->fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED)
        goto out;
      pthread_mutex_unlock(&event_mutex);
      return;
    }
    tmp = tmp->next;
  }
out:
  pthread_mutex_unlock(&event_mutex);

  RK_MPI_MB_ReleaseBuffer(mb);
}

void ser_event_cb(RK_VOID *pHandle, RK_VOID *pstEvent) {
  pthread_mutex_lock(&event_mutex);
  CB_LIST_S *tmp = event_list;
  MPP_CHN_S *pstChn = (MPP_CHN_S *)pHandle;
  int id = (pstChn->enModId << 16) + pstChn->s32ChnId;

  while (tmp) {
    if ((tmp->id == id) && (tmp->type == CB_TYPE_EVENT)) {
      int len;

      if (pstChn->enModId == RK_ID_ALGO_OD) {
        len = sizeof(OD_EVENT_S);
        sock_write(tmp->fd, &len, sizeof(int));
        sock_write(tmp->fd, pstEvent, sizeof(OD_EVENT_S));
      } else if (pstChn->enModId == RK_ID_ALGO_MD) {
        len = sizeof(MD_EVENT_S);
        sock_write(tmp->fd, &len, sizeof(int));
        sock_write(tmp->fd, pstEvent, sizeof(MD_EVENT_S));
      }
    }
    tmp = tmp->next;
  }

  pthread_mutex_unlock(&event_mutex);
}

int ser_event_server_run(int fd) {
  int ret = 0;
  MPP_CHN_S stChn;
  CB_LIST_S *node = NULL;
  CB_TYPE_E type;

  if (sock_read(fd, &stChn, sizeof(MPP_CHN_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (sock_read(fd, &type, sizeof(CB_TYPE_E)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  node = ser_event_list_add(&stChn, fd, type);

  if (node) {
    if (type == CB_TYPE_EVENT)
      RK_MPI_SYS_RegisterEventCb(&stChn, &stChn, ser_event_cb);
    else if (type == CB_TYPE_OUT)
      RK_MPI_SYS_RegisterOutCb(&stChn, ser_out_cb);
    if (sock_read(fd, &ret, sizeof(int)) == SOCKERR_CLOSED)
      ret = -1;
  }

  ser_event_list_del_fd(fd);
out:

  return ret;
}

int ser_example_0(int fd) {
  printf("%s fd = %d\n", __func__, fd);
  // example_0(void);

  return 0;
}

int ser_example_1(int fd) {
  int ret = 0;
  int a, b, c, d;
  printf("%s fd = %d\n", __func__, fd);

  if (sock_read(fd, &a, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &b, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  c = 11;
  d = 10;
  printf("%s, a = %d, b = %d, c = %d, ret = %d\n", __func__, a, b, c, ret);
  // d = example_1(a, b, &c);
  if (sock_write(fd, &c, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &d, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

out:
  return ret;
}

int ser_RK_MPI_SYS_Init(int fd) {
  int ret = 0;
  int err = 0;

  err = RK_MPI_SYS_Init();
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED)
    ret = -1;

  return ret;
}

int ser_RK_MPI_SYS_Bind(int fd) {
  int ret = 0;
  int err = 0;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;

  if (sock_read(fd, &stSrcChn, sizeof(MPP_CHN_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stDestChn, sizeof(MPP_CHN_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_SYS_UnBind(int fd) {
  int ret = 0;
  int err = 0;
  MPP_CHN_S stSrcChn;
  MPP_CHN_S stDestChn;

  if (sock_read(fd, &stSrcChn, sizeof(MPP_CHN_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stDestChn, sizeof(MPP_CHN_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_SYS_SendMediaBuffer(int fd) {
  int ret = 0;
  MOD_ID_E enModID;
  RK_S32 s32ChnID;
  MEDIA_BUFFER mb;
  int err = 0;

  if (sock_read(fd, &enModID, sizeof(MOD_ID_E)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32ChnID, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_SYS_SendMediaBuffer(enModID, s32ChnID, mb);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_SYS_GetMediaBuffer(int fd) {
  int ret = 0;
  MOD_ID_E enModID;
  RK_S32 s32ChnID;
  RK_S32 s32MilliSec;
  MEDIA_BUFFER mb;

  if (sock_read(fd, &enModID, sizeof(MOD_ID_E)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32ChnID, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32MilliSec, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  mb = RK_MPI_SYS_GetMediaBuffer(enModID, s32ChnID, s32MilliSec);

  if (sock_write(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_SetChnAttr(int fd) {
  int ret = 0;
  int err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;
  VI_CHN_ATTR_S stChnAttr;
  int len;
  char *pcVideoNode = NULL;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stChnAttr, sizeof(VI_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (len) {
    pcVideoNode = (char *)malloc(len);
    if (sock_read(fd, pcVideoNode, len) == SOCKERR_CLOSED) {
      ret = -1;
      free(pcVideoNode);
      goto out;
    }

    for (unsigned int i = 0; i < CONST_STRING_NUM; i++) {
      if (const_string[i]) {
        if (strcmp(pcVideoNode, const_string[i]) == 0) {
          stChnAttr.pcVideoNode = const_string[i];
          free(pcVideoNode);
          break;
        }
      } else {
        const_string[i] = pcVideoNode;
        stChnAttr.pcVideoNode = const_string[i];
        break;
      }
    }
  }

  err = RK_MPI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_EnableChn(int fd) {
  int ret = 0;
  int err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_VI_EnableChn(ViPipe, ViChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_DisableChn(int fd) {
  int ret = 0;
  int err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_VI_DisableChn(ViPipe, ViChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_StartRegionLuma(int fd) {
  int ret = 0;
  RK_S32 err = 0;
  VI_CHN ViChn;

  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_VI_StartRegionLuma(ViChn);
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_StopRegionLuma(int fd) {
  int ret = 0;
  int err = 0;
  VI_CHN ViChn;

  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_VI_StopRegionLuma(ViChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_GetChnRegionLuma(int fd) {
  int ret = 0;
  RK_S32 err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;
  VIDEO_REGION_INFO_S stRegionInfo;
  RK_U64 u64LumaData[2];
  RK_S32 s32MilliSec;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRegionInfo, sizeof(VIDEO_REGION_INFO_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32MilliSec, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VI_GetChnRegionLuma(ViPipe, ViChn, &stRegionInfo, u64LumaData,
                                   s32MilliSec);

  if (sock_write(fd, &u64LumaData, sizeof(RK_U64) * 2) == SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_RGN_SetCover(int fd) {
  int ret = 0;
  int err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;
  OSD_REGION_INFO_S stRgnInfo;
  COVER_INFO_S stCoverInfo;
  int haveinfo;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRgnInfo, sizeof(OSD_REGION_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &haveinfo, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (haveinfo) {
    if (sock_read(fd, &stCoverInfo, sizeof(COVER_INFO_S)) == SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }
    err = RK_MPI_VI_RGN_SetCover(ViPipe, ViChn, &stRgnInfo, &stCoverInfo);
  } else {
    err = RK_MPI_VI_RGN_SetCover(ViPipe, ViChn, &stRgnInfo, NULL);
  }

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VI_StartStream(int fd) {
  int ret = 0;
  int err = 0;
  VI_PIPE ViPipe;
  VI_CHN ViChn;

  if (sock_read(fd, &ViPipe, sizeof(VI_PIPE)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &ViChn, sizeof(VI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VI_StartStream(ViPipe, ViChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_RGA_CreateChn(int fd) {
  int ret = 0;
  int err = 0;
  RGA_CHN RgaChn;
  RGA_ATTR_S stAttr;

  if (sock_read(fd, &RgaChn, sizeof(RGA_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(RGA_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  err = RK_MPI_RGA_CreateChn(RgaChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_RGA_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  RGA_CHN RgaChn;

  if (sock_read(fd, &RgaChn, sizeof(RGA_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_RGA_DestroyChn(RgaChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_RGA_GetChnAttr(int fd) {
  int ret = 0;
  int err = 0;
  RGA_CHN RgaChn;
  RGA_ATTR_S stAttr;

  if (sock_read(fd, &RgaChn, sizeof(RGA_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_RGA_GetChnAttr(RgaChn, &stAttr);
  if (sock_write(fd, &stAttr, sizeof(RGA_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_RGA_SetChnAttr(int fd) {
  int ret = 0;
  int err = 0;
  RGA_CHN RgaChn;
  RGA_ATTR_S stAttr;

  if (sock_read(fd, &RgaChn, sizeof(RGA_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(RGA_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_RGA_SetChnAttr(RgaChn, &stAttr);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VO_CreateChn(int fd) {
  VO_CHN VoChn;
  VO_CHN_ATTR_S stAttr;
  int ret = 0;
  int err = 0;
  int len;
  char *pcDevNode = NULL;

  if (sock_read(fd, &VoChn, sizeof(VO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(VO_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (len) {
    pcDevNode = (char *)malloc(len);
    if (sock_read(fd, pcDevNode, len) == SOCKERR_CLOSED) {
      ret = -1;
      free(pcDevNode);
      goto out;
    }

    for (unsigned int i = 0; i < CONST_STRING_NUM; i++) {
      if (const_string[i]) {
        if (strcmp(pcDevNode, const_string[i]) == 0) {
          stAttr.pcDevNode = const_string[i];
          free(pcDevNode);
          break;
        }
      } else {
        const_string[i] = pcDevNode;
        stAttr.pcDevNode = const_string[i];
        break;
      }
    }
  }

  err = RK_MPI_VO_CreateChn(VoChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VO_GetChnAttr(int fd) {
  VO_CHN VoChn;
  VO_CHN_ATTR_S stAttr;
  int ret = 0;
  int err = 0;
  int len;
  char *pcDevNode = NULL;

  if (sock_read(fd, &VoChn, sizeof(VO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VO_GetChnAttr(VoChn, &stAttr);

  sock_write(fd, &stAttr, sizeof(VO_CHN_ATTR_S));
  if (stAttr.pcDevNode) {
    len = strlen(stAttr.pcDevNode) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, stAttr.pcDevNode, len);
  } else {
    len = 0;
    sock_write(fd, &len, sizeof(int));
  }

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }

out:
  if (pcDevNode)
    free(pcDevNode);

  return ret;
}

int ser_RK_MPI_VO_SetChnAttr(int fd) {
  VO_CHN VoChn;
  VO_CHN_ATTR_S stAttr;
  int ret = 0;
  int err = 0;
  int len;
  char *pcDevNode = NULL;

  if (sock_read(fd, &VoChn, sizeof(VO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(VO_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (len) {
    pcDevNode = (char *)malloc(len);
    if (sock_read(fd, pcDevNode, len) == SOCKERR_CLOSED) {
      ret = -1;
      free(pcDevNode);
      goto out;
    }

    for (unsigned int i = 0; i < CONST_STRING_NUM; i++) {
      if (const_string[i]) {
        if (strcmp(pcDevNode, const_string[i]) == 0) {
          stAttr.pcDevNode = const_string[i];
          free(pcDevNode);
          break;
        }
      } else {
        const_string[i] = pcDevNode;
        stAttr.pcDevNode = const_string[i];
        break;
      }
    }
  }

  err = RK_MPI_VO_SetChnAttr(VoChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VO_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  VO_CHN VoChn;

  if (sock_read(fd, &VoChn, sizeof(VO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VO_DestroyChn(VoChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_MD_CreateChn(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_MD_CHN MdChn;
  ALGO_MD_ATTR_S stChnAttr;

  if (sock_read(fd, &MdChn, sizeof(ALGO_MD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stChnAttr, sizeof(ALGO_MD_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_MD_CreateChn(MdChn, &stChnAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_MD_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_MD_CHN MdChn;

  if (sock_read(fd, &MdChn, sizeof(ALGO_MD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_MD_DestroyChn(MdChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_MD_EnableSwitch(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_MD_CHN MdChn;
  RK_BOOL bEnable;

  if (sock_read(fd, &MdChn, sizeof(ALGO_MD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &bEnable, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_MD_EnableSwitch(MdChn, bEnable);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_OD_CreateChn(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_OD_CHN OdChn;
  ALGO_OD_ATTR_S stChnAttr;

  if (sock_read(fd, &OdChn, sizeof(ALGO_OD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stChnAttr, sizeof(ALGO_OD_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_OD_CreateChn(OdChn, &stChnAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_OD_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_OD_CHN OdChn;

  if (sock_read(fd, &OdChn, sizeof(ALGO_OD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_OD_DestroyChn(OdChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ALGO_OD_EnableSwitch(int fd) {
  int ret = 0;
  int err = 0;
  ALGO_OD_CHN OdChn;
  RK_BOOL bEnable;

  if (sock_read(fd, &OdChn, sizeof(ALGO_OD_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &bEnable, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ALGO_OD_EnableSwitch(OdChn, bEnable);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ADEC_CreateChn(int fd) {
  int ret = 0;
  int err = 0;
  ADEC_CHN AdecChn;
  ADEC_CHN_ATTR_S stAttr;

  if (sock_read(fd, &AdecChn, sizeof(ADEC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(ADEC_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ADEC_CreateChn(AdecChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_ADEC_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  ADEC_CHN AdecChn;

  if (sock_read(fd, &AdecChn, sizeof(ADEC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_ADEC_DestroyChn(AdecChn);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_ReleaseBuffer(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  int err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_ReleaseBuffer(mb);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetPtr(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  int devfd = 0;
  int handle = 0;
  int size = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  handle = RK_MPI_MB_GetHandle(mb);
  size = RK_MPI_MB_GetSize(mb);
  devfd = RK_MPI_MB_GetDevFD(mb);

  sock_send_devfd(fd, devfd);

  if (sock_write(fd, &handle, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &size, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetFD(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  int devfd = 0;
  int handle = 0;
  int size = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  handle = RK_MPI_MB_GetHandle(mb);
  size = RK_MPI_MB_GetSize(mb);
  devfd = RK_MPI_MB_GetDevFD(mb);

  sock_send_devfd(fd, devfd);

  if (sock_write(fd, &handle, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &size, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }

out:

  return ret;
}

int ser_RK_MPI_MB_GetImageInfo(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  int err = 0;
  MB_IMAGE_INFO_S stImageInfo;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);
  if (sock_write(fd, &stImageInfo, sizeof(MB_IMAGE_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
  }

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetSize(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  size_t err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetSize(mb);

  if (sock_write(fd, &err, sizeof(size_t)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetModeID(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  MOD_ID_E err;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetModeID(mb);

  if (sock_write(fd, &err, sizeof(MOD_ID_E)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetChannelID(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_S16 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetChannelID(mb);

  if (sock_write(fd, &err, sizeof(RK_S16)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetTimestamp(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_U64 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetTimestamp(mb);

  if (sock_write(fd, &err, sizeof(RK_U64)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_SetTimestamp(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_U64 timestamp;
  RK_S32 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &timestamp, sizeof(RK_U64)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_SetTimestamp(mb, timestamp);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_SetSize(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_U32 size;
  RK_S32 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &size, sizeof(RK_U32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_SetSize(mb, size);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_CreateBuffer(int fd) {
  int ret = 0;
  RK_U32 u32Size;
  RK_BOOL boolHardWare;
  RK_U8 u8Flag;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &u32Size, sizeof(RK_U32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &boolHardWare, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &u8Flag, sizeof(RK_U8)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_CreateBuffer(u32Size, boolHardWare, u8Flag);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_CreateImageBuffer(int fd) {
  int ret = 0;
  MB_IMAGE_INFO_S stImageInfo;
  RK_BOOL boolHardWare;
  RK_U8 u8Flag;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &stImageInfo, sizeof(MB_IMAGE_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &boolHardWare, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &u8Flag, sizeof(RK_U8)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_CreateImageBuffer(&stImageInfo, boolHardWare, u8Flag);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_CreateAudioBuffer(int fd) {
  int ret = 0;
  RK_U32 u32BufferSize;
  RK_BOOL boolHardWare;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &u32BufferSize, sizeof(RK_U32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &boolHardWare, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_CreateAudioBuffer(u32BufferSize, boolHardWare);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_POOL_Create(int fd) {
  int ret = 0;
  MB_POOL_PARAM_S stPoolParam;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &stPoolParam, sizeof(MB_POOL_PARAM_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_POOL_Create(&stPoolParam);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_POOL_Destroy(int fd) {
  int ret = 0;
  MEDIA_BUFFER_POOL MBPHandle;
  RK_S32 err = 0;

  if (sock_read(fd, &MBPHandle, sizeof(MEDIA_BUFFER_POOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_POOL_Destroy(MBPHandle);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_POOL_GetBuffer(int fd) {
  int ret = 0;
  MEDIA_BUFFER_POOL MBPHandle;
  RK_BOOL bIsBlock;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &MBPHandle, sizeof(MEDIA_BUFFER_POOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &bIsBlock, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_POOL_GetBuffer(MBPHandle, bIsBlock);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_ConvertToImgBuffer(int fd) {
  MEDIA_BUFFER mb;
  MB_IMAGE_INFO_S stImageInfo;
  int ret = 0;
  MEDIA_BUFFER err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stImageInfo, sizeof(MB_IMAGE_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_ConvertToImgBuffer(mb, &stImageInfo);

  if (sock_write(fd, &err, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetFlag(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_S32 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetFlag(mb);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_GetTsvcLevel(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_S32 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_GetTsvcLevel(mb);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_IsViFrame(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_BOOL err;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_IsViFrame(mb);

  if (sock_write(fd, &err, sizeof(RK_BOOL)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_MB_TsNodeDump(int fd) {
  int ret = 0;
  MEDIA_BUFFER mb;
  RK_S32 err = 0;

  if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_MB_TsNodeDump(mb);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VDEC_CreateChn(int fd) {
  int ret = 0;
  VDEC_CHN VdChn;
  VDEC_CHN_ATTR_S stAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VdChn, sizeof(VDEC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(VDEC_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VDEC_CreateChn(VdChn, &stAttr);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VDEC_DestroyChn(int fd) {
  int ret = 0;
  VDEC_CHN VdChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VdChn, sizeof(VDEC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VDEC_DestroyChn(VdChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_CreateChn(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_CHN_ATTR_S stVencChnAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVencChnAttr, sizeof(VENC_CHN_ATTR_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_DestroyChn(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_DestroyChn(VencChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_CreateJpegLightChn(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_CHN_ATTR_S stVencChnAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVencChnAttr, sizeof(VENC_CHN_ATTR_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_CreateJpegLightChn(VencChn, &stVencChnAttr);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_GetVencChnAttr(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_CHN_ATTR_S stVencChnAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_GetVencChnAttr(VencChn, &stVencChnAttr);

  if (sock_write(fd, &stVencChnAttr, sizeof(VENC_CHN_ATTR_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetVencChnAttr(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_CHN_ATTR_S stVencChnAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVencChnAttr, sizeof(VENC_CHN_ATTR_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetVencChnAttr(VencChn, &stVencChnAttr);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_GetRcParam(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_RC_PARAM_S stRcParam;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_GetRcParam(VencChn, &stRcParam);

  if (sock_write(fd, &stRcParam, sizeof(VENC_RC_PARAM_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetRcParam(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_RC_PARAM_S stRcParam;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRcParam, sizeof(VENC_RC_PARAM_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetRcParam(VencChn, &stRcParam);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetJpegParam(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_JPEG_PARAM_S stJpegParam;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stJpegParam, sizeof(VENC_JPEG_PARAM_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetJpegParam(VencChn, &stJpegParam);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_RGN_Init(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_COLOR_TBL_S stColorTbl;
  int havetbl = 0;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &havetbl, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (havetbl) {
    if (sock_read(fd, &stColorTbl, sizeof(VENC_COLOR_TBL_S)) ==
        SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }
    err = RK_MPI_VENC_RGN_Init(VencChn, &stColorTbl);
  } else {
    err = RK_MPI_VENC_RGN_Init(VencChn, NULL);
  }

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_RGN_SetBitMap(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  OSD_REGION_INFO_S stRgnInfo;
  BITMAP_S stBitmap;
  int len;
  char *pData = NULL;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRgnInfo, sizeof(OSD_REGION_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stBitmap, sizeof(BITMAP_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (len > 0) {
    pData = (char *)malloc(len);
    if (sock_read(fd, pData, len) == SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }
    stBitmap.pData = pData;
  }

  err = RK_MPI_VENC_RGN_SetBitMap(VencChn, &stRgnInfo, &stBitmap);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:
  if (pData)
    free(pData);

  return ret;
}

int ser_RK_MPI_VENC_StartRecvFrame(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_RECV_PIC_PARAM_S stRecvParam;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRecvParam, sizeof(VENC_RECV_PIC_PARAM_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_RGN_SetCover(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  OSD_REGION_INFO_S stRgnInfo;
  COVER_INFO_S stCoverInfo;
  int haveinfo;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRgnInfo, sizeof(OSD_REGION_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &haveinfo, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (haveinfo) {
    if (sock_read(fd, &stCoverInfo, sizeof(COVER_INFO_S)) == SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }

    err = RK_MPI_VENC_RGN_SetCover(VencChn, &stRgnInfo, &stCoverInfo);
  } else {
    err = RK_MPI_VENC_RGN_SetCover(VencChn, &stRgnInfo, NULL);
  }

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_RGN_SetCoverEx(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  OSD_REGION_INFO_S stRgnInfo;
  COVER_INFO_S stCoverInfo;
  int haveinfo;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRgnInfo, sizeof(OSD_REGION_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &haveinfo, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (haveinfo) {
    if (sock_read(fd, &stCoverInfo, sizeof(COVER_INFO_S)) == SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }

    err = RK_MPI_VENC_RGN_SetCoverEx(VencChn, &stRgnInfo, &stCoverInfo);
  } else {
    err = RK_MPI_VENC_RGN_SetCoverEx(VencChn, &stRgnInfo, NULL);
  }

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetGopMode(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_GOP_ATTR_S stGopModeAttr;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stGopModeAttr, sizeof(VENC_GOP_ATTR_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetGopMode(VencChn, &stGopModeAttr);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_GetRoiAttr(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_ROI_ATTR_S stRoiAttr;
  RK_S32 roi_index;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &roi_index, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_GetRoiAttr(VencChn, &stRoiAttr, roi_index);

  if (sock_write(fd, &stRoiAttr, sizeof(VENC_ROI_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetRoiAttr(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_ROI_ATTR_S stRoiAttr;
  RK_S32 region_cnt;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRoiAttr, sizeof(VENC_ROI_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &region_cnt, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetRoiAttr(VencChn, &stRoiAttr, region_cnt);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VENC_SetResolution(int fd) {
  int ret = 0;
  VENC_CHN VencChn;
  VENC_RESOLUTION_PARAM_S stResolutionParam;
  RK_S32 err = 0;

  if (sock_read(fd, &VencChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stResolutionParam, sizeof(VENC_RESOLUTION_PARAM_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VENC_SetResolution(VencChn, stResolutionParam);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_CreateDev(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_DEV_INFO_S stDevInfo;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stDevInfo, sizeof(VMIX_DEV_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_CreateDev(VmDev, &stDevInfo);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_DestroyDev(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_DestroyDev(VmDev);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_EnableChn(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_EnableChn(VmDev, VmChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_DisableChn(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_DisableChn(VmDev, VmChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_SetLineInfo(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  VMIX_LINE_INFO_S VmLine;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmLine, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_SetLineInfo(VmDev, VmChn, VmLine);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_ShowChn(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_ShowChn(VmDev, VmChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_HideChn(int fd) {
  int ret = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  RK_S32 err = 0;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_HideChn(VmDev, VmChn);

  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_GetChnRegionLuma(int fd) {
  int ret = 0;
  RK_S32 err = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  VIDEO_REGION_INFO_S stRegionInfo;
  RK_U64 u64LumaData[2];
  RK_S32 s32MilliSec;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRegionInfo, sizeof(VIDEO_REGION_INFO_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32MilliSec, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_VMIX_GetChnRegionLuma(VmDev, VmChn, &stRegionInfo, u64LumaData,
                                     s32MilliSec);

  if (sock_write(fd, &u64LumaData, sizeof(RK_U64) * 2) == SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &err, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_VMIX_RGN_SetCover(int fd) {
  int ret = 0;
  int err = 0;
  VMIX_DEV VmDev;
  VMIX_CHN VmChn;
  OSD_REGION_INFO_S stRgnInfo;
  COVER_INFO_S stCoverInfo;
  int haveinfo;

  if (sock_read(fd, &VmDev, sizeof(VMIX_DEV)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &VmChn, sizeof(VMIX_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stRgnInfo, sizeof(OSD_REGION_INFO_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &haveinfo, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (haveinfo) {
    if (sock_read(fd, &stCoverInfo, sizeof(COVER_INFO_S)) == SOCKERR_CLOSED) {
      ret = -1;
      goto out;
    }
    err = RK_MPI_VMIX_RGN_SetCover(VmDev, VmChn, &stRgnInfo, &stCoverInfo);
  } else {
    err = RK_MPI_VMIX_RGN_SetCover(VmDev, VmChn, &stRgnInfo, NULL);
  }

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_SetChnAttr(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  AO_CHN_ATTR_S stAttr;
  int len;
  char *pcNode = NULL;

  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(AO_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (len) {
    pcNode = (char *)malloc(len);
    if (sock_read(fd, pcNode, len) == SOCKERR_CLOSED) {
      ret = -1;
      free(pcNode);
      goto out;
    }

    for (unsigned int i = 0; i < CONST_STRING_NUM; i++) {
      if (const_string[i]) {
        if (strcmp(pcNode, const_string[i]) == 0) {
          stAttr.pcAudioNode = (char *)const_string[i];
          free(pcNode);
          break;
        }
      } else {
        const_string[i] = pcNode;
        stAttr.pcAudioNode = (char *)const_string[i];
        break;
      }
    }
  }

  err = RK_MPI_AO_SetChnAttr(AoChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_EnableChn(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_EnableChn(AoChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_DisableChn(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_DisableChn(AoChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_SetVolume(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  RK_S32 s32Volume;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32Volume, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_SetVolume(AoChn, s32Volume);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_GetVolume(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  RK_S32 s32Volume;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_GetVolume(AoChn, &s32Volume);

  if (sock_write(fd, &s32Volume, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_SetVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  AO_VQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVqeConfig, sizeof(AO_VQE_CONFIG_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_SetVqeAttr(AoChn, &stVqeConfig);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_GetVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  AO_VQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_GetVqeAttr(AoChn, &stVqeConfig);

  if (sock_write(fd, &stVqeConfig, sizeof(AO_VQE_CONFIG_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_EnableVqe(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_EnableVqe(AoChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_DisableVqe(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_DisableVqe(AoChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_QueryChnStat(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  AO_CHN_STATE_S stStatus;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_QueryChnStat(AoChn, &stStatus);

  if (sock_write(fd, &stStatus, sizeof(AO_CHN_STATE_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AO_ClearChnBuf(int fd) {
  int ret = 0;
  int err = 0;
  AO_CHN AoChn;
  if (sock_read(fd, &AoChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AO_ClearChnBuf(AoChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AENC_CreateChn(int fd) {
  int ret = 0;
  int err = 0;
  AENC_CHN AencChn;
  AENC_CHN_ATTR_S stAttr;
  if (sock_read(fd, &AencChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(AENC_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AENC_CreateChn(AencChn, &stAttr);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AENC_DestroyChn(int fd) {
  int ret = 0;
  int err = 0;
  AENC_CHN AencChn;
  if (sock_read(fd, &AencChn, sizeof(AO_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AENC_DestroyChn(AencChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_SetChnAttr(int fd) {
  int ret = 0;
  int err = 0;
  VENC_CHN AiChn;
  AI_CHN_ATTR_S stAttr;
  int len;
  char *pcNode = NULL;

  if (sock_read(fd, &AiChn, sizeof(VENC_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stAttr, sizeof(AI_CHN_ATTR_S)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  if (len) {
    pcNode = (char *)malloc(len);
    if (sock_read(fd, pcNode, len) == SOCKERR_CLOSED) {
      ret = -1;
      free(pcNode);
      goto out;
    }

    for (unsigned int i = 0; i < CONST_STRING_NUM; i++) {
      if (const_string[i]) {
        if (strcmp(pcNode, const_string[i]) == 0) {
          stAttr.pcAudioNode = (char *)const_string[i];
          free(pcNode);
          break;
        }
      } else {
        const_string[i] = pcNode;
        stAttr.pcAudioNode = (char *)const_string[i];
        break;
      }
    }
  }

  err = RK_MPI_AI_SetChnAttr(AiChn, &stAttr);
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_EnableChn(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_EnableChn(AiChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_DisableChn(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_DisableChn(AiChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_SetVolume(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  RK_S32 s32Volume;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &s32Volume, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_SetVolume(AiChn, s32Volume);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_GetVolume(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  RK_S32 s32Volume;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_GetVolume(AiChn, &s32Volume);

  if (sock_write(fd, &s32Volume, sizeof(RK_S32)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_SetTalkVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  AI_TALKVQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVqeConfig, sizeof(AI_TALKVQE_CONFIG_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
  }

  err = RK_MPI_AI_SetTalkVqeAttr(AiChn, &stVqeConfig);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_GetTalkVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  AI_TALKVQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_GetTalkVqeAttr(AiChn, &stVqeConfig);

  if (sock_write(fd, &stVqeConfig, sizeof(AI_TALKVQE_CONFIG_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_SetRecordVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  AI_RECORDVQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (sock_read(fd, &stVqeConfig, sizeof(AI_RECORDVQE_CONFIG_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
  }

  err = RK_MPI_AI_SetRecordVqeAttr(AiChn, &stVqeConfig);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_GetRecordVqeAttr(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  AI_RECORDVQE_CONFIG_S stVqeConfig;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_GetRecordVqeAttr(AiChn, &stVqeConfig);

  if (sock_write(fd, &stVqeConfig, sizeof(AI_RECORDVQE_CONFIG_S)) ==
      SOCKERR_CLOSED) {
    ret = -1;
  }
  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_EnableVqe(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_EnableVqe(AiChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_StartStream(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_StartStream(AiChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

int ser_RK_MPI_AI_DisableVqe(int fd) {
  int ret = 0;
  int err = 0;
  AI_CHN AiChn;
  if (sock_read(fd, &AiChn, sizeof(AI_CHN)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  err = RK_MPI_AI_DisableVqe(AiChn);

  if (sock_write(fd, &err, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
  }
out:

  return ret;
}

static void *rec_thread(void *arg) {
  int fd = (int)arg;
  char *name = NULL;
  int len;
  int ret = 0;
  int i;
  int maplen = sizeof(map) / sizeof(struct FunMap);
  pthread_detach(pthread_self());

  if (sock_write(fd, &ret, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
again:
  if (sock_read(fd, &len, sizeof(int)) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }
  if (len <= 0) {
    ret = -1;
    goto out;
  }
  name = (char *)malloc(len);
  if (sock_read(fd, name, len) == SOCKERR_CLOSED) {
    ret = -1;
    goto out;
  }

  for (i = 0; i < maplen; i++) {
    // printf("%s, %s\n", map[i].fun_name, name);
    if (!strcmp(map[i].fun_name, name)) {
      ret = map[i].fun(fd);
    }
  }
out:
  if (name)
    free(name);
  name = NULL;
  if (ret == 0) {
    sock_write(fd, &ret, sizeof(int));
    goto again;
  }

  close(fd);
  pthread_exit(NULL);

  return 0;
}

static void *rkmedia_server_thread(void *arg) {
  int clifd;
  RKMEDIA_LOGD("#Start %s thread, arg:%p\n", __func__, arg);
  pthread_detach(pthread_self());

  if ((listenfd = serv_listen(CS_PATH)) < 0)
    printf("listen fail\n");

  while (RkMediaServerRun) {
    pthread_t thread_id;

    if ((clifd = serv_accept(listenfd)) < 0) {
      printf("accept fail\n");
    }

    if (clifd >= 0)
      pthread_create(&thread_id, NULL, rec_thread, (void *)clifd);
  }
  RkMediaServerTid = 0;
  pthread_exit(NULL);
  return 0;
}

void handle_pipe(int sig) { printf("%s sig = %d\n", __func__, sig); }

int rkmedia_server_run(void) {
  struct sigaction action;
  action.sa_handler = handle_pipe;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGPIPE, &action, NULL);

  RkMediaServerRun = 1;
  if (RkMediaServerTid == 0)
    pthread_create(&RkMediaServerTid, NULL, rkmedia_server_thread, NULL);
  return 0;
}
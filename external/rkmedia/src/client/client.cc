#include <assert.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>

#include "../socket/socket.h"
#include "rkmedia_api.h"

#define CONST_STRING_NUM 256
const char *const_string[CONST_STRING_NUM] = {NULL};

typedef enum rkCB_TYPE { CB_TYPE_EVENT = 0, CB_TYPE_OUT } CB_TYPE_E;

typedef struct EVENT_CALL_LIST {
  int id;
  MPP_CHN_S stChn;
  void *cb;
  RK_VOID *pHandle;
  CB_TYPE_E type;
  struct EVENT_CALL_LIST *next;
} EVENT_CALL_LIST_S;

typedef struct MB_LIST {
  MEDIA_BUFFER mb;
  void *ptr;
  int devfd;
  int fd;
  int handle;
  int size;
  struct MB_LIST *next;
} MB_LIST_S;

static EVENT_CALL_LIST_S *event_cb_list = NULL;
static pthread_mutex_t event_mutex = PTHREAD_MUTEX_INITIALIZER;

static MB_LIST_S *mb_list = NULL;
static pthread_mutex_t mb_mutex = PTHREAD_MUTEX_INITIALIZER;

MB_LIST_S *mb_list_add(MEDIA_BUFFER mb, void *ptr, int devfd, int fd,
                       int handle, int size) {
  MB_LIST_S *ret = NULL;
  MB_LIST_S *tmp = NULL;

  pthread_mutex_lock(&mb_mutex);
  if (mb_list) {
    tmp = mb_list;
    while (tmp) {
      if (tmp->mb == mb) {
        ret = tmp;
        break;
      }
      tmp = tmp->next;
    }
  }

  if (ret == NULL) {
    tmp = (MB_LIST_S *)malloc(sizeof(MB_LIST_S));
    tmp->next = mb_list;
    tmp->mb = mb;
    tmp->ptr = ptr;
    tmp->devfd = devfd;
    tmp->fd = fd;
    tmp->handle = handle;
    tmp->size = size;
    mb_list = tmp;
    ret = mb_list;
  }

  pthread_mutex_unlock(&mb_mutex);

  return ret;
}

int mb_list_update(MEDIA_BUFFER mb, MB_LIST_S *info) {
  int ret = -1;
  MB_LIST_S *tmp = NULL;
  pthread_mutex_lock(&mb_mutex);
  if (mb_list) {
    tmp = mb_list;
    while (tmp) {
      if (tmp->mb == mb) {
        break;
      }
      tmp = tmp->next;
    }
  }

  if (tmp) {
    ret = 0;
    tmp->ptr = info->ptr;
    tmp->devfd = info->devfd;
    tmp->fd = info->fd;
    tmp->handle = info->handle;
    tmp->size = info->size;
  }

  pthread_mutex_unlock(&mb_mutex);

  return ret;
}

int mb_list_get(MEDIA_BUFFER mb, MB_LIST_S *info) {
  int ret = -1;
  MB_LIST_S *tmp = NULL;
  pthread_mutex_lock(&mb_mutex);
  if (mb_list) {
    tmp = mb_list;
    while (tmp) {
      if (tmp->mb == mb) {
        break;
      }
      tmp = tmp->next;
    }
  }

  if (tmp) {
    ret = 0;
    memcpy(info, tmp, sizeof(MB_LIST_S));
  }

  pthread_mutex_unlock(&mb_mutex);

  return ret;
}

void del_list_del(MEDIA_BUFFER mb) {
  pthread_mutex_lock(&mb_mutex);
again:
  if (mb_list) {
    MB_LIST_S *tmp = mb_list;
    MB_LIST_S *next = tmp->next;
    if (tmp->mb == mb) {
      mb_list = tmp->next;
      free(tmp);
      goto again;
    }
    while (next) {
      if (next->mb == mb) {
        tmp->next = next->next;
        free(next);
      }
      tmp = next;
      next = tmp->next;
    }
  }
  pthread_mutex_unlock(&mb_mutex);
}

void *MbMap(int devfd, int handle, int size) {
  struct drm_mode_map_dumb dmmd;
  memset(&dmmd, 0, sizeof(dmmd));
  dmmd.handle = handle;
  if (drmIoctl(devfd, DRM_IOCTL_MODE_MAP_DUMB, &dmmd))
    printf("Failed to map dumb\n");

  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, devfd,
              dmmd.offset);
}

int MbHandleToFD(int devfd, int handle) {
  int fd;

  drmPrimeHandleToFD(devfd, handle, DRM_CLOEXEC, &fd);

  return fd;
}

EVENT_CALL_LIST_S *event_list_add(const MPP_CHN_S *pstChn, RK_VOID *pHandle,
                                  void *cb, CB_TYPE_E type) {
  EVENT_CALL_LIST_S *ret = NULL;
  EVENT_CALL_LIST_S *tmp = NULL;
  int id = (pstChn->enModId << 16) + pstChn->s32ChnId;
  pthread_mutex_lock(&event_mutex);
  if (event_cb_list) {
    tmp = event_cb_list;
    while (tmp) {
      if (tmp->id == id && tmp->cb == cb) {
        ret = tmp;
        break;
      }
      tmp = tmp->next;
    }
  }
  if (ret == NULL) {
    tmp = (EVENT_CALL_LIST_S *)malloc(sizeof(EVENT_CALL_LIST_S));
    memcpy(&tmp->stChn, pstChn, sizeof(MPP_CHN_S));
    tmp->next = event_cb_list;
    tmp->id = id;
    tmp->pHandle = pHandle;
    tmp->cb = cb;
    tmp->type = type;
    event_cb_list = tmp;
    ret = event_cb_list;
  }

  pthread_mutex_unlock(&event_mutex);

  return ret;
}

void *event_server_run(void *arg) {
  int fd;
  EVENT_CALL_LIST_S *node = (EVENT_CALL_LIST_S *)arg;
  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &node->stChn, sizeof(MPP_CHN_S));
  sock_write(fd, &node->type, sizeof(CB_TYPE_E));
  while (1) {
    if (node->cb) {
      if (node->type == CB_TYPE_EVENT) {
        int len;
        RK_VOID *data = NULL;
        if (sock_read(fd, &len, sizeof(int)) <= 0)
          break;
        data = (RK_VOID *)malloc(len);
        if (sock_read(fd, data, len) <= 0)
          break;
        EventCbFunc cb = (EventCbFunc)node->cb;
        cb(node->pHandle, data);
        if (data)
          free(data);
      } else if (node->type == CB_TYPE_OUT) {
        MEDIA_BUFFER mb;

        if (sock_read(fd, &mb, sizeof(MEDIA_BUFFER)) <= 0)
          break;
        OutCbFunc cb = (OutCbFunc)node->cb;
        cb(mb);
      }
    }
  }
  /* End transmission parameters */
  cli_end(fd);
  pthread_exit(NULL);

  return 0;
}

void example_0(void) {
  int fd;

  printf("%s, in\n", __func__);
  fd = cli_begin((char *)__func__);
  /* Transmission parameters */

  /* End transmission parameters */
  cli_end(fd);
  printf("%s, out\n", __func__);
}

int example_1(int a, int b, int *c) {
  int fd;
  int ret = 0;

  printf("%s, in\n", __func__);
  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &a, sizeof(int));
  sock_write(fd, &b, sizeof(int));
  sock_read(fd, c, sizeof(int));
  sock_read(fd, &ret, sizeof(int));
  printf("%s, a = %d, b = %d, c = %d, ret = %d\n", __func__, a, b, *c, ret);
  /* End transmission parameters */
  cli_end(fd);
  printf("%s, out\n", __func__);

  return ret;
}
#if 0
_CAPI RK_S32 SAMPLE_COMM_ISP_Init(RK_S32 CamId, rk_aiq_working_mode_t WDRMode,
                                  RK_BOOL MultiCam, const char *iq_file_dir)
{
    int fd;
    int len;
    RK_S32 ret = RK_ERR_SYS_OK;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &CamId, sizeof(RK_S32));
    sock_write(fd, &WDRMode, sizeof(rk_aiq_working_mode_t));
    sock_write(fd, &MultiCam, sizeof(RK_BOOL));
    len = strlen(iq_file_dir) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, iq_file_dir, len);

    sock_read(fd, &ret, sizeof(RK_S32));
    /* End transmission parameters */
    cli_end(fd);

    return ret;
}

_CAPI RK_S32 SAMPLE_COMM_ISP_Stop(RK_S32 CamId)
{
    int fd;
    RK_S32 ret = RK_ERR_SYS_OK;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &CamId, sizeof(RK_S32));
    sock_read(fd, &ret, sizeof(RK_S32));
    /* End transmission parameters */
    cli_end(fd);

    return ret;
}

_CAPI RK_S32 SAMPLE_COMM_ISP_Run(RK_S32 CamId)
{
    int fd;
    RK_S32 ret = RK_ERR_SYS_OK;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &CamId, sizeof(RK_S32));
    sock_read(fd, &ret, sizeof(RK_S32));
    /* End transmission parameters */
    cli_end(fd);

    return ret;
}

_CAPI RK_S32 SAMPLE_COMM_ISP_SetFrameRate(RK_S32 CamId, RK_U32 uFps)
{
    int fd;
    RK_S32 ret = RK_ERR_SYS_OK;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &CamId, sizeof(RK_S32));
    sock_write(fd, &uFps, sizeof(RK_U32));
    sock_read(fd, &ret, sizeof(RK_S32));
    /* End transmission parameters */
    cli_end(fd);

    return ret;
}

_CAPI RK_S32 ISP_RUN(int s32CamId, int fps)
{
    int fd;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &s32CamId, sizeof(int));
    sock_write(fd, &fps, sizeof(int));
    /* End transmission parameters */
    cli_end(fd);

    return RK_ERR_SYS_OK;
}

_CAPI RK_S32 ISP_STOP(int s32CamId)
{
    int fd;

    fd = cli_begin((char *)__func__);
    /* Transmission parameters */
    sock_write(fd, &s32CamId, sizeof(int));
    /* End transmission parameters */
    cli_end(fd);

    return RK_ERR_SYS_OK;
}
#endif
_CAPI RK_S32 RK_MPI_SYS_Init() {
  int fd;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */

  /* End transmission parameters */
  cli_end(fd);

  return RK_ERR_SYS_OK;
}

_CAPI RK_S32 RK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn,
                             const MPP_CHN_S *pstDestChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, pstSrcChn, sizeof(MPP_CHN_S));
  sock_write(fd, pstDestChn, sizeof(MPP_CHN_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn,
                               const MPP_CHN_S *pstDestChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, pstSrcChn, sizeof(MPP_CHN_S));
  sock_write(fd, pstDestChn, sizeof(MPP_CHN_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_SetChnAttr(VI_PIPE ViPipe, VI_CHN ViChn,
                                  const VI_CHN_ATTR_S *pstChnAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));
  sock_write(fd, pstChnAttr, sizeof(VI_CHN_ATTR_S));

  len = strlen(pstChnAttr->pcVideoNode) + 1;
  sock_write(fd, &len, sizeof(int));
  if (len)
    sock_write(fd, pstChnAttr->pcVideoNode, len);

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_EnableChn(VI_PIPE ViPipe, VI_CHN ViChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_DisableChn(VI_PIPE ViPipe, VI_CHN ViChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_StartRegionLuma(VI_CHN ViChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViChn, sizeof(VI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_StopRegionLuma(VI_CHN ViChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViChn, sizeof(VI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_GetChnRegionLuma(
    VI_PIPE ViPipe, VI_CHN ViChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    RK_U64 *pu64LumaData, RK_S32 s32MilliSec) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));
  sock_write(fd, pstRegionInfo, sizeof(VIDEO_REGION_INFO_S));
  sock_write(fd, &s32MilliSec, sizeof(RK_S32));

  sock_read(fd, pu64LumaData, sizeof(RK_U64) * 2);
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_RGN_SetCover(VI_PIPE ViPipe, VI_CHN ViChn,
                                    const OSD_REGION_INFO_S *pstRgnInfo,
                                    const COVER_INFO_S *pstCoverInfo) {
  int fd;
  RK_S32 ret = 0;
  int haveinfo = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));
  sock_write(fd, pstRgnInfo, sizeof(OSD_REGION_INFO_S));
  if (pstCoverInfo) {
    haveinfo = 1;
    sock_write(fd, &haveinfo, sizeof(int));
    sock_write(fd, pstCoverInfo, sizeof(COVER_INFO_S));
  } else {
    sock_write(fd, &haveinfo, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VI_StartStream(VI_PIPE ViPipe, VI_CHN ViChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &ViPipe, sizeof(VI_PIPE));
  sock_write(fd, &ViChn, sizeof(VI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_RGA_CreateChn(RGA_CHN RgaChn, RGA_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &RgaChn, sizeof(RGA_CHN));
  sock_write(fd, pstAttr, sizeof(RGA_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_RGA_DestroyChn(RGA_CHN RgaChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &RgaChn, sizeof(RGA_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_RGA_GetChnAttr(RGA_CHN RgaChn, RGA_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &RgaChn, sizeof(RGA_CHN));
  sock_read(fd, pstAttr, sizeof(RGA_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_RGA_SetChnAttr(RGA_CHN RgaChn, const RGA_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &RgaChn, sizeof(RGA_CHN));
  sock_write(fd, pstAttr, sizeof(RGA_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VO_CreateChn(VO_CHN VoChn, const VO_CHN_ATTR_S *pstAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VoChn, sizeof(VO_CHN));
  sock_write(fd, pstAttr, sizeof(VO_CHN_ATTR_S));

  if (pstAttr->pcDevNode) {
    len = strlen(pstAttr->pcDevNode) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, pstAttr->pcDevNode, len);
  } else {
    len = 0;
    sock_write(fd, &len, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VO_GetChnAttr(VO_CHN VoChn, VO_CHN_ATTR_S *pstAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;
  char *pcDevNode = NULL;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VoChn, sizeof(VO_CHN));

  if (sock_read(fd, pstAttr, sizeof(VO_CHN_ATTR_S)) == SOCKERR_CLOSED) {
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
          pstAttr->pcDevNode = const_string[i];
          free(pcDevNode);
          break;
        }
      } else {
        const_string[i] = pcDevNode;
        pstAttr->pcDevNode = const_string[i];
        break;
      }
    }
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
out:
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VO_SetChnAttr(VO_CHN VoChn, const VO_CHN_ATTR_S *pstAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VoChn, sizeof(VO_CHN));
  sock_write(fd, pstAttr, sizeof(VO_CHN_ATTR_S));

  if (pstAttr->pcDevNode) {
    len = strlen(pstAttr->pcDevNode) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, pstAttr->pcDevNode, len);
  } else {
    len = 0;
    sock_write(fd, &len, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VO_DestroyChn(VO_CHN VoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VoChn, sizeof(VO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_MD_CreateChn(ALGO_MD_CHN MdChn,
                                      const ALGO_MD_ATTR_S *pstChnAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &MdChn, sizeof(ALGO_MD_CHN));
  sock_write(fd, pstChnAttr, sizeof(ALGO_MD_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_MD_DestroyChn(ALGO_MD_CHN MdChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &MdChn, sizeof(ALGO_MD_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_MD_EnableSwitch(ALGO_MD_CHN MdChn, RK_BOOL bEnable) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &MdChn, sizeof(ALGO_MD_CHN));
  sock_write(fd, &bEnable, sizeof(RK_BOOL));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_OD_CreateChn(ALGO_OD_CHN OdChn,
                                      const ALGO_OD_ATTR_S *pstChnAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &OdChn, sizeof(ALGO_OD_CHN));
  sock_write(fd, pstChnAttr, sizeof(ALGO_OD_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_OD_DestroyChn(ALGO_OD_CHN OdChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &OdChn, sizeof(ALGO_OD_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ALGO_OD_EnableSwitch(ALGO_OD_CHN OdChn, RK_BOOL bEnable) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &OdChn, sizeof(ALGO_OD_CHN));
  sock_write(fd, &bEnable, sizeof(RK_BOOL));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ADEC_CreateChn(ADEC_CHN AdecChn,
                                   const ADEC_CHN_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AdecChn, sizeof(ADEC_CHN));
  sock_write(fd, pstAttr, sizeof(ADEC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_ADEC_DestroyChn(ADEC_CHN AdecChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AdecChn, sizeof(ADEC_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_SYS_RegisterEventCb(const MPP_CHN_S *pstChn,
                                        RK_VOID *pHandle, EventCbFunc cb) {
  printf("%s 1\n", __func__);
  EVENT_CALL_LIST_S *node =
      event_list_add(pstChn, pHandle, (void *)cb, CB_TYPE_EVENT);
  printf("%s 2\n", __func__);
  if (node) {
    pthread_t tid = 0;
    printf("%s 3\n", __func__);
    pthread_create(&tid, NULL, event_server_run, (void *)node);
  }
  printf("%s 4\n", __func__);
  return 0;
}

_CAPI RK_S32 RK_MPI_SYS_RegisterOutCb(const MPP_CHN_S *pstChn, OutCbFunc cb) {
  EVENT_CALL_LIST_S *node =
      event_list_add(pstChn, NULL, (void *)cb, CB_TYPE_OUT);
  if (node) {
    pthread_t tid = 0;
    pthread_create(&tid, NULL, event_server_run, (void *)node);
  }

  return 0;
}

_CAPI RK_S32 RK_MPI_SYS_SendMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID,
                                        MEDIA_BUFFER buffer) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &enModID, sizeof(MOD_ID_E));
  sock_write(fd, &s32ChnID, sizeof(RK_S32));
  sock_write(fd, &buffer, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_SYS_GetMediaBuffer(MOD_ID_E enModID, RK_S32 s32ChnID,
                                             RK_S32 s32MilliSec) {
  int fd;
  MEDIA_BUFFER mb;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &enModID, sizeof(MOD_ID_E));
  sock_write(fd, &s32ChnID, sizeof(RK_S32));
  sock_write(fd, &s32MilliSec, sizeof(RK_S32));

  sock_read(fd, &mb, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return mb;
}

_CAPI RK_S32 RK_MPI_MB_ReleaseBuffer(MEDIA_BUFFER mb) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;
  MB_LIST_S info;

  if (mb_list_get(mb, &info) == 0) {
    del_list_del(mb);
    if (info.ptr)
      munmap(info.ptr, info.size);
    close(info.fd);
    close(info.devfd);
  }

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI void *RK_MPI_MB_GetPtr(MEDIA_BUFFER mb) {
  int fd;
  int handle = 0;
  int size = 0;
  int mbfd = 0;
  void *ptr = NULL;
  MB_LIST_S info;
  int devfd;

  if (mb_list_get(mb, &info) == 0) {
    if (info.ptr == NULL) {
      info.ptr = MbMap(info.devfd, info.handle, info.size);
      mb_list_update(mb, &info);
    }
    printf("%s 1 ptr = %p\n", __func__, info.ptr);
    return info.ptr;
  }

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_recv_devfd(fd, &devfd);
  sock_read(fd, &handle, sizeof(int));
  sock_read(fd, &size, sizeof(int));
  /* End transmission parameters */
  cli_end(fd);

  ptr = MbMap(devfd, handle, size);
  mbfd = MbHandleToFD(devfd, handle);
  mb_list_add(mb, ptr, devfd, mbfd, handle, size);
  printf("%s 2 ptr = %p, handle = %d, mbfd = %d\n", __func__, ptr, handle,
         mbfd);
  return ptr;
}

_CAPI int RK_MPI_MB_GetFD(MEDIA_BUFFER mb) {
  int fd;
  int handle = 0;
  int size = 0;
  int mbfd = 0;
  MB_LIST_S info;
  int devfd;

  if (mb_list_get(mb, &info) == 0) {
    return info.fd;
  }

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_recv_devfd(fd, &devfd);
  sock_read(fd, &handle, sizeof(int));
  sock_read(fd, &size, sizeof(int));
  /* End transmission parameters */
  cli_end(fd);
  mbfd = MbHandleToFD(devfd, handle);
  mb_list_add(mb, NULL, devfd, mbfd, handle, size);

  return mbfd;
}

_CAPI RK_S32 RK_MPI_MB_GetImageInfo(MEDIA_BUFFER mb,
                                    MB_IMAGE_INFO_S *pstImageInfo) {
  int fd;
  int ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_read(fd, pstImageInfo, sizeof(MB_IMAGE_INFO_S));

  sock_read(fd, &ret, sizeof(int));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI size_t RK_MPI_MB_GetSize(MEDIA_BUFFER mb) {
  int fd;
  size_t ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(size_t));
  /* End transmission parameters */
  cli_end(fd);
  printf("%s size = %d\n", __func__, ret);
  return ret;
}

_CAPI MOD_ID_E RK_MPI_MB_GetModeID(MEDIA_BUFFER mb) {
  int fd;
  MOD_ID_E ret;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(MOD_ID_E));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S16 RK_MPI_MB_GetChannelID(MEDIA_BUFFER mb) {
  int fd;
  RK_S16 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S16));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_U64 RK_MPI_MB_GetTimestamp(MEDIA_BUFFER mb) {
  int fd;
  RK_U64 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_U64));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_SetTimestamp(MEDIA_BUFFER mb, RK_U64 timestamp) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_write(fd, &timestamp, sizeof(RK_U64));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_SetSize(MEDIA_BUFFER mb, RK_U32 size) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_write(fd, &size, sizeof(RK_U32));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_MB_CreateBuffer(RK_U32 u32Size, RK_BOOL boolHardWare,
                                          RK_U8 u8Flag) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &u32Size, sizeof(RK_U32));
  sock_write(fd, &boolHardWare, sizeof(RK_BOOL));
  sock_write(fd, &u8Flag, sizeof(RK_U8));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_MB_CreateImageBuffer(MB_IMAGE_INFO_S *pstImageInfo,
                                               RK_BOOL boolHardWare,
                                               RK_U8 u8Flag) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, pstImageInfo, sizeof(MB_IMAGE_INFO_S));
  sock_write(fd, &boolHardWare, sizeof(RK_BOOL));
  sock_write(fd, &u8Flag, sizeof(RK_U8));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_MB_CreateAudioBuffer(RK_U32 u32BufferSize,
                                               RK_BOOL boolHardWare) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &u32BufferSize, sizeof(RK_U32));
  sock_write(fd, &boolHardWare, sizeof(RK_BOOL));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER_POOL RK_MPI_MB_POOL_Create(MB_POOL_PARAM_S *pstPoolParam) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, pstPoolParam, sizeof(MB_POOL_PARAM_S));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_POOL_Destroy(MEDIA_BUFFER_POOL MBPHandle) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &MBPHandle, sizeof(MEDIA_BUFFER_POOL));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_MB_POOL_GetBuffer(MEDIA_BUFFER_POOL MBPHandle,
                                            RK_BOOL bIsBlock) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &MBPHandle, sizeof(MEDIA_BUFFER_POOL));
  sock_write(fd, &bIsBlock, sizeof(RK_BOOL));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI MEDIA_BUFFER RK_MPI_MB_ConvertToImgBuffer(MEDIA_BUFFER mb,
                                                MB_IMAGE_INFO_S *pstImageInfo) {
  int fd;
  MEDIA_BUFFER ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));
  sock_write(fd, pstImageInfo, sizeof(MB_IMAGE_INFO_S));

  sock_read(fd, &ret, sizeof(MEDIA_BUFFER));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_GetFlag(MEDIA_BUFFER mb) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_GetTsvcLevel(MEDIA_BUFFER mb) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_BOOL RK_MPI_MB_IsViFrame(MEDIA_BUFFER mb) {
  int fd;
  RK_BOOL ret;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_BOOL));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_MB_TsNodeDump(MEDIA_BUFFER mb) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &mb, sizeof(MEDIA_BUFFER));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VDEC_CreateChn(VDEC_CHN VdChn,
                                   const VDEC_CHN_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VdChn, sizeof(VDEC_CHN));
  sock_write(fd, pstAttr, sizeof(VDEC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VDEC_DestroyChn(VDEC_CHN VdChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VdChn, sizeof(VDEC_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_CreateChn(VENC_CHN VencChn,
                                   VENC_CHN_ATTR_S *stVencChnAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, stVencChnAttr, sizeof(VENC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_DestroyChn(VENC_CHN VencChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_CreateJpegLightChn(VENC_CHN VencChn,
                                            VENC_CHN_ATTR_S *stVencChnAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, stVencChnAttr, sizeof(VENC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_GetVencChnAttr(VENC_CHN VencChn,
                                        VENC_CHN_ATTR_S *stVencChnAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));

  sock_read(fd, stVencChnAttr, sizeof(VENC_CHN_ATTR_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetVencChnAttr(VENC_CHN VencChn,
                                        VENC_CHN_ATTR_S *stVencChnAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, stVencChnAttr, sizeof(VENC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_GetRcParam(VENC_CHN VencChn,
                                    VENC_RC_PARAM_S *pstRcParam) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));

  sock_read(fd, pstRcParam, sizeof(VENC_RC_PARAM_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetRcParam(VENC_CHN VencChn,
                                    const VENC_RC_PARAM_S *pstRcParam) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRcParam, sizeof(VENC_RC_PARAM_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetJpegParam(VENC_CHN VencChn,
                                      const VENC_JPEG_PARAM_S *pstJpegParam) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstJpegParam, sizeof(VENC_JPEG_PARAM_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_RGN_Init(VENC_CHN VencChn,
                                  VENC_COLOR_TBL_S *stColorTbl) {
  int fd;
  RK_S32 ret = 0;
  int havetbl = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  if (stColorTbl) {
    havetbl = 1;
    sock_write(fd, &havetbl, sizeof(int));
    sock_write(fd, stColorTbl, sizeof(VENC_COLOR_TBL_S));
  } else {
    sock_write(fd, &havetbl, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_RGN_SetBitMap(VENC_CHN VencChn,
                                       const OSD_REGION_INFO_S *pstRgnInfo,
                                       const BITMAP_S *pstBitmap) {
  int fd;
  RK_S32 ret = 0;
  int len = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRgnInfo, sizeof(OSD_REGION_INFO_S));
  sock_write(fd, pstBitmap, sizeof(BITMAP_S));
  if (pstBitmap->pData) {
    if (pstBitmap->enPixelFormat == PIXEL_FORMAT_ARGB_8888)
      len = pstBitmap->u32Width * pstBitmap->u32Height * 4;
    // len = malloc_usable_size(pstBitmap->pData) - 4;
    printf("%s %d, %d\n", __func__, len,
           malloc_usable_size(pstBitmap->pData) - 4);
  }
  sock_write(fd, &len, sizeof(int));
  if (len > 0) {
    sock_write(fd, pstBitmap->pData, len);
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_StartRecvFrame(
    VENC_CHN VencChn, const VENC_RECV_PIC_PARAM_S *pstRecvParam) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRecvParam, sizeof(VENC_RECV_PIC_PARAM_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_RGN_SetCover(VENC_CHN VencChn,
                                      const OSD_REGION_INFO_S *pstRgnInfo,
                                      const COVER_INFO_S *pstCoverInfo) {
  int fd;
  RK_S32 ret = 0;
  int haveinfo = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRgnInfo, sizeof(OSD_REGION_INFO_S));
  if (pstCoverInfo) {
    haveinfo = 1;
    sock_write(fd, &haveinfo, sizeof(int));
    sock_write(fd, pstCoverInfo, sizeof(COVER_INFO_S));
  } else {
    sock_write(fd, &haveinfo, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_RGN_SetCoverEx(VENC_CHN VencChn,
                                        const OSD_REGION_INFO_S *pstRgnInfo,
                                        const COVER_INFO_S *pstCoverInfo) {
  int fd;
  RK_S32 ret = 0;
  int haveinfo = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRgnInfo, sizeof(OSD_REGION_INFO_S));
  if (pstCoverInfo) {
    haveinfo = 1;
    sock_write(fd, &haveinfo, sizeof(int));
    sock_write(fd, pstCoverInfo, sizeof(COVER_INFO_S));
  } else {
    sock_write(fd, &haveinfo, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetGopMode(VENC_CHN VencChn,
                                    VENC_GOP_ATTR_S *pstGopModeAttr) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstGopModeAttr, sizeof(VENC_GOP_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_GetRoiAttr(VENC_CHN VencChn,
                                    VENC_ROI_ATTR_S *pstRoiAttr,
                                    RK_S32 roi_index) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, &roi_index, sizeof(RK_S32));

  sock_read(fd, pstRoiAttr, sizeof(VENC_ROI_ATTR_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetRoiAttr(VENC_CHN VencChn,
                                    const VENC_ROI_ATTR_S *pstRoiAttr,
                                    RK_S32 region_cnt) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, pstRoiAttr, sizeof(VENC_ROI_ATTR_S));
  sock_write(fd, &region_cnt, sizeof(RK_S32));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VENC_SetResolution(
    VENC_CHN VencChn, VENC_RESOLUTION_PARAM_S stResolutionParam) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VencChn, sizeof(VENC_CHN));
  sock_write(fd, &stResolutionParam, sizeof(VENC_RESOLUTION_PARAM_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_CreateDev(VMIX_DEV VmDev,
                                   VMIX_DEV_INFO_S *pstDevInfo) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, pstDevInfo, sizeof(VMIX_DEV_INFO_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_DestroyDev(VMIX_DEV VmDev) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_EnableChn(VMIX_DEV VmDev, VMIX_CHN VmChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_DisableChn(VMIX_DEV VmDev, VMIX_CHN VmChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_SetLineInfo(VMIX_DEV VmDev, VMIX_CHN VmChn,
                                     VMIX_LINE_INFO_S VmLine) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));
  sock_write(fd, &VmLine, sizeof(VMIX_LINE_INFO_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_ShowChn(VMIX_DEV VmDev, VMIX_CHN VmChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_HideChn(VMIX_DEV VmDev, VMIX_CHN VmChn) {
  int fd;
  RK_S32 ret = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_GetChnRegionLuma(
    VMIX_DEV VmDev, VMIX_CHN VmChn, const VIDEO_REGION_INFO_S *pstRegionInfo,
    RK_U64 *pu64LumaData, RK_S32 s32MilliSec) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));
  sock_write(fd, pstRegionInfo, sizeof(VIDEO_REGION_INFO_S));
  sock_write(fd, &s32MilliSec, sizeof(RK_S32));

  sock_read(fd, pu64LumaData, sizeof(RK_U64) * 2);
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_VMIX_RGN_SetCover(VMIX_DEV VmDev, VMIX_CHN VmChn,
                                      const OSD_REGION_INFO_S *pstRgnInfo,
                                      const COVER_INFO_S *pstCoverInfo) {
  int fd;
  RK_S32 ret = 0;
  int haveinfo = 0;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &VmDev, sizeof(VMIX_DEV));
  sock_write(fd, &VmChn, sizeof(VMIX_CHN));
  sock_write(fd, pstRgnInfo, sizeof(OSD_REGION_INFO_S));
  if (pstCoverInfo) {
    haveinfo = 1;
    sock_write(fd, &haveinfo, sizeof(int));
    sock_write(fd, pstCoverInfo, sizeof(COVER_INFO_S));
  } else {
    sock_write(fd, &haveinfo, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_SetChnAttr(AO_CHN AoChn, const AO_CHN_ATTR_S *pstAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));
  sock_write(fd, pstAttr, sizeof(AO_CHN_ATTR_S));

  if (pstAttr->pcAudioNode) {
    len = strlen(pstAttr->pcAudioNode) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, pstAttr->pcAudioNode, len);
  } else {
    len = 0;
    sock_write(fd, &len, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_EnableChn(AO_CHN AoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_DisableChn(AO_CHN AoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_SetVolume(AO_CHN AoChn, RK_S32 s32Volume) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));
  sock_write(fd, &s32Volume, sizeof(RK_S32));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_GetVolume(AO_CHN AoChn, RK_S32 *ps32Volume) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, ps32Volume, sizeof(RK_S32));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_SetVqeAttr(AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));
  sock_write(fd, pstVqeConfig, sizeof(AO_VQE_CONFIG_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_GetVqeAttr(AO_CHN AoChn, AO_VQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, pstVqeConfig, sizeof(AO_VQE_CONFIG_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_EnableVqe(AO_CHN AoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_DisableVqe(AO_CHN AoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_QueryChnStat(AO_CHN AoChn, AO_CHN_STATE_S *pstStatus) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, pstStatus, sizeof(AO_CHN_STATE_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AO_ClearChnBuf(AO_CHN AoChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AoChn, sizeof(AO_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AENC_CreateChn(AENC_CHN AencChn,
                                   const AENC_CHN_ATTR_S *pstAttr) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AencChn, sizeof(AENC_CHN));
  sock_write(fd, pstAttr, sizeof(AENC_CHN_ATTR_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AENC_DestroyChn(AENC_CHN AencChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AencChn, sizeof(AENC_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_SetChnAttr(VENC_CHN AiChn,
                                  const AI_CHN_ATTR_S *pstAttr) {
  int fd;
  int len;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(VENC_CHN));
  sock_write(fd, pstAttr, sizeof(AI_CHN_ATTR_S));

  if (pstAttr->pcAudioNode) {
    len = strlen(pstAttr->pcAudioNode) + 1;
    sock_write(fd, &len, sizeof(int));
    sock_write(fd, pstAttr->pcAudioNode, len);
  } else {
    len = 0;
    sock_write(fd, &len, sizeof(int));
  }

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_EnableChn(AI_CHN AiChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_DisableChn(AI_CHN AiChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_SetVolume(AI_CHN AiChn, RK_S32 s32Volume) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));
  sock_write(fd, &s32Volume, sizeof(RK_S32));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_GetVolume(AI_CHN AiChn, RK_S32 *ps32Volume) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, ps32Volume, sizeof(RK_S32));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_SetTalkVqeAttr(AI_CHN AiChn,
                                      AI_TALKVQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));
  sock_write(fd, pstVqeConfig, sizeof(AI_TALKVQE_CONFIG_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_GetTalkVqeAttr(AI_CHN AiChn,
                                      AI_TALKVQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, pstVqeConfig, sizeof(AI_TALKVQE_CONFIG_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_SetRecordVqeAttr(AI_CHN AiChn,
                                        AI_RECORDVQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));
  sock_write(fd, pstVqeConfig, sizeof(AI_RECORDVQE_CONFIG_S));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_GetRecordVqeAttr(AI_CHN AiChn,
                                        AI_RECORDVQE_CONFIG_S *pstVqeConfig) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, pstVqeConfig, sizeof(AI_RECORDVQE_CONFIG_S));
  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_EnableVqe(AI_CHN AiChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_StartStream(AI_CHN AiChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}

_CAPI RK_S32 RK_MPI_AI_DisableVqe(AI_CHN AiChn) {
  int fd;
  RK_S32 ret = RK_ERR_SYS_OK;

  fd = cli_begin((char *)__func__);
  /* Transmission parameters */
  sock_write(fd, &AiChn, sizeof(AI_CHN));

  sock_read(fd, &ret, sizeof(RK_S32));
  /* End transmission parameters */
  cli_end(fd);

  return ret;
}
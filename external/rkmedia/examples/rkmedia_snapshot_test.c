// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#define __USE_GNU
#define _GNU_SOURCE
#include "common/sample_common.h"
#include "common/sample_fake_isp.h"
#include "rkmedia_api.h"
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

#include <rga/im2d.h>
#include <rga/rga.h>

#include <linux/serial.h>
#include <sys/ioctl.h>
#include <termios.h>

#define HUNT_FRAME_COUNT 5

// Offline frame variable declaration
int frame_count = 1;
static rk_aiq_sys_ctx_t *g_aiq_ctx = NULL;
static const char *g_subdev_node = "/dev/v4l-subdev7";
static uint8_t *rkraws[HUNT_FRAME_COUNT];
static int index_raw = 0;
struct mcu_rkaiq_rkraw mcu_rkraws[HUNT_FRAME_COUNT];

static bool quit = false;

int ret = 0;
int fake_width = 2688;
int fake_height = 1520;
int video_width = 1920;
int video_height = 1080;
int disp_width = 720;
int disp_height = 1280;
int disp1_width = 360;
int disp1_height = 640;
RK_S32 s32CamId = 0;
int err_flag = 0;
const RK_CHAR *pcVideoNode = "/dev/video34";
// #ifdef RKAIQ
RK_BOOL bMultictx = RK_FALSE;
// #endif
char *iq_file_dir = NULL;

// rkmedia
VI_CHN_ATTR_S vi_chn_attr;
VI_CHN_ATTR_S vi_chn_attr1;
VENC_RECV_PIC_PARAM_S stRecvParam;
VENC_CHN_ATTR_S venc_chn_attr;
MPP_CHN_S stEncChn;
MPP_CHN_S stRgaChn;
RGA_ATTR_S stRgaAttr;
MPP_CHN_S stSrcChn = {0};
MPP_CHN_S stDestChn = {0};
RGA_ATTR_S stRgaAttr1;
VO_CHN_ATTR_S stVoAttr = {0};
VO_CHN_ATTR_S stVoAttr1 = {0};
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::a::I:M:s:d:n:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"subdev", optional_argument, NULL, 'S'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static char timestamp_char[3];
// uart
static void read_timestamp_from_uart() {
  int fd;
  int baud = B115200;
  struct termios newtio;
  struct serial_rs485 rs485;

  fd = open("/dev/ttyS5", O_RDWR);
  if (fd < 0) {
    printf("Error opening serial port\n");
    return;
  }

  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

  /* man termios get more info on below settings */
  newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = 0;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  // block for up till 128 characters
  newtio.c_cc[VMIN] = 4;
  // 0.5 seconds read timeout
  newtio.c_cc[VTIME] = 1;
  /* now clean the modem line and activate the settings for the port */
  tcflush(fd, TCIOFLUSH);
  tcsetattr(fd, TCSANOW, &newtio);
  if (ioctl(fd, TIOCGRS485, &rs485) >= 0) {
    /* disable RS485 */
    rs485.flags &=
        ~(SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND | SER_RS485_RTS_AFTER_SEND);
    rs485.delay_rts_after_send = 0;
    rs485.delay_rts_before_send = 0;
    if (ioctl(fd, TIOCSRS485, &rs485) < 0) {
      perror("Error setting RS-232 mode");
    }
  }

  /*
   * The flag ASYNC_SPD_CUST might have already been set, so
   * clear it to avoid confusing the kernel uart dirver.
   */
  struct serial_struct ss;
  if (ioctl(fd, TIOCGSERIAL, &ss) < 0) {
    // return silently as some devices do not support TIOCGSERIAL
    printf("return silently as some devices do not support TIOCGSERIAL\n");
    return;
  }
  // if ((ss.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST)
  // 	return;
  ss.flags &= ~ASYNC_SPD_MASK;
  if (ioctl(fd, TIOCSSERIAL, &ss) < 0) {
    printf("TIOCSSERIAL failed");
    return;
  }

  // write
  int written = write(fd, "\x67\x74", 2); // gt
  if (written < 0)
    printf("write()");
  else
    printf("written is %d\n", written);
  // read
  char read_buffer[4];
  int read_num = read(fd, &read_buffer, 4);
  if (read_num < 4)
    printf("read_num is %d\n", read_num);
  else {
    printf("read_num is %d, read_buffer is 0x%x 0x%x 0x%x 0x%x\n", read_num,
           read_buffer[0], read_buffer[1], read_buffer[2], read_buffer[3]);
    int timestamp = (int)read_buffer[2] | ((int)read_buffer[3] << 8);
    printf("timestamp is %d\n", timestamp);
    timestamp_char[2] = timestamp % 10 + '0';
    timestamp_char[1] = timestamp / 10 % 10 + '0';
    timestamp_char[0] = timestamp / 100 % 10 + '0';
    printf("timestamp_char is %s\n", timestamp_char);
  }
}

// timestamp
#define TEST_ARGB32_RED 0xFFFF0033
#define TEST_ARGB32_TRANS 0x00000000

// Font : 32x32 Horizontal modulus left high position
const unsigned char ASIICTable[] = {
    /* Arial */
    /*  0[0x0030]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x7C,
    0x3C,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x7C,
    0x3E,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  1[0x0031]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x30,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0xF0,
    0x00,
    0x00,
    0x01,
    0xF0,
    0x00,
    0x00,
    0x07,
    0xF0,
    0x00,
    0x00,
    0x0F,
    0x70,
    0x00,
    0x00,
    0x1E,
    0x70,
    0x00,
    0x00,
    0x18,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  2[0x0032]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0F,
    0xE0,
    0x00,
    0x00,
    0x3F,
    0xF8,
    0x00,
    0x00,
    0x7F,
    0xFC,
    0x00,
    0x00,
    0x78,
    0x3C,
    0x00,
    0x00,
    0xE0,
    0x1E,
    0x00,
    0x00,
    0xE0,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0xE0,
    0x00,
    0x00,
    0x01,
    0xC0,
    0x00,
    0x00,
    0x07,
    0x80,
    0x00,
    0x00,
    0x0F,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x7F,
    0xFE,
    0x00,
    0x00,
    0xFF,
    0xFE,
    0x00,
    0x00,
    0xFF,
    0xFE,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  3[0x0033]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x78,
    0x3C,
    0x00,
    0x00,
    0xF0,
    0x0E,
    0x00,
    0x00,
    0xE0,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x1E,
    0x00,
    0x00,
    0x00,
    0x3C,
    0x00,
    0x00,
    0x03,
    0xF8,
    0x00,
    0x00,
    0x03,
    0xF8,
    0x00,
    0x00,
    0x03,
    0xFC,
    0x00,
    0x00,
    0x00,
    0x1E,
    0x00,
    0x00,
    0x00,
    0x0F,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xF0,
    0x0E,
    0x00,
    0x00,
    0x78,
    0x1E,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  4[0x0034]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x78,
    0x00,
    0x00,
    0x00,
    0xF8,
    0x00,
    0x00,
    0x00,
    0xF8,
    0x00,
    0x00,
    0x01,
    0xF8,
    0x00,
    0x00,
    0x03,
    0xB8,
    0x00,
    0x00,
    0x07,
    0xB8,
    0x00,
    0x00,
    0x07,
    0x38,
    0x00,
    0x00,
    0x0E,
    0x38,
    0x00,
    0x00,
    0x1C,
    0x38,
    0x00,
    0x00,
    0x1C,
    0x38,
    0x00,
    0x00,
    0x38,
    0x38,
    0x00,
    0x00,
    0x70,
    0x38,
    0x00,
    0x00,
    0xE0,
    0x38,
    0x00,
    0x00,
    0xFF,
    0xFF,
    0x00,
    0x00,
    0xFF,
    0xFF,
    0x00,
    0x00,
    0xFF,
    0xFF,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  5[0x0035]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x3F,
    0xFE,
    0x00,
    0x00,
    0x3F,
    0xFE,
    0x00,
    0x00,
    0x3F,
    0xFE,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0x73,
    0xE0,
    0x00,
    0x00,
    0x6F,
    0xF8,
    0x00,
    0x00,
    0xFF,
    0xFC,
    0x00,
    0x00,
    0xF8,
    0x1E,
    0x00,
    0x00,
    0xE0,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xF0,
    0x0E,
    0x00,
    0x00,
    0x78,
    0x1E,
    0x00,
    0x00,
    0x7F,
    0xFC,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x0F,
    0xE0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  6[0x0036]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0xF0,
    0x00,
    0x00,
    0x0F,
    0xFC,
    0x00,
    0x00,
    0x1F,
    0xFE,
    0x00,
    0x00,
    0x3C,
    0x1E,
    0x00,
    0x00,
    0x78,
    0x0F,
    0x00,
    0x00,
    0x70,
    0x07,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0xE0,
    0x00,
    0x00,
    0x00,
    0xE3,
    0xF0,
    0x00,
    0x00,
    0xEF,
    0xF8,
    0x00,
    0x00,
    0xFF,
    0xFC,
    0x00,
    0x00,
    0xFC,
    0x1E,
    0x00,
    0x00,
    0xF0,
    0x0F,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0x70,
    0x07,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x3C,
    0x1E,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x0F,
    0xF8,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  7[0x0037]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xFF,
    0xFE,
    0x00,
    0x00,
    0xFF,
    0xFE,
    0x00,
    0x00,
    0xFF,
    0xFE,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x00,
    0x00,
    0x00,
    0x18,
    0x00,
    0x00,
    0x00,
    0x38,
    0x00,
    0x00,
    0x00,
    0x70,
    0x00,
    0x00,
    0x00,
    0xE0,
    0x00,
    0x00,
    0x00,
    0xE0,
    0x00,
    0x00,
    0x01,
    0xC0,
    0x00,
    0x00,
    0x01,
    0xC0,
    0x00,
    0x00,
    0x03,
    0x80,
    0x00,
    0x00,
    0x03,
    0x80,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x1C,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  8[0x0038]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x78,
    0x1C,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0x38,
    0x1C,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x0F,
    0xF0,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x78,
    0x1E,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xF0,
    0x0F,
    0x00,
    0x00,
    0x78,
    0x1E,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x1F,
    0xF8,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  9[0x0039]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x07,
    0xE0,
    0x00,
    0x00,
    0x1F,
    0xF0,
    0x00,
    0x00,
    0x3F,
    0xFC,
    0x00,
    0x00,
    0x78,
    0x1C,
    0x00,
    0x00,
    0x70,
    0x0E,
    0x00,
    0x00,
    0xE0,
    0x06,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xE0,
    0x07,
    0x00,
    0x00,
    0xF0,
    0x0F,
    0x00,
    0x00,
    0x78,
    0x1F,
    0x00,
    0x00,
    0x3F,
    0xFF,
    0x00,
    0x00,
    0x1F,
    0xF7,
    0x00,
    0x00,
    0x0F,
    0xC7,
    0x00,
    0x00,
    0x00,
    0x07,
    0x00,
    0x00,
    0x00,
    0x0E,
    0x00,
    0x00,
    0xE0,
    0x0E,
    0x00,
    0x00,
    0xF0,
    0x1E,
    0x00,
    0x00,
    0x78,
    0x3C,
    0x00,
    0x00,
    0x7F,
    0xF8,
    0x00,
    0x00,
    0x3F,
    0xF0,
    0x00,
    0x00,
    0x0F,
    0xC0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /*  .[0x002E]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x03,
    0x80,
    0x00,
    0x00,
    0x03,
    0x80,
    0x00,
    0x00,
    0x03,
    0x80,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /* 毫[0x6BEB]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x00,
    0x00,
    0x00,
    0x03,
    0x00,
    0x00,
    0x00,
    0x03,
    0x00,
    0x60,
    0x3F,
    0xFF,
    0xFF,
    0xF0,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x00,
    0x01,
    0xFF,
    0xFE,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0xFF,
    0xFE,
    0x00,
    0x01,
    0x80,
    0x04,
    0x00,
    0x12,
    0x00,
    0x00,
    0x10,
    0x1F,
    0xFF,
    0xFF,
    0xF8,
    0x30,
    0x00,
    0x02,
    0x30,
    0x70,
    0x00,
    0x1F,
    0x20,
    0x60,
    0x0F,
    0xFC,
    0xC0,
    0x0F,
    0xF6,
    0x00,
    0x00,
    0x00,
    0x06,
    0x06,
    0x00,
    0x00,
    0x07,
    0xFF,
    0x00,
    0x0F,
    0xFE,
    0x00,
    0x80,
    0x00,
    0x06,
    0x01,
    0xC0,
    0x00,
    0x1F,
    0xFF,
    0xB0,
    0x3F,
    0xE6,
    0x00,
    0x10,
    0x00,
    0x06,
    0x00,
    0x10,
    0x00,
    0x06,
    0x00,
    0x10,
    0x00,
    0x07,
    0xFF,
    0xB8,
    0x00,
    0x03,
    0xFF,
    0xF0,
    0x00,
    0x00,
    0x00,
    0x00,
    /* Arial */
    /* 秒[0x79D2]   32x32 */
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x00,
    0x00,
    0x38,
    0x06,
    0x00,
    0x00,
    0xFC,
    0x06,
    0x00,
    0x1F,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x06,
    0x80,
    0x01,
    0x80,
    0xE6,
    0x60,
    0x01,
    0x88,
    0xC6,
    0x30,
    0x3F,
    0xFC,
    0xC6,
    0x18,
    0x01,
    0x80,
    0x86,
    0x0C,
    0x01,
    0x80,
    0x86,
    0x0C,
    0x03,
    0x81,
    0x86,
    0x08,
    0x03,
    0xF1,
    0x06,
    0x00,
    0x03,
    0xB9,
    0x06,
    0x00,
    0x07,
    0x9A,
    0x06,
    0x30,
    0x05,
    0x9A,
    0x06,
    0x38,
    0x09,
    0x84,
    0x06,
    0x60,
    0x19,
    0x80,
    0x06,
    0xE0,
    0x11,
    0x80,
    0x05,
    0xC0,
    0x21,
    0x80,
    0x01,
    0x80,
    0x41,
    0x80,
    0x03,
    0x00,
    0x01,
    0x80,
    0x06,
    0x00,
    0x01,
    0x80,
    0x1C,
    0x00,
    0x01,
    0x80,
    0x30,
    0x00,
    0x01,
    0x80,
    0xE0,
    0x00,
    0x01,
    0x83,
    0x80,
    0x00,
    0x01,
    0x9C,
    0x00,
    0x00,
    0x01,
    0x20,
    0x00,
    0x00,
};

static unsigned char *get_assiic_table(char c) {
  unsigned char *ASIICTable_x;
  switch (c) {
  case '0':
    ASIICTable_x = (unsigned char *)&ASIICTable[0];
    break;
  case '1':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 1];
    break;
  case '2':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 2];
    break;
  case '3':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 3];
    break;
  case '4':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 4];
    break;
  case '5':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 5];
    break;
  case '6':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 6];
    break;
  case '7':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 7];
    break;
  case '8':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 8];
    break;
  case '9':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 9];
    break;
  case '.':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 10];
    break;
  case 'm':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 11];
    break;
  case 's':
    ASIICTable_x = (unsigned char *)&ASIICTable[32 * 32 / 8 * 12];
    break;
  case '?':
  default:
    ASIICTable_x = NULL;
    break;
  }
  printf("ASIICTable_x is 0x%p\n", ASIICTable_x);
  return ASIICTable_x;
}

static void draw_buffer(RK_U32 *ColorData) {
  unsigned char *ASIICTable_0 = NULL;
  unsigned char *ASIICTable_1 = NULL;
  unsigned char *ASIICTable_2 = NULL;
  unsigned char *ASIICTable_3 = NULL;
  unsigned char *ASIICTable_4 = NULL;
  unsigned char *ASIICTable_total;
  ASIICTable_total = malloc(32 * 32 / 8 * 5 * 2); // 160 * 4
  memset(ASIICTable_total, 0, 32 * 32 / 8 * 5 * 2);
  ASIICTable_0 = get_assiic_table(timestamp_char[0]);
  ASIICTable_1 = get_assiic_table(timestamp_char[1]);
  ASIICTable_2 = get_assiic_table(timestamp_char[2]);
  ASIICTable_3 = get_assiic_table('m');
  ASIICTable_4 = get_assiic_table('s');
  printf("ASIICTable_total is 0x%p\n", ASIICTable_total);

  // splice
  for (int i = 0; i < 32; i++) {                                // for per line
    memcpy(ASIICTable_total + i * 20, ASIICTable_0 + i * 4, 4); // 4byte = 32bit
    memcpy(ASIICTable_total + i * 20 + 4, ASIICTable_1 + i * 4, 4);
    memcpy(ASIICTable_total + i * 20 + 8, ASIICTable_2 + i * 4, 4);
    memcpy(ASIICTable_total + i * 20 + 12, ASIICTable_3 + i * 4, 4);
    memcpy(ASIICTable_total + i * 20 + 16, ASIICTable_4 + i * 4, 4);
  }

  // debug
  // for (int i = 0; i < 32 * 32 * 5 / 8; i++) {
  //   printf("0x%02x,", ASIICTable_total[i]);
  //   if (!((i + 1) % 16))
  //     printf("\n");
  // }

  for (int i = 0; i < 32 * 32 * 5 / 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (ASIICTable_total[i] & (1 << (7 - j)))
        ColorData[i * 8 + j] = TEST_ARGB32_RED;
      else
        ColorData[i * 8 + j] = TEST_ARGB32_TRANS;
      // printf("i is %d, j is %d, ColorData[%d] is 0x%x\n", i, j, i*8+j,
      // ColorData[i*8+j]);
    }
  }

  free(ASIICTable_total);
}

void rga_draw_to_vo_cb(MEDIA_BUFFER mb) {
  printf("Get RGA packet, ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld\n",
         RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
         RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
         RK_MPI_MB_GetTimestamp(mb));

  read_timestamp_from_uart();

  // Use rga's API to do rotate and overlay
  MEDIA_BUFFER timestamp_mb = NULL;
  MB_IMAGE_INFO_S stImageInfo = {160, 32, 160, 32, IMAGE_TYPE_ARGB8888};
  timestamp_mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, 0);
  if (!timestamp_mb) {
    printf("ERROR: RK_MPI_MB_CreateImageBuffer get null buffer!\n");
    return;
  }
  draw_buffer(RK_MPI_MB_GetPtr(timestamp_mb));

#if 0
  // rga_buffer_t timestamp_rga;
  // rga_buffer_t dst_rga;
  // timestamp_rga = wrapbuffer_fd(RK_MPI_MB_GetFD(timestamp_mb), 160, 32,
  //                               RK_FORMAT_BGRA_8888);
  // dst_rga = wrapbuffer_fd(RK_MPI_MB_GetFD(mb), disp1_width, disp1_height,
  //                         RK_FORMAT_BGRA_8888);
  // rga_buffer_t pat = {0};
  // im_rect src_rect = {0, 0, 160, 32};
  // im_rect dst_rect = {0, 0, 32, 160};
  // im_rect pat_rect = {0};
  // ret = improcess(timestamp_rga, dst_rga, pat, src_rect, dst_rect, pat_rect,
  //                 IM_HAL_TRANSFORM_ROT_270 | IM_ALPHA_BLEND_SRC_OVER);
  // printf("improcess ret is %d, reason: %s\n", ret, imStrError(ret));
#else
  for (int i = 0; i < 32; i++) {
    memcpy(RK_MPI_MB_GetPtr(mb) + i * disp1_width * 4,
           RK_MPI_MB_GetPtr(timestamp_mb) + i * 160 * 4, 160 * 4);
  }
#endif

  RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 1, mb);
  RK_MPI_MB_ReleaseBuffer(mb);
}

void hunt_video_packet_cb(MEDIA_BUFFER mb) {
  unsigned int random;
  printf("Get JPEG packet[%d]:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
         "timestamp:%lld\n",
         index_raw, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
         RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
         RK_MPI_MB_GetChannelID(mb), RK_MPI_MB_GetTimestamp(mb));

  char jpeg_path[64];
  getrandom(&random, sizeof(random), GRND_NONBLOCK);
  sprintf(jpeg_path, "/mnt/%08x-%d.jpeg", random, index_raw);
  FILE *file = fopen(jpeg_path, "w");
  printf("write jpeg start, jpeg_path=%s\n", jpeg_path);
  if (file) {
    fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), file);
    printf("write jpeg over\n");
    fclose(file);
  }
  RK_MPI_MB_ReleaseBuffer(mb);
}

static int thread_bind_cpu(int target_cpu) {
  cpu_set_t mask;
  int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
  int i;

  if (target_cpu >= cpu_num)
    return -1;

  CPU_ZERO(&mask);
  CPU_SET(target_cpu, &mask);

  if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    perror("pthread_setaffinity_np");

  if (pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    perror("pthread_getaffinity_np");

  // printf("Thread(%ld) bound to cpu:", gettid());
  for (i = 0; i < CPU_SETSIZE; i++) {
    if (CPU_ISSET(i, &mask)) {
      printf(" %d", i);
      break;
    }
  }
  printf("\n");

  return i >= cpu_num ? -1 : i;
}

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");
  printf("\t%s [-a [iqfiles_dir]]"
         "\n",
         name);
  printf("\t-a | --aiq: enable aiq with dirpath provided, eg:-a "
         "/oem/etc/iqfiles/, "
         "set dirpath empty to using path by default, without this option aiq "
         "should run in other application\n");
  printf("\t-s | --subdev: real camera isp sub devices \n");
  printf("\t-d | --device: fake camera video devices \n");
  printf("\t-n | --num: dump num \n");
}

void release_buffer_hunt(void *addr) {
  printf(">>>>release buffer called: addr=%p\n", addr);
}

void *init_aiq_fake_thread(void *arg) {
  int bind = 0;
  bind = thread_bind_cpu(2);
  if (bind == -1) {
    printf("bind cpu2 is err !!!!\n");
    err_flag = 1;
    return 0;
  }
  (void)arg;
  printf(" start to init_aiq_fake_thread!\n");
  // offline frame init
  printf("g_subdev_node =%s.\n", g_subdev_node);
  parse_mcu_rkraws(g_subdev_node, mcu_rkraws);
  alloc_rkraws(rkraws);
  // if (g_subdev_node)
  make_rkraws(mcu_rkraws, rkraws);

  rk_aiq_raw_prop_t prop;
  prop.format = RK_PIX_FMT_SBGGR10;
  prop.frame_width = fake_width;
  prop.frame_height = fake_height;
  prop.rawbuf_type = RK_AIQ_RAW_FD;

  g_aiq_ctx = aiq_fake_init(RK_AIQ_WORKING_MODE_NORMAL, iq_file_dir, prop,
                            release_buffer_hunt);
  printf("g_subdev_node_end =%s.\n", g_subdev_node);
  // offline frame init over

  // memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
  // VI_CHN_ATTR_S vi_chn_attr1;
  vi_chn_attr1.pcVideoNode = pcVideoNode;
  vi_chn_attr1.u32BufCnt = 6;
  vi_chn_attr1.u32Width = fake_width;
  vi_chn_attr1.u32Height = fake_height;
  vi_chn_attr1.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr1.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(1, 1, &vi_chn_attr1);
  ret |= RK_MPI_VI_EnableChn(1, 1);
  if (ret) {
    printf("#############Create vi[1] failed! ret=%d\n", ret);
    err_flag = 1;
  }

  return 0;
}

void *init_aiq_fake_vi_vo_thread(void *arg) {
  (void)arg;
  int bind = 0;
  bind = thread_bind_cpu(3);
  if (bind == -1) {
    printf("##############bind cpu3 is err !!!!\n");
    err_flag = 1;
    return 0;
  }
  // rga1 for overlay plane
  // RGA_ATTR_S stRgaAttr1;
  printf("Create rga[1]  ret=%d\n", ret);
  stRgaAttr1.bEnBufPool = RK_TRUE;
  stRgaAttr1.u16BufPoolCnt = 2;
  stRgaAttr1.u16Rotaion = 90;
  stRgaAttr1.stImgIn.u32X = 0;
  stRgaAttr1.stImgIn.u32Y = 0;
  stRgaAttr1.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr1.stImgIn.u32Width = fake_width;
  stRgaAttr1.stImgIn.u32Height = fake_height;
  stRgaAttr1.stImgIn.u32HorStride = fake_width;
  stRgaAttr1.stImgIn.u32VirStride = fake_height;
  stRgaAttr1.stImgOut.u32X = 0;
  stRgaAttr1.stImgOut.u32Y = 0;
  stRgaAttr1.stImgOut.imgType = IMAGE_TYPE_ARGB8888;
  stRgaAttr1.stImgOut.u32Width = disp1_width;
  stRgaAttr1.stImgOut.u32Height = disp1_height;
  stRgaAttr1.stImgOut.u32HorStride = disp1_width;
  stRgaAttr1.stImgOut.u32VirStride = disp1_height;
  ret = RK_MPI_RGA_CreateChn(1, &stRgaAttr1);
  if (ret) {
    printf("###########Create rga[1] falied! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  stRgaChn.enModId = RK_ID_RGA;
  stRgaChn.s32ChnId = 1;
  ret = RK_MPI_SYS_RegisterOutCb(&stRgaChn, rga_draw_to_vo_cb);
  if (ret) {
    printf("###############Register Output callback failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  // VO[1] for overlay plane
  // VO_CHN_ATTR_S stVoAttr1 = {0};
  printf("Create vo[1]\n");
  memset(&stVoAttr1, 0, sizeof(stVoAttr1));
  stVoAttr1.pcDevNode = "/dev/dri/card0";
  stVoAttr1.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr1.enImgType = IMAGE_TYPE_ARGB8888;
  stVoAttr1.u16Zpos = 1;
  stVoAttr1.stImgRect.s32X = 0;
  stVoAttr1.stImgRect.s32Y = 0;
  stVoAttr1.stImgRect.u32Width = disp1_width;
  stVoAttr1.stImgRect.u32Height = disp1_height;
  stVoAttr1.stDispRect.s32X = 0;
  stVoAttr1.stDispRect.s32Y = 0;
  stVoAttr1.stDispRect.u32Width = disp1_width;
  stVoAttr1.stDispRect.u32Height = disp1_height;
  ret = RK_MPI_VO_CreateChn(1, &stVoAttr1);
  if (ret) {
    printf("#############Create vo[1] failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  venc_chn_attr.stVencAttr.u32PicWidth = fake_width;
  venc_chn_attr.stVencAttr.u32PicHeight = fake_height;
  venc_chn_attr.stVencAttr.u32VirWidth = fake_width;
  venc_chn_attr.stVencAttr.u32VirHeight = fake_height;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomWidth = fake_width;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomHeight = fake_height;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomVirWidth = fake_width;
  venc_chn_attr.stVencAttr.stAttrJpege.u32ZoomVirHeight = fake_height;
  venc_chn_attr.stVencAttr.enRotation = VENC_ROTATION_0;

  ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
  if (ret) {
    printf("##############Create Venc failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  printf("Create callback[0]  ret=%d\n", ret);
  // MPP_CHN_S stEncChn;
  stEncChn.enModId = RK_ID_VENC;
  stEncChn.s32ChnId = 0;
  ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, hunt_video_packet_cb);
  if (ret) {
    printf("###############Register Output callback failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  // printf("Create callback[1]  ret=%d\n", ret);
  // //MPP_CHN_S stEncChn;
  // memset(&stEncChn, 0, sizeof(stEncChn));
  // stEncChn.enModId = RK_ID_VI;
  // stEncChn.s32ChnId = 1;
  // ret = RK_MPI_SYS_RegisterOutCb(&stEncChn, hunt_vi_packet_cb);
  // if (ret) {
  //     printf("#############Register Output callback failed! ret=%d\n", ret);
  //     err_flag = 1;
  //     return 0;
  // }

  // The encoder defaults to continuously receiving frames from the previous
  // stage. Before performing the bind operation, set s32RecvPicNum to 0 to
  // make the encoding enter the pause state.
  stRecvParam.s32RecvPicNum = 0;
  RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);
  return 0;
}

void *init_aiq_isp_thread(void *arg) {
  (void)arg;
  int bind = 0;
  bind = thread_bind_cpu(3);
  if (bind == -1) {
    printf("bind cpu3 is err !!!!\n");
    err_flag = 1;
    return 0;
  }
  printf("start to init_isp_thread!\n");
  printf("Create vi[0] \n");
  // VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.u32BufCnt = 6;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("#############Create vi[0] failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }
  printf("Create rga[0]  ret=%d\n", ret);
  // RGA_ATTR_S stRgaAttr;
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  stRgaAttr.u16BufPoolCnt = 2;
  stRgaAttr.u16Rotaion = 270;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_RGB888;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("###############Create rga[0] falied! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  printf("Create vo[0]\n");
  // VO_CHN_ATTR_S stVoAttr = {0};
  // VO[0] for primary plane
  memset(&stVoAttr, 0, sizeof(stVoAttr));
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
  stVoAttr.enImgType = IMAGE_TYPE_RGB888;
  stVoAttr.u16Zpos = 0;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;
  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("###############Create vo[0] failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }
  printf("Bind VI[0] to RGA[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("##############Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }

  printf("Bind RGA[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("###############Bind rga[0] to vo[0] failed! ret=%d\n", ret);
    err_flag = 1;
    return 0;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int c;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
        iq_file_dir = (char *)tmp_optarg;
      } else {
        iq_file_dir = "/etc/iqfiles";
      }
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;

#ifdef RKAIQ
    case 'M':
      if (atoi(optarg)) {
        // bMultictx = RK_TRUE;
      }
      break;
#endif

    case 's':
      g_subdev_node = optarg;
      break;
    case 'd':
      pcVideoNode = optarg;
      break;
    case 'n':
      frame_count = atoi(optarg);
      if (frame_count < 0) {
        printf("Parameter error, reset min frame_count = 1.\n");
        frame_count = 1;
      }
      if (frame_count > 5) {
        printf("Parameter error, reset max frame_count = 5.\n");
        frame_count = 5;
      }
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#CameraIdx: %d\n\n", s32CamId);

  pthread_t tid_isp;
  pthread_t tid_fake;
  pthread_t tid_fak_vi_vo;
  int pth = 0;
  if (iq_file_dir) {
    // #ifdef RKAIQ
    printf("#Rkaiq XML DirPath: %s\n", iq_file_dir);
    printf("#bMultictx: %d\n\n", bMultictx);
    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    int fps = 30;
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, iq_file_dir);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, fps);
    // #endif
  }
  printf("end to init_isp_thread!\n");
  RK_MPI_SYS_Init();
  pth = pthread_create(&tid_fake, NULL, init_aiq_fake_thread, NULL);
  if (pth != 0) {
    printf("pthread_create is err\n");
    return -1;
  }
  pth = pthread_create(&tid_fak_vi_vo, NULL, init_aiq_fake_vi_vo_thread, NULL);
  if (pth != 0) {
    printf("pthread_create is err\n");
    return -1;
  }
  pth = pthread_create(&tid_isp, NULL, init_aiq_isp_thread, NULL);
  if (pth != 0) {
    printf("pthread_create is err\n");
    return -1;
  }
  // pthread_join(tid_isp,NULL);
  pthread_join(tid_fake, NULL);
  pthread_join(tid_fak_vi_vo, NULL);
  if (err_flag == 1) {
    printf("rkmedia init is err!!!\n");
    return -1;
  }

  printf("Bind VI[1] to VENC[0]\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind VI[1] to VENC[0]::JPEG failed! ret=%d\n", ret);
    return -1;
  }
  printf("Bind VI[1] to RGA[1]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }
  // printf("Bind RGA[1] to VO[1]....\n");
  // stSrcChn.enModId = RK_ID_RGA;
  // stSrcChn.s32ChnId = 1;
  // stDestChn.enModId = RK_ID_VO;
  // stDestChn.s32ChnId = 1;
  // ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  // if (ret) {
  //   printf("Bind rga[1] to vo[1] failed! ret=%d\n", ret);
  //   return -1;
  // }
  ret = 0;
  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);

  // The algorithm requires two frames to be input as reference frames
  for (int p = 0; p < 2; p++) {
    ret = rk_aiq_uapi_sysctl_enqueueRkRawBuf(g_aiq_ctx,
                                             (void *)rkraws[index_raw], false);
    printf(">>>>>>>>> #### enqueue reference raw. ret = %d.\n", ret);
    usleep(10 * 1000);
  }
  for (; index_raw < frame_count; index_raw++) {
    ret = rk_aiq_uapi_sysctl_enqueueRkRawBuf(g_aiq_ctx,
                                             (void *)rkraws[index_raw], false);
    printf(">>>>>>>>> enqueue raw. ret = %d.\n", ret);
    usleep(10 * 10000);
    stRecvParam.s32RecvPicNum = 1;
    ret = RK_MPI_VENC_StartRecvFrame(0, &stRecvParam);
    if (ret) {
      printf("RK_MPI_VENC_StartRecvFrame failed!\n");
      break;
    }
    printf("RK_MPI_VENC_StartRecvFrame success!\n");
  }

  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_VENC;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[1] to venc[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[1] to rga[1] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 1;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 1;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[1] to vo[1] failed! ret=%d\n", ret);
    return -1;
  }
  err_flag = 0;
  RK_MPI_VO_DestroyChn(0);
  RK_MPI_VO_DestroyChn(1);
  RK_MPI_RGA_DestroyChn(0);
  // RK_MPI_RGA_DestroyChn(1);
  RK_MPI_VI_DisableChn(s32CamId, 0);
  // RK_MPI_VI_DisableChn(s32CamId, 1);

  // offline frame free memory
  free_rkraws(rkraws);
  free_rkisp_reserve_mem(g_subdev_node);
  rk_aiq_uapi_sysctl_stop(g_aiq_ctx, false);
  printf("rk_aiq_uapi_sysctl_deinit enter\n");
  rk_aiq_uapi_sysctl_deinit(g_aiq_ctx);
  printf("rk_aiq_uapi_sysctl_deinit exit\n");
  g_aiq_ctx = NULL;

  if (iq_file_dir) {
#ifdef RKAIQ
    SAMPLE_COMM_ISP_Stop(s32CamId);
#endif
  }
  return 0;
}

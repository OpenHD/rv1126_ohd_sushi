// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "utils.h"

#include "mpp_jpeg_packet.h"

static char exif_header[8] = {0x00, 0x00, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00};
static char tiff_header[8] = {0x49, 0x49, 0x2A, 0x00, 0x08, 0x00, 0x00, 0x00};
static int format_2_len[13] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
static char mp_header[6] = {0x00, 0xE3, 0x4D, 0x50, 0x46, 0x00};

int PackageApp1(IFD_S ifd0[], IFD_S ifd1[], int ifd0_len, int ifd1_len,
                char *app1_buf, char *thumbnail_buf, int thumbnail_len) {
  int dir_start = 0;
  int data_start = 0;
  int data_len = 0;
  int total_len = 0;
  app1_buf[0] = 0xff;
  app1_buf[1] = 0xe1;
  memcpy(app1_buf + 2, exif_header, 8);
  app1_buf += 10;
  memcpy(app1_buf + dir_start, tiff_header, 8);
  dir_start += 8;
  // package ifd0
  *(short *)(app1_buf + dir_start) = ifd0_len;
  dir_start += 2;
  data_start = dir_start + ifd0_len * 12 + 4;
  for (int i = 0; i < ifd0_len; i++, dir_start += 12) {
    int value_len = ifd0[i].length * format_2_len[ifd0[i].format];
    if (value_len > 4) {
      if (ifd0[i].format == 2) {
        memcpy(app1_buf + data_start, ifd0[i].value[0].asv,
               strlen(ifd0[i].value[0].asv));
      } else {
        memcpy(app1_buf + data_start, (char *)ifd0[i].value, value_len);
      }
      ifd0[i].offset = data_start;
      data_start += value_len;
      data_len += value_len;
    }
    memcpy(app1_buf + dir_start, (char *)&ifd0[i], 12);
  }
  *(int *)(app1_buf + dir_start) = data_start;
  dir_start = data_start;
  // package ifd1
  *(short *)(app1_buf + dir_start) = ifd1_len;
  dir_start += 2;
  data_len = 0;
  data_start = dir_start + ifd1_len * 12 + 4;
  for (int i = 0; i < ifd1_len; i++, dir_start += 12) {
    int value_len = ifd1[i].length * format_2_len[ifd1[i].format];
    if (ifd1[i].tag_no == 0x0201) {
      ifd1[i].offset = ((data_start + 0x200) / 0x200) * 0x200;
      memcpy(app1_buf + ifd1[i].offset, thumbnail_buf, thumbnail_len);
      total_len = ifd1[i].offset + thumbnail_len;
    } else if (ifd1[i].tag_no == 0x0202) {
      ifd1[i].offset = thumbnail_len;
    } else if (value_len > 4) {
      if (ifd1[i].format == 2) {
        memcpy(app1_buf + data_start, ifd1[i].value[0].asv,
               strlen(ifd1[i].value[0].asv));
      } else {
        memcpy(app1_buf + data_start, (char *)ifd1[i].value, value_len);
      }
      ifd1[i].offset = data_start;
      data_start += value_len;
      data_len += value_len;
    }
    memcpy(app1_buf + dir_start, (char *)&ifd1[i], 12);
  }
  *(int *)(app1_buf + dir_start) = 0;
  app1_buf -= 10;
  total_len += 10;
  app1_buf[2] = ((total_len - 2) >> 8) & 0xff;
  app1_buf[3] = (total_len - 2) & 0xff;
  return total_len;
}

int PackageApp2(IFD_S mpfd[], int mpfd_len, int image_cnt, char *app2_buf,
                int thumbnail_len[]) {
  int dir_start = 0;
  int data_start = 0;
  int data_len = 0;
  int mpf_offset[3];
  int total_len = 0;

  mpf_offset[0] = 0;
  mpf_offset[1] = thumbnail_len[0] - 0x08;
  mpf_offset[2] = mpf_offset[1] + thumbnail_len[1];

  app2_buf[0] = 0xff;
  app2_buf[1] = 0xe2;
  memcpy(app2_buf + 2, mp_header, 6);
  app2_buf += 8;
  memcpy(app2_buf + dir_start, tiff_header, 8);
  dir_start += 8;
  *(short *)(app2_buf + dir_start) = mpfd_len;
  dir_start += 2;
  data_start = dir_start + mpfd_len * 12 + 4;
  for (int i = 0; i < mpfd_len; i++, dir_start += 12) {
    int value_len = mpfd[i].length * format_2_len[mpfd[i].format];
    if (mpfd[i].tag_no == 0xB001) {
      mpfd[i].offset = image_cnt;
    } else if (mpfd[i].tag_no == 0xB002) {
      mpfd[i].length = image_cnt * 16;
      mpfd[i].offset = data_start;
      for (int j = 0; j < image_cnt; j++) {
        if (j == 0) {
          *(long *)(app2_buf + data_start + j * 16) =
              (1 << 31) | (0 << 30) | (1 << 29) | 0x030000;
        } else {
          *(long *)(app2_buf + data_start + j * 16) =
              (0 << 31) | (1 << 30) | (0 << 29) | 0x010001;
        }
        *(long *)(app2_buf + data_start + j * 16 + 4) = thumbnail_len[j];
        *(long *)(app2_buf + data_start + j * 16 + 8) = mpf_offset[j];
        if ((image_cnt > 1) && (j == 0)) {
          *(short *)(app2_buf + data_start + j * 16 + 12) = 0x2;
        } else {
          *(short *)(app2_buf + data_start + j * 16 + 12) = 0;
        }
        if ((image_cnt > 2) && (j == 0)) {
          *(short *)(app2_buf + data_start + j * 16 + 14) = 0x3;
        } else {
          *(short *)(app2_buf + data_start + j * 16 + 14) = 0;
        }
      }
      data_start += mpfd[i].length;
      data_len += mpfd[i].length;
    } else if (mpfd[i].tag_no == 0xB003) {
      mpfd[i].length = image_cnt * 33;
      mpfd[i].offset = data_start;
      memset(app2_buf + data_start, 0x00, mpfd[i].length);
      data_start += mpfd[i].length;
      data_len += mpfd[i].length;
    } else if (value_len > 4) {
      if (mpfd[i].format == 2) {
        memcpy(app2_buf + data_start, mpfd[i].value[0].asv,
               strlen(mpfd[i].value[0].asv));
      } else {
        memcpy(app2_buf + data_start, (char *)mpfd[i].value, value_len);
      }
      mpfd[i].offset = data_start;
      data_start += value_len;
      data_len += value_len;
    }
    memcpy(app2_buf + dir_start, (char *)&mpfd[i], 12);
  }
  *(int *)(app2_buf + dir_start) = 0;
  app2_buf -= 8;
  total_len = data_start + 0x08;
  app2_buf[2] = ((total_len - 2) >> 8) & 0xff;
  app2_buf[3] = (total_len - 2) & 0xff;
  return total_len;
}

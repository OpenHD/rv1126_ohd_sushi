// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utils.h"

#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#ifdef RKMEDIA_SUPPORT_MINILOG
#include "minilogger/log.h"
#endif
#undef NDEBUG
#include <assert.h>
enum { LOG_METHOD_MINILOG, LOG_METHOD_PRINT };

int rkmedia_log_method = LOG_METHOD_PRINT;
static int rkmedia_log_level = LOG_LEVEL_INFO;

_API short g_level_list[RK_ID_BUTT] = {
    LOG_LEVEL_INFO, // g_unknow_log_level
    LOG_LEVEL_INFO, // g_vb_log_level
    LOG_LEVEL_INFO, // g_sys_log_level
    LOG_LEVEL_INFO, // g_vdec_log_level
    LOG_LEVEL_INFO, // g_venc_log_level
    LOG_LEVEL_INFO, // g_h264e_log_level
    LOG_LEVEL_INFO, // g_jpege_log_level
    LOG_LEVEL_INFO, // g_h265e_log_level
    LOG_LEVEL_INFO, // g_vo_log_level
    LOG_LEVEL_INFO, // g_vi_log_level
    LOG_LEVEL_INFO, // g_vp_log_level
    LOG_LEVEL_INFO, // g_aio_log_level
    LOG_LEVEL_INFO, // g_ai_log_level
    LOG_LEVEL_INFO, // g_ao_log_level
    LOG_LEVEL_INFO, // g_aenc_log_level
    LOG_LEVEL_INFO, // g_adec_log_level
    LOG_LEVEL_INFO, // g_algo_md_log_level
    LOG_LEVEL_INFO, // g_algo_od_log_level
    LOG_LEVEL_INFO, // g_rga_log_level
};

_API const char *mod_tag_list[RK_ID_BUTT] = {
    "UNKNOW", "VB",   "SYS",     "VDEC",    "VENC", "H264E", "JPEGE",
    "H265E",  "VO",   "VI",      "VP",      "AIO",  "AI",    "AO",
    "AENC",   "ADEC", "ALGO_MD", "ALGO_OD", "RGA",  "VMIX",  "MUXER"};

#define LOG_FILE_PATH "/tmp/loglevel"
static bool monitor_log_level_quit = false;

static void read_log_level() {
  char text[20], module[10];
  int log_level;
  int fd = open(LOG_FILE_PATH, O_RDONLY);
  if (fd == -1)
    return;
  int len = read(fd, text, 20);
  close(fd);
  if (len < 5) {
    RKMEDIA_LOGE("cmd too short, example: echo \"all=4\" > /tmp/loglevel\n");
    return;
  }

  RKMEDIA_LOGI("text is %s\n", text);
  sscanf(text, "%[^=]=%d", module, &log_level);
  RKMEDIA_LOGI("module is %s, log_level is %d\n", module, log_level);
  if (!strcmp(module, "unknow")) {
    g_level_list[RK_ID_UNKNOW] = log_level;
  } else if (!strcmp(module, "vb")) {
    g_level_list[RK_ID_VB] = log_level;
  } else if (!strcmp(module, "sys")) {
    g_level_list[RK_ID_SYS] = log_level;
  } else if (!strcmp(module, "vdec")) {
    g_level_list[RK_ID_VDEC] = log_level;
  } else if (!strcmp(module, "venc")) {
    g_level_list[RK_ID_VENC] = log_level;
  } else if (!strcmp(module, "h264e")) {
    g_level_list[RK_ID_H264E] = log_level;
  } else if (!strcmp(module, "jpege")) {
    g_level_list[RK_ID_JPEGE] = log_level;
  } else if (!strcmp(module, "h265e")) {
    g_level_list[RK_ID_H265E] = log_level;
  } else if (!strcmp(module, "vo")) {
    g_level_list[RK_ID_VO] = log_level;
  } else if (!strcmp(module, "vi")) {
    g_level_list[RK_ID_VI] = log_level;
  } else if (!strcmp(module, "aio")) {
    g_level_list[RK_ID_AIO] = log_level;
  } else if (!strcmp(module, "ai")) {
    g_level_list[RK_ID_AI] = log_level;
  } else if (!strcmp(module, "ao")) {
    g_level_list[RK_ID_AO] = log_level;
  } else if (!strcmp(module, "aenc")) {
    g_level_list[RK_ID_AENC] = log_level;
  } else if (!strcmp(module, "adec")) {
    g_level_list[RK_ID_ADEC] = log_level;
  } else if (!strcmp(module, "algo_md")) {
    g_level_list[RK_ID_ALGO_MD] = log_level;
  } else if (!strcmp(module, "algo_od")) {
    g_level_list[RK_ID_ALGO_OD] = log_level;
  } else if (!strcmp(module, "rga")) {
    g_level_list[RK_ID_RGA] = log_level;
  } else if (!strcmp(module, "vp")) {
    g_level_list[RK_ID_VP] = log_level;
  } else if (!strcmp(module, "all")) {
    for (int i = 0; i < RK_ID_BUTT; i++) {
      g_level_list[i] = log_level;
    }
  } else {
    RKMEDIA_LOGE("unknow module is %s\n", module);
  }
}

static void *monitor_log_level(void *arg) {
  RKMEDIA_LOGD("#Start %s thread, arg:%p\n", __func__, arg);
  int fd;
  int wd;
  char buf[BUFSIZ];
  struct inotify_event *event;

  fd = inotify_init();
  if (fd < 0) {
    fprintf(stderr, "inotify_init failed\n");
    return NULL;
  }
  wd = inotify_add_watch(fd, LOG_FILE_PATH, IN_CLOSE_WRITE);
  if (wd < 0) {
    fprintf(stderr, "inotify_add_watch failed\n");
    return NULL;
  }
  buf[sizeof(buf) - 1] = 0;

  fd_set rfds;
  int nfds = wd + 1;
  struct timeval timeout;
  while (!monitor_log_level_quit) {
    // The rfds collection must be emptied every time,
    // otherwise the descriptor changes cannot be detected
    timeout.tv_sec = 1;
    FD_ZERO(&rfds);
    FD_SET(wd, &rfds);
    select(nfds, &rfds, NULL, NULL, &timeout);
    // wait for the key event to occur
    if (FD_ISSET(wd, &rfds)) {
      int len = read(fd, buf, sizeof(buf) - 1);
      int nread = 0;
      while (len > 0) {
        event = (struct inotify_event *)&buf[nread];
        read_log_level();
        nread = nread + sizeof(struct inotify_event) + event->len;
        len = len - sizeof(struct inotify_event) - event->len;
      }
    }
  }
  RKMEDIA_LOGI("monitor_log_level quit\n");
  inotify_rm_watch(fd, wd);
  close(fd);

  return NULL;
}

_API void LOG_INIT() {
  char *ptr = NULL;
  for (int i = 0; i < RK_ID_BUTT; i++)
    assert(mod_tag_list[i] != NULL);
  rkmedia_log_method = LOG_METHOD_PRINT;
#ifdef RKMEDIA_SUPPORT_MINILOG
  ptr = getenv("RKMEDIA_LOG_METHOD");
  if (ptr && strstr(ptr, "MINILOG")) {
    fprintf(stderr, "##RKMEDIA Method: MINILOG\n");
    __minilog_log_init("RKMEDIA", NULL, false, false, "RKMEDIA", "0.1");
    rkmedia_log_method = LOG_METHOD_MINILOG;
  } else {
    fprintf(stderr, "##RKMEDIA Method: PRINT\n");
    rkmedia_log_method = LOG_METHOD_PRINT;
  }
#endif //#ifdef RKMEDIA_SUPPORT_MINILOG

  ptr = getenv("RKMEDIA_LOG_LEVEL");
  if (ptr && strstr(ptr, "ERROR"))
    rkmedia_log_level = LOG_LEVEL_ERROR;
  else if (ptr && strstr(ptr, "WARN"))
    rkmedia_log_level = LOG_LEVEL_WARN;
  else if (ptr && strstr(ptr, "DBG"))
    rkmedia_log_level = LOG_LEVEL_DBG;
  else
    rkmedia_log_level = LOG_LEVEL_INFO;
  fprintf(stderr, "##RKMEDIA Log level: %d\n", rkmedia_log_level);

  int log_file_fd = open(LOG_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC);
  if (log_file_fd != -1) {
    char init_log_level[6];
    sprintf(init_log_level, "all=%d", rkmedia_log_level);
    write(log_file_fd, init_log_level, 6);
    close(log_file_fd);
    read_log_level();
    pthread_t log_level_read_thread;
    pthread_create(&log_level_read_thread, NULL, monitor_log_level, NULL);
  } else {
    RKMEDIA_LOGE("open " LOG_FILE_PATH " fail\n");
  }
}

_API void LOG_DEINIT() { monitor_log_level_quit = true; }

namespace easymedia {

bool parse_media_param_map(const char *param,
                           std::map<std::string, std::string> &map) {
  if (!param)
    return false;

  std::string token;
  std::istringstream tokenStream(param);
  while (std::getline(tokenStream, token)) {
    std::string key, value;
    std::istringstream subTokenStream(token);
    if (std::getline(subTokenStream, key, '='))
      std::getline(subTokenStream, value);
    map[key] = value;
  }

  return true;
}

bool parse_media_param_list(const char *param, std::list<std::string> &list,
                            const char delim) {
  if (!param)
    return false;

  std::string token;
  std::istringstream tokenStream(param);
  while (std::getline(tokenStream, token, delim))
    list.push_back(token);

  return true;
}

int parse_media_param_match(
    const char *param, std::map<std::string, std::string> &map,
    std::list<std::pair<const std::string, std::string &>> &list) {
  if (!easymedia::parse_media_param_map(param, map))
    return 0;
  int match_num = 0;
  for (auto &key : list) {
    const std::string &req_key = key.first;
    for (auto &entry : map) {
      const std::string &entry_key = entry.first;
      if (req_key == entry_key) {
        key.second = entry.second;
        match_num++;
        break;
      }
    }
  }

  return match_num;
}

bool has_intersection(const char *str, const char *expect,
                      std::list<std::string> *expect_list) {
  std::list<std::string> request;
  // RKMEDIA_LOGI("request: %s; expect: %s\n", str, expect);

  if (!expect || (strlen(expect) == 0))
    return true;

  if (!parse_media_param_list(str, request, ','))
    return false;
  if (expect_list->empty() && !parse_media_param_list(expect, *expect_list))
    return false;
  for (auto &req : request) {
    if (std::find(expect_list->begin(), expect_list->end(), req) !=
        expect_list->end()) {
      return true;
    }
  }
  return false;
}

std::string get_media_value_by_key(const char *param, const char *key) {
  std::map<std::string, std::string> param_map;
  if (!parse_media_param_map(param, param_map))
    return std::string();
  return param_map[key];
}

bool string_start_withs(std::string const &fullString,
                        std::string const &starting) {
  if (fullString.length() >= starting.length()) {
    return (0 == fullString.find(starting));
  } else {
    return false;
  }
}

bool string_end_withs(std::string const &fullString,
                      std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}

#ifndef NDEBUG

#include <fcntl.h>
#include <unistd.h>

bool DumpToFile(std::string path, const char *ptr, size_t len) {
  int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_FSYNC);
  if (fd < 0) {
    RKMEDIA_LOGI("Fail to open %s\n", path.c_str());
    return false;
  }
  RKMEDIA_LOGD("dump to %s, size %d\n", path.c_str(), (int)len);
  write(fd, ptr, len);
  close(fd);
  return true;
}

#endif // #ifndef NDEBUG

} // namespace easymedia

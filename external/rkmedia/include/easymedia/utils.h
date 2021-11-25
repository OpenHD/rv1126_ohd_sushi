// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_UTILS_H_
#define EASYMEDIA_UTILS_H_

#include <stdarg.h>
#include <string.h>
#ifdef RKMEDIA_SUPPORT_MINILOG
#include "minilogger/log.h"
#endif
#include "rkmedia_common.h"

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DBG 3

#ifndef MOD_TAG
#define MOD_TAG RK_ID_SYS
#endif

extern short g_level_list[RK_ID_BUTT];
extern int rkmedia_log_method;
extern const char *mod_tag_list[RK_ID_BUTT];

#define LOG_LEVEL_JUDGE(FILTER_LEVEL)                                          \
  {                                                                            \
    if (g_level_list[MOD_TAG] < FILTER_LEVEL)                                  \
      break;                                                                   \
  }

#ifdef RKMEDIA_SUPPORT_MINILOG
#define RKMEDIA_LOGE(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_ERROR);                                          \
    minilog_error("[RKMEDIA][%s][Error]:" format, mod_tag_list[MOD_TAG],       \
                  ##__VA_ARGS__);                                              \
  } while (0)

#define RKMEDIA_LOGW(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_WARN);                                           \
    minilog_warn("[RKMEDIA][%s][Warn]:" format, mod_tag_list[MOD_TAG],         \
                 ##__VA_ARGS__);                                               \
  } while (0)

#define RKMEDIA_LOGI(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_INFO);                                           \
    minilog_info("[RKMEDIA][%s][Info]:" format, mod_tag_list[MOD_TAG],         \
                 ##__VA_ARGS__);                                               \
  } while (0)

#define RKMEDIA_LOGD(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_DBG);                                            \
    minilog_debug("[RKMEDIA][%s][Debug]:" format, mod_tag_list[MOD_TAG],       \
                  ##__VA_ARGS__);                                              \
  } while (0)

#else // RKMEDIA_SUPPORT_MINILOG

#define RKMEDIA_LOGE(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_ERROR);                                          \
    fprintf(stderr, "[RKMEDIA][%s][Error]:" format, mod_tag_list[MOD_TAG],     \
            ##__VA_ARGS__);                                                    \
  } while (0)

#define RKMEDIA_LOGW(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_WARN);                                           \
    fprintf(stderr, "[RKMEDIA][%s][Warn]:" format, mod_tag_list[MOD_TAG],      \
            ##__VA_ARGS__);                                                    \
  } while (0)

#define RKMEDIA_LOGI(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_INFO);                                           \
    fprintf(stderr, "[RKMEDIA][%s][Info]:" format, mod_tag_list[MOD_TAG],      \
            ##__VA_ARGS__);                                                    \
  } while (0)

#define RKMEDIA_LOGD(format, ...)                                              \
  do {                                                                         \
    LOG_LEVEL_JUDGE(LOG_LEVEL_DBG);                                            \
    fprintf(stderr, "[RKMEDIA][%s][Debug]:" format, mod_tag_list[MOD_TAG],     \
            ##__VA_ARGS__);                                                    \
  } while (0)
#endif // RKMEDIA_SUPPORT_MINILOG

#define _UNUSED __attribute__((unused))
#define UNUSED(x) (void)x

#define _LOCAL __attribute__((visibility("hidden")))
#define _API __attribute__((visibility("default")))

_API void LOG_INIT();
_API void LOG_DEINIT();

#define LOG_NO_MEMORY()                                                        \
  RKMEDIA_LOGI("No memory %s: %d\n", __FUNCTION__, __LINE__)
#define LOG_FILE_FUNC_LINE()                                                   \
  RKMEDIA_LOGI("%s : %s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define LOG_TODO()                                                             \
  RKMEDIA_LOGI("TODO, %s : %s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))

#define UPALIGNTO16(value) UPALIGNTO(value, 16)

#define DOWNALIGNTO16(value) (UPALIGNTO(value, 16) - 16)

#define ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define MATH_LOG2(x) (31 - __builtin_clz((x) | 1))

#ifndef PAGE_SIZE
#define PAGE_SIZE (sysconf(_SC_PAGESIZE))
#endif

#define DUMP_FOURCC(f)                                                         \
  f & 0xFF, (f >> 8) & 0xFF, (f >> 16) & 0xFF, (f >> 24) & 0xFF

#define VALUE_MAX(A, B) ((A) > (B) ? (A) : (B))
#define VALUE_MIN(A, B) ((A) < (B) ? (A) : (B))
#define VALUE_SCOPE_CHECK(X, MIN, MAX) assert((X >= MIN) && (X <= MAX))

template <typename T, typename TBase> class IsDerived {
public:
  static int t(TBase *base) { return 1; }
  static char t(void *t2) { return 0; }
  static const bool Result = (sizeof(int) == sizeof(t((T *)nullptr)));
};

#include <list>
#include <map>
#include <string>
#include <thread>

namespace easymedia {

#define GET_STRING_TO_INT(var, map, key, defalut)                              \
  if (!map[key].empty())                                                       \
    var = std::stoi(map[key]);                                                 \
  else                                                                         \
    var = defalut;

#define CHECK_EMPTY_SETERRNO_RETURN(v_type, v, map, k, seterrno, ret)          \
  v_type v = map[k];                                                           \
  if (v.empty()) {                                                             \
    RKMEDIA_LOGE("%s: miss %s\n", __func__, k);                                \
    seterrno;                                                                  \
    return ret;                                                                \
  }

#define CHECK_EMPTY(v, map, k) CHECK_EMPTY_SETERRNO_RETURN(, v, map, k, , false)

#define CHECK_EMPTY_WITH_DECLARE(v_type, v, map, k)                            \
  CHECK_EMPTY_SETERRNO_RETURN(v_type, v, map, k, , false)

#define CHECK_EMPTY_SETERRNO(v, map, k, err)                                   \
  CHECK_EMPTY_SETERRNO_RETURN(, v, map, k, errno = err, )

#define PARAM_STRING_APPEND(s, s1, s2) s.append(s1 "=").append(s2).append("\n")

#define PARAM_STRING_APPEND_TO(s, s1, s2)                                      \
  s.append(s1 "=").append(std::to_string(s2)).append("\n")

#define PARAM_STRING_APPEND_PARAM_STRING(p1, p2) p1.append(" ").append(p2)

// delim: '=', '\n'
_API bool parse_media_param_map(const char *param,
                                std::map<std::string, std::string> &map);
bool parse_media_param_list(const char *param, std::list<std::string> &list,
                            const char delim = '\n');
int parse_media_param_match(
    const char *param, std::map<std::string, std::string> &map,
    std::list<std::pair<const std::string, std::string &>> &list);
_API bool has_intersection(const char *str, const char *expect,
                           std::list<std::string> *expect_list);

std::string get_media_value_by_key(const char *param, const char *key);

_API bool string_start_withs(std::string const &fullString,
                             std::string const &starting);
_API bool string_end_withs(std::string const &fullString,
                           std::string const &ending);

#define FIND_ENTRY_TARGET(INPUT, MAP, KEY, TARGET)                             \
  for (size_t i = 0; i < ARRAY_ELEMS(MAP); i++) {                              \
    if (INPUT == MAP[i].KEY)                                                   \
      return MAP[i].TARGET;                                                    \
  }

#define FIND_ENTRY_TARGET_BY_STRCMP(INPUT, MAP, KEY, TARGET)                   \
  if (INPUT) {                                                                 \
    for (size_t i = 0; i < ARRAY_ELEMS(MAP); i++) {                            \
      if (!strcmp(INPUT, MAP[i].KEY))                                          \
        return MAP[i].TARGET;                                                  \
    }                                                                          \
  }

// return microseconds
_API inline int64_t gettimeofday() {
  std::chrono::microseconds us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  return us.count();
}

_API inline void msleep(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

_API inline void usleep(int us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}

typedef int (*Ioctl_f)(int fd, unsigned long int request, ...);
inline int xioctl(Ioctl_f f, int fd, int request, void *argp) {
  int r;

  do
    r = f(fd, request, argp);
  while (-1 == r && EINTR == errno);

  return r;
}

#ifndef NDEBUG
_API bool DumpToFile(std::string path, const char *ptr, size_t len);
#endif

class AutoDuration {
public:
  AutoDuration() { Reset(); }
  int64_t Get() { return gettimeofday() - start; }
  void Reset() { start = gettimeofday(); }
  int64_t GetAndReset() {
    int64_t now = gettimeofday();
    int64_t pretime = start;
    start = now;
    return now - pretime;
  }

private:
  int64_t start;
};

#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))

class AutoPrintLine {
#ifndef NDEBUG
public:
  AutoPrintLine(const char *f) : func(f) { RKMEDIA_LOGD("Enter %s\n", f); }
  ~AutoPrintLine() { RKMEDIA_LOGD("Exit %s\n", func); }

private:
  const char *func;
#else
public:
  AutoPrintLine(const char *f _UNUSED) {}
#endif
};

} // namespace easymedia

#endif // #ifndef EASYMEDIA_UTILS_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "ftp_func.h"
#include "ftp.h"

static char *g_name = NULL;
static char *g_passwd = NULL;
static char *g_ip = NULL;
static int g_port = -1;
static char *g_path = NULL;
static int init_flag = 0;

static int ftp_login_func() {
  int sk = ftp_connect(g_ip, g_port, g_name, g_passwd);
  if (sk < 0) {
    printf("ftp login fail, ip: %s, port: %d, name: %s, pw: %s\n", g_ip, g_port,
           g_name, g_passwd);
    return -1;
  }
  return sk;
}

static void quit_ftp(int sk) {
  if (sk != -1) {
    int quit = ftp_quit(sk);
    if (0)
      printf("quit status is %d!\n", quit);
    return;
  }
  printf("no sk to quit!\n");
}

static int ftp_entry_dir_step_func(int sk, char *dir) {
  if (!dir) {
    printf("enter dir is null!\n");
    return -1;
  }
  if (sk == -1) {
    printf("please login ftp first!\n");
    return -1;
  }
  int cd;
  int mk;
  cd = ftp_cwd(sk, dir);
  if (cd) {
    mk = ftp_mkdir(sk, dir);
    if (mk) {
      printf("mkdir %s fail, code is %d", dir, mk);
      return mk;
    }
    cd = ftp_cwd(sk, dir);
    if (cd) {
      printf("cd after mk %s fail, code is %d", dir, cd);
      return cd;
    }
  }
  return 0;
}

int ftp_file_name_format(char *file_name, char *address) {
  if (NULL == address) {
    printf("address is null\n");
    return -1;
  }
  for (int i = strlen(address) - 1; i >= 0; i--) {
    if (address[i] == '/') {
      strcpy(file_name, address + i + 1);
      printf("upload file name is %s\n", file_name);
      break;
    }
  }
  if (strlen(file_name) == 0) {
    sprintf(file_name, "%s", address);
  }
  return 0;
}

int ftp_file_put(char *addr) {
  if (!init_flag) {
    printf("ftp no init\n");
    return -1;
  }
  int sk = ftp_login_func();
  if (sk == -1) {
    printf("%s: file: %s login fail\n", __FUNCTION__, addr);
    return -1;
  }
  if (g_path)
    ftp_entry_dir_step_func(sk, g_path);

  char file_name[64];
  memset(file_name, 0, sizeof(file_name));
  int sig = ftp_file_name_format(file_name, addr);
  if (sig) {
    quit_ftp(sk);
    return sig;
  }

  sig = ftp_file_upload(sk, addr, file_name, NULL, NULL);
  if (sig) {
    printf("upload file fail, address is %s, name is %s\n", addr, file_name);
    quit_ftp(sk);
    return -1;
  }
  quit_ftp(sk);
  return 0;
}

int ftp_file_put_data(void *data, int64_t size, char *addr) {
  if (!init_flag) {
    printf("ftp no init\n");
    return -1;
  }
  int sk = ftp_login_func();
  if (sk == -1) {
    printf("%s: file: %s login fail\n", __FUNCTION__, addr);
    return -1;
  }
  if (g_path)
    ftp_entry_dir_step_func(sk, g_path);
  char file_name[64];
  memset(file_name, 0, sizeof(file_name));
  int sig = ftp_file_name_format(file_name, addr);
  if (sig) {
    quit_ftp(sk);
    return sig;
  }
  int ret = ftp_file_upload_buf(sk, data, size, file_name);
  quit_ftp(sk);
  return ret;
}

int ftp_init(char *servers, int port, char *userName, char *password,
             char *save_dir) {
  g_name = userName;
  g_passwd = password;
  g_ip = servers;
  g_port = port;
  g_path = save_dir;
  init_flag = 1;
  return 0;
}

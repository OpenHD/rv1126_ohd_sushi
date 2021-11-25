#ifndef __FTP_H
#define __FTP_H

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int ftp_init(char *servers, int port, char *userName, char *password,
             char *save_dir);
int ftp_file_put(char *addr);
int ftp_file_put_data(void *data, int64_t size, char *addr);

#ifdef __cplusplus
}
#endif

#endif
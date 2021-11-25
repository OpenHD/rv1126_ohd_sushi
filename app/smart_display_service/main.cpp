#include <assert.h>
#include <atomic>
#include <thread>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "net/tcp_control.h"
#include "shmc/shm_queue.h"
#include "shmc/shm_array.h"
#include <net/shm_control.h>
#include "utils/log.h"

using namespace shmc;
using namespace std;
using namespace rndis_server;

enum {
  LOG_ERROR,
  LOG_WARN,
  LOG_INFO,
  LOG_DEBUG
};

int enable_minilog = 0;
#define LOG_TAG "smart_display_service"
int smart_display_service_log_level = LOG_INFO;
int isOut = 0;
ShmControl *control = NULL;
int app_quit = 0;

    //template <class Alloc>
    /*
void test(){
     shmc::ShmQueue<SVIPC> queue_w_;

     //Alloc alloc_;

     assert(queue_w_.InitForWrite(kShmKey1, kQueueBufSize));
     //assert(queue_r_.InitForRead(kShmKey));

    std::string src("hello");
     // Pop(void*, size_t*)
     char buf[64] = {0};
     size_t len = sizeof(buf);
     queue_w_.Push(src);
     //queue_r_.Pop(buf, &len);
     //ASSERT_EQ(src.size(), len);
     //ASSERT_EQ(src, buf);
    printf("test src = %s from %s \n",src.c_str(),buf);


  char buf2[2048];
  std::string out;

  std::string src2 ="hello";
  memset(buf2,'1',sizeof(buf2));
  printf("test push %d, %s\n",sizeof(buf2),buf2);
  for (int i = 0; i < 10; i++) {
    size_t len = rand() % sizeof(buf2);
    src2 += std::to_string(i);
    assert(queue_w_.Push(src2));
    //assert(queue_r_.Pop(&out));
     //printf("test pop %s\n",&out);
     printf("test push %s,len = %d\n",src2.c_str(),len);
    //ASSERT_EQ(src, out);
  }

}*/
static void sighandler(int signal) {
    fprintf(stdout, "Received signal %d: %s.  Shutting down.\n", signal,
            strsignal(signal));
}
int setSignalHandle()
{
	/* Set signal handlers */
	sigset_t sigset;
	struct sigaction siginfo;

	siginfo.sa_handler = sighandler;
	sigemptyset(&siginfo.sa_mask);
	siginfo.sa_flags = SA_RESTART;

	sigaction(SIGINT, &siginfo, NULL);
	sigaction(SIGTERM, &siginfo, NULL);
}

void sigterm_handler(int sig) {
  LOG_INFO("signal %d\n", sig);
  app_quit = sig;
  if (control && app_quit == SIGUSR1) {
    control->StopUvcEngine(true);
    delete control;
    control = new ShmControl();
    control->StartSendThread();
    app_quit = 0;
  }
}

int main(int argc, char **argv) {
  // Start network connection
  //test();
  //setSignalHandle();
  signal(SIGUSR1, sigterm_handler);
#ifdef ENABLE_MINILOGGER
  enable_minilog = 1;
#endif
  __minilog_log_init(argv[0], NULL, false, true, argv[0],"1.0.0");
  //Alloc alloc_;
  control = new ShmControl();
  control->StartSendThread();

  rndis_device *rndis_dev = rndis_dev_new();
  rndis_initialize_tcp(rndis_dev);
  while(TRUE) {
    sleep(1);
    if(isOut == 1)
        break;
  }
}

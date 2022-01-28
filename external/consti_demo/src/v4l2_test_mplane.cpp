/* csdn: 专题讲解
 * https://blog.csdn.net/ldl617/category_11380464.html
 */
// https://developer.ridgerun.com/wiki/index.php?title=Color_space_conversion_tools

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>
#include <chrono>
#include <iostream>
#include <sstream>
#include "consti_formats_helper.hpp"
#include <array>

// gc2053 uses MEDIA_BUS_FMT_SGRBG10_1X10
// which should be equal to V4L2_PIX_FMT_SGRBG10
//constexpr auto WANTED_PIX_FMT=MEDIA_BUS_FMT_SGRBG10_1X10;
//constexpr auto WANTED_PIX_FMT=V4L2_PIX_FMT_SGRBG10; // This one works on gc2053 !!
//constexpr auto WANTED_PIX_FMT=V4L2_PIX_FMT_SRGGB12;
auto WANTED_PIX_FMT=V4L2_PIX_FMT_SGBRG10; // I think this one works on imx415

constexpr auto N_REQUESTED_V4l2_BUFFERS=5;

struct plane_start_t {
    void * start;
};

struct buffer {
    struct plane_start_t* plane_start;
    struct v4l2_plane* planes_buffer;
};

static std::array<uint8_t,1920*1080*100> dataCopyBuffer;

int main(int argc, char **argv)
{
    int fd;
    fd_set fds;
    FILE *file_fd;
    struct timeval tv;
    int ret = -1, i, j, r;
    int num_planes;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane* planes_buffer;
    struct plane_start_t* plane_start;
    struct buffer *buffers;
    enum v4l2_buf_type type;
    int num = 0;

    const char* VIDEO_DEVICE="/dev/video0";
    int N_FRAMES=1;
    const char* OUTPUT_FILENAME="/tmp/out.raw";
    int FORMAT_ID=0;

    int option;
    while ((option = getopt (argc, argv, "i:o:n:f:")) != -1){
        switch (option){
            case 'i':
                VIDEO_DEVICE=optarg;
                break;
            case 'o':
                OUTPUT_FILENAME=optarg;
                break;
            case 'n':
                N_FRAMES=atoi(optarg);
                break;
            case 'f':
                FORMAT_ID=atoi(optarg);
                break;
            default:
                printf("Usage: v4l2_test -i <input_device> -o <output_filename> -n <n frames> -f <format id>\n");
                printf("Format id: 0=gc2053, 1=imx415\n");
                printf("example: v4l2_test -i /dev/video0 -o test.raw -n 1 -f 0\n");
                break;
        }
    }
    if(FORMAT_ID==0){
        WANTED_PIX_FMT=V4L2_PIX_FMT_SGRBG10; // This one works on gc2053 !!
        std::cout<<"GC2053 raw\n";
    }else{
        WANTED_PIX_FMT=V4L2_PIX_FMT_SGBRG10; // This one works on imx415
        std::cout<<"IMX415 raw\n";
    }

    fd = open(VIDEO_DEVICE, O_RDWR);
    if (fd < 0) {
        printf("open device: %s fail\n",VIDEO_DEVICE);
        goto err;
    }

    file_fd = fopen(OUTPUT_FILENAME, "wb+");
    if (!file_fd) {
        printf("open save_file: %s fail\n",OUTPUT_FILENAME);
        goto err1;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        printf("Get video capability error!\n");
        goto err1;
    }

    if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
        printf("Video device not support capture!\n");
        goto err1;
    }

    printf("Support capture!\n");

    memset(&fmt, 0, sizeof(struct v4l2_format));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = 1920;
    fmt.fmt.pix_mp.height = 1080;
    // gc2053:
    //fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SRGGB12;
    // from procfs.c : { "BA10", V4L2_PIX_FMT_SGRBG10},
    fmt.fmt.pix_mp.pixelformat =WANTED_PIX_FMT;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        printf("Set format fail\n");
        goto err1;
    }

    printf("width = %d\n", fmt.fmt.pix_mp.width);
    printf("height = %d\n", fmt.fmt.pix_mp.height);
    printf("numplane = %d\n", fmt.fmt.pix_mp.num_planes);
    // #define MEDIA_BUS_FMT_SGRBG10_1X10		0x300a
    // gc2053 gives 842090322 == 0x32314752
    printf("pixelformat wanted = %d %s\n", WANTED_PIX_FMT, consti10_rkcif_pixelcode_to_string(WANTED_PIX_FMT));
    printf("pixelformat actual = %d %s\n", fmt.fmt.pix_mp.pixelformat, consti10_rkcif_pixelcode_to_string(fmt.fmt.pix_mp.pixelformat));

    //memset(&fmt, 0, sizeof(struct v4l2_format));
    //fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    //if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
    //	printf("Set format fail\n");
    //	goto err;
    //}

    //printf("nmplane = %d\n", fmt.fmt.pix_mp.num_planes);

    req.count = N_REQUESTED_V4l2_BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        printf("Reqbufs fail\n");
        goto err1;
    }

    printf("buffer number: %d\n", req.count);

    num_planes = fmt.fmt.pix_mp.num_planes;

    buffers = (buffer*)malloc(req.count * sizeof(*buffers));

    for(i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        planes_buffer = (v4l2_plane*)calloc(num_planes, sizeof(*planes_buffer));
        plane_start = (plane_start_t*)calloc(num_planes, sizeof(plane_start_t));
        memset(planes_buffer, 0, sizeof(*planes_buffer));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes_buffer;
        buf.length = num_planes;
        buf.index = i;
        if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) {
            printf("Querybuf fail\n");
            req.count = i;
            goto err2;
        }

        (buffers + i)->planes_buffer = planes_buffer;
        (buffers + i)->plane_start = plane_start;
        for(j = 0; j < num_planes; j++) {
            printf("plane[%d]: length = %d\n", j, (planes_buffer + j)->length);
            printf("plane[%d]: offset = %d\n", j, (planes_buffer + j)->m.mem_offset);
            (plane_start + j)->start = mmap (NULL /* start anywhere */,
                                             (planes_buffer + j)->length,
                                             PROT_READ | PROT_WRITE /* required */,
                                             MAP_SHARED /* recommended */,
                                             fd,
                                             (planes_buffer + j)->m.mem_offset);
            if (MAP_FAILED == (plane_start +j)->start) {
                printf ("mmap failed\n");
                req.count = i;
                goto unmmap;
            }
        }
    }

    // Enqueue all the created mmaped buffers
    for (i = 0; i < req.count; ++i) {
        memset(&buf, 0, sizeof(buf));

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.length   	= num_planes;
        buf.index       = i;
        buf.m.planes 	= (buffers + i)->planes_buffer;

        if (ioctl (fd, VIDIOC_QBUF, &buf) < 0)
            printf ("VIDIOC_QBUF failed\n");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
        printf ("VIDIOC_STREAMON failed\n");

    struct v4l2_plane *tmp_plane;
    tmp_plane = (v4l2_plane*)calloc(num_planes, sizeof(*tmp_plane));

    while (1)
    {
        FD_ZERO (&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        r = select (fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r)
        {
            if (EINTR == errno)
                continue;
            printf ("select err\n");
        }
        if (0 == r)
        {
            fprintf (stderr, "select timeout\n");
            exit (EXIT_FAILURE);
        }

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = tmp_plane;
        buf.length = num_planes;
        if (ioctl (fd, VIDIOC_DQBUF, &buf) < 0)
            printf("dqbuf fail\n");

        /*const uint64_t timestampUs=(buf.timestamp.tv_sec*1000*1000)+buf.timestamp.tv_usec;
        const uint64_t latencyUs=getTimeUs()-timestampUs;
        printf("Ts: %lld Delay: %f ms\n",timestampUs,latencyUs/1000.0f);*/
        const auto timestamp=timevalToTimePointSystemClock(buf.timestamp);
        const auto delay=std::chrono::system_clock::now()-timestamp;
        std::cout<<"Buff idx:"<<buf.index<<" Delay:"<<std::chrono::duration_cast<std::chrono::microseconds>(delay).count()/1000.0f<<" ms"
        "timestamp: "<<timestamp.time_since_epoch().count()<<"\n";

        for (j = 0; j < num_planes; j++) {
            buffer* thisBuffer=&buffers[buf.index];
            //v4l2_plane* thisPlane=thisBuffer->planes_buffer;
            const uint8_t* data=(uint8_t*)((buffers + buf.index)->plane_start + j)->start;
            const int data_len=(tmp_plane + j)->bytesused;

            printf("plane[%d] start = %p, bytesused = %d\n", j, ((buffers + buf.index)->plane_start + j)->start,data_len);

            // for gc2053:
            // data len is     3317760
            // 1920*1080*12/8= 3110400
            // 1920*1080*16/8= 4147200
            // 3317760-3110400= 207360
            // when I use v4l2, I get 3317760 -> same
            // -------- hm data len changes when I use different formats ?! ------------------
            // with V4L2_PIX_FMT_SGRBG10 aka "BA10"
            // here https://git.yoctoproject.org/linux-yocto-contrib/plain/drivers/media/test-drivers/vimc/vimc-common.c
            // 2 bpp ?! ->
            //                    2764800
            // 1920*1080=         2073600
            // 1920*1080*8/(10)=  1658880
            // 1920*1080*8*3/10=  4976640
            // from https://picamera.readthedocs.io/en/release-1.12/recipes2.html

            std::stringstream ss;
            for(int i=0;i<10;i++){
                ss<<(int)data[i]<<",";
            }
            std::cout<<ss.str()<<"\n";

            const auto before=std::chrono::steady_clock::now();
            //memcpy(dataCopyBuffer.data(),data,data_len);
            const auto deltaMemcpy=std::chrono::steady_clock::now()-before;
            //std::cout<<"Buff memcpy took: "<<(std::chrono::duration_cast<std::chrono::microseconds>(delay).count()/1000.0f)<<" ms\n";

            fwrite(data,data_len, 1, file_fd);
        }

        num++;
        if(num >= N_FRAMES)
            break;

        if (ioctl (fd, VIDIOC_QBUF, &buf) < 0)
            printf("failture VIDIOC_QBUF\n");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        printf("VIDIOC_STREAMOFF fail\n");

    free(tmp_plane);

    ret = 0;
    unmmap:
    err2:
    for (i = 0; i < req.count; i++) {
        for (j = 0; j < num_planes; j++) {
            if (MAP_FAILED != ((buffers + i)->plane_start + j)->start) {
                if (-1 == munmap(((buffers + i)->plane_start + j)->start, ((buffers + i)->planes_buffer + j)->length))
                    printf ("munmap error\n");
            }
        }
    }

    for (i = 0; i < req.count; i++) {
        free((buffers + i)->planes_buffer);
        free((buffers + i)->plane_start);
    }

    free(buffers);

    fclose(file_fd);
    err1:
    close(fd);
    err:
    return ret;
}


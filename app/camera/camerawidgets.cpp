/*
    TODO review
    Examples:
    ./gst-camerabin2-test --image-width=2048 --image-height=1536
    ./gst-camerabin2-test --mode=2 --capture-time=10 --image-width=848 --image-height=480 --view-framerate-num=2825 \
    --view-framerate-den=100

    gst-camerabin2-test --help
    Usage:
    gst-camerabin2-test [OPTION...]

    camerabin command line test application.

    Help Options:
    -h, --help                        Show help options
    --help-all                        Show all help options
    --help-gst                        Show GStreamer Options

    Application Options:
    --ev-compensation                 EV compensation (-2.5..2.5, default = 0)
    --aperture                        Aperture (size of lens opening, default = 0 (auto))
    --flash-mode                      Flash mode (default = 0 (auto))
    --scene-mode                      Scene mode (default = 6 (auto))
    --exposure                        Exposure (default = 0 (auto))
    --iso-speed                       ISO speed (default = 0 (auto))
    --white-balance-mode              White balance mode (default = 0 (auto))
    --colour-tone-mode                Colour tone mode (default = 0 (auto))
    --directory                       Directory for capture file(s) (default is current directory)
    --mode                            Capture mode (default = 0 (image), 1 = video)
    --capture-time                    Time to capture video in seconds (default = 10)
    --capture-total                   Total number of captures to be done (default = 1)
    --zoom                            Zoom (100 = 1x (default), 200 = 2x etc.)
    --wrapper-source                  Camera source wrapper used for setting the video source
    --video-source                    Video source used in still capture and video recording
    --video-device                    Video device to be set on the video source (e.g. /dev/video0)
    --audio-source                    Audio source used in video recording
    --image-pp                        List of image post-processing elements separated with comma
    --viewfinder-sink                 Viewfinder sink (default = fakesink)
    --image-width                     Width for capture (only used if the caps
    arguments aren't set)
    --image-height                    Height for capture (only used if the caps
    arguments aren't set)
    --view-framerate-num              Framerate numerator for viewfinder
    --view-framerate-den              Framerate denominator for viewfinder
    --preview-caps                    Preview caps (e.g. video/x-raw-rgb,width=320,height=240)
    --viewfinder-filter               Filter to process all frames going to viewfinder sink
    --x-width                         X window width (default = 320)
    --x-height                        X window height (default = 240)
    --no-xwindow                      Do not create XWindow
    --encoding-target                 Video encoding target name
    --encoding-profile                Video encoding profile name
    --encoding-profile-filename       Video encoding profile filename
    --image-capture-caps              Image capture caps (e.g. video/x-raw-rgb,width=640,height=480)
    --viewfinder-caps                 Viewfinder caps (e.g. video/x-raw-rgb,width=640,height=480)
    --video-capture-caps              Video capture caps (e.g. video/x-raw-rgb,width=640,height=480)
    --performance-measure             Collect timing information about the
    captures and provides performance statistics at the end
    --performance-targets             A list of doubles that are the performance target
    times for each of the measured timestamps. The order is
    startup time, change mode time, shot to save, shot to snapshot,
    shot to shot, preview to precapture, shot to buffer.
    e.g. 3.5,1.0,5.0,2.5,5.0,1.5,1.0
    * Startup time -> time it takes for camerabin to reach playing
    * Change mode time -> time it takes for camerabin to change to the selected
    mode in playing
    * Shot to save -> time it takes from start-capture to having the image saved
    to disk
    * Shot to snapshot -> time it takes from start-capture to getting a snapshot
    * Shot to shot -> time from one start-capture to the next one
    * Preview to precapture -> time it takes from getting the snapshot to the
    next buffer that reaches the viewfinder
    * Shot to buffer -> time it takes from start-capture to the moment a buffer
    is pushed out of the camera source

    typedef enum {
  GST_LEVEL_NONE = 0,
  GST_LEVEL_ERROR = 1,
  GST_LEVEL_WARNING = 2,
  GST_LEVEL_FIXME = 3,
  GST_LEVEL_INFO = 4,
  GST_LEVEL_DEBUG = 5,
  GST_LEVEL_LOG = 6,
  GST_LEVEL_TRACE = 7,
  GST_LEVEL_MEMDUMP = 9,
  GST_LEVEL_COUNT
} GstDebugLevel;

  */


#include "camerawidgets.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QDateTime>
#include <gst/video/videooverlay.h>
#include <private/qgstutils_p.h>

QPushButton *cameraWidgets::m_capture;
QPushButton *cameraWidgets::m_mode;
QComboBox *cameraWidgets::m_camerasbox;
QComboBox *cameraWidgets::m_previewsizesbox;
cameraWidgets *cameraWidgets::m_cameraWidgets;
gboolean cameraWidgets::m_camera_error;
CameraTopWidgets *cameraWidgets::m_topWid;
gchar *cameraWidgets::videodevice_name;
QString cameraWidgets::m_location;
gint cameraWidgets::image_width;
gint cameraWidgets::image_height;
gfloat cameraWidgets::view_framerate_num;
gint cameraWidgets::view_framerate_den;
gchar *cameraWidgets::image_capture_caps_str;
gchar *cameraWidgets::viewfinder_caps_str;
gchar *cameraWidgets::video_capture_caps_str;
QList<camerainfo> cameraWidgets::m_caminfoList;

int cameraWidgets::m_cam_index;
int cameraWidgets::m_preview_index;

static GstEncodingProfile *
create_ts_profile (void)
{
    GstEncodingContainerProfile *container;
    GstCaps *caps = NULL;

    caps = gst_caps_new_simple ("video/mpegts", "systemstream", G_TYPE_BOOLEAN,
                                TRUE, "packetsize", G_TYPE_INT, 188, NULL);
    container = gst_encoding_container_profile_new ("ts", NULL, caps, NULL);
    gst_caps_unref (caps);

    caps = gst_caps_new_empty_simple ("video/x-h264");
    gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
                                               gst_encoding_video_profile_new (caps, NULL, NULL, 1));
    gst_caps_unref (caps);

    return (GstEncodingProfile *) container;
}
 
cameraWidgets::cameraWidgets(QWidget *parent):BaseWidget(parent)
{
    setObjectName("cameraWidgets");
    setStyleSheet("#cameraWidgets{background:rgb(10,10,10);}");
    init();
    initLayout();
}

cameraWidgets::~cameraWidgets()
{
    qDebug() << "cameraWidgets destroy";
    /* free */
    {
    GList *iter;

    for (iter = capture_times; iter; iter = g_list_next (iter)) {
        g_slice_free (CaptureTiming, iter->data);
    }
        g_list_free (capture_times);
    }

    g_string_free (filename, TRUE);
    g_free (wrappersrc_name);
    g_free (videosrc_name);
    g_free (videodevice_name);
    g_free (audiosrc_name);
    g_free (imagepp_name);
    g_free (vfsink_name);
    g_free (gep_targetname);
    g_free (gep_profilename);
    g_free (gep_filename);
    g_free (performance_targets_str);
}

int cameraWidgets::getCameraInfos(struct rk_cams_dev_info* cam_infos) {
   int ret = 0, i = 0, j = 0, fd;
   char video_dev_path[15];
   struct v4l2_capability capability;
   v4l2_input input;
   camerainfo caminfo;
   for (i = 0; i < RK_ENUM_VIDEO_NUM_MAX; i++) {
     int video_node_num;
     int video_node_input_num;
     struct rk_cam_video_node* node;
     video_dev_path[0] = 0;
     snprintf(video_dev_path, sizeof(video_dev_path), "/dev/video%d", i);
     fd = open(video_dev_path, O_RDONLY);
     if (fd < 0) {
       qDebug() << "Open " << video_dev_path << " failed! strr:" << strerror(errno);
       continue;
     }
 
     qDebug() << "open " << video_dev_path << " sucess!";
     memset(&capability, 0, sizeof(struct v4l2_capability));
     if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
       qDebug() << "Video device(" << video_dev_path << "): query capability not supported.";
       ::close(fd);
       continue;
     }
     qDebug() << "driver:" << (char*)(capability.driver);
     if (strstr((char*)(capability.driver), "rkisp")) {
       struct rk_isp_dev_info* isp_dev = &cam_infos->isp_dev;
       video_node_num = isp_dev->isp_dev_node_nums;
       //isp dev
       if ((strstr((char*)(capability.card), "self"))
           || (strstr((char*)(capability.card), "main"))
           || (strstr((char*)(capability.card), "ispdev"))
           || (strstr((char*)(capability.card), "y12"))
           || strstr((char*)(capability.card), "dma")) {
         node = &(isp_dev->video_nodes[video_node_num]);
         node->video_index = i;
         strncpy(node->card, (char*)(capability.card), strlen((char*)(capability.card)));
         qDebug() << "isp CARD " << node->card;
         for (j = 0; j < RK_CAM_ATTRACED_INUPUT_MAX; j++) {
           video_node_input_num = node->input_nums;
           input.index = j;
           if (ioctl(fd, VIDIOC_ENUMINPUT, &input) == 0) {
             node->input[video_node_input_num].index = j;
             node->input[video_node_input_num].dev = node;
             node->input[video_node_input_num].type = RK_CAM_ATTACHED_TO_ISP;
             if (strstr((char*)(input.name), "DMA"))
               strncpy(node->input[video_node_input_num].name,
                       (char*)(input.name), strlen((char*)(input.name)));
             else {
               // sensor name
               strcpy(node->input[video_node_input_num].name,
                      strtok((char*)(input.name), " "));
              //only involve once
               if (strstr((char*)(capability.card), "self")) {
                 cam_infos->cam[cam_infos->num_camers] =
                     &(node->input[video_node_input_num]);
                 cam_infos->num_camers++;
                 caminfo.video_index = i;
                 caminfo.cam = node->input[video_node_input_num];
                 m_caminfoList.append(caminfo);
               }
             }
             qDebug() << "input name " << node->input[video_node_input_num].name << ", id " << j;
           } else
             break;
           node->input_nums++;
         }
         isp_dev->isp_dev_node_nums++;
       }
    } else if (strstr((char*)(capability.driver), "cif")) {
        //cif dev
        struct rk_cif_dev_infos* cifs = &cam_infos->cif_devs;
        video_node_num = cifs->cif_dev_node_nums;
        struct rk_cam_video_node* node = &(cifs->cif_devs[video_node_num].video_node);
        cifs->cif_devs[video_node_num].cif_index = video_node_num;
        node->video_index = i;
        strncpy(node->card, (char*)(capability.card), strlen((char*)(capability.card)));
        qDebug() << "cif CARD " << node->card;
        for (j = 0; j < RK_CAM_ATTRACED_INUPUT_MAX; j++) {
            video_node_input_num = node->input_nums;
            input.index = j;
            if (ioctl(fd, VIDIOC_ENUMINPUT, &input) == 0) {
                node->input[video_node_input_num].index = j;
                node->input[video_node_input_num].dev = &(cifs->cif_devs[video_node_num]);
                node->input[video_node_input_num].type = RK_CAM_ATTACHED_TO_CIF;
                strncpy(node->input[video_node_input_num].name,
                    (char*)(input.name), strlen((char*)(input.name)));
                cam_infos->cam[cam_infos->num_camers] =
                    &(node->input[video_node_input_num]);
                cam_infos->num_camers++;
                caminfo.video_index = i;
                caminfo.cam = node->input[video_node_input_num];
                m_caminfoList.append(caminfo);
                qDebug() << "input name " << node->input[video_node_input_num].name << ",id " << j;
            } else
                break;
                node->input_nums++;
        }
        cifs->cif_dev_node_nums++;
    } else if (strstr((char*)(capability.driver), "uvc")) {
        //usb dev
        struct rk_usb_cam_dev_infos* usb_cams = &cam_infos->usb_devs;
        video_node_num = usb_cams->usb_dev_node_nums;
        struct rk_cam_video_node* node = &(usb_cams->video_nodes[video_node_num]);
        node->video_index = i;
        strncpy(node->card, (char*)(capability.card), strlen((char*)(capability.card)));
        qDebug() << "uvc CARD " << node->card;
        for (j = 0; j < RK_CAM_ATTRACED_INUPUT_MAX; j++) {
            video_node_input_num = node->input_nums;
            input.index = j;
            if (ioctl(fd, VIDIOC_ENUMINPUT, &input) == 0) {
                node->input[video_node_input_num].index = j;
                node->input[video_node_input_num].dev = node;
                node->input[video_node_input_num].type = RK_CAM_ATTACHED_TO_USB;
                strncpy(node->input[video_node_input_num].name,
                    (char*)(input.name), strlen((char*)(input.name)));
                cam_infos->cam[cam_infos->num_camers] =
                    &(node->input[video_node_input_num]);
                cam_infos->num_camers++;
                caminfo.video_index = i;
                caminfo.cam = node->input[video_node_input_num];
                m_caminfoList.append(caminfo);
                qDebug() << "input name " << node->input[video_node_input_num].name << ",id " << j;
            } else
                break;
            node->input_nums++;
        }
        usb_cams->usb_dev_node_nums++;
    } else {
        ::close(fd);
        continue;
    }
    ::close(fd);   
  }
  qDebug() << "connected  camera nums " << cam_infos->num_camers;
  for (i = 0; i < cam_infos->num_camers; i++)
    qDebug() << "cam name " << cam_infos->cam[i]->name << ",connect to controller type " << cam_infos->cam[i]->type 
    << ",input index " << cam_infos->cam[i]->index;
  return ret;                                                                                            
}

void cameraWidgets::updateSupportedViewfinderSettings() {
    m_supportedViewfinderSettings.clear();

    GstCaps *supportedCaps = this->supportedCaps(mode);

    // Convert caps to QCameraViewfinderSettings
    if (supportedCaps) {
        supportedCaps = qt_gst_caps_normalize(supportedCaps);

        for (uint i = 0; i < gst_caps_get_size(supportedCaps); i++) {
            const GstStructure *structure = gst_caps_get_structure(supportedCaps, i);

            QCameraViewfinderSettings s;
            s.setResolution(QGstUtils::structureResolution(structure));
            s.setPixelFormat(QGstUtils::structurePixelFormat(structure));
            s.setPixelAspectRatio(QGstUtils::structurePixelAspectRatio(structure));

            QPair<qreal, qreal> frameRateRange = QGstUtils::structureFrameRateRange(structure);
            s.setMinimumFrameRate(frameRateRange.first);
            s.setMaximumFrameRate(frameRateRange.second);

            if (!s.resolution().isEmpty()
                    && s.pixelFormat() != QVideoFrame::Format_Invalid
                    && !m_supportedViewfinderSettings.contains(s)) {

                m_supportedViewfinderSettings.append(s);
                qDebug() << "support ViewfinderSetting===" << "Resolution:" << s.resolution()
                    << ",PixelFormat:" << s.pixelFormat() << ",PixelAspectRatio:"
                    << s.pixelAspectRatio() << ",framerate:" << s.maximumFrameRate() << "/"
                    << s.minimumFrameRate();
            }
        }
        qDebug() << "support ViewfinderSetting size:" << m_supportedViewfinderSettings.size();

        if (m_supportedViewfinderSettings.size() == 0
                && !strcmp(m_caminfoList.at(m_cam_index).cam.name, "tc358749xbg")) {
            //hdmi in, support 1080p60fps, 720p60fps
            qDebug() << "hdmi in camera ViewfinderSettings";
            QCameraViewfinderSettings s;
            s.setResolution(1920, 1080);
            s.setPixelFormat(QVideoFrame::Format_YUYV);
            s.setPixelAspectRatio(QSize(1, 1));

            s.setMinimumFrameRate(60.0f);
            s.setMaximumFrameRate(60.0f);
            m_supportedViewfinderSettings.append(s);

            /*s.setResolution(1280, 720);
            s.setPixelFormat(QVideoFrame::Format_YUYV);
            s.setPixelAspectRatio(QSize(1, 1));

            s.setMinimumFrameRate(60.0f);
            s.setMaximumFrameRate(60.0f);
            m_supportedViewfinderSettings.append(s);*/
        } else if (m_supportedViewfinderSettings.size() == 0) {
            qDebug() << "isp camera ViewfinderSettings";
            QCameraViewfinderSettings s;
            s.setResolution(640, 480);
            s.setPixelFormat(QVideoFrame::Format_YUYV);
            s.setPixelAspectRatio(QSize(1, 1));

            s.setMinimumFrameRate(30.0f);
            s.setMaximumFrameRate(30.0f);
            m_supportedViewfinderSettings.append(s);
        }
        gst_caps_unref(supportedCaps);
    }
}

GstCaps *cameraWidgets::supportedCaps(int mode) const
{
    GstCaps *supportedCaps = 0;

    // When using wrappercamerabinsrc, get the supported caps directly from the video source element.
    // This makes sure we only get the caps actually supported by the video source element.
    if (m_videoSrc) {
        GstPad *pad = gst_element_get_static_pad(m_videoSrc, "src");
        if (pad) {
            supportedCaps = qt_gst_pad_get_caps(pad);
            gst_object_unref(GST_OBJECT(pad));
        }
    }

    // Otherwise, let the camerabin handle this.
    if (!supportedCaps) {
        const gchar *prop;
        switch (mode) {
        case MODE_IMAGE:
            prop = SUPPORTED_IMAGE_CAPTURE_CAPS_PROPERTY;
            break;
        case MODE_VIDEO:
            prop = SUPPORTED_VIDEO_CAPTURE_CAPS_PROPERTY;
            break;
        case MODE_VIEWFINDER:
        default:
            prop = SUPPORTED_VIEWFINDER_CAPS_PROPERTY;
            break;
        }

        g_object_get(G_OBJECT(camerabin), prop, &supportedCaps, NULL);
    }

    return supportedCaps;
}

bool compare(const camerainfo info0, const camerainfo info1) {
    if (info0.cam.type > info1.cam.type) {
        return true;
    } if (info0.cam.type == info1.cam.type) {
        if (info0.video_index > info1.video_index)
            return false;
        else
            return true;
    } else {
        return false;
    }
}

void cameraWidgets::showEvent(QShowEvent *event) {
    qDebug() << "cameraWidgets::showEvent";
    m_topWid->setTitle(tr("opening camera(%1)").arg(videodevice_name));
    m_caminfoList.clear();

    mode = MODE_IMAGE;
    updateCamerabarStatus(false);
    memset(&g_cam_infos, 0, sizeof(g_cam_infos));
    getCameraInfos(&g_cam_infos);
    qSort(m_caminfoList.begin(), m_caminfoList.end(), compare);
    updateCamerasBox();

    if (m_caminfoList.size() == 0) {
        m_topWid->m_btnreturn->setEnabled(true);
        return;
    }

    OpenCameraThread *workerThread = new OpenCameraThread(this);
    connect(workerThread, SIGNAL(resultReady(QString)), this, SLOT(handleResults(QString)));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
    workerThread->start();
}

void cameraWidgets::hideEvent(QHideEvent *event) {
    qDebug() << "cameraWidgets::hideEvent";
}
void cameraWidgets::openCamera()
{
    qDebug() << "openCamera start";
    
    gst_debug_set_default_threshold(GST_LEVEL_FIXME);
    gst_debug_set_threshold_for_name ("camerabin", GST_LEVEL_FIXME);
    gst_debug_set_threshold_for_name ("kmssink", GST_LEVEL_FIXME);

    recording = FALSE;
    ready_for_capture = FALSE;
    m_camera_error = FALSE;

    if (setup_pipeline()) {
        g_idle_add ((GSourceFunc) run_preview_pipeline, NULL);
    }
    qDebug() << "openCamera end";
}

void cameraWidgets::closeCamera()
{
    cleanup_pipeline ();
    m_preview_index = 0;
    m_cam_index = 0;
    updateCamerabarStatus(true);
    stop_timer_count();
}

void cameraWidgets::uevent(const char *action, const char *dev)
{
     if(strcmp(action,"remove")==0 && videodevice_name && dev)
     {
         ///dev/video%d
         int len = 6;
         char devPath[len+1];
         memset(devPath, 0x0, sizeof(devPath));
         strncpy(devPath, videodevice_name+(strlen(videodevice_name)-len), len);
         if(strstr(dev, devPath)){
             qDebug() << "camera uevent" << action<< devPath;
             cleanup_pipeline ();
             m_camera_error = true;
             ready_for_capture = false;
             if(m_cameraWidgets){
                  emit m_cameraWidgets->sig_camera_error("dev disconnect");
             }
         }
    }
}

void cameraWidgets::slot_pressed()
{
    qDebug() << "mode btn pressed";
    m_mode->setText(tr("Switching Mode"));
}

void cameraWidgets::slot_released()
{
    qDebug() << "capture btn released";
}

void cameraWidgets::slot_capture()
{
    qDebug() << "capture btn clicked";
    if (mode == MODE_IMAGE || (mode == MODE_VIDEO && !recording)) {
        if (camerabin && ready_for_capture) {
            qDebug() << "start " << (mode == MODE_IMAGE ? "capture" : "recorder");
            m_topWid->setTitle(mode == MODE_IMAGE ? "capturing" : "recording");
            updateCamerabarStatus(false);
            g_idle_add ((GSourceFunc) run_taking_pipeline, NULL);                 
        }
    } else {
        qDebug() << "stop recorder";
        if (camerabin && recording) {
            stop_timer_count();
            updateCamerabarStatus(false);
            stop_capture(NULL);
        }
    }
}

void cameraWidgets::slot_modeswitch()
{
    qDebug() << "mode switch btn clicked";
    switchMode();
}

void cameraWidgets::updateRecordTime()
{
    m_recorder_time++;
    time_t t;
    struct tm *p;
    t = m_recorder_time;
    p = gmtime(&t);
    char s[10];
    strftime(s, sizeof(s), "%H:%M:%S", p);
    qDebug() << "updateRecordTime:" << s;

    m_record_time->setText(s);
}

void cameraWidgets::handleResults(const QString &) {
    qDebug() << "Thread run over";
}

void cameraWidgets::slot_camera_error (QString error)
{
    qDebug() << "slot_camera_error:" << error;

    m_topWid->setTitle(QString("camera error:%1").arg(error));
    updateCamerabarStatus(false);
    m_topWid->m_btnreturn->setEnabled(true);
    stop_timer_count();
}

void cameraWidgets::slot_capture_done(QString location) {
    qDebug() << "slot_capture_done:" << location;
    m_topWid->setTitle(tr("capture %1").arg(location));
    updateCamerabarStatus(true);
    if (mode == MODE_VIDEO && ready_for_capture) {
        recording = FALSE;
        m_capture->setText(tr("start Recorder"));
    }
}

void cameraWidgets::slot_camera_changed(int index) {
    qDebug() << "camera changed index:" << index;
    if (index >= 0 && index != m_cam_index) {
        cleanup_pipeline();
        m_preview_index = 0;
        m_cam_index = index;
        mode = MODE_IMAGE;
        updateCamerabarStatus(true);
        openCamera();
    }
}

void cameraWidgets::slot_previewsize_changed(int index) {
    qDebug() << "previewsize changed index:" << index;
    if (index >= 0 && index != m_preview_index) {
        if (/*!strcmp(m_caminfoList.at(m_cam_index).cam.name, "tc358749xbg")*/
                m_caminfoList.at(m_cam_index).cam.type == RK_CAM_ATTACHED_TO_ISP) {
            cleanup_pipeline();
            m_preview_index = index;
            openCamera();
        } else {
            m_preview_index = index;
            updateCameraParameter();
            g_idle_add ((GSourceFunc) run_preview_pipeline, NULL);
        }
    }
}

GstPadProbeReturn
cameraWidgets::camera_src_get_timestamp_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer udata)
{
  /*CaptureTiming *timing;

  timing = (CaptureTiming *) g_list_first (capture_times)->data;
  timing->camera_capture = gst_util_get_timestamp ();*/

  qDebug() << "camera_src_get_timestamp_probe";

  return GST_PAD_PROBE_REMOVE;
}

GstPadProbeReturn
cameraWidgets::viewfinder_get_timestamp_probe (GstPad * pad, GstPadProbeInfo * info,
    gpointer udata)
{
  CaptureTiming *timing;

  timing = (CaptureTiming *) g_list_first (capture_times)->data;
  timing->precapture = gst_util_get_timestamp ();

  qDebug() << "viewfinder_get_timestamp_probe";

  return GST_PAD_PROBE_REMOVE;
}

GstBusSyncReply
cameraWidgets::sync_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  const GstStructure *st;
  const GValue *image;
  GstBuffer *buf = NULL;
  gchar *preview_filename = NULL;
  FILE *f = NULL;
  size_t written;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ELEMENT:{
      st = gst_message_get_structure (message);
      if (st) {
         //if (!gst_structure_has_name (st, "preview-image"))
         //   qDebug() << "gst_structure:" << gst_structure_to_string(st);
         if (gst_message_has_name (message, "prepare-window-handle")) {
            GST_FIXME ("prepare-window-handle");
         } else if (gst_structure_has_name (st, "image-done")) {
            const gchar *filename = gst_structure_get_string(st, "filename");
            qDebug() << "image-done:" << QString("%1").arg(filename);
            emit m_cameraWidgets->sig_capture_done(QString("%1").arg(filename));
         } else if (gst_structure_has_name (st, "video-done")) {
             qDebug() << "video-done:" << m_location;
             emit m_cameraWidgets->sig_capture_done(m_location);
         } else if (gst_structure_has_name (st, "preview-image")) {
            CaptureTiming *timing;

            GST_FIXME ("preview-image");

            timing = (CaptureTiming *) g_list_first (capture_times)->data;
            timing->got_preview = gst_util_get_timestamp ();

            {
            /* set up probe to check when the viewfinder gets data */
            GstPad *pad = gst_element_get_static_pad (viewfinder_sink, "sink");

            viewfinder_probe_id =
                gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
                viewfinder_get_timestamp_probe, NULL, NULL);

            gst_object_unref (pad);
            }

            /* extract preview-image from msg */
            image = gst_structure_get_value (st, "buffer");
            qDebug() << "gst_structure_get_value image:" << image;
            if (image) {
                buf = gst_value_get_buffer (image);
                preview_filename = g_strdup_printf ("/mnt/sdcard/test_vga.rgb");
                f = g_fopen (preview_filename, "w");
                qDebug() << "preview_filename open:" << f; 
                if (f) {
                  GstMapInfo map;

                  gst_buffer_map (buf, &map, GST_MAP_READ);
                  written = fwrite (map.data, map.size, 1, f);
                  gst_buffer_unmap (buf, &map);
                  if (!written) {
                    g_print ("error writing file\n");
                  }
                  fclose (f);
                } else {
                  g_print ("error opening file for raw image writing\n");
                }
                g_free (preview_filename);
            }
        }
      }
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
      if (GST_MESSAGE_SRC (message) == (GstObject *) camerabin) {
        GstState newstate;

        gst_message_parse_state_changed (message, NULL, &newstate, NULL);
        if (newstate == GST_STATE_PLAYING) {
          startup_time = gst_util_get_timestamp ();
        }
      }
      break;
    case GST_MESSAGE_ERROR:{
      GST_FIXME ("GST_MESSAGE_ERROR in sync_bus_callback");
      GError *err = NULL;
      gchar *dbg_info = NULL;

      gst_message_parse_error (message, &err, &dbg_info);
      g_printerr ("ERROR from element %s: %s\n",
           GST_OBJECT_NAME (message->src), err->message);
      g_printerr ("Debugging info: %s\n", (dbg_info) ? dbg_info : "none");

      m_camera_error = true;
      ready_for_capture = false;
      if(m_cameraWidgets){
        emit m_cameraWidgets->sig_camera_error(QString("%1").arg(err->message));
      }
      g_error_free (err);
      g_free (dbg_info);
    }
    default:
      /* unhandled message */
      break;
  }
  return GST_BUS_PASS;
}

gboolean cameraWidgets::bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  qDebug() << "bus_callback:" << GST_MESSAGE_SRC_NAME (message);
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_clear_error (&err);
      g_free (debug);

      /* Write debug graph to file */
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (GST_BIN (camerabin),
          GST_DEBUG_GRAPH_SHOW_ALL, "camerabin.error");

      //g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
      if (GST_IS_BIN (GST_MESSAGE_SRC (message))) {
        GstState oldstate, newstate;

        gst_message_parse_state_changed (message, &oldstate, &newstate, NULL);
        GST_FIXME_OBJECT (GST_MESSAGE_SRC (message), "state-changed: %s -> %s",
            gst_element_state_get_name (oldstate),
            gst_element_state_get_name (newstate));
      }
      break;
    case GST_MESSAGE_EOS:
      /* end-of-stream */
      GST_FIXME ("got eos() - should not happen");
      //g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_ELEMENT:
      if (GST_MESSAGE_SRC (message) == (GstObject *) camerabin) {
        const GstStructure *structure = gst_message_get_structure (message);

        if (gst_structure_has_name (structure, "image-done")) {
          CaptureTiming *timing;
#ifndef GST_DISABLE_GST_DEBUG
          const gchar *fname = gst_structure_get_string (structure, "filename");

          GST_FIXME ("image done: %s", fname);
#endif
          timing = (CaptureTiming *) g_list_first (capture_times)->data;
          timing->capture_done = gst_util_get_timestamp ();

          if (capture_count < capture_total) {
            g_idle_add ((GSourceFunc) run_preview_pipeline, NULL);
          } else {
            //g_main_loop_quit (loop);
          }
        }
      }
      break;
    default:
      /* unhandled message */
      break;
  }
  return TRUE;
}

void cameraWidgets::switchMode() {
    //cleanup_pipeline();
    mode = mode == MODE_IMAGE ? MODE_VIDEO : MODE_IMAGE;
    m_topWid->setTitle(QString("switch to %1").arg(mode == MODE_IMAGE ? "VIDEO MODE" : "IMAGE MODE"));
    updateCamerabarStatus(false);

    g_idle_add ((GSourceFunc) run_preview_pipeline, NULL);
}

void cameraWidgets::cleanup_pipeline (void)
{
  if (camerabin) {
    GST_FIXME_OBJECT (camerabin, "stopping and destroying");
    GstElement *camera_source = NULL;
    g_object_get (camerabin, "camera-source", &camera_source, NULL);
    if (camera_source)
        g_signal_handler_disconnect (camera_source, ready_for_capture_id);
    gst_element_set_state (camerabin, GST_STATE_NULL);
    gst_object_unref (camerabin);
    camerabin = NULL;
  }
  if (viewfinder_sink)
     gst_object_unref(viewfinder_sink);
}

GstElement *
cameraWidgets::create_ipp_bin (void)
{
  GstElement *bin = NULL, *element = NULL;
  GstPad *pad = NULL;
  gchar **elements;
  GList *element_list = NULL, *current = NULL, *next = NULL;
  int i;

  bin = gst_bin_new ("ippbin");

  elements = g_strsplit (imagepp_name, ",", 0);

  for (i = 0; elements[i] != NULL; i++) {
    element = gst_element_factory_make (elements[i], NULL);
    if (element) {
      element_list = g_list_append (element_list, element);
      gst_bin_add (GST_BIN (bin), element);
    } else
      GST_WARNING ("Could create element %s for ippbin", elements[i]);
  }

  for (i = 1; i < g_list_length (element_list); i++) {
    current = g_list_nth (element_list, i - 1);
    next = g_list_nth (element_list, i);
    gst_element_link ((GstElement * ) current->data, (GstElement * ) next->data);
  }

  current = g_list_first (element_list);
  pad = gst_element_get_static_pad ((GstElement * ) current->data, "sink");
  gst_element_add_pad (bin, gst_ghost_pad_new ("sink", pad));
  gst_object_unref (GST_OBJECT (pad));

  current = g_list_last (element_list);
  pad = gst_element_get_static_pad ((GstElement * ) current->data, "src");
  gst_element_add_pad (bin, gst_ghost_pad_new ("src", pad));
  gst_object_unref (GST_OBJECT (pad));

  g_list_free (element_list);
  g_strfreev (elements);

  return bin;
}

GstEncodingProfile *
cameraWidgets::load_encoding_profile (void)
{
  GstEncodingProfile *prof = NULL;
  GstEncodingTarget *target = NULL;
  GError *error = NULL;

  /* if profile file was given, try to load profile from there */
  if (gep_filename && gep_profilename) {
    target = gst_encoding_target_load_from_file (gep_filename, &error);
    if (!target) {
      GST_WARNING ("Could not load target %s from file %s", gep_targetname,
          gep_filename);
      if (error) {
        GST_WARNING ("Error from file loading: %s", error->message);
        g_clear_error (&error);
      }
    } else {
      prof = gst_encoding_target_get_profile (target, gep_profilename);
      if (prof)
        GST_FIXME ("Loaded encoding profile %s from %s", gep_profilename,
            gep_filename);
      else
        GST_WARNING
            ("Could not load specified encoding profile %s from file %s",
            gep_profilename, gep_filename);
    }
    /* if we could not load profile from file then try to find one from system */
  } else if (gep_profilename && gep_targetname) {
    prof = gst_encoding_profile_find (gep_targetname, gep_profilename, NULL);
    if (prof)
      GST_FIXME ("Loaded encoding profile %s from target %s", gep_profilename,
          gep_targetname);
  } else
    GST_FIXME
        ("Encoding profile not set, using camerabin default encoding profile");

  return prof;
}

gboolean
cameraWidgets::setup_pipeline_element (GstElement * element, const gchar * property_name,
    const gchar * element_name, GstElement ** res_elem)
{
  gboolean res = TRUE;
  GstElement *elem = NULL;

  if (element_name) {
    GError *error = NULL;

    elem = gst_parse_launch (element_name, &error);
    if (elem) {
      g_object_set (element, property_name, elem, NULL);
      g_object_unref (elem);
    } else {
      GST_WARNING ("can't create element '%s' for property '%s'", element_name,
          property_name);
      if (error) {
        GST_ERROR ("%s", error->message);
        g_clear_error (&error);
      }
      res = FALSE;
    }
  } else {
    GST_FIXME ("no element for property '%s' given", property_name);
  }
  if (res_elem)
    *res_elem = elem;
  return res;
}


void cameraWidgets::set_camerabin_caps_from_string (void)
{
  GstCaps *caps = NULL;
  if (image_capture_caps_str != NULL) {
    caps = gst_caps_from_string (image_capture_caps_str);
    if (GST_CAPS_IS_SIMPLE (caps) && image_width > 0 && image_height > 0) {
      gst_caps_set_simple (caps, "width", G_TYPE_INT, image_width, "height",
          G_TYPE_INT, image_height, NULL);
    }
    GST_FIXME ("setting image-capture-caps: %" GST_PTR_FORMAT, caps);
    g_object_set (camerabin, "image-capture-caps", caps, NULL);
    gst_caps_unref (caps);
  }

  if (viewfinder_caps_str != NULL) {
    caps = gst_caps_from_string (viewfinder_caps_str);
    if (GST_CAPS_IS_SIMPLE (caps) && view_framerate_num > 0
        && view_framerate_den > 0) {
      gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION,
          view_framerate_num, view_framerate_den, NULL);
    }
    GST_FIXME ("setting viewfinder-caps: %" GST_PTR_FORMAT, caps);
    g_object_set (camerabin, "viewfinder-caps", caps, NULL);
    gst_caps_unref (caps);
  }

  if (video_capture_caps_str != NULL) {
    caps = gst_caps_from_string (video_capture_caps_str);
    GST_FIXME ("setting video-capture-caps: %" GST_PTR_FORMAT, caps);
    g_object_set (camerabin, "video-capture-caps", caps, NULL);
    gst_caps_unref (caps);
  }

}

gboolean cameraWidgets::setup_pipeline (void)
{
  gboolean res = TRUE;
  GstBus *bus;
  GstElement *sink = NULL, *ipp = NULL;
  GstEncodingProfile *prof = NULL;
  GIOChannel *key_control = NULL;
  GstElement *camera_source = NULL;
  gboolean idle = FALSE;

  initial_time = gst_util_get_timestamp ();

  camerabin = gst_element_factory_make ("camerabin", NULL);
  if (NULL == camerabin) {
    g_warning ("can't create camerabin element\n");
    goto error;
  }

  bus = gst_pipeline_get_bus (GST_PIPELINE (camerabin));
  /* Add sync handler for time critical messages that need to be handled fast */
  gst_bus_set_sync_handler (bus, sync_bus_callback, NULL, NULL);
  /* Handle normal messages asynchronously */
  gst_bus_add_watch (bus, bus_callback, NULL);
  gst_object_unref (bus);

  GST_FIXME_OBJECT (camerabin, "camerabin created");

  if (camerabin_flags)
    gst_util_set_object_arg (G_OBJECT (camerabin), "flags", camerabin_flags);
  else
    gst_util_set_object_arg (G_OBJECT (camerabin), "flags", "");

  if (videosrc_name) {
    GstElement *wrapper;

    if (wrappersrc_name)
      wrapper = gst_element_factory_make (wrappersrc_name, NULL);
    else
      wrapper = gst_element_factory_make ("wrappercamerabinsrc", NULL);

    if (setup_pipeline_element (wrapper, "video-source", videosrc_name, NULL)) {
      g_object_set (camerabin, "camera-source", wrapper, NULL);
      g_object_unref (wrapper);
    } else {
      GST_WARNING ("Failed to set videosrc to %s", videosrc_name);
    }

    g_object_get (wrapper, "video-source", &m_videoSrc, NULL);
    videodevice_name = g_strdup_printf ("/dev/video%d", m_caminfoList.at(m_cam_index).video_index);
    if (videodevice_name == NULL)
      videodevice_name = g_strdup("/dev/video0");
    g_warning ("opening %s\n", videodevice_name);
    if (m_videoSrc && videodevice_name &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (m_videoSrc),
            "device")) {
      g_object_set (m_videoSrc, "device", videodevice_name, NULL);
    }
  }

  /* configure used elements */
  res &=
      setup_pipeline_element (camerabin, "audio-source", audiosrc_name, NULL);
  res &=
      setup_pipeline_element (camerabin, "viewfinder-sink", vfsink_name, &sink);
  res &=
      setup_pipeline_element (camerabin, "viewfinder-filter", viewfinder_filter,
      NULL);
  
  if (imagepp_name) {
    ipp = create_ipp_bin ();
    if (ipp) {
      g_object_set (camerabin, "image-filter", ipp, NULL);
      g_object_unref (ipp);
    } else
      GST_WARNING ("Could not create ipp elements");
  }

  prof = create_ts_profile();
  if (prof) {
    g_object_set (G_OBJECT (camerabin), "video-profile", prof, NULL);
    gst_encoding_profile_unref (prof);
  }

  GST_FIXME_OBJECT (camerabin, "elements created");

  if (sink) {
    g_object_set (sink, "sync", FALSE, NULL);
    gst_object_ref_sink(sink);
  } else {
    /* Get the inner viewfinder sink, this uses fixed names given
     * by default in camerabin */
    sink = gst_bin_get_by_name (GST_BIN (camerabin), "vf-bin");
    g_assert (sink);
    gst_object_unref (sink);

    sink = gst_bin_get_by_name (GST_BIN (sink), "vfbin-sink");
    g_assert (sink);
    gst_object_unref (sink);
  }
  viewfinder_sink = sink;

  GST_FIXME_OBJECT (camerabin, "elements configured");

  if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (camerabin, GST_STATE_READY)) {
    g_warning ("can't set camerabin to ready\n");
    emit m_cameraWidgets->sig_camera_error(QString("can't set camerabin to ready"));
    goto error;
  }
  GST_FIXME_OBJECT (camerabin, "camera ready");

  if (/*!strcmp(m_caminfoList.at(m_cam_index).cam.name, "tc358749xbg")*/
          m_caminfoList.at(m_cam_index).cam.type == RK_CAM_ATTACHED_TO_ISP) {
      qDebug() << "hdmi in need setparameter before camerabin playing";
      updateSupportedViewfinderSettings();
      updateCameraParameter();
      if (image_width > 0 && image_height > 0) {
          if (mode == MODE_VIDEO) {
            GstCaps *caps = NULL;
            if (view_framerate_num > 0)
              caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                      "width", G_TYPE_INT, image_width,
                      "height", G_TYPE_INT, image_height,
                      "framerate", GST_TYPE_FRACTION, view_framerate_num,
                      view_framerate_den, NULL), NULL);
            else
              caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                      "width", G_TYPE_INT, image_width,
                      "height", G_TYPE_INT, image_height, NULL), NULL);

            g_object_set (camerabin, "video-capture-caps", caps, NULL);
            gst_caps_unref (caps);
          } else {
            GstCaps *caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                    "width", G_TYPE_INT, image_width,
                    "height", G_TYPE_INT, image_height, NULL), NULL);

            g_object_set (camerabin, "image-capture-caps", caps, NULL);
            gst_caps_unref (caps);
          }
      }

      set_camerabin_caps_from_string ();
  }

  if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (camerabin, GST_STATE_PLAYING)) {
    g_warning ("can't set camerabin to playing\n");
    emit m_cameraWidgets->sig_camera_error(QString("can't set camerabin to playing"));
    goto error;
  }

  GST_FIXME_OBJECT (camerabin, "camera started");

  g_object_get (camerabin, "idle", &idle, NULL);

  qDebug() << "camera idle:" << idle;

  updateSupportedViewfinderSettings();
  updatePreviewSizesBox();
  updateCameraParameter();

  /* configure a resolution and framerate */
  /*if (image_width > 0 && image_height > 0) {
    if (mode == MODE_VIDEO) {
      GstCaps *caps = NULL;
      if (view_framerate_num > 0)
        caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                "width", G_TYPE_INT, image_width,
                "height", G_TYPE_INT, image_height,
                "framerate", GST_TYPE_FRACTION, view_framerate_num,
                view_framerate_den, NULL), NULL);
      else
        caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                "width", G_TYPE_INT, image_width,
                "height", G_TYPE_INT, image_height, NULL), NULL);

      g_object_set (camerabin, "video-capture-caps", caps, NULL);
      gst_caps_unref (caps);
    } else {
      GstCaps *caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
              "width", G_TYPE_INT, image_width,
              "height", G_TYPE_INT, image_height, NULL), NULL);

      g_object_set (camerabin, "image-capture-caps", caps, NULL);
      gst_caps_unref (caps);
    }
  }

  set_camerabin_caps_from_string ();*/

  /*if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (camerabin, GST_STATE_READY)) {
    g_warning ("can't set camerabin to ready\n");
    emit m_cameraWidgets->sig_camera_error(QString("can't set camerabin to ready"));
    goto error;
  }
  GST_FIXME_OBJECT (camerabin, "camera ready");

  if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (camerabin, GST_STATE_PLAYING)) {
    g_warning ("can't set camerabin to playing\n");
    emit m_cameraWidgets->sig_camera_error(QString("can't set camerabin to playing"));
    goto error;
  }

  GST_FIXME_OBJECT (camerabin, "camera started");

  g_object_get (camerabin, "idle", &idle, NULL);
  
  qDebug() << "camera idle:" << idle;*/

  /*g_object_get (camerabin, "camera-source", &camera_source, NULL);
  if (camera_source) {
    ready_for_capture_id = g_signal_connect(camera_source, "notify::ready-for-capture",
      (GCallback) notify_readyforcapture, camera_source);
    g_object_get (camera_source, "ready-for-capture", &ready_for_capture, NULL);
    qDebug() << "ready_for_capture_id:" << ready_for_capture_id << ",ready for captrue:" << ready_for_capture;
  }*/

  return TRUE;
error:
  cleanup_pipeline ();
  return FALSE;
}

gboolean
cameraWidgets::stop_capture (gpointer user_data)
{
  g_signal_emit_by_name (camerabin, "stop-capture", 0);
  return FALSE;
}

void cameraWidgets::notify_readyforcapture(GObject * obj,GParamSpec * pspec,gpointer user_data) {
    g_object_get (obj, "ready-for-capture", &ready_for_capture, NULL);
        qDebug() << "notify_readyforcapture:" << ready_for_capture;
}

void cameraWidgets::set_metadata (GstElement * camera)
{
  GstTagSetter *setter = GST_TAG_SETTER (camera);
  GstDateTime *datetime;
  gchar *desc_str;

  datetime = gst_date_time_new_now_local_time ();

  desc_str = g_strdup_printf ("captured by %s", g_get_real_name ());

  gst_tag_setter_add_tags (setter, GST_TAG_MERGE_REPLACE,
      GST_TAG_DATE_TIME, datetime,
      GST_TAG_DESCRIPTION, desc_str,
      GST_TAG_TITLE, "rockchip camera capture",
      GST_TAG_GEO_LOCATION_LONGITUDE, 1.0,
      GST_TAG_GEO_LOCATION_LATITUDE, 2.0,
      GST_TAG_GEO_LOCATION_ELEVATION, 3.0,
      GST_TAG_DEVICE_MANUFACTURER, "gst-camerabin-test manufacturer",
      GST_TAG_DEVICE_MODEL, "gst-camerabin-test model", NULL);

  g_free (desc_str);
  gst_date_time_unref (datetime);
}

gboolean cameraWidgets::run_taking_pipeline (gpointer user_data)
{
  gchar *filename_str = NULL;
  GstElement *video_source = NULL;
  const gchar *filename_suffix;
  CaptureTiming *timing;

    /* Construct filename */
  if (mode == MODE_VIDEO)
    filename_suffix = ".ts";
  else
    filename_suffix = ".jpg";
  QDateTime current_date_time = QDateTime::currentDateTime();
  QString current_date = current_date_time.toString("yyyyMMddhhmmss");
  qDebug() << current_date.toStdString().c_str();
  filename_str =
      g_strdup_printf ("%s/cam_%s%s", filename->str, current_date.toStdString().c_str(),
      filename_suffix);
  GST_FIXME ("Setting filename: %s", filename_str);
  m_location = QString("%1").arg(filename_str);
  g_object_set (camerabin, "location", filename_str, NULL);
  g_free (filename_str);

  g_object_get (camerabin, "camera-source", &video_source, NULL);
  if (video_source) {
    if (GST_IS_ELEMENT (video_source) && GST_IS_PHOTOGRAPHY (video_source)) {
      /* Set GstPhotography interface options. If option not given as
         command-line parameter use default of the source element. */
      if (scene_mode != SCENE_MODE_NONE)
        g_object_set (video_source, "scene-mode", scene_mode, NULL);
      if (ev_compensation != EV_COMPENSATION_NONE)
        g_object_set (video_source, "ev-compensation", ev_compensation, NULL);
      if (aperture != APERTURE_NONE)
        g_object_set (video_source, "aperture", aperture, NULL);
      if (flash_mode != FLASH_MODE_NONE)
        g_object_set (video_source, "flash-mode", flash_mode, NULL);
      if (exposure != EXPOSURE_NONE)
        g_object_set (video_source, "exposure", exposure, NULL);
      if (iso_speed != ISO_SPEED_NONE)
        g_object_set (video_source, "iso-speed", iso_speed, NULL);
      if (wb_mode != WHITE_BALANCE_MODE_NONE)
        g_object_set (video_source, "white-balance-mode", wb_mode, NULL);
      if (color_mode != COLOR_TONE_MODE_NONE)
        g_object_set (video_source, "colour-tone-mode", color_mode, NULL);
    }
    g_object_unref (video_source);
  } else {
    video_source = gst_bin_get_by_name (GST_BIN (camerabin), "camerasrc");
    gst_object_unref (video_source);
  }
  g_object_set (camerabin, "zoom", zoom / 100.0f, NULL);

  capture_count++;

  //timing = g_slice_new0 (CaptureTiming);
  //capture_times = g_list_prepend (capture_times, timing);

  /* set pad probe to check when buffer leaves the camera source */
  if (mode == MODE_IMAGE) {
    GstPad *pad;

    pad = gst_element_get_static_pad (video_source, "imgsrc");
    camera_probe_id = gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
        camera_src_get_timestamp_probe, NULL, NULL);

    gst_object_unref (pad);
  }
  //timing->start_capture = gst_util_get_timestamp ();
  g_signal_emit_by_name (camerabin, "start-capture", 0);

  if (mode == MODE_VIDEO) {
    qDebug() << "update recorder ui";
    recording = TRUE;
    m_capture->setEnabled(true);
    m_capture->setText(tr("stop Recorder"));
    m_cameraWidgets->start_timer_count();
  }


  return FALSE;
}

gboolean cameraWidgets::run_preview_pipeline (gpointer user_data)
{
  qDebug() << "run_preview_pipeline";
  GstElement *camera_source = NULL;
  if (/*strcmp(m_caminfoList.at(m_cam_index).cam.name, "tc358749xbg")*/
          m_caminfoList.at(m_cam_index).cam.type != RK_CAM_ATTACHED_TO_ISP) {
      /* configure a resolution and framerate */
      if (image_width > 0 && image_height > 0) {
        if (mode == MODE_VIDEO) {
          GstCaps *caps = NULL;
          if (view_framerate_num > 0)
            caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                    "width", G_TYPE_INT, image_width,
                    "height", G_TYPE_INT, image_height,
                    "framerate", GST_TYPE_FRACTION, view_framerate_num,
                    view_framerate_den, NULL), NULL);
          else
            caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                    "width", G_TYPE_INT, image_width,
                    "height", G_TYPE_INT, image_height, NULL), NULL);

          g_object_set (camerabin, "video-capture-caps", caps, NULL);
          gst_caps_unref (caps);
        } else {
          GstCaps *caps = gst_caps_new_full (gst_structure_new ("video/x-raw",
                  "width", G_TYPE_INT, image_width,
                  "height", G_TYPE_INT, image_height, NULL), NULL);

          g_object_set (camerabin, "image-capture-caps", caps, NULL);
          gst_caps_unref (caps);
        }
      }

      set_camerabin_caps_from_string ();
  }

  g_object_get (camerabin, "camera-source", &camera_source, NULL);
  if (camera_source) {
    g_signal_handler_disconnect (camera_source, ready_for_capture_id);
    ready_for_capture_id = g_signal_connect(camera_source, "notify::ready-for-capture",
      (GCallback) notify_readyforcapture, camera_source);
    g_object_get (camera_source, "ready-for-capture", &ready_for_capture, NULL);
    qDebug() << "ready_for_capture_id:" << ready_for_capture_id << ",ready for captrue:" << ready_for_capture;
  }

  GstCaps *preview_caps = NULL;

  g_object_set (camerabin, "mode", mode, NULL);

  if (preview_caps_name != NULL) {
    preview_caps = gst_caps_from_string (preview_caps_name);
    if (preview_caps) {
      g_object_set (camerabin, "post-previews", FALSE, NULL);
      g_object_set (camerabin, "preview-caps", preview_caps, NULL);
      GST_FIXME ("Preview caps set");
    } else
      GST_FIXME ("Preview caps set but could not create caps from string");
  }

  set_metadata (camerabin);

  GST_FIXME ("ready_for_capture=%d, m_camera_error=%d", ready_for_capture, m_camera_error);
  if(ready_for_capture && !m_camera_error){
    updateCamerabarStatus(true);
    m_topWid->setTitle(tr("Camera(%1) open success").arg(videodevice_name));
  } else {
    updateCamerabarStatus(false);
    m_topWid->setTitle(tr("Camera(%1) open fail").arg(videodevice_name));
  }
    if(mode == MODE_IMAGE){
        m_capture->setText( tr("Take Picture"));
    }else{
        m_capture->setText( tr("Start Recorder"));
    }
    if(mode == MODE_IMAGE){
        m_mode->setText(tr("Capture Image Mode"));
    }else{
        m_mode->setText(tr("Video Recorder Mode"));
    }


  return FALSE;
}

void
cameraWidgets::parse_target_values (void)
{
  gdouble startup = 0, change_mode = 0, shot_to_save = 0, shot_to_snapshot = 0;
  gdouble shot_to_shot = 0, preview_to_precapture = 0, shot_to_buffer = 0;

  if (performance_targets_str == NULL)
    return;

  /*
     startup time, change mode time, shot to save, shot to snapshot,
     shot to shot, preview to precapture, shot to buffer.
   */
  sscanf (performance_targets_str, "%lf,%lf,%lf,%lf,%lf,%lf,%lf",
      &startup, &change_mode, &shot_to_save,
      &shot_to_snapshot, &shot_to_shot, &preview_to_precapture,
      &shot_to_buffer);

  target_startup = (GstClockTime) (startup * GST_SECOND);
  target_change_mode = (GstClockTime) (change_mode * GST_SECOND);
  target_shot_to_save = (GstClockTime) (shot_to_save * GST_SECOND);
  target_shot_to_snapshot = (GstClockTime) (shot_to_snapshot * GST_SECOND);
  target_shot_to_shot = (GstClockTime) (shot_to_shot * GST_SECOND);
  target_preview_to_precapture =
      (GstClockTime) (preview_to_precapture * GST_SECOND);
  target_shot_to_buffer = (GstClockTime) (shot_to_buffer * GST_SECOND);
}

void
cameraWidgets::print_performance_data (void)
{
  GList *iter;
  gint i = 0;
  GstClockTime last_start = 0;
  CaptureTimingStats max;
  CaptureTimingStats min;
  CaptureTimingStats avg;
  CaptureTimingStats avg_wo_first;
  GstClockTime shot_to_shot;

  if (!performance_measure)
    return;

  parse_target_values ();

  /* Initialize stats */
  min.shot_to_shot = -1;
  min.shot_to_save = -1;
  min.shot_to_snapshot = -1;
  min.preview_to_precapture = -1;
  min.shot_to_buffer = -1;
  memset (&avg, 0, sizeof (CaptureTimingStats));
  memset (&avg_wo_first, 0, sizeof (CaptureTimingStats));
  memset (&max, 0, sizeof (CaptureTimingStats));

  g_print ("-- Performance results --\n");
  g_print ("Startup time: %" TIME_FORMAT "; Target: %" TIME_FORMAT "\n",
      TIME_ARGS (startup_time - initial_time), TIME_ARGS (target_startup));
  g_print ("Change mode time: %" TIME_FORMAT "; Target: %" TIME_FORMAT "\n",
      TIME_ARGS (change_mode_after - change_mode_before),
      TIME_ARGS (target_change_mode));

  g_print
      ("\n   | Shot to save |Shot to snapshot| Shot to shot |"
      "Preview to precap| Shot to buffer\n");
  capture_times = g_list_reverse (capture_times);
  for (iter = capture_times; iter; iter = g_list_next (iter)) {
    CaptureTiming *t = (CaptureTiming *) iter->data;
    CaptureTimingStats stats;

    stats.shot_to_save = SHOT_TO_SAVE (t);
    stats.shot_to_snapshot = SHOT_TO_SNAPSHOT (t);
    stats.shot_to_shot = i == 0 ? 0 : t->start_capture - last_start;
    stats.preview_to_precapture = PREVIEW_TO_PRECAPTURE (t);
    stats.shot_to_buffer = SHOT_TO_BUFFER (t);

    PRINT_STATS (i, &stats);

    if (i != 0) {
      capture_timing_stats_add (&avg_wo_first, &stats);
    }
    capture_timing_stats_add (&avg, &stats);

    if (stats.shot_to_save < min.shot_to_save) {
      min.shot_to_save = stats.shot_to_save;
    }
    if (stats.shot_to_snapshot < min.shot_to_snapshot) {
      min.shot_to_snapshot = stats.shot_to_snapshot;
    }
    if (stats.shot_to_shot < min.shot_to_shot && stats.shot_to_shot > 0) {
      min.shot_to_shot = stats.shot_to_shot;
    }
    if (stats.preview_to_precapture < min.preview_to_precapture) {
      min.preview_to_precapture = stats.preview_to_precapture;
    }
    if (stats.shot_to_buffer < min.shot_to_buffer) {
      min.shot_to_buffer = stats.shot_to_buffer;
    }


    if (stats.shot_to_save > max.shot_to_save) {
      max.shot_to_save = stats.shot_to_save;
    }
    if (stats.shot_to_snapshot > max.shot_to_snapshot) {
      max.shot_to_snapshot = stats.shot_to_snapshot;
    }
    if (stats.shot_to_shot > max.shot_to_shot) {
      max.shot_to_shot = stats.shot_to_shot;
    }
    if (stats.preview_to_precapture > max.preview_to_precapture) {
      max.preview_to_precapture = stats.preview_to_precapture;
    }
    if (stats.shot_to_buffer > max.shot_to_buffer) {
      max.shot_to_buffer = stats.shot_to_buffer;
    }

    last_start = t->start_capture;
    i++;
  }

  if (i == 0)
    return;

  if (i > 1)
    shot_to_shot = avg.shot_to_shot / (i - 1);
  else
    shot_to_shot = GST_CLOCK_TIME_NONE;
  capture_timing_stats_div (&avg, i);
  avg.shot_to_shot = shot_to_shot;
  if (i > 1)
    capture_timing_stats_div (&avg_wo_first, i - 1);
  else {
    memset (&avg_wo_first, 0, sizeof (CaptureTimingStats));
  }

  g_print ("\n    Stats             |     MIN      |     MAX      |"
      "     AVG      | AVG wo First |   Target     | Diff \n");
  g_print ("Shot to shot          | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIME_FORMAT " | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIMEDIFF_FORMAT "\n",
      TIME_ARGS (min.shot_to_shot), TIME_ARGS (max.shot_to_shot),
      TIME_ARGS (avg.shot_to_shot),
      TIME_ARGS (avg_wo_first.shot_to_shot),
      TIME_ARGS (target_shot_to_shot),
      TIMEDIFF_ARGS (TIME_DIFF (avg.shot_to_shot, target_shot_to_shot)));
  g_print ("Shot to save          | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIME_FORMAT " | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIMEDIFF_FORMAT "\n",
      TIME_ARGS (min.shot_to_save), TIME_ARGS (max.shot_to_save),
      TIME_ARGS (avg.shot_to_save),
      TIME_ARGS (avg_wo_first.shot_to_save),
      TIME_ARGS (target_shot_to_save),
      TIMEDIFF_ARGS (TIME_DIFF (avg.shot_to_save, target_shot_to_save)));
  g_print ("Shot to snapshot      | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIME_FORMAT " | %" TIME_FORMAT " | %" TIME_FORMAT
      " | %" TIMEDIFF_FORMAT "\n",
      TIME_ARGS (min.shot_to_snapshot),
      TIME_ARGS (max.shot_to_snapshot),
      TIME_ARGS (avg.shot_to_snapshot),
      TIME_ARGS (avg_wo_first.shot_to_snapshot),
      TIME_ARGS (target_shot_to_snapshot),
      TIMEDIFF_ARGS (TIME_DIFF (avg.shot_to_snapshot,
              target_shot_to_snapshot)));
  g_print ("Preview to precapture | %" TIME_FORMAT " | %" TIME_FORMAT " | %"
      TIME_FORMAT " | %" TIME_FORMAT " | %" TIME_FORMAT " | %" TIMEDIFF_FORMAT
      "\n", TIME_ARGS (min.preview_to_precapture),
      TIME_ARGS (max.preview_to_precapture),
      TIME_ARGS (avg.preview_to_precapture),
      TIME_ARGS (avg_wo_first.preview_to_precapture),
      TIME_ARGS (target_preview_to_precapture),
      TIMEDIFF_ARGS (TIME_DIFF (avg.preview_to_precapture,
              target_preview_to_precapture)));
  g_print ("Shot to buffer        | %" TIME_FORMAT " | %" TIME_FORMAT " | %"
      TIME_FORMAT " | %" TIME_FORMAT " | %" TIME_FORMAT " | %" TIMEDIFF_FORMAT
      "\n", TIME_ARGS (min.shot_to_buffer), TIME_ARGS (max.shot_to_buffer),
      TIME_ARGS (avg.shot_to_buffer), TIME_ARGS (avg_wo_first.shot_to_buffer),
      TIME_ARGS (target_shot_to_buffer),
      TIMEDIFF_ARGS (TIME_DIFF (avg.shot_to_buffer, target_shot_to_buffer)));
}

void cameraWidgets::updateCamerabarStatus(gboolean enable) {
    m_capture->setEnabled(enable);
    m_mode->setEnabled(enable);
    m_camerasbox->setEnabled(enable);
    m_previewsizesbox->setEnabled(enable);
    m_topWid->m_btnreturn->setEnabled(enable);
}

void cameraWidgets::updateCamerasBox() {
    disconnect(m_camerasbox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slot_camera_changed(int)));
    m_camerasbox->clear();
    for (int i = 0; i < m_caminfoList.size(); i++) {
        QString cam_nam = QString(m_caminfoList.at(i).cam.name);
        m_camerasbox->insertItem(i,QString("%1(/dev/video%2)").arg(cam_nam)
                                 .arg(m_caminfoList.at(i).video_index));
    }
    connect(m_camerasbox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slot_camera_changed(int)));
    qDebug() << "updateCamerasBox end";
}

void cameraWidgets::updatePreviewSizesBox() {
    disconnect(m_previewsizesbox, SIGNAL(currentIndexChanged(int)), this,
            SLOT(slot_previewsize_changed(int)));
    m_previewsizesbox->clear();
    for (int i = 0; i < m_supportedViewfinderSettings.size(); i++) {
        QCameraViewfinderSettings setting = m_supportedViewfinderSettings.at(i);
        QString size = QString("%1x%2").arg(setting.resolution().width())
                .arg(setting.resolution().height());
        QString format;
        QTextStream(&format) << "" << setting.pixelFormat();
        QString framerate = QString("rate:%1/%2").arg(setting.maximumFrameRate())
                .arg(setting.minimumFrameRate());
        QString preview = QString("%1,%2").arg(size).arg(framerate);
        m_previewsizesbox->insertItem(i,preview);
    }
    m_previewsizesbox->setCurrentIndex(m_preview_index);
    connect(m_previewsizesbox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(slot_previewsize_changed(int)));
    qDebug() << "updatePreviewSizesBox end";

}

void cameraWidgets::updateCameraParameter() {
    if (m_supportedViewfinderSettings.size() == 0) return;
    qDebug() << "updateCameraParameter m_preview_index:" << m_preview_index
             << ", m_cam_index:" << m_cam_index;

    view_framerate_num = m_supportedViewfinderSettings.at(m_preview_index).maximumFrameRate();
    view_framerate_den = 1;

    int width = m_supportedViewfinderSettings.at(m_preview_index).resolution().width();
    int height = m_supportedViewfinderSettings.at(m_preview_index).resolution().height();
    preview_caps_name = g_strdup_printf ("video/x-raw,format=NV12,width=%d,height=%d",
                                         width, height);
    videodevice_name = g_strdup_printf ("/dev/video%d", m_caminfoList.at(m_cam_index).video_index);
    viewfinder_caps_str = g_strdup_printf ("video/x-raw,format=NV12,width=%d,height=%d",
                                           width, height);
    image_width = width;
    image_height = height;
    qDebug() << "framerate:" << view_framerate_num << "/" << view_framerate_den;
    qDebug() << "videodevice_name:" << videodevice_name;
    qDebug() << "viewfinder_caps_str:" << viewfinder_caps_str;
    qDebug() << "updateCameraParameter end";
}

void cameraWidgets::start_timer_count()
{
    if(m_cameraWidgets && m_record_time){
        m_timer = new QTimer(m_cameraWidgets);
        m_recorder_time = 0;
        connect(m_timer, SIGNAL(timeout()), m_cameraWidgets, SLOT(slot_time_out()) );
        m_timer->start(1000);
        qDebug() << "start timer";
        m_record_time->setVisible(true);
        m_record_time->setText("00:00:00");
    }
}

void cameraWidgets::stop_timer_count()
{
    if(m_timer && m_timer->isActive()){
      m_timer->stop();
      qDebug() << "stop timer count";
      disconnect(m_timer, SIGNAL(timeout()), m_cameraWidgets, SLOT(slot_time_out()));
      m_record_time->setVisible(false);
    }
}

void cameraWidgets::slot_time_out() {
    updateRecordTime();
}

void cameraWidgets::init()
{
    m_cameraWidgets = this;
    gchar *target_times = NULL;
    gchar *ev_option = NULL;
    gchar *fn_option = NULL;

    GOptionEntry options[] = {
    {"ev-compensation", '\0', 0, G_OPTION_ARG_STRING, &ev_option,
        "EV compensation for source element GstPhotography interface", NULL},
    {"aperture", '\0', 0, G_OPTION_ARG_INT, &aperture,
          "Aperture (size of lens opening) for source element GstPhotography interface",
        NULL},
    {"flash-mode", '\0', 0, G_OPTION_ARG_INT,
          &flash_mode,
        "Flash mode for source element GstPhotography interface", NULL},
    {"scene-mode", '\0', 0, G_OPTION_ARG_INT,
          &scene_mode,
        "Scene mode for source element GstPhotography interface", NULL},
    {"exposure", '\0', 0, G_OPTION_ARG_INT64,
          &exposure,
          "Exposure time (in ms) for source element GstPhotography interface",
        NULL},
    {"iso-speed", '\0', 0, G_OPTION_ARG_INT,
          &iso_speed,
        "ISO speed for source element GstPhotography interface", NULL},
    {"white-balance-mode", '\0', 0, G_OPTION_ARG_INT,
          &wb_mode,
        "White balance mode for source element GstPhotography interface", NULL},
    {"colour-tone-mode", '\0', 0, G_OPTION_ARG_INT,
          &color_mode,
        "Colour tone mode for source element GstPhotography interface", NULL},
    {"directory", '\0', 0, G_OPTION_ARG_STRING, &fn_option,
        "Directory for capture file(s) (default is current directory)", NULL},
    {"mode", '\0', 0, G_OPTION_ARG_INT, &mode,
        "Capture mode (default = 1 (image), 2 = video)", NULL},
    {"capture-time", '\0', 0, G_OPTION_ARG_INT,
          &capture_time,
        "Time to capture video in seconds (default = 10)", NULL},
    {"capture-total", '\0', 0, G_OPTION_ARG_INT, &capture_total,
        "Total number of captures to be done (default = 1)", NULL},
    {"zoom", '\0', 0, G_OPTION_ARG_INT, &zoom,
        "Zoom (100 = 1x (default), 200 = 2x etc.)", NULL},
    {"wrapper-source", '\0', 0, G_OPTION_ARG_STRING, &wrappersrc_name,
          "Camera source wrapper used for setting the video source (default is wrappercamerabinsrc)",
        NULL},
    {"video-source", '\0', 0, G_OPTION_ARG_STRING, &videosrc_name,
        "Video source used in still capture and video recording", NULL},
    {"video-device", '\0', 0, G_OPTION_ARG_STRING, &videodevice_name,
        "Video device to be set on the video source", NULL},
    {"audio-source", '\0', 0, G_OPTION_ARG_STRING, &audiosrc_name,
        "Audio source used in video recording", NULL},
    {"image-pp", '\0', 0, G_OPTION_ARG_STRING, &imagepp_name,
        "List of image post-processing elements separated with comma", NULL},
    {"viewfinder-sink", '\0', 0, G_OPTION_ARG_STRING, &vfsink_name,
        "Viewfinder sink (default = fakesink)", NULL},
    {"image-width", '\0', 0, G_OPTION_ARG_INT, &image_width,
        "Width for image capture", NULL},
    {"image-height", '\0', 0, G_OPTION_ARG_INT, &image_height,
        "Height for image capture", NULL},
    {"view-framerate-num", '\0', 0, G_OPTION_ARG_INT, &view_framerate_num,
        "Framerate numerator for viewfinder", NULL},
    {"view-framerate-den", '\0', 0, G_OPTION_ARG_INT, &view_framerate_den,
        "Framerate denominator for viewfinder", NULL},
    {"preview-caps", '\0', 0, G_OPTION_ARG_STRING, &preview_caps_name,
        "Preview caps (e.g. video/x-raw-rgb,width=320,height=240)", NULL},
    {"viewfinder-filter", '\0', 0, G_OPTION_ARG_STRING, &viewfinder_filter,
        "Filter to process all frames going to viewfinder sink", NULL},
    {"x-width", '\0', 0, G_OPTION_ARG_INT, &x_width,
        "X window width (default = 320)", NULL},
    {"x-height", '\0', 0, G_OPTION_ARG_INT, &x_height,
        "X window height (default = 240)", NULL},
    {"no-xwindow", '\0', 0, G_OPTION_ARG_NONE, &no_xwindow,
        "Do not create XWindow", NULL},
    {"encoding-target", '\0', 0, G_OPTION_ARG_STRING, &gep_targetname,
        "Video encoding target name", NULL},
    {"encoding-profile", '\0', 0, G_OPTION_ARG_STRING, &gep_profilename,
        "Video encoding profile name", NULL},
    {"encoding-profile-filename", '\0', 0, G_OPTION_ARG_STRING, &gep_filename,
        "Video encoding profile filename", NULL},
    {"image-capture-caps", '\0', 0,
          G_OPTION_ARG_STRING, &image_capture_caps_str,
        "Image capture caps (e.g. video/x-raw-rgb,width=640,height=480)", NULL},
    {"viewfinder-caps", '\0', 0, G_OPTION_ARG_STRING,
          &viewfinder_caps_str,
        "Viewfinder caps (e.g. video/x-raw-rgb,width=640,height=480)", NULL},
    {"video-capture-caps", '\0', 0,
          G_OPTION_ARG_STRING, &video_capture_caps_str,
        "Video capture caps (e.g. video/x-raw-rgb,width=640,height=480)", NULL},
    {"audio-capture-caps", '\0', 0,
          G_OPTION_ARG_STRING, &audio_capture_caps_str,
          "Audio capture caps (e.g. audio/x-raw-int,width=16,depth=16,rate=44100,channels=2)",
        NULL},
    {"performance-measure", '\0', 0,
          G_OPTION_ARG_NONE, &performance_measure,
          "If performance information should be printed at the end of execution",
        NULL},
    {"performance-targets", '\0', 0,
          G_OPTION_ARG_STRING, &performance_targets_str,
          "Comma separated list of doubles representing the target values in "
          "seconds. The order is: startup time, change mode time, shot to save"
          ", shot to snapshot, shot to shot, preview to shot, shot to buffer. "
          "e.g. 3.5,1.0,5.0,2.5,5.0,1.5,1.0",
        NULL},
    {"flags", '\0', 0, G_OPTION_ARG_STRING, &camerabin_flags,
        "camerabin element flags (default = 0)", NULL},
    {NULL}
  };

    GST_DEBUG_CATEGORY_INIT (camerabin_test, "camerabin-test", 0,
      "camerabin test");

    vfsink_name = g_strdup ("kmssink"); //default fakesink / waylandsink /autovideosink
    videosrc_name = g_strdup ("rkv4l2src");

    preview_caps_name = g_strdup ("video/x-raw,format=NV12,width=640,height=480");
    filename = g_string_new ("/tmp");
    videodevice_name = g_strdup ("/dev/video0");
    viewfinder_caps_str = g_strdup ("video/x-raw,format=NV12,width=640,height=480");
    image_width = 640;
    image_height = 480;

    m_capture = new QPushButton(this);
    if(mode == MODE_IMAGE){
        m_capture->setText(tr("Take Picture"));
    }else{
        m_capture->setText(tr("start Recorder"));
    }


    m_capture->setStyleSheet("QPushButton{background-color:rgba(190,190,190,1);}");

    m_mode = new QPushButton(this);
    if(mode == MODE_IMAGE){
        m_mode->setText(tr("Capture Image Mode"));
    }else{
        m_mode->setText(tr("Video Recorder Mode"));
    }
    m_mode->setStyleSheet("QPushButton{background-color:rgba(190,190,190,1);}");

    m_previewWid = new cameraPreviewwidgets(this);

    m_camerasbox = new QComboBox;
    m_camerasbox->setStyleSheet("QComboBox::drop-down {width:100px;}"
                                "QComboBox{background-color:rgba(190,190,190,1);}");
    m_previewsizesbox = new QComboBox;
    m_previewsizesbox->setStyleSheet("QComboBox::drop-down {width:100px;}"
                                     "QComboBox{background-color:rgba(190,190,190,1);}");

    connect(m_capture, SIGNAL(released()),this,SLOT(slot_capture()));

    connect(m_mode, SIGNAL(pressed()),this,SLOT(slot_pressed()));
    connect(m_mode, SIGNAL(released()),this,SLOT(slot_modeswitch()));

    connect(this, SIGNAL(sig_camera_error(QString)), this, SLOT(slot_camera_error(QString)));
    connect(this, SIGNAL(sig_capture_done(QString)), this, SLOT(slot_capture_done(QString)));
}

void cameraWidgets::initLayout()
{
    QVBoxLayout  *vmainlyout = new QVBoxLayout;
    QStackedLayout *stackedLayout = new QStackedLayout;
    stackedLayout->setStackingMode(QStackedLayout::StackAll);

    m_topWid = new CameraTopWidgets(this);

    QVBoxLayout *hmiddlelyout = new QVBoxLayout;

    stackedLayout->addWidget(m_previewWid);
    
    QHBoxLayout *buttonbar = new QHBoxLayout;
    buttonbar->addWidget(m_camerasbox,1);
    buttonbar->addWidget(m_previewsizesbox,1);
    buttonbar->addWidget(m_capture,1);
    buttonbar->addWidget(m_mode,1);
    buttonbar->setContentsMargins(10,10,10,10);
    buttonbar->setSpacing(10);

    QHBoxLayout *camerabar = new QHBoxLayout;
    camerabar->addWidget(m_camerasbox);
    camerabar->addWidget(m_previewsizesbox);
    camerabar->setContentsMargins(10,10,10,10);
    camerabar->setSpacing(10);

    m_record_time = new QLabel(this);
    m_record_time->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_record_time->setStyleSheet("QLabel{color:red;}");
    QFont font = m_record_time->font();
    font.setPointSize(15);
    m_record_time->setFont(font);
    m_record_time->setVisible(false);
    
    hmiddlelyout->addWidget(m_topWid);
    hmiddlelyout->addWidget(m_record_time);
    hmiddlelyout->addLayout(buttonbar);
    hmiddlelyout->setAlignment(m_record_time,Qt::AlignTop | Qt::AlignRight);
    hmiddlelyout->setAlignment(buttonbar,Qt::AlignBottom);
    hmiddlelyout->setContentsMargins(0,0,0,0);
    hmiddlelyout->setSpacing(0);

    QWidget * ui = new QWidget;
    ui->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->setLayout(hmiddlelyout);

    stackedLayout->addWidget(ui);
    
    vmainlyout->addLayout(stackedLayout);
    vmainlyout->setContentsMargins(0,0,0,0);
    vmainlyout->setSpacing(0);

    setLayout(vmainlyout);
}

#ifndef CAMERAWIDGETS_H
#define CAMERAWIDGETS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define GST_USE_UNSTABLE_API 1


#include "base/basewidget.h"
#include "cameratopwidgets.h"
#include "camerapreviewwidgets.h"

#include <QApplication>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QDebug>
#include <QThread>
#include <QPushButton>
#include <QMediaRecorder>
#include <QFrame>
#include <QImage>
#include <QWidget>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <QLabel>
#include <QComboBox>
#include <QTimer>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stddef.h>

#include <linux/v4l2-subdev.h>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gst/interfaces/photography.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gst/pbutils/encoding-profile.h>
#include <gst/pbutils/encoding-target.h>

GST_DEBUG_CATEGORY_STATIC (camerabin_test);
#define GST_CAT_DEFAULT camerabin_test

#define TIME_DIFF(a,b) ((((gint64)(a)) - ((gint64)(b))) / (gdouble) GST_SECOND)

#define TIME_FORMAT "02d.%09u"
#define TIMEDIFF_FORMAT "0.6lf"

#define TIME_ARGS(t) \
        (GST_CLOCK_TIME_IS_VALID (t) && (t) < 99 * GST_SECOND) ? \
        (gint) ((((GstClockTime)(t)) / GST_SECOND) % 60) : 99, \
        (GST_CLOCK_TIME_IS_VALID (t) && ((t) < 99 * GST_SECOND)) ? \
        (guint) (((GstClockTime)(t)) % GST_SECOND) : 999999999

#define TIMEDIFF_ARGS(t) (t)

#define RK_ENUM_VIDEO_NUM_MAX 12
#define RK_CAM_ATTRACED_INUPUT_MAX 4


/* capture mode options*/
#define MODE_VIDEO 2
#define MODE_IMAGE 1
#define MODE_VIEWFINDER 0

#define SUPPORTED_IMAGE_CAPTURE_CAPS_PROPERTY "image-capture-supported-caps"
#define SUPPORTED_VIDEO_CAPTURE_CAPS_PROPERTY "video-capture-supported-caps"
#define SUPPORTED_VIEWFINDER_CAPS_PROPERTY "viewfinder-supported-caps"


/* photography interface command line options */
#define EV_COMPENSATION_NONE -G_MAXFLOAT
#define APERTURE_NONE -G_MAXINT
#define FLASH_MODE_NONE -G_MAXINT
#define SCENE_MODE_NONE -G_MAXINT
#define EXPOSURE_NONE -G_MAXINT64
#define ISO_SPEED_NONE -G_MAXINT
#define WHITE_BALANCE_MODE_NONE -G_MAXINT
#define COLOR_TONE_MODE_NONE -G_MAXINT

enum RK_CAM_ATTACHED_TO_e {
	 RK_CAM_ATTACHED_TO_INVALID,
	 RK_CAM_ATTACHED_TO_ISP,
	 RK_CAM_ATTACHED_TO_CIF,
	 RK_CAM_ATTACHED_TO_USB
};

struct rk_cam_video_input_infos {
   int index;
   char name[30];
   void* dev;
   RK_CAM_ATTACHED_TO_e type;
};

struct rk_cam_video_node {
   char card[30]; //
   int video_index;
   int input_nums;
   struct rk_cam_video_input_infos input[RK_CAM_ATTRACED_INUPUT_MAX];
};

#define ISP_DEV_VIDEO_NODES_NUM_MAX 5
struct rk_isp_dev_info {
	int isp_dev_node_nums;
	struct rk_cam_video_node video_nodes[ISP_DEV_VIDEO_NODES_NUM_MAX];
};
 
#define CIF_DEV_VIDEO_NODES_NUM_MAX 4
struct rk_cif_dev_info {
	int cif_index;
	struct rk_cam_video_node video_node;
};

struct rk_cif_dev_infos {
	int cif_dev_node_nums;
	struct rk_cif_dev_info cif_devs[CIF_DEV_VIDEO_NODES_NUM_MAX];
};

#define USB_CAM_DEV_VIDEO_NODES_NUM_MAX 2
struct rk_usb_cam_dev_infos {
	int usb_dev_node_nums;
	struct rk_cam_video_node video_nodes[USB_CAM_DEV_VIDEO_NODES_NUM_MAX];
};
 
#define RK_SUPPORTED_CAMERA_NUM_MAX 12
struct rk_cams_dev_info {
	int num_camers;
	struct rk_cam_video_input_infos*  cam[RK_SUPPORTED_CAMERA_NUM_MAX];
	struct rk_isp_dev_info isp_dev;
	struct rk_cif_dev_infos cif_devs;
	struct rk_usb_cam_dev_infos usb_devs;
};

typedef struct rk_cam_info {
    int video_index;
    struct rk_cam_video_input_infos cam;
} camerainfo;


typedef struct _CaptureTiming
{
  GstClockTime start_capture;
  GstClockTime got_preview;
  GstClockTime capture_done;
  GstClockTime precapture;
  GstClockTime camera_capture;
} CaptureTiming;

typedef struct _CaptureTimingStats
{
  GstClockTime shot_to_shot;
  GstClockTime shot_to_save;
  GstClockTime shot_to_snapshot;
  GstClockTime preview_to_precapture;
  GstClockTime shot_to_buffer;
} CaptureTimingStats;

static void
capture_timing_stats_add (CaptureTimingStats * a, CaptureTimingStats * b)
{
  a->shot_to_shot += b->shot_to_shot;
  a->shot_to_snapshot += b->shot_to_snapshot;
  a->shot_to_save += b->shot_to_save;
  a->preview_to_precapture += b->preview_to_precapture;
  a->shot_to_buffer += b->shot_to_buffer;
}

static void
capture_timing_stats_div (CaptureTimingStats * stats, gint div)
{
  stats->shot_to_shot /= div;
  stats->shot_to_snapshot /= div;
  stats->shot_to_save /= div;
  stats->preview_to_precapture /= div;
  stats->shot_to_buffer /= div;
}

static GstEncodingProfile *
create_ogg_profile (void)
{
  GstEncodingContainerProfile *container;
  GstCaps *caps = NULL;

  caps = gst_caps_new_empty_simple ("application/ogg");
  container = gst_encoding_container_profile_new ("ogg", NULL, caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_new_empty_simple ("video/x-theora");
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_video_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  caps = gst_caps_new_empty_simple ("audio/x-vorbis");
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_audio_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  return (GstEncodingProfile *) container;
}

static GstEncodingProfile *
create_webm_profile (void)
{
  GstEncodingContainerProfile *container;
  GstCaps *caps = NULL;

  caps = gst_caps_new_empty_simple ("video/webm");
  container = gst_encoding_container_profile_new ("webm", NULL, caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_new_empty_simple ("video/x-vp8");
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_video_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  caps = gst_caps_new_empty_simple ("audio/x-vorbis");
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_audio_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  return (GstEncodingProfile *) container;
}

static GstEncodingProfile *
create_mp4_profile (void)
{
  GstEncodingContainerProfile *container;
  GstCaps *caps = NULL;

  caps =
      gst_caps_new_simple ("video/quicktime", "variant", G_TYPE_STRING, "iso",
      NULL);
  container = gst_encoding_container_profile_new ("mp4", NULL, caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_new_empty_simple ("video/x-h264");
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_video_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  caps = gst_caps_new_simple ("audio/mpeg", "version", G_TYPE_INT, 4, NULL);
  gst_encoding_container_profile_add_profile (container, (GstEncodingProfile *)
      gst_encoding_audio_profile_new (caps, NULL, NULL, 1));
  gst_caps_unref (caps);

  return (GstEncodingProfile *) container;
}


#define PRINT_STATS(d,s) g_print ("%02d | %" TIME_FORMAT " | %" \
    TIME_FORMAT "   | %" TIME_FORMAT " | %" TIME_FORMAT \
    "    | %" TIME_FORMAT "\n", d, \
    TIME_ARGS ((s)->shot_to_save), TIME_ARGS ((s)->shot_to_snapshot), \
    TIME_ARGS ((s)->shot_to_shot), \
    TIME_ARGS ((s)->preview_to_precapture), \
    TIME_ARGS ((s)->shot_to_buffer))

#define SHOT_TO_SAVE(t) ((t)->capture_done - (t)->start_capture)
#define SHOT_TO_SNAPSHOT(t) ((t)->got_preview - (t)->start_capture)
#define PREVIEW_TO_PRECAPTURE(t) ((t)->precapture - (t)->got_preview)
#define SHOT_TO_BUFFER(t) ((t)->camera_capture - (t)->start_capture)

static GstElement *camerabin = NULL;
static GstElement *viewfinder_sink = NULL;
static GstElement *m_videoSrc = NULL;
static gulong camera_probe_id = 0;
static gulong viewfinder_probe_id = 0;
static gulong m_winid = 0;

/* timing data */
static GstClockTime initial_time = 0;
static GstClockTime startup_time = 0;
static GstClockTime change_mode_before = 0;
static GstClockTime change_mode_after = 0;

static GList *capture_times = NULL;
static gchar *preview_caps_name = NULL;
static GString *filename = NULL;

static gint capture_total = 1;
static gint capture_time = 10;
static gint capture_count = 0;
static gulong stop_capture_cb_id = 0;

static gint mode = MODE_IMAGE;
static gint zoom = 100;

static gfloat ev_compensation = EV_COMPENSATION_NONE;
static gint aperture = APERTURE_NONE;
static gint flash_mode = FLASH_MODE_NONE;
static gint scene_mode = SCENE_MODE_NONE;
static gint64 exposure = EXPOSURE_NONE;
static gint iso_speed = ISO_SPEED_NONE;
static gint wb_mode = WHITE_BALANCE_MODE_NONE;
static gint color_mode = COLOR_TONE_MODE_NONE;

static gboolean ready_for_capture = FALSE;
static gulong ready_for_capture_id = 0;
static gboolean recording = FALSE;

static struct rk_cams_dev_info g_cam_infos;

class QGstreamerVideoRendererInterface;

class cameraWidgets:public BaseWidget
{
	Q_OBJECT
public:
    cameraWidgets(QWidget *parent=0);
	~cameraWidgets();

	void closeCamera();
	void openCamera();
	void uevent(const char *action, const char *dev);

    static CameraTopWidgets *m_topWid;

private:
    /*QCamera *m_camera;
    static QCameraViewfinder *m_viewfinder;
	static QWidget *m_frame;
	QGstreamerVideoRendererInterface *m_viewfinderInterface;
    QCameraImageCapture *m_imageCapture;
	QMediaRecorder *m_mediaRecorder;*/
	static QPushButton *m_capture;
	static QPushButton *m_mode;
    static QComboBox *m_camerasbox;
    static QComboBox *m_previewsizesbox;
	static cameraWidgets *m_cameraWidgets;
	static gboolean m_camera_error;

	cameraPreviewwidgets *m_previewWid;

    static QList<camerainfo> m_caminfoList;

    static int m_cam_index;
    static int m_preview_index;
	
	/*
	 * Global vars
	 */
	GMainLoop *loop = NULL;
	
	/* commandline options */
	gchar *videosrc_name = NULL;
    static gchar *videodevice_name;
	gchar *audiosrc_name = NULL;
	gchar *wrappersrc_name = NULL;
	gchar *imagepp_name = NULL;
	gchar *vfsink_name = NULL;
    static gint image_width;
    static gint image_height;
    static gfloat view_framerate_num;
    static gint view_framerate_den;
	gboolean no_xwindow = FALSE;
	gchar *gep_targetname = NULL;
	gchar *gep_profilename = NULL;
	gchar *gep_filename = NULL;
    static gchar *image_capture_caps_str;
    static gchar *viewfinder_caps_str;
    static gchar *video_capture_caps_str;
	gchar *audio_capture_caps_str = NULL;
	gboolean performance_measure = FALSE;
	gchar *performance_targets_str = NULL;
	gchar *camerabin_flags = NULL;

	/* test configuration for common callbacks */

	gchar *viewfinder_filter = NULL;

	int x_width = 320;
	int x_height = 240;

	GstClockTime target_startup;
	GstClockTime target_change_mode;
	GstClockTime target_shot_to_shot;
	GstClockTime target_shot_to_save;
	GstClockTime target_shot_to_snapshot;
	GstClockTime target_preview_to_precapture;
	GstClockTime target_shot_to_buffer;

	QList<QCameraViewfinderSettings> m_supportedViewfinderSettings;
    static QString m_location;
    QLabel *m_record_time = NULL;
    QTimer *m_timer = NULL;
    int m_recorder_time;

    void init();
    void initLayout();

    int getCameraInfos(struct rk_cams_dev_info* cam_infos);
	void updateSupportedViewfinderSettings();
    void updateCamerasBox();
    void updatePreviewSizesBox();
	GstCaps *supportedCaps(int mode) const;
    void updateCameraParameter();

	static GstPadProbeReturn camera_src_get_timestamp_probe (GstPad * pad, GstPadProbeInfo * info,
    	gpointer udata);
    static GstPadProbeReturn viewfinder_get_timestamp_probe (GstPad * pad, GstPadProbeInfo * info,
    	gpointer udata);
	GstElement * create_ipp_bin (void);
	GstEncodingProfile * load_encoding_profile (void);
	static void stop_capture_cb (GObject * self, GParamSpec * pspec, gpointer user_data);
	static gboolean stop_capture (gpointer user_data);
	void parse_target_values (void);
	void print_performance_data (void);

	void switchMode();
	static gboolean run_preview_pipeline (gpointer user_data);
	static void set_metadata (GstElement * camera);
	static gboolean run_taking_pipeline (gpointer user_data);
	static GstBusSyncReply sync_bus_callback (GstBus * bus, GstMessage * message, gpointer data);
	static gboolean bus_callback (GstBus * bus, GstMessage * message, gpointer data);
	void cleanup_pipeline (void);
	gboolean setup_pipeline_element (GstElement * element, const gchar * property_name,
    	const gchar * element_name, GstElement ** res_elem);
    static void set_camerabin_caps_from_string (void);
	gboolean setup_pipeline (void);
	static void notify_readyforcapture (GObject * obj, GParamSpec * pspec,
		 gpointer user_data);

    static void updateCamerabarStatus(gboolean enable);

    void start_timer_count();
    void stop_timer_count();
    void updateRecordTime();

protected:
	virtual void	showEvent(QShowEvent *event);
	virtual void	hideEvent(QHideEvent *event);


private slots:
	void slot_pressed();
	void slot_capture();
	void slot_released();

	void slot_modeswitch();
	void handleResults(const QString &);
    void slot_camera_error(QString error);
    void slot_capture_done(QString location);

    void slot_camera_changed(int index);
    void slot_previewsize_changed(int index);
    void slot_time_out();
signals:
	void sig_openCamera();
    void sig_camera_error(QString error);
    void sig_capture_done(QString location);

};


class OpenCameraThread : public QThread
{
    Q_OBJECT
public:
	OpenCameraThread(cameraWidgets *parent=0) {mWidgets = parent;}
    ~OpenCameraThread(){qDebug() << "~OpenCameraThread()";}

	void run() {
        QString result;
        /* expensive or blocking operation  */
		if (mWidgets)
			mWidgets->openCamera();
        emit resultReady(result);
    }

private:
	cameraWidgets* mWidgets;


signals:
    void resultReady(const QString &s);
};



#endif // CAMERAWIDGETS_H

////////////////////////////////////////////////////////////////////////////
//
//  This file is part of SNC
//
//  Copyright (c) 2014-2021, Richard Barnett
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of
//  this software and associated documentation files (the "Software"), to deal in
//  the Software without restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
//  Software, and to permit persons to whom the Software is furnished to do so,
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "RTSPIF.h"
#include <qdebug.h>
#include <qbuffer.h>

#define TAG "RTSPIF"

//#define GSTBUSMSG

#ifdef USE_GST10
static void sinkEOS(GstAppSink * /*appsink*/, gpointer /*user_data*/)
{
}
#endif

static GstFlowReturn sinkPreroll(GstAppSink * /*appsink*/, gpointer user_data)
{
    ((RTSPIF *)user_data)->processVideoPrerollData();
    return GST_FLOW_OK;
}

static GstFlowReturn sinkSample(GstAppSink * /*appsink*/, gpointer user_data)
{
    ((RTSPIF *)user_data)->processVideoSinkData();
    return GST_FLOW_OK;
}

#ifdef GSTBUSMSG
static gboolean videoBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((RTSPIF *)data)->m_pipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
            qDebug() << "Video element " << GST_OBJECT_NAME (message->src) << " changed state from "
                << gst_element_state_get_name (old_state) << " to " << gst_element_state_get_name (new_state);
    }
    return TRUE;
}
#endif


RTSPIF::RTSPIF() : SNCThread(TAG)
{
    m_pipeline = NULL;
    m_pipelineActive = false;
    m_appVideoSink = NULL;
    m_widthPrefix = "width=(int)";
    m_heightPrefix = "height=(int)";
    m_frameratePrefix = "framerate=(fraction)";
}

RTSPIF::~RTSPIF()
{
}

void RTSPIF::initThread()
{
//    thread()->setPriority(QThread::TimeCriticalPriority);
    open();
}

void RTSPIF::finishThread()
{
    close();
}

bool RTSPIF::open()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

    if (!settings->contains(RTSP_CAMERA_TYPE))
        settings->setValue(RTSP_CAMERA_TYPE, RTSP_CAMERA_TYPE_FOSCAM);

    if (!settings->contains(RTSP_CAMERA_IPADDRESS))
        settings->setValue(RTSP_CAMERA_IPADDRESS, "192.168.0.253");

    if (!settings->contains(RTSP_CAMERA_USERNAME))
        settings->setValue(RTSP_CAMERA_USERNAME, "admin");

    if (!settings->contains(RTSP_CAMERA_PASSWORD))
        settings->setValue(RTSP_CAMERA_PASSWORD, "default");

    if (!settings->contains(RTSP_CAMERA_TCPPORT))
        settings->setValue(RTSP_CAMERA_TCPPORT, 80);


    m_IPAddress = settings->value(RTSP_CAMERA_IPADDRESS).toString();
    m_port = settings->value(RTSP_CAMERA_TCPPORT).toInt();
    m_user = settings->value(RTSP_CAMERA_USERNAME).toString();
    m_pw = settings->value(RTSP_CAMERA_PASSWORD).toString();

    //	This is just to get started. Real value will come in from the camera

    m_width = 640;
    m_height = 480;
    m_frameRate = 10;
    m_frameSize = m_width * m_height * 3;

    newPipeline();

    m_frameCount = 0;
    m_lastFrameTime = SNCUtils::clock();

    emit cameraState(QString("Connecting to ") + QString(m_IPAddress));


    settings->endGroup();

    delete settings;

    m_firstFrame = true;
    m_startTime = SNCUtils::clock();
    m_timer = startTimer(RTSP_BGND_INTERVAL);

    return true;
}

void RTSPIF::close()
{
    killTimer(m_timer);
    deletePipeline();
}

void RTSPIF::timerEvent(QTimerEvent * /*event*/)
{
    qint64 now = SNCUtils::clock();

    if (m_cameraType == RTSP_CAMERA_TYPE_SV3CB01) {

        if ((now - m_startTime) > RTSP_PIPELINE_RESET_SV3CB01) {
            close();
            SNCUtils::logInfo(TAG, "Resetting pipeline");
            open();
        }
    }

    if (m_cameraType == RTSP_CAMERA_TYPE_SV3CB06) {

        if ((now - m_startTime) > RTSP_PIPELINE_RESET_SV3CB06) {
            close();
            SNCUtils::logInfo(TAG, "Resetting pipeline");
            open();
        }
    }

    if (m_cameraType == RTSP_CAMERA_TYPE_FOSCAM) {

        if ((now - m_startTime) > RTSP_PIPELINE_RESET_FOSCAM) {
            close();
            SNCUtils::logInfo(TAG, "Resetting pipeline");
            open();
        }
    }

    if ((now - m_lastFrameTime) > RTSP_NOFRAME_TIMEOUT) {
        close();
        m_lastFrameTime = now;
        SNCUtils::logInfo(TAG, "Retrying connect");
        open();
    }
}

void RTSPIF::newCamera()
{
    close();
    open();
}

int RTSPIF::getCapsValue(const QString &caps, const QString& key, const QString& terminator)
{
    QString sub;
    int start;
    int end;

    start = caps.indexOf(key);
    if (start == -1)
        return -1;

    start += key.length();
    sub = caps;
    sub = sub.remove(0, start);
    end = sub.indexOf(terminator);
    if (end == -1)
        return -1;
    sub = sub.left(end);
    return sub.toInt();
}


void RTSPIF::processVideoSinkData()
{
    GstBuffer *buffer;
    QImage frame;

    m_videoLock.lock();
    if (m_appVideoSink != NULL) {
#ifdef USE_GST10
        GstSample *sinkSample = gst_app_sink_pull_sample((GstAppSink *)(m_appVideoSink));
        if (sinkSample != NULL) {
            m_lastFrameTime = SNCUtils::clock();

            frame = QImage(m_width, m_height, QImage::Format_RGB888);
            buffer = gst_sample_get_buffer(sinkSample);
            GstMemory *sinkMem = gst_buffer_get_all_memory(buffer);
            GstMapInfo *info = new GstMapInfo;
            gst_memory_map(sinkMem, info, GST_MAP_READ);
            memcpy(frame.bits(), info->data, m_frameSize);
            gst_memory_unmap(sinkMem, info);
            delete info;
            gst_memory_unref(sinkMem);
            gst_sample_unref(sinkSample);

            QByteArray jpeg;
            QBuffer buffer(&jpeg);
            buffer.open(QIODevice::WriteOnly);
            frame.save(&buffer, "JPG");
            emit newJpeg(jpeg);

            if (m_firstFrame) {
                emit cameraState(QString("Connected to ") + QString(m_IPAddress));
                m_firstFrame = false;
            }
        }
#else
        uchar *data;
        if ((buffer = gst_app_sink_pull_buffer((GstAppSink *)(m_appVideoSink))) != NULL) {
            m_lastFrameTime = SNCUtils::clock();
            data = (uchar *)malloc(GST_BUFFER_SIZE(buffer));
            memcpy(data, GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
            QImage image(data, m_width, m_height, QImage::Format_RGB888, free, data);
            emit newImage(image);
            gst_buffer_unref(buffer);
        }
#endif
    }
    m_videoLock.unlock();
}

void RTSPIF::processVideoPrerollData()
{
    int val;

    m_videoLock.lock();
    if (m_appVideoSink != NULL) {
        // get details of frame size and rate

#ifdef USE_GST10
        GstCaps *sinkCaps = gst_pad_get_current_caps(m_appVideoSinkPad);
#else
        GstCaps *sinkCaps = gst_pad_get_negotiated_caps(m_appVideoSinkPad);
#endif
        gchar *caps = gst_caps_to_string(sinkCaps);
        qDebug() << "Caps = " << caps;
        gst_caps_unref(sinkCaps);
        if (caps != m_caps) {
            // must have changed
            m_caps = caps;
            if ((val = getCapsValue(caps, m_widthPrefix, ",")) != -1)
                m_width = val;
            if ((val = getCapsValue(caps, m_heightPrefix, ",")) != -1)
                m_height = val;
            if ((val = getCapsValue(caps, m_frameratePrefix, "/")) != -1)
                m_frameRate = val;
            m_frameSize = m_width * m_height * 3;
            emit videoFormat(m_width, m_height, m_frameRate);
        }
        g_free(caps);
    }
    m_videoLock.unlock();
}


bool RTSPIF::newPipeline()
{
    gchar *launch;
    GError *error = NULL;
    GstStateChangeReturn ret;

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);
    m_cameraType = settings->value(RTSP_CAMERA_TYPE).toString();
    settings->endGroup();
    delete settings;

    //  Construct the pipelines

#ifdef USE_GST10
        if (m_cameraType == RTSP_CAMERA_TYPE_FOSCAM) {
            launch = g_strdup_printf (
                " rtspsrc location=rtsp://%s:%d/videoMain user-id=%s user-pw=%s "
                 " ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink0 "
                 ,qPrintable(m_IPAddress), m_port, qPrintable(m_user), qPrintable(m_pw));
        }
        if (m_cameraType == RTSP_CAMERA_TYPE_SV3CB01) {
            launch = g_strdup_printf (
                " rtspsrc location=rtsp://%s:%d/11"
                 " ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink0 "
                        ,qPrintable(m_IPAddress), m_port);
        }
        if (m_cameraType == RTSP_CAMERA_TYPE_SV3CB06) {
            launch = g_strdup_printf (
                " rtspsrc location=rtsp://%s:%s@%s:%d/stream0"
                 " ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw,format=RGB\" ! appsink name=videoSink0 "
                        ,qPrintable(m_user), qPrintable(m_pw), qPrintable(m_IPAddress), m_port);
        }
#else
    launch = g_strdup_printf (
            " rtspsrc location=rtsp://%s:%d/videoMain user-id=%s user-pw=%s "
            " ! gstrtpjitterbuffer ! rtph264depay ! queue ! nv_omx_h264dec "
            " ! capsfilter caps=\"video/x-raw-yuv\" ! ffmpegcolorspace ! capsfilter caps=\"video/x-raw-rgb\" ! queue ! appsink name=videoSink0 "
            ,qPrintable(m_IPAddress), m_port, qPrintable(m_user), qPrintable(m_pw));
#endif
    m_pipeline = gst_parse_launch(launch, &error);
    g_free(launch);

    if (error != NULL) {
        qDebug() << "Could not construct video pipeline: " << error->message;
        g_error_free (error);
        m_pipeline = NULL;
        return false;
    }

    //  find the appsink

    gchar *videoSink = g_strdup_printf("videoSink%d", 0);
    if ((m_appVideoSink = gst_bin_get_by_name (GST_BIN (m_pipeline), videoSink)) == NULL) {
        qDebug() << "Unable to find video appsink";
        g_free(videoSink);
        deletePipeline();
        return false;
    }
    g_free(videoSink);

    GList *pads = m_appVideoSink->pads;
    m_appVideoSinkPad = (GstPad *)pads->data;

#ifdef USE_GST10
    GstAppSinkCallbacks cb;
    cb.eos = sinkEOS;
    cb.new_preroll = sinkPreroll;
    cb.new_sample = sinkSample;
    gst_app_sink_set_callbacks((GstAppSink *)m_appVideoSink, &cb, this, NULL);
#else
    g_signal_connect (m_appVideoSink, "new-buffer", G_CALLBACK (sinkSample), this);
    g_signal_connect (m_appVideoSink, "new-preroll", G_CALLBACK (sinkPreroll), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), TRUE);

#endif

    ret = gst_element_set_state (m_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "Unable to set the video pipeline to the play state" ;
        deletePipeline();
        return false;
    }

#ifdef GSTBUSMSG
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_pipeline));
    m_videoBusWatch = gst_bus_add_watch (bus, videoBusMessage, this);
    gst_object_unref (bus);
#endif

    m_pipelineActive = true;
    return true;
}

void RTSPIF::deletePipeline()
{
    if (!m_pipelineActive)
        return;

    m_pipelineActive = false;
    m_videoLock.lock();
    if (m_pipeline != NULL) {
#ifdef GSTBUSMSG
        if (m_videoBusWatch != -1) {
            g_source_remove(m_videoBusWatch);
            m_videoBusWatch = -1;
        }
#endif
#ifdef USE_GST10
        GstAppSinkCallbacks cb;
        cb.eos = NULL;
        cb.new_preroll = NULL;
        cb.new_sample = NULL;
        gst_app_sink_set_callbacks((GstAppSink *)m_appVideoSink, &cb, this, NULL);
#else
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appVideoSink, this);
#endif
        gst_element_set_state (m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
    }

    m_pipeline = NULL;
    m_appVideoSink = NULL;
    m_videoLock.unlock();
    m_caps = "";
}

QSize RTSPIF::getImageSize()
{
    return QSize(m_width, m_height);
}


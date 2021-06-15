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

#include "AVMuxEncodeGS.h"

#include <qimage.h>
#include <qbuffer.h>
#include <qbytearray.h>

//  Note: enabling GSTBUSMSG can cause problems when deleting pipelines.
//  Only uncomment this symbol for debugging.

//#define GSTBUSMSG

#define PIPELINE_MS_TO_NS       ((qint64)1000000)

#ifdef USING_GSTREAMER
static void newVideoSinkData (GstElement * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxEncode *)user_data)->processVideoSinkData();
}

static void newAudioSinkData (GstElement * /*appsink*/,
          gpointer user_data)
{
    ((AVMuxEncode *)user_data)->processAudioSinkData();
}

static void needAudioSrcData (GstElement * /*appsrc*/, guint /*unused_size*/, gpointer user_data)
{
    ((AVMuxEncode *)user_data)->needAudioData();
}

#ifdef GSTBUSMSG
static gboolean videoBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((AVMuxEncode *)data)->m_videoPipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
           g_print ("Video element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
    }
    return TRUE;
}


static gboolean audioBusMessage(GstBus * /*bus*/, GstMessage *message, gpointer data)
{
    GstState old_state, new_state;

    if ((GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED)  &&
            ((GstElement *)(message->src) == ((AVMuxEncode *)data)->m_audioPipeline)){
         gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
           g_print ("Audio element %s changed state from %s to %s.\n",
            GST_OBJECT_NAME (message->src),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
    }
    return TRUE;
}
#endif  // GSTBUSMSG

static int g_pipelineIndex = 0;

AVMuxEncode::AVMuxEncode(int AVMode) : SyntroThread("AVMuxEncode", "AVMuxEncode")
{
    m_AVMode = AVMode;

    m_videoPipeline = NULL;
    m_audioPipeline = NULL;
    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_appVideoSink = NULL;
    m_appAudioSrc = NULL;
    m_videoCaps = NULL;
    m_audioCaps = NULL;
    m_videoBusWatch = -1;
    m_audioBusWatch = -1;
    m_audioTimestamp = -1;
    m_pipelinesActive = false;
    m_videoCompressionRate = 1000000;
    m_audioCompressionRate = 128000;
}


void AVMuxEncode::initThread()
{
    m_timer = startTimer(AVMUXENCODE_INTERVAL);
}

void AVMuxEncode::finishThread()
{
    killTimer(m_timer);
    m_pipelinesActive = false;
    deletePipelines();
}

void AVMuxEncode::timerEvent(QTimerEvent * /*event*/)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch() * PIPELINE_MS_TO_NS;

    if (!m_pipelinesActive)
        return;

    if ((m_videoPipeline != NULL) && (now >= m_nextVideoTime)) {
         needVideoData();
        m_nextVideoTime += m_videoInterval;
    }
}

void AVMuxEncode::newVideoData(QByteArray videoData, qint64 timestamp, int param)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = videoData;
    qd->timestamp = timestamp;
    qd->param = param;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_videoSrcLock.lock();
        m_videoSrcQ.enqueue(qd);
        m_videoSrcLock.unlock();
    } else {
        m_videoSinkLock.lock();
        m_videoSinkQ.enqueue(qd);
        m_videoSinkLock.unlock();
    }
}

void AVMuxEncode::newVideoData(QByteArray videoData)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = videoData;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_videoSrcLock.lock();
        m_videoSrcQ.enqueue(qd);
        m_videoSrcLock.unlock();
    } else {
        m_videoSinkLock.lock();
        m_videoSinkQ.enqueue(qd);
        m_videoSinkLock.unlock();
    }
}

void AVMuxEncode::newAudioData(QByteArray audioData, qint64 timestamp, int param)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = audioData;
    qd->timestamp = timestamp;
    qd->param = param;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_audioSrcLock.lock();
        m_audioSrcQ.enqueue(qd);
        m_audioSrcLock.unlock();
    } else {
        m_audioSinkLock.lock();
        m_audioSinkQ.enqueue(qd);
        m_audioSinkLock.unlock();
    }
}

void AVMuxEncode::newAudioData(QByteArray audioData)
{
    AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
    qd->data = audioData;

    if (m_AVMode != AVMUXENCODE_AV_TYPE_MJPPCM) {
        m_audioSrcLock.lock();
        m_audioSrcQ.enqueue(qd);
        m_audioSrcLock.unlock();
    } else {
        m_audioSinkLock.lock();
        m_audioSinkQ.enqueue(qd);
        m_audioSinkLock.unlock();
    }
}

bool AVMuxEncode::getCompressedVideo(QByteArray& videoData, qint64& timestamp, int& param)
{
    QMutexLocker lock(&m_videoSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_videoSinkQ.empty())
        return false;

    qd = m_videoSinkQ.dequeue();
    videoData = qd->data;
    timestamp = qd->timestamp;
    param = qd->param;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedVideo(QByteArray& videoData)
{
    QMutexLocker lock(&m_videoSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_videoSinkQ.empty())
        return false;

    qd = m_videoSinkQ.dequeue();
    videoData = qd->data;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedAudio(QByteArray& audioData, qint64& timestamp, int& param)
{
    QMutexLocker lock(&m_audioSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_audioSinkQ.empty())
        return false;

    qd = m_audioSinkQ.dequeue();
    audioData = qd->data;
    timestamp = qd->timestamp;
    param = qd->param;
    delete qd;
    return true;
}

bool AVMuxEncode::getCompressedAudio(QByteArray& audioData)
{
    QMutexLocker lock(&m_audioSinkLock);
    AVMUX_QUEUEDATA *qd;

    if (m_audioSinkQ.empty())
        return false;

    qd = m_audioSinkQ.dequeue();
    audioData = qd->data;
    delete qd;
    return true;
}

void AVMuxEncode::processVideoSinkData()
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo info;
    AVMUX_QUEUEDATA *qd;

    m_videoSinkLock.lock();
    g_signal_emit_by_name((GstAppSink *)(m_appVideoSink), "pull-sample", &sample);
    if (sample) {
        buffer = gst_sample_get_buffer(sample);
        qd = new AVMUX_QUEUEDATA;
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        qd->data = QByteArray((const char *)info.data, info.size);
        gst_buffer_unmap(buffer, &info);
        qd->timestamp = m_lastQueuedVideoTimestamp;
        qd->param = m_lastQueuedVideoParam;
        m_videoSinkQ.enqueue(qd);

        if (m_videoCaps == NULL)
            m_videoCaps = gst_caps_to_string(gst_sample_get_caps(sample));
//        qDebug() << "Video ts " << GST_BUFFER_TIMESTAMP(buffer);
        gst_sample_unref(sample);
     }
    m_videoSinkLock.unlock();
}

void AVMuxEncode::processAudioSinkData()
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo info;

    m_audioSinkLock.lock();
    g_signal_emit_by_name((GstAppSink *)(m_appAudioSink), "pull-sample", &sample);
    if (sample) {
        AVMUX_QUEUEDATA *qd = new AVMUX_QUEUEDATA;
        buffer = gst_sample_get_buffer(sample);
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        qd->data = QByteArray((const char *)info.data, info.size);
        gst_buffer_unmap(buffer, &info);
        qd->timestamp = m_lastQueuedAudioTimestamp;
        qd->param = m_lastQueuedAudioParam;
        m_audioSinkQ.enqueue(qd);
        if (m_audioCaps == NULL)
            m_audioCaps = gst_caps_to_string(gst_sample_get_caps(sample));
        gst_sample_unref(sample);
    }
    m_audioSinkLock.unlock();
}

//----------------------------------------------------------
//
//  Pipeline management stuff

bool AVMuxEncode::newPipelines(SYNTRO_AVPARAMS *avParams)
{
    gchar *videoLaunch;
    gchar *audioLaunch;
    GError *error = NULL;
    GstStateChangeReturn ret;

    m_avParams = *avParams;

    printf("width=%d, height=%d, rate=%d\n", avParams->videoWidth, avParams->videoHeight, avParams->videoFramerate);
    printf("channels=%d, rate=%d, size=%d\n", avParams->audioChannels, avParams->audioSampleRate, avParams->audioSampleSize);

    //  Construct the pipelines

    g_pipelineIndex++;

    if (m_AVMode == AVMUXENCODE_AV_TYPE_RTPMP4) {
        videoLaunch = g_strdup_printf (
              " appsrc name=videoSrc%d ! jpegdec ! queue ! ffenc_mpeg4 bitrate=%d "
              " ! queue ! rtpmp4vpay pt=96 ! queue ! appsink name=videoSink%d"
                , g_pipelineIndex, m_videoCompressionRate, g_pipelineIndex);
    } else {
        videoLaunch = g_strdup_printf (
#ifdef TK1
              " appsrc name=videoSrc%d ! jpegdec ! queue"
              " ! nvvidconv "
              " ! nv_omx_h264enc bitrate=%d quality-level=2 rc-mode=0"
              " ! queue ! rtph264pay pt=96 ! queue ! appsink name=videoSink%d"
#else
              " appsrc name=videoSrc%d ! jpegdec ! queue ! x264enc bitrate=%d tune=zerolatency rc-lookahead=0"
              " ! queue ! rtph264pay pt=96 ! queue ! appsink name=videoSink%d"
#endif
              , g_pipelineIndex, m_videoCompressionRate / 1000, g_pipelineIndex);

    }

    m_videoPipeline = gst_parse_launch(videoLaunch, &error);
    g_free(videoLaunch);

    if (error != NULL) {
        g_print ("could not construct video pipeline: %s\n", error->message);
        g_error_free (error);
        m_videoPipeline = NULL;
        return false;
    }

    audioLaunch = g_strdup_printf (
#ifdef GST_IMX6
                " appsrc name=audioSrc%d ! faac bitrate=%d ! rtpmp4apay pt=97 ! appsink name=audioSink%d "
#elif TK1
                " appsrc name=audioSrc%d ! voaacenc bitrate=%d ! rtpmp4apay pt=97 ! appsink name=audioSink%d "
#else
                " appsrc name=audioSrc%d ! voaacenc bitrate=%d ! rtpmp4apay pt=97 min-ptime=1000000000 ! appsink name=audioSink%d "
#endif
             , g_pipelineIndex, m_audioCompressionRate, g_pipelineIndex);

    m_audioPipeline = gst_parse_launch(audioLaunch, &error);
    g_free(audioLaunch);

    if (error != NULL) {
        g_print ("could not construct audio pipeline: %s\n", error->message);
        g_error_free (error);
        gst_object_unref(m_videoPipeline);
        m_videoPipeline = NULL;
        m_audioPipeline = NULL;
        return false;
    }

    //  find the appsrcs and appsinks

    gchar *videoSink = g_strdup_printf("videoSink%d", g_pipelineIndex);
    if ((m_appVideoSink = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSink)) == NULL) {
        g_printerr("Unable to find video appsink\n");
        g_free(videoSink);
        deletePipelines();
        return false;
    }
    g_free(videoSink);

    gchar *videoSrc = g_strdup_printf("videoSrc%d", g_pipelineIndex);
    if ((m_appVideoSrc = gst_bin_get_by_name (GST_BIN (m_videoPipeline), videoSrc)) == NULL) {
            g_printerr("Unable to find video appsrc\n");
            g_free(videoSrc);
            deletePipelines();
            return false;
        }
    g_free(videoSrc);

    gchar *audioSink = g_strdup_printf("audioSink%d", g_pipelineIndex);
    if ((m_appAudioSink = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSink)) == NULL) {
        g_printerr("Unable to find audio appsink\n");
        g_free(audioSink);
        deletePipelines();
        return false;
    }
    g_free(audioSink);

    gchar *audioSrc = g_strdup_printf("audioSrc%d", g_pipelineIndex);
    if ((m_appAudioSrc = gst_bin_get_by_name (GST_BIN (m_audioPipeline), audioSrc)) == NULL) {
            g_printerr("Unable to find audio appsrc\n");
            g_free(audioSrc);
            deletePipelines();
            return false;
        }
    g_free(audioSrc);

    g_signal_connect (m_appVideoSink, "new-sample", G_CALLBACK (newVideoSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), TRUE);

    g_signal_connect (m_appAudioSink, "new-sample", G_CALLBACK (newAudioSinkData), this);
    gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), TRUE);

    gst_app_src_set_caps((GstAppSrc *) (m_appVideoSrc),
             gst_caps_new_simple ("image/jpeg",
             "width", G_TYPE_INT, m_avParams.videoWidth,
             "height", G_TYPE_INT, m_avParams.videoHeight,
             "framerate", GST_TYPE_FRACTION, m_avParams.videoFramerate, 1,
             NULL));
    gst_app_src_set_stream_type((GstAppSrc *)(m_appVideoSrc), GST_APP_STREAM_TYPE_STREAM);

    gst_app_src_set_caps((GstAppSrc *) (m_appAudioSrc),
            gst_caps_new_simple ("audio/x-raw-int",
                      "width", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                      "depth", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                      "channels" ,G_TYPE_INT, (gint)m_avParams.audioChannels,
                      "rate",G_TYPE_INT, m_avParams.audioSampleRate,
                      "endianness",G_TYPE_INT,(gint)1234,
                      "signed", G_TYPE_BOOLEAN, (gboolean)TRUE,
                      NULL));

    gst_app_src_set_stream_type((GstAppSrc *)(m_appAudioSrc), GST_APP_STREAM_TYPE_STREAM);
    g_signal_connect(m_appAudioSrc, "need-data", G_CALLBACK (needAudioSrcData), this);

    ret = gst_element_set_state (m_videoPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the video pipeline to the play state.\n");
        deletePipelines();
        return false;
    }

#ifdef GSTBUSMSG
    GstBus *bus;

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_videoPipeline));
    m_videoBusWatch = gst_bus_add_watch (bus, videoBusMessage, this);
    gst_object_unref (bus);

    bus = gst_pipeline_get_bus(GST_PIPELINE (m_audioPipeline));
    m_audioBusWatch = gst_bus_add_watch (bus, audioBusMessage, this);
    gst_object_unref (bus);
#endif

    ret = gst_element_set_state (m_audioPipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the audio pipeline to the play state.\n");
        deletePipelines();
        return false;
    }
    m_videoInterval = (1000 * PIPELINE_MS_TO_NS) / m_avParams.videoFramerate;
    m_nextVideoTime = QDateTime::currentMSecsSinceEpoch() * PIPELINE_MS_TO_NS;

    qDebug() << "Pipelines established";
    m_pipelinesActive = true;
    return true;
}

void AVMuxEncode::deletePipelines()
{
    m_videoSrcLock.lock();
    m_audioSrcLock.lock();
    m_videoSinkLock.lock();
    m_audioSinkLock.lock();

    if (m_videoPipeline != NULL) {
        gst_element_set_state (m_videoPipeline, GST_STATE_NULL);
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appVideoSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appVideoSink, this);
        gst_object_unref(m_videoPipeline);
        m_videoPipeline = NULL;
    }

#ifdef GSTBUSMSG
    if (m_videoBusWatch != -1) {
        g_source_remove(m_videoBusWatch);
        m_videoBusWatch = -1;
    }
#endif

    if (m_audioPipeline != NULL) {
        gst_element_set_state (m_audioPipeline, GST_STATE_NULL);
        gst_app_sink_set_emit_signals((GstAppSink *)(m_appAudioSink), FALSE);
        g_signal_handlers_disconnect_by_data(m_appAudioSink, this);
        gst_object_unref(m_audioPipeline);
        m_audioPipeline = NULL;
    }

#ifdef GSTBUSMSG
    if (m_audioBusWatch != -1) {
        g_source_remove(m_audioBusWatch);
        m_audioBusWatch = -1;
    }
#endif

    if (m_videoCaps != NULL)
        g_free(m_videoCaps);
    if (m_audioCaps != NULL)
        g_free(m_audioCaps);
    m_videoCaps = NULL;
    m_audioCaps = NULL;

    m_appAudioSink = NULL;
    m_appAudioSrc = NULL;
    m_appVideoSink = NULL;
    m_appAudioSrc = NULL;

    while (!m_videoSrcQ.empty())
        delete m_videoSrcQ.dequeue();

    while (!m_audioSrcQ.empty())
        delete m_audioSrcQ.dequeue();

    while (!m_videoSinkQ.empty())
        delete m_videoSinkQ.dequeue();

    while (!m_audioSinkQ.empty())
        delete m_audioSinkQ.dequeue();

    m_audioTimestamp = -1;

    m_videoSrcLock.unlock();
    m_audioSrcLock.unlock();
    m_videoSinkLock.unlock();
    m_audioSinkLock.unlock();
}

void AVMuxEncode::needVideoData()
{
    GstFlowReturn ret;
    GstBuffer *buffer;

    m_videoSrcLock.lock();
    if (m_videoSrcQ.empty()) {
        m_videoSrcLock.unlock();
        return;
    }

    AVMUX_QUEUEDATA *qd = m_videoSrcQ.dequeue();
    QByteArray frame = qd->data;
    m_lastQueuedVideoTimestamp = qd->timestamp;
    m_lastQueuedVideoParam = qd->param;
    delete qd;

    buffer = gst_buffer_new_and_alloc(frame.length());
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_WRITE);
    memcpy(info.data, (unsigned char *)frame.data(), frame.length());
    gst_buffer_unmap(buffer, &info);
    m_videoSrcLock.unlock();

    g_signal_emit_by_name ((GstAppSrc *)(m_appVideoSrc), "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        qDebug() << "video push error ";
    }
}

void AVMuxEncode::needAudioData()
{
    GstFlowReturn ret;
    GstBuffer *buffer;
    GstMapInfo info;
    quint64 audioLength;
    QByteArray audio;
    qint64 duration;

    m_audioSrcLock.lock();
    if (m_audioSrcQ.empty()) {
//        qDebug() << "Using dummy audio";

        audioLength = (m_avParams.audioSampleRate *
                (m_avParams.audioSampleSize / 8) *
                m_avParams.audioChannels) / 10;

        buffer = gst_buffer_new_and_alloc(audioLength);
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        guint8 *ad = info.data;
        for (unsigned int i = 0; i < audioLength; i++)
            ad[i] = 0;
        gst_buffer_unmap(buffer, &info);
    } else {
//        qDebug() << "Queue " << m_audioSrcQ.count();
        AVMUX_QUEUEDATA *qd = m_audioSrcQ.dequeue();
        QByteArray audio = qd->data;
        m_lastQueuedAudioTimestamp = qd->timestamp;
        m_lastQueuedAudioParam = qd->param;
        delete qd;

        audioLength = audio.length();
        buffer = gst_buffer_new_and_alloc(audio.length());
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        memcpy(info.data, (unsigned char *)audio.data(), audio.length());
        gst_buffer_unmap(buffer, &info);
    }
    m_audioSrcLock.unlock();

    guint64 bytesPerSecond = m_avParams.audioSampleRate *
                            (m_avParams.audioSampleSize / 8) *
                            m_avParams.audioChannels;
    duration = ((quint64)1000000000 * audioLength) / bytesPerSecond;
//    GST_BUFFER_DURATION (buffer) = duration;

    if (m_audioTimestamp == -1)
        m_audioTimestamp = 0;
//    GST_BUFFER_TIMESTAMP(buffer) = m_audioTimestamp;
    m_audioTimestamp += duration;

    GstCaps *caps = gst_caps_new_simple(
                "audio/x-raw-int",
                "width", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                "depth", G_TYPE_INT, (gint)m_avParams.audioSampleSize,
                "channels" ,G_TYPE_INT, (gint)m_avParams.audioChannels,
                "rate",G_TYPE_INT,m_avParams.audioSampleRate,
                "endianness",G_TYPE_INT,(gint)1234,
                "signed", G_TYPE_BOOLEAN, (gboolean)TRUE,
                NULL);

    gst_app_src_set_caps((GstAppSrc *)(m_appAudioSrc), caps);

    g_signal_emit_by_name ((GstAppSrc *)(m_appAudioSrc), "push-buffer", buffer, &ret);
    gst_caps_unref(caps);
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        qDebug() << "audio push error";
    }
}

void AVMuxEncode::setCompressionRates(int videoCompressionRate, int audioCompressionRate)
{
    m_videoCompressionRate = videoCompressionRate;
    m_audioCompressionRate = audioCompressionRate;
}
#endif

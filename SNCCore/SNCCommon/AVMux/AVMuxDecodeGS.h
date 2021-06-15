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

#ifndef _AVMUXDECODE_H_
#define _AVMUXDECODE_H_

#include "SNCDefs.h"
#include "SNCThread.h"
#include "SNCAVDefs.h"
#include <qimage.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <qmutex.h>
#include <qqueue.h>

#define AVMUXDECODE_INTERVAL  (SNC_CLOCKS_PER_SEC / 100)

class AVMuxDecode : public SNCThread
{
    Q_OBJECT

public:
    AVMuxDecode();

    // gstreamer callbacks

    void processVideoSinkData();
    void processAudioSinkData();

    GstElement *m_videoPipeline;
    GstElement *m_audioPipeline;


public slots:
    void newAVMuxData(QByteArray avmuxArray);

signals:
    void newImage(QImage image, qint64 timestamp);
    void newRefreshImage(QImage image, qint64 timestamp);
    void newAudioSamples(QByteArray dataArray, qint64 timestamp, int rate, int channels, int size);
    void newMuxData(QByteArray dataArray, qint64 timestamp);

protected:
    void initThread();
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    void processMJPPCM(SNC_RECORD_AVMUX *avmux, int length);
    void processRTP(SNC_RECORD_AVMUX *avmux, int length);
    void processAVData(QByteArray avmuxArray);

    bool newPipelines(SNC_AVPARAMS *avParams);
    void putRTPVideoCaps(unsigned char *data, int length);
    void putRTPAudioCaps(unsigned char *data, int length);
    void putVideoData(const unsigned char *data, int length);
    void putAudioData(const unsigned char *data, int length);

    void deletePipelines();

    int m_timer;

    QMutex m_avmuxLock;
    QQueue <QByteArray> m_avmuxQ;

    QMutex m_videoLock;
    QQueue <GstBuffer *> m_videoSinkQ;

    bool m_enableAudio;
    QMutex m_audioLock;
    QQueue <GstBuffer *> m_audioSinkQ;

    qint64 m_lastVideoTimestamp;
    qint64 m_lastAudioTimestamp;

    // gstreamer stuff

    SNC_AVPARAMS m_avParams;
    SNC_AVPARAMS m_oldAvParams;

    QMutex m_lock;

    GstElement *m_appVideoSink;
    GstElement *m_appAudioSink;
    GstElement *m_appVideoSrc;
    GstElement *m_appAudioSrc;

    bool m_waitingForVideoCaps;
    bool m_waitingForAudioCaps;

    qint64 m_videoOffset;
    qint64 m_audioOffset;

    bool m_pipelinesActive;

    gint m_videoBusWatch;
    gint m_audioBusWatch;

};

#endif // _AVMUXDECODE_H_

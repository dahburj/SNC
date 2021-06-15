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


#ifndef RTSPIF_H
#define RTSPIF_H

#include "CameraClient.h"
#include "SNCUtils.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#define	RTSP_BGND_INTERVAL              (SNC_CLOCKS_PER_SEC/50)
#define RTSP_NOFRAME_TIMEOUT            (SNC_CLOCKS_PER_SEC * 10)
#define RTSP_PIPELINE_RESET_SV3CB01     (SNC_CLOCKS_PER_SEC * 60 * 60 * 2)
#define RTSP_PIPELINE_RESET_SV3CB06     (SNC_CLOCKS_PER_SEC * 60 * 60 * 12)
#define RTSP_PIPELINE_RESET_FOSCAM      (SNC_CLOCKS_PER_SEC * 60 * 60 * 24)

class RTSPIF : public SNCThread
{
	Q_OBJECT

public:
	RTSPIF();
	virtual ~RTSPIF();

	QSize getImageSize();

    // gstreamer callbacks

    void processVideoSinkData();
    void processVideoPrerollData();

	GstElement *m_pipeline;
 
public slots:
    void newCamera();

signals:
    void newJpeg(QByteArray jpeg);
    void cameraState(QString status);
    void videoFormat(int width, int height, int frameRate);

protected:
	 void timerEvent(QTimerEvent *event);
	 void initThread();
	 void finishThread();

private:
	bool open();
	void close();
    bool newPipeline();
    void deletePipeline();
	int getCapsValue(const QString &caps, const QString& key, const QString& termChar);

	bool m_pipelineActive;
    GstElement *m_appVideoSink;
    GstPad *m_appVideoSinkPad;
    QMutex m_videoLock;
    gint m_videoBusWatch;
	QString m_caps;
	QString m_widthPrefix;
	QString m_heightPrefix;
	QString m_frameratePrefix;
 
	QString m_IPAddress;
	int m_port;
	QString m_user;
	QString m_pw;

    QString m_cameraType;

	int m_controlState;										// control connection state
	int m_width;
	int m_height;
	int m_frameRate;
    int m_frameSize;

	QImage m_lastImage;										// used to store last image for when required
	int m_frameCount;										// number of frames received

	QMutex m_netCamLock;
	int m_timer;											// ID for background timer

    qint64 m_lastFrameTime;
    bool m_firstFrame;
    qint64 m_startTime;
};

#endif // RTSPIF_H

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

#ifndef CAMERACLIENT_H
#define CAMERACLIENT_H

#include <qimage.h>
#include <qmutex.h>

#include "SNCAVDefs.h"
#include "ChangeDetector.h"
#include "SNCEndpoint.h"

//  Settings keys

//----------------------------------------------------------
//  Camera group

#define CAMERA_GROUP                    "CameraGroup"

#define	CAMERA_CAMERA                   "CameraDevice"
#define	CAMERA_WIDTH                    "CameraWidth"
#define	CAMERA_HEIGHT                   "CameraHeight"
#define	CAMERA_FRAMERATE                "CameraFrameRate"

//  For netcams

#define CAMERA_IPADDRESS                "CameraIPAddress"   // camera IP address
#define CAMERA_USERNAME                 "CameraUsername"    // username
#define CAMERA_PASSWORD                 "CameraPassword"    // password
#define CAMERA_TCPPORT                  "CameraTcpport"     // TCP port to use

#define CAMERA_DEFAULT_DEVICE           "<default device>"

// For RTSP cameras

#define RTSP_CAMERA_TYPE                "CameraType"                    // type of camera
#define	RTSP_CAMERA_IPADDRESS           "CameraIPAddress"				// camera IP address
#define	RTSP_CAMERA_USERNAME            "CameraUsername"				// username
#define	RTSP_CAMERA_PASSWORD            "CameraPassword"				// password
#define	RTSP_CAMERA_TCPPORT             "CameraTcpport"					// TCP port to use

//  RTSP camera types

#define RTSP_CAMERA_TYPE_FOSCAM         "Foscam"
#define RTSP_CAMERA_TYPE_SV3CB01        "SV3C B01"
#define RTSP_CAMERA_TYPE_SV3CB06        "SV3C B06"

//----------------------------------------------------------
//  Stream group

//  group name for stream-related entries

#define	CAMERACLIENT_STREAM_GROUP                  "StreamGroup"

#define	CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL     "HighRateMinInterval"
#define	CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL     "HighRateMaxInterval"
#define	CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL    "HighRateNullInterval"
#define CAMERACLIENT_GENERATE_LOWRATE              "GenerateLowRate"
#define CAMERACLIENT_LOWRATE_HALFRES               "LowRateHalfRes"
#define CAMERACLIENT_LOWRATEVIDEO_MININTERVAL      "LowRateMinInterval"
#define CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL      "LowRateMaxInterval"
#define CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL     "LowRateNullInterval"
#define CAMERACLIENT_GENERATE_RAW                  "GenerateRaw"

//----------------------------------------------------------
//	Motion group

// group name for motion detection entries

#define	CAMERACLIENT_MOTION_GROUP                  "MotionGroup"

#define CAMERACLIENT_MOTION_TILESTOSKIP            "MotionTilesToSkip"
#define CAMERACLIENT_MOTION_INTERVALSTOSKIP        "MotionIntervalsToSkip"
#define	CAMERACLIENT_MOTION_MIN_DELTA              "MotionMinDelta"
#define	CAMERACLIENT_MOTION_MIN_NOISE              "MotionMinNoise"

// interval between frames checked for deltas in mS. 0 means never check - always send image

#define CAMERACLIENT_MOTION_DELTA_INTERVAL         "MotionDeltaInterval"

//  length of preroll. 0 turns off the feature

#define CAMERACLIENT_MOTION_PREROLL                "MotionPreroll"

// length of postroll. 0 turns off the feature

#define CAMERACLIENT_MOTION_POSTROLL               "MotionPostroll"

// maximum rate - 120 per second (allows for 4x rate during preroll send)

#define	CAMERA_IMAGE_INTERVAL   ((qint64)SNC_CLOCKS_PER_SEC/120)

typedef struct
{
    QByteArray data;                                        // the data
    qint64 timestamp;                                       // the timestamp
    int param;                                              // param for frame
} PREROLL;

typedef struct
{
    QByteArray data;                                        // the data
    qint64 timestamp;                                       // the timestamp
} CLIENT_QUEUEDATA;

// These defines are for the motion sequence state machine

#define CAMERACLIENT_STATE_IDLE            0                   // waiting for a motion event
#define CAMERACLIENT_STATE_PREROLL         1                   // sending preroll saved frames from the queue
#define CAMERACLIENT_STATE_INSEQUENCE      2                   // sending frames as normal during motion sequence
#define CAMERACLIENT_STATE_POSTROLL        3                   // sending the postroll frames
#define CAMERACLIENT_STATE_CONTINUOUS      4                   // no motion detect so continuous sending

class CameraClient : public SNCEndpoint
{
    Q_OBJECT

public:
    CameraClient(QObject *parent);
    virtual ~CameraClient();
    int getFrameCount();
    int getAudioSampleCount();

public slots:
    void newStream();
    void newJpeg(QByteArray);
    void newAudio(QByteArray);
    void videoFormat(int width, int height, int framerate);
    void audioFormat(int sampleRate, int channels, int sampleSize);

signals:
    void newHighRateVideoData(int bytes);
    void newLowRateVideoData(int bytes);
    void newHighRateAudioData(int bytes);
    void newLowRateAudioData(int bytes);

protected:
    void appClientInit();
    void appClientExit();
    void appClientReceiveE2E(int servicePort, SNC_EHEAD *header, int length); // process an E2E message
    void appClientConnected();								// called when endpoint is connected to SyntroControl
    void appClientBackground();
    void processAudioQueue();  								// processes the audio queue

    int m_avmuxPortRaw;                                     // the local port assigned to the raw avmux service
    int m_avmuxPortHighRate;								// the local port assigned to the high rate avmux service
    int m_avmuxPortLowRate;                                 // the local port assigned to the low rate avmux service
    int m_controlPort;										// the local port assigned to the control E2E service

private:
    void processAVQueueMJPPCM();                            // processes the video and audio data in MJPPCM mode
    void sendRawQueue();                                    // sends raw queue if enabled
    void sendHeartbeatFrameMJPPCM(qint64 now, const QByteArray& jpeg);  // see if need to send null or full frame
    bool sendAVMJPPCM(qint64 now, int param, bool checkMotion); // sends a audio and video if there is any. Returns true if motion
    void sendNullFrameMJPPCM(qint64 now, bool highRate);    // sends a null frame
    void sendPrerollMJPPCM(bool highRate);                  // sends a preroll audio and/or video frame

    bool m_generateRaw;
    bool m_generateLowRate;
    bool m_lowRateHalfRes;

    void checkForMotion(qint64 now, QByteArray& jpeg);      // checks to see if a motion event has occured
    bool dequeueVideoFrame(QByteArray& videoData, qint64& timestamp);
    bool dequeueAudioFrame(QByteArray& audioData, qint64& timestamp);
    void clearVideoQueue();
    void clearAudioQueue();
    void clearQueues();
    void ageOutPrerollQueues(qint64 now);

    qint64 m_lastChangeTime;                                // time last frame change was detected
    bool m_imageChanged;                                    // if image has changed

    qint64 m_lastHighRateFrameTime;                         // last time any frame was sent - null or full
    qint64 m_lastLowRateFrameTime;                          // last time any frame was sent - null or full - on low rate
    qint64 m_lastHighRatePrerollFrameTime;                  // last time a frame was added to the preroll
    qint64 m_lastLowRatePrerollFrameTime;                   // last time a low rate frame was added to the preroll
    qint64 m_lastHighRateFullFrameTime;                     // last time a full frame was sent
    qint64 m_lastLowRateFullFrameTime;                      // last time a full frame was sent on low rate

    qint64 m_highRateMinInterval;                           // min interval between frames
    qint64 m_highRateFullFrameMaxInterval;                  // max interval between full frame refreshes
    qint64 m_highRateNullFrameInterval;                     // max interval between null or real frames

    qint64 m_lowRateMinInterval;                            // low rate min interval between frames
    qint64 m_lowRateFullFrameMaxInterval;                   // low rate max interval between full frame refreshes
    qint64 m_lowRateNullFrameInterval;                      // low rate max interval between null or real frames

    int m_minDelta;                                         // min image change required
    int m_minNoise;                                         // min chunk change for its delta to be counted
    qint64 m_deltaInterval;                                 // interval between frames checked for motion
    qint64 m_preroll;                                       // length in mS of preroll
    qint64 m_postroll;                                      // length in mS of postroll

    int m_tilesToSkip;                                      // number of tiles in an interval to skip
    int m_intervalsToSkip;                                  // number of intervals to skip

    int m_sequenceState;                                    // the state of the motion sequence state machine
    qint64 m_postrollStart;                                 // time that the postroll started

    QQueue<PREROLL *> m_videoPrerollQueue;                  // queue of PREROLL structures for the video preroll queue
    QQueue<PREROLL *> m_videoLowRatePrerollQueue;           // queue of PREROLL structures for the low rate video preroll queue
    QQueue<PREROLL *> m_audioPrerollQueue;                  // queue of PREROLL structures for the audio preroll queue
    QQueue<PREROLL *> m_audioLowRatePrerollQueue;           // queue of PREROLL structures for the low rate audio preroll queue

    qint64 m_lastDeltaTime;                                 // when the delta was last checked

    QQueue <QByteArray> m_videoRawQ;
    QQueue <CLIENT_QUEUEDATA *> m_videoFrameQ;
    QMutex m_videoQMutex;

    QQueue <CLIENT_QUEUEDATA *> m_audioFrameQ;
    QMutex m_audioQMutex;

    ChangeDetector m_cd;                                    // the change detector instance

    int m_frameCount;
    QMutex m_frameCountLock;

    int m_audioSampleCount;
    QMutex m_audioSampleLock;

    int m_recordIndex;                                      // increments for every avmux record constructed

    SNC_AVPARAMS m_avParams;                                // used to hold stream parameters

    bool m_gotVideoFormat;
    bool m_gotAudioFormat;
};

#endif // CAMERACLIENT_H


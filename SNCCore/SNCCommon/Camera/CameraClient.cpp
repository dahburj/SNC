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

#include "CameraClient.h"
#include "SNCUtils.h"

#include <qbuffer.h>
#include <qdebug.h>


//#define STATE_DEBUG_ENABLE

#ifdef STATE_DEBUG_ENABLE
#define STATE_DEBUG(x) qDebug() << x
#else
#define STATE_DEBUG(x)
#endif

#define TAG "CameraClient"

CameraClient::CameraClient(QObject *) : SNCEndpoint(CAMERA_IMAGE_INTERVAL, "CameraClient")
{
    m_avmuxPortRaw = -1;
    m_avmuxPortHighRate = -1;
    m_avmuxPortLowRate = -1;
    m_controlPort = -1;
    m_sequenceState = CAMERACLIENT_STATE_IDLE;
    m_frameCount = 0;
    m_audioSampleCount = 0;
    m_recordIndex = 0;

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_STREAM_GROUP);

    if (!settings->contains(CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL))
        settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL, "100");

    if (!settings->contains(CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL))
        settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL, "10000");

    if (!settings->contains(CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL))
        settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL, "2000");

    if (!settings->contains(CAMERACLIENT_GENERATE_LOWRATE))
        settings->setValue(CAMERACLIENT_GENERATE_LOWRATE, true);

    if (!settings->contains(CAMERACLIENT_LOWRATE_HALFRES))
        settings->setValue(CAMERACLIENT_LOWRATE_HALFRES, true);

     if (!settings->contains(CAMERACLIENT_LOWRATEVIDEO_MININTERVAL))
        settings->setValue(CAMERACLIENT_LOWRATEVIDEO_MININTERVAL, "6000");

    if (!settings->contains(CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL))
        settings->setValue(CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL, "60000");

    if (!settings->contains(CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL))
        settings->setValue(CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL, "8000");

    if (!settings->contains(CAMERACLIENT_GENERATE_RAW))
        settings->setValue(CAMERACLIENT_GENERATE_RAW, false);

    settings->endGroup();

    settings->beginGroup(CAMERACLIENT_MOTION_GROUP);

    if (!settings->contains(CAMERACLIENT_MOTION_TILESTOSKIP))
        settings->setValue(CAMERACLIENT_MOTION_TILESTOSKIP, "0");

    if (!settings->contains(CAMERACLIENT_MOTION_INTERVALSTOSKIP))
        settings->setValue(CAMERACLIENT_MOTION_INTERVALSTOSKIP, "0");

    if (!settings->contains(CAMERACLIENT_MOTION_MIN_DELTA))
        settings->setValue(CAMERACLIENT_MOTION_MIN_DELTA, "400");

    if (!settings->contains(CAMERACLIENT_MOTION_MIN_NOISE))
        settings->setValue(CAMERACLIENT_MOTION_MIN_NOISE, "40");

    if (!settings->contains(CAMERACLIENT_MOTION_DELTA_INTERVAL))
        settings->setValue(CAMERACLIENT_MOTION_DELTA_INTERVAL, "200");

    if (!settings->contains(CAMERACLIENT_MOTION_PREROLL))
        settings->setValue(CAMERACLIENT_MOTION_PREROLL, "1000");

    if (!settings->contains(CAMERACLIENT_MOTION_POSTROLL))
        settings->setValue(CAMERACLIENT_MOTION_POSTROLL, "1000");

    settings->endGroup();

    delete settings;
}

CameraClient::~CameraClient()
{
}

int CameraClient::getFrameCount()
{
    int count;

    QMutexLocker lock(&m_frameCountLock);

    count = m_frameCount;
    m_frameCount = 0;
    return count;
}

int CameraClient::getAudioSampleCount()
{
    int count;

    QMutexLocker lock(&m_audioSampleLock);

    count = m_audioSampleCount;
    m_audioSampleCount = 0;
    return count;
}

void CameraClient::ageOutPrerollQueues(qint64 now)
{
    while (!m_videoPrerollQueue.empty()) {
        if ((now - m_videoPrerollQueue.head()->timestamp) < m_preroll)
            break;

        delete m_videoPrerollQueue.dequeue();
    }

    while (!m_audioPrerollQueue.empty()) {
        if ((now - m_audioPrerollQueue.head()->timestamp) < m_preroll)
            break;

        delete m_audioPrerollQueue.dequeue();
    }

    while (!m_videoLowRatePrerollQueue.empty()) {
        if ((now - m_videoLowRatePrerollQueue.head()->timestamp) < m_preroll)
            break;

        delete m_videoLowRatePrerollQueue.dequeue();
    }

    while (!m_audioLowRatePrerollQueue.empty()) {
        if ((now - m_audioLowRatePrerollQueue.head()->timestamp) < m_preroll)
            break;

        delete m_audioLowRatePrerollQueue.dequeue();
    }
}

//----------------------------------------------------------
//
//  MJPPCM processing

void CameraClient::processAVQueueMJPPCM()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QByteArray jpeg;
    QByteArray audioFrame;
    PREROLL *preroll;
    QString stateString;
    qint64 timestamp;

    switch (m_sequenceState) {
        // waiting for a motion event
        case CAMERACLIENT_STATE_IDLE:

        ageOutPrerollQueues(now);

        // if there is a frame, put on preroll queue and check for motion


        if (dequeueVideoFrame(jpeg, timestamp) && SNCUtils::timerExpired(now, m_lastHighRatePrerollFrameTime, m_highRateMinInterval)) {
            m_lastHighRatePrerollFrameTime = now;
            preroll = new PREROLL;
            preroll->data = jpeg;
            preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_videoPrerollQueue.enqueue(preroll);

            if (m_generateLowRate && SNCUtils::timerExpired(now, m_lastLowRatePrerollFrameTime, m_lowRateMinInterval)) {
                m_lastLowRatePrerollFrameTime = now;
                preroll = new PREROLL;
                preroll->data = jpeg;
                preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
                preroll->timestamp = timestamp;
                m_videoLowRatePrerollQueue.enqueue(preroll);
            }

            // now check for motion if it's time

            if ((now - m_lastDeltaTime) > m_deltaInterval)
                checkForMotion(now, jpeg);
            if (m_imageChanged) {
                m_sequenceState = CAMERACLIENT_STATE_PREROLL; // send the preroll frames
                stateString = QString("STATE_PREROLL: queue size %1").arg(m_videoPrerollQueue.size());
                STATE_DEBUG(stateString);
            } else {
                sendHeartbeatFrameMJPPCM(now, jpeg);
            }
        }
        if (dequeueAudioFrame(audioFrame, timestamp)) {
            preroll = new PREROLL;
            preroll->data = audioFrame;
            preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
            preroll->timestamp = timestamp;
            m_audioPrerollQueue.enqueue(preroll);

            if (m_generateLowRate) {
                preroll = new PREROLL;
                preroll->data = audioFrame;
                preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
                preroll->timestamp = timestamp;
                m_audioLowRatePrerollQueue.enqueue(preroll);
            }
        }
        break;

        // sending the preroll queue
        case CAMERACLIENT_STATE_PREROLL:
        if (clientIsServiceActive(m_avmuxPortHighRate)) {
            if (clientClearToSend(m_avmuxPortHighRate) && (!m_videoPrerollQueue.empty() || !m_audioPrerollQueue.empty())) {
                if (SNCUtils::timerExpired(now, m_lastHighRateFrameTime, m_highRateMinInterval / 4 + 1)) {
                    sendPrerollMJPPCM(true);
                }
            }
        } else {
            while (!m_videoPrerollQueue.empty())             // clear queue if connection not active
                delete m_videoPrerollQueue.dequeue();
            while (!m_audioPrerollQueue.empty())             // clear queue if connection not active
                delete m_audioPrerollQueue.dequeue();
        }
        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
            if ((!m_videoLowRatePrerollQueue.empty() || !m_audioLowRatePrerollQueue.empty())) {
                if (SNCUtils::timerExpired(now, m_lastLowRateFrameTime, m_lowRateMinInterval / 4 + 1)) {
                    sendPrerollMJPPCM(false);
                }
            }
        } else {
            while (!m_videoLowRatePrerollQueue.empty())       // clear queue if connection not active
                delete m_videoLowRatePrerollQueue.dequeue();
            while (!m_audioLowRatePrerollQueue.empty())       // clear queue if connection not active
                delete m_audioLowRatePrerollQueue.dequeue();
        }
        if (m_videoPrerollQueue.empty() && m_audioPrerollQueue.empty() &&
                m_videoLowRatePrerollQueue.empty() && (m_audioLowRatePrerollQueue.empty())) {
            m_sequenceState = CAMERACLIENT_STATE_INSEQUENCE;
            STATE_DEBUG("STATE_INSEQUENCE");
            m_lastChangeTime = now;                             // in case pre-roll sending took a while
        }

        // keep putting frames on preroll queue while sending real preroll

        if (dequeueVideoFrame(jpeg, timestamp) && SNCUtils::timerExpired(now, m_lastHighRatePrerollFrameTime, m_highRateMinInterval)) {
                m_lastHighRatePrerollFrameTime = now;
                preroll = new PREROLL;
                preroll->data = jpeg;
                preroll->param = SNC_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_videoPrerollQueue.enqueue(preroll);
                if (m_generateLowRate && SNCUtils::timerExpired(now, m_lastLowRatePrerollFrameTime, m_lowRateMinInterval)) {
                    m_lastLowRatePrerollFrameTime = now;
                    preroll = new PREROLL;
                    preroll->data = jpeg;
                    preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
                    preroll->timestamp = timestamp;
                    m_videoLowRatePrerollQueue.enqueue(preroll);
                }
            }

            if (dequeueAudioFrame(audioFrame, timestamp)) {
                preroll = new PREROLL;
                preroll->data = audioFrame;
                preroll->param = SNC_RECORDHEADER_PARAM_NORMAL;
                preroll->timestamp = timestamp;
                m_audioPrerollQueue.enqueue(preroll);

                if (m_generateLowRate) {
                    preroll = new PREROLL;
                    preroll->data = audioFrame;
                    preroll->param = SNC_RECORDHEADER_PARAM_PREROLL;
                    preroll->timestamp = timestamp;
                    m_audioLowRatePrerollQueue.enqueue(preroll);
                }
            }
            break;

        // in the motion sequence
        case CAMERACLIENT_STATE_INSEQUENCE:
            if (!sendAVMJPPCM(now, SNC_RECORDHEADER_PARAM_NORMAL, true)) {
                // no motion detected
                m_sequenceState = CAMERACLIENT_STATE_POSTROLL; // no motion so go into postroll state
                m_lastChangeTime = now;                        // this sets the start tiem for the postroll
                STATE_DEBUG("STATE_POSTROLL");
            }
            break;

        // handle the post roll stage
        case CAMERACLIENT_STATE_POSTROLL:

            if (SNCUtils::timerExpired(now, m_lastChangeTime, m_postroll)) {
                // postroll complete
                m_sequenceState = CAMERACLIENT_STATE_IDLE;
                m_lastHighRatePrerollFrameTime = m_lastHighRateFrameTime;
                m_lastLowRatePrerollFrameTime = m_lastLowRateFrameTime;
                STATE_DEBUG("STATE_IDLE");
                break;
            }

            // see if anything to send

            if (sendAVMJPPCM(now, SNC_RECORDHEADER_PARAM_POSTROLL, true)) {
                // motion detected again
                m_sequenceState = CAMERACLIENT_STATE_INSEQUENCE;
                STATE_DEBUG("Returning to STATE_INSEQUENCE");
            }
            break;

        // in the motion sequence
        case CAMERACLIENT_STATE_CONTINUOUS:
            sendAVMJPPCM(now, SNC_RECORDHEADER_PARAM_NORMAL, false);
            break;
    }
}

void CameraClient::sendRawQueue()
{
    QMutexLocker lock(&m_videoQMutex);

    while (!m_videoRawQ.empty()) {
        QByteArray frame = m_videoRawQ.dequeue();
        if ((m_avmuxPortRaw != -1) && clientIsServiceActive(m_avmuxPortRaw) && clientClearToSend(m_avmuxPortRaw))
        {
            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortRaw, sizeof(SNC_RECORD_AVMUX) + frame.size());
            SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(avHead, &m_avParams, SNC_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, frame.size(), 0);
            SNCUtils::convertInt64ToUC8(QDateTime::currentMSecsSinceEpoch(), avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            memcpy(ptr, frame.data(), frame.size());
            clientSendMessage(m_avmuxPortRaw, multiCast, sizeof(SNC_RECORD_AVMUX) + frame.size(), SNCLINK_LOWPRI);
        }
    }
}

void CameraClient::sendPrerollMJPPCM(bool highRate)
{
    PREROLL *videoPreroll = NULL;
    PREROLL *audioPreroll = NULL;
    int videoSize = 0;
    int audioSize = 0;

    if (highRate) {
        if (!m_videoPrerollQueue.empty()) {
            videoPreroll = m_videoPrerollQueue.dequeue();
            videoSize = videoPreroll->data.size();
            m_lastHighRateFrameTime = QDateTime::currentMSecsSinceEpoch();
        }
        if (!m_audioPrerollQueue.empty()) {
            audioPreroll = m_audioPrerollQueue.dequeue();
            audioSize = audioPreroll->data.size();
        }

        if ((videoPreroll != NULL) || (audioPreroll != NULL)) {
            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SNC_RECORD_AVMUX) + videoSize + audioSize);
            SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(avHead, &m_avParams, SNC_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, videoSize, audioSize);

            if (audioPreroll != NULL)
                SNCUtils::convertInt64ToUC8(audioPreroll->timestamp, avHead->recordHeader.timestamp);
            if (videoPreroll != NULL)
                SNCUtils::convertInt64ToUC8(videoPreroll->timestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (videoSize > 0) {
                memcpy(ptr, videoPreroll->data.data(), videoSize);
                ptr += videoSize;
                emit newHighRateVideoData(videoSize);
            }

            if (audioSize > 0) {
                memcpy(ptr, audioPreroll->data.data(), audioSize);
                emit newHighRateAudioData(audioSize);
            }

            int length = sizeof(SNC_RECORD_AVMUX) + videoSize + audioSize;
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SNCLINK_MEDPRI);
        }
    } else {
        if (!m_videoLowRatePrerollQueue.empty()) {
            videoPreroll = m_videoLowRatePrerollQueue.dequeue();
            videoSize = videoPreroll->data.size();
            m_lastLowRateFrameTime = SNCUtils::clock();
        }
        if (!m_audioLowRatePrerollQueue.empty()) {
            audioPreroll = m_audioLowRatePrerollQueue.dequeue();
            audioSize = audioPreroll->data.size();
        }

        if ((videoPreroll != NULL) || (audioPreroll == NULL)) {
            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SNC_RECORD_AVMUX) + videoSize + audioSize);
            SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(avHead, &m_avParams, SNC_RECORDHEADER_PARAM_PREROLL, m_recordIndex++, 0, videoSize, audioSize);

            if (audioPreroll != NULL)
                SNCUtils::convertInt64ToUC8(audioPreroll->timestamp, avHead->recordHeader.timestamp);
            if (videoPreroll != NULL)
                SNCUtils::convertInt64ToUC8(videoPreroll->timestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (videoSize > 0) {
                memcpy(ptr, videoPreroll->data.data(), videoSize);
                ptr += videoSize;
                emit newLowRateVideoData(videoSize);
            }

            if (audioSize > 0) {
                memcpy(ptr, audioPreroll->data.data(), audioSize);
                emit newLowRateAudioData(audioSize);
            }

            int length = sizeof(SNC_RECORD_AVMUX) + videoSize + audioSize;
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SNCLINK_MEDPRI);
        }
    }

    if (videoPreroll != NULL)
        delete videoPreroll;
    if (audioPreroll != NULL)
        delete audioPreroll;
}

bool CameraClient::sendAVMJPPCM(qint64 now, int param, bool checkMotion)
{
    qint64 videoTimestamp;
    qint64 audioTimestamp;
    QByteArray audioFrame;
    bool highRateExpired;
    bool lowRateExpired;
    QImage frame;
    QImage smallFrame;
    QByteArray jpeg;
    QByteArray largeJpeg;
    QByteArray smallJpeg;
    bool videoValid = false;
    bool audioValid = false;


    highRateExpired = SNCUtils::timerExpired(now, m_lastHighRateFullFrameTime, m_highRateMinInterval);
    lowRateExpired = SNCUtils::timerExpired(now, m_lastLowRateFullFrameTime, m_lowRateMinInterval);

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate)) {
        if (!lowRateExpired) {
            if (SNCUtils::timerExpired(now, m_lastLowRateFrameTime, m_lowRateNullFrameInterval)) {
                sendNullFrameMJPPCM(now, false);            // in case very long time
            }
        }
    }

    if (highRateExpired) {
        videoValid = dequeueVideoFrame(jpeg, videoTimestamp);
        audioValid = dequeueAudioFrame(audioFrame, audioTimestamp);
    } else {
        return false;
    }

    if (videoValid || audioValid) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate) && (highRateExpired || audioValid)) {

            if (highRateExpired)
                largeJpeg = jpeg;
            else
                largeJpeg = QByteArray();

            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SNC_RECORD_AVMUX) + largeJpeg.size() + audioFrame.size());
            SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, largeJpeg.size(), audioFrame.size());
            if (audioValid)
                SNCUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SNCUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (largeJpeg.size() > 0) {
                memcpy(ptr, largeJpeg.data(), largeJpeg.size());
                m_lastHighRateFullFrameTime = m_lastHighRateFrameTime = now;
                ptr += largeJpeg.size();
                emit newHighRateVideoData(largeJpeg.size());
            }

            if (audioFrame.size() > 0) {
                memcpy(ptr, audioFrame.data(), audioFrame.size());
                emit newHighRateAudioData(audioFrame.size());
            }

            int length = sizeof(SNC_RECORD_AVMUX) + largeJpeg.size() + audioFrame.size();
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SNCLINK_MEDPRI);
        }

        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate) &&
                (lowRateExpired || audioValid)) {
            if (lowRateExpired)
                m_lastLowRateFrameTime = now;

            if (lowRateExpired) {
                if (m_lowRateHalfRes) {
                    frame = QImage::fromData(jpeg, "JPEG");
                    smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
                    QBuffer buffer(&smallJpeg);
                    buffer.open(QIODevice::WriteOnly);
                    smallFrame.save(&buffer, "JPG");
                } else {
                    smallJpeg = jpeg;
                }
            } else {
                smallJpeg = QByteArray();
            }

            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SNC_RECORD_AVMUX) + smallJpeg.size() + audioFrame.size());
            SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(avHead, &m_avParams, param, m_recordIndex++, 0, smallJpeg.size(), audioFrame.size());
            if (audioValid)
                SNCUtils::convertInt64ToUC8(audioTimestamp, avHead->recordHeader.timestamp);
            if (videoValid)
                SNCUtils::convertInt64ToUC8(videoTimestamp, avHead->recordHeader.timestamp);

            unsigned char *ptr = (unsigned char *)(avHead + 1);

            if (smallJpeg.size() > 0) {
                memcpy(ptr, smallJpeg.data(), smallJpeg.size());
                m_lastLowRateFullFrameTime = m_lastLowRateFrameTime = now;
                ptr += smallJpeg.size();
                emit newLowRateVideoData(smallJpeg.size());
            }

            if (audioFrame.size() > 0) {
                memcpy(ptr, audioFrame.data(), audioFrame.size());
                emit newLowRateAudioData(audioFrame.size());
            }

            int length = sizeof(SNC_RECORD_AVMUX) + smallJpeg.size() + audioFrame.size();
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SNCLINK_MEDPRI);
        }
    }

    if ((jpeg.size() > 0) && checkMotion) {
        if ((now - m_lastDeltaTime) > m_deltaInterval)
            checkForMotion(now, jpeg);
        return m_imageChanged;                              // image may have changed
    }
    return false;                                           // change not processed
}


void CameraClient::sendNullFrameMJPPCM(qint64 now, bool highRate)
{
    if (highRate) {
        if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate)) {
            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SNC_RECORD_AVMUX));
            SNC_RECORD_AVMUX *videoHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(videoHead, &m_avParams, SNC_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
            int length = sizeof(SNC_RECORD_AVMUX);
            clientSendMessage(m_avmuxPortHighRate, multiCast, length, SNCLINK_LOWPRI);
            m_lastHighRateFrameTime = now;
        }
    } else {

        if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate)) {
            SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SNC_RECORD_AVMUX));
            SNC_RECORD_AVMUX *videoHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
            SNCUtils::avmuxHeaderInit(videoHead, &m_avParams, SNC_RECORDHEADER_PARAM_NOOP, m_recordIndex++, 0, 0, 0);
            int length = sizeof(SNC_RECORD_AVMUX);
            clientSendMessage(m_avmuxPortLowRate, multiCast, length, SNCLINK_LOWPRI);
            m_lastLowRateFrameTime = now;
        }
    }
}

void CameraClient::sendHeartbeatFrameMJPPCM(qint64 now, const QByteArray& jpeg)
{
    QImage frame;
    QImage smallFrame;
    QByteArray smallJpeg;

    if (clientIsServiceActive(m_avmuxPortHighRate) && clientClearToSend(m_avmuxPortHighRate) &&
            SNCUtils::timerExpired(now, m_lastHighRateFullFrameTime, m_highRateFullFrameMaxInterval)) {
        SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortHighRate, sizeof(SNC_RECORD_AVMUX) + jpeg.size());
        SNC_RECORD_AVMUX *videoHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
        SNCUtils::avmuxHeaderInit(videoHead, &m_avParams, SNC_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, jpeg.size(), 0);
        memcpy((unsigned char *)(videoHead + 1), jpeg.data(), jpeg.size());
        int length = sizeof(SNC_RECORD_AVMUX) + jpeg.size();
        clientSendMessage(m_avmuxPortHighRate, multiCast, length, SNCLINK_LOWPRI);
        emit newHighRateVideoData(jpeg.size());
        m_lastHighRateFrameTime = m_lastHighRateFullFrameTime = now;
    }

    if (m_generateLowRate && clientIsServiceActive(m_avmuxPortLowRate) && clientClearToSend(m_avmuxPortLowRate) &&
            SNCUtils::timerExpired(now, m_lastLowRateFullFrameTime, m_lowRateFullFrameMaxInterval)) {
        if (m_lowRateHalfRes) {
            frame = QImage::fromData(jpeg, "JPEG");
            smallFrame = frame.scaled(frame.width() / 2, frame.height() / 2);
            QBuffer buffer(&smallJpeg);
            buffer.open(QIODevice::WriteOnly);
            smallFrame.save(&buffer, "JPG");
        } else {
            smallJpeg = jpeg;
        }

        SNC_EHEAD *multiCast = clientBuildMessage(m_avmuxPortLowRate, sizeof(SNC_RECORD_AVMUX) + smallJpeg.size());
        SNC_RECORD_AVMUX *videoHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
        SNCUtils::avmuxHeaderInit(videoHead, &m_avParams, SNC_RECORDHEADER_PARAM_REFRESH, m_recordIndex++, 0, smallJpeg.size(), 0);
        memcpy((unsigned char *)(videoHead + 1), smallJpeg.data(), smallJpeg.size());
        int length = sizeof(SNC_RECORD_AVMUX) + smallJpeg.size();
        emit newLowRateVideoData(smallJpeg.size());
        clientSendMessage(m_avmuxPortLowRate, multiCast, length, SNCLINK_LOWPRI);
        m_lastLowRateFrameTime = m_lastLowRateFullFrameTime = now;
    }

    if (SNCUtils::timerExpired(now, m_lastHighRateFrameTime, m_highRateNullFrameInterval))
        sendNullFrameMJPPCM(now, true);
    if (SNCUtils::timerExpired(now, m_lastLowRateFrameTime, m_lowRateNullFrameInterval))
        sendNullFrameMJPPCM(now, false);
}



//----------------------------------------------------------


void CameraClient::checkForMotion(qint64 now, QByteArray& jpeg)
{
    // these five lines are a hack because my jpeg decoder doesn't work for webcam jpegs
    // Qt produces a jpeg that my code likes

 /*   QImage frame;
    frame.loadFromData(jpeg, "JPEG");
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG");
*/
    m_imageChanged = m_cd.imageChanged(jpeg);
    if (m_imageChanged)
        m_lastChangeTime = now;
    m_lastDeltaTime = now;
}


void CameraClient::appClientInit()
{
    newStream();
}

void CameraClient::appClientExit()
{
    clearQueues();
}

void CameraClient::appClientBackground()
{
    sendRawQueue();
    processAVQueueMJPPCM();
}

void CameraClient::appClientConnected()
{
    clearQueues();
}

void CameraClient::appClientReceiveE2E(int servicePort, SNC_EHEAD *header, int /*length*/)
{
    if (servicePort != m_controlPort) {
        SNCUtils::logWarn(TAG, QString("Received E2E on incorrect port %1").arg(m_controlPort));
        free(header);
        return;
    }

    free(header);
}

bool CameraClient::dequeueVideoFrame(QByteArray& videoData, qint64& timestamp)
{
    QMutexLocker lock(&m_videoQMutex);

    if (m_videoFrameQ.empty())
        return false;

    CLIENT_QUEUEDATA *qd = m_videoFrameQ.dequeue();
    videoData = qd->data;
    timestamp = qd->timestamp;
    delete qd;
    return true;
}

bool CameraClient::dequeueAudioFrame(QByteArray& audioData, qint64& timestamp)
{
    QMutexLocker lock(&m_audioQMutex);

    if (m_audioFrameQ.empty())
        return false;

    CLIENT_QUEUEDATA *qd = m_audioFrameQ.dequeue();
    audioData = qd->data;
    timestamp = qd->timestamp;
    delete qd;
    return true;
}

void CameraClient::clearVideoQueue(){
    QMutexLocker lock(&m_videoQMutex);

    m_videoRawQ.clear();

    while (!m_videoFrameQ.empty())
        delete m_videoFrameQ.dequeue();
}

void CameraClient::clearAudioQueue(){
    QMutexLocker lock(&m_audioQMutex);

    while (!m_audioFrameQ.empty())
        delete m_audioFrameQ.dequeue();
}

void CameraClient::clearQueues()
{
    clearVideoQueue();
    clearAudioQueue();

    while (!m_videoPrerollQueue.empty())
        delete m_videoPrerollQueue.dequeue();

    while (!m_videoLowRatePrerollQueue.empty())
        delete m_videoLowRatePrerollQueue.dequeue();

    while (!m_audioPrerollQueue.empty())
        delete m_audioPrerollQueue.dequeue();

    while (!m_audioLowRatePrerollQueue.empty())
        delete m_audioLowRatePrerollQueue.dequeue();

     if (m_deltaInterval == 0) {
        m_sequenceState = CAMERACLIENT_STATE_CONTINUOUS;    // motion detection inactive
        STATE_DEBUG("STATE_CONTINUOUS");
    } else {
        m_sequenceState = CAMERACLIENT_STATE_IDLE;          // use motion detect state machine
        STATE_DEBUG("STATE_IDLE");
    }
}



void CameraClient::newJpeg(QByteArray frame)
{
    m_videoQMutex.lock();

    if (m_videoFrameQ.count() > 5)
        delete m_videoFrameQ.dequeue();

    CLIENT_QUEUEDATA *qd = new CLIENT_QUEUEDATA;
    qd->data = frame;
    qint64 ts = QDateTime::currentMSecsSinceEpoch();
    qd->timestamp = ts;
    m_videoFrameQ.enqueue(qd);

    if (m_avmuxPortRaw != -1) {
        m_videoRawQ.enqueue(frame);
        if (m_videoRawQ.count() > 5) {
            m_videoRawQ.dequeue();
        }
    }

    m_videoQMutex.unlock();

    QMutexLocker lock(&m_frameCountLock);
    m_frameCount++;
}

void CameraClient::newAudio(QByteArray audioFrame)
{
    m_audioQMutex.lock();

    if (m_audioFrameQ.count() > 5)
        delete m_audioFrameQ.dequeue();

    CLIENT_QUEUEDATA *qd = new CLIENT_QUEUEDATA;
    qd->data = audioFrame;
    qd->timestamp = QDateTime::currentMSecsSinceEpoch();
    m_audioFrameQ.enqueue(qd);

    m_audioQMutex.unlock();

    QMutexLocker lock(&m_audioSampleLock);
    m_audioSampleCount += audioFrame.length();
}


void CameraClient::newStream()
{
    // remove the old streams

    if (m_avmuxPortRaw != -1)
        clientRemoveService(m_avmuxPortRaw);
    m_avmuxPortRaw = -1;

    if (m_avmuxPortHighRate != -1)
        clientRemoveService(m_avmuxPortHighRate);
    m_avmuxPortHighRate = -1;

    if (m_avmuxPortLowRate != -1)
        clientRemoveService(m_avmuxPortLowRate);
    m_avmuxPortLowRate = -1;

    if (m_controlPort != -1)
        clientRemoveService(m_controlPort);

    // and start the new streams

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_STREAM_GROUP);

    m_highRateMinInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL).toInt();
    m_highRateFullFrameMaxInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL).toInt();
    m_highRateNullFrameInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL).toInt();

    m_generateLowRate = settings->value(CAMERACLIENT_GENERATE_LOWRATE).toBool();
    m_generateRaw = settings->value(CAMERACLIENT_GENERATE_RAW).toBool();
    m_lowRateHalfRes = settings->value(CAMERACLIENT_LOWRATE_HALFRES).toBool();
    m_lowRateMinInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_MININTERVAL).toInt();
    m_lowRateFullFrameMaxInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL).toInt();
    m_lowRateNullFrameInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL).toInt();

    m_avmuxPortHighRate = clientAddService(SNC_STREAMNAME_AVMUX, SERVICETYPE_MULTICAST, true);
    if (m_generateLowRate)
        m_avmuxPortLowRate = clientAddService(SNC_STREAMNAME_AVMUXLR, SERVICETYPE_MULTICAST, true);

    if (m_generateRaw)
        m_avmuxPortRaw = clientAddService(SNC_STREAMNAME_AVMUXRAW, SERVICETYPE_MULTICAST, true);

    m_controlPort = clientAddService(SNC_STREAMNAME_CONTROL, SERVICETYPE_E2E, true);


    settings->endGroup();

    m_gotAudioFormat = false;
    m_gotVideoFormat = false;

    settings->beginGroup(CAMERACLIENT_MOTION_GROUP);

    m_tilesToSkip = settings->value(CAMERACLIENT_MOTION_TILESTOSKIP).toInt();
    m_intervalsToSkip = settings->value(CAMERACLIENT_MOTION_INTERVALSTOSKIP).toInt();
    m_minDelta = settings->value(CAMERACLIENT_MOTION_MIN_DELTA).toInt();
    m_minNoise = settings->value(CAMERACLIENT_MOTION_MIN_NOISE).toInt();
    m_deltaInterval = settings->value(CAMERACLIENT_MOTION_DELTA_INTERVAL).toInt();
    m_preroll = settings->value(CAMERACLIENT_MOTION_PREROLL).toInt();
    m_postroll = settings->value(CAMERACLIENT_MOTION_POSTROLL).toInt();

    settings->endGroup();

    delete settings;

    m_cd.setDeltaThreshold(m_minDelta);
    m_cd.setNoiseThreshold(m_minNoise);
    m_cd.setTilesToSkip(m_tilesToSkip);
    m_cd.setIntervalsToSkip(m_intervalsToSkip);

    qint64 now = QDateTime::currentMSecsSinceEpoch();

    m_lastHighRateFrameTime = now;
    m_lastLowRateFrameTime = now;
    m_lastHighRateFullFrameTime = now;
    m_lastLowRateFullFrameTime = now;
    m_lastHighRatePrerollFrameTime = now;
    m_lastLowRatePrerollFrameTime = now;
    m_lastChangeTime = now;
    m_imageChanged = false;

    m_cd.setUninitialized();

    clearQueues();

    m_avParams.avmuxSubtype = SNC_RECORD_TYPE_AVMUX_MJPPCM;
    m_avParams.videoSubtype = SNC_RECORD_TYPE_VIDEO_MJPEG;
    m_avParams.audioSubtype = SNC_RECORD_TYPE_AUDIO_PCM;
}

void CameraClient::videoFormat(int width, int height, int framerate)
{
    m_avParams.videoWidth = width;
    m_avParams.videoHeight = height;
    m_avParams.videoFramerate = framerate;
    m_gotVideoFormat = true;
}

void CameraClient::audioFormat(int sampleRate, int channels, int sampleSize)
{
    m_avParams.audioSampleRate = sampleRate;
    m_avParams.audioChannels = channels;
    m_avParams.audioSampleSize = sampleSize;
    m_gotAudioFormat = true;
}

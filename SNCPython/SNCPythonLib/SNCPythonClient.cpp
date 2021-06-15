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

#include "SNCPythonClient.h"
#include "SNCUtils.h"
#include "SNCPythonGlue.h"

#include "SNCDirectoryEntry.h"

#include <qdebug.h>
#include <qbuffer.h>

SNCPythonClient::SNCPythonClient(SNCPythonGlue *glue)
    : SNCEndpoint(100, "SNCPythonClient")
{
    m_glue = glue;
    m_recordIndex = 0;
    m_avParams.avmuxSubtype = SNC_RECORD_TYPE_AVMUX_MJPPCM;
    m_avParams.videoSubtype = SNC_RECORD_TYPE_VIDEO_MJPEG;
    m_avParams.audioSubtype = SNC_RECORD_TYPE_AUDIO_PCM;
    m_avParams.videoWidth = 640;
    m_avParams.videoHeight = 480;
    m_avParams.videoFramerate = 10;
    m_avParams.audioSampleRate = 8000;
    m_avParams.audioChannels = 2;
    m_avParams.audioSampleSize = 16;
}

SNCPythonClient::~SNCPythonClient()
{
}

void SNCPythonClient::setVideoParams(int width, int height, int rate)
{
    m_avParams.videoWidth = width;
    m_avParams.videoHeight = height;
    m_avParams.videoFramerate = rate;
}

void SNCPythonClient::setAudioParams(int rate, int size, int channels)
{
    m_avParams.audioSampleRate = rate;
    m_avParams.audioChannels = channels;
    m_avParams.audioSampleSize = size;
}

void SNCPythonClient::appClientInit()
{
}

void SNCPythonClient::appClientExit()
{

}

void SNCPythonClient::appClientBackground()
{
    if (clientIsConnected() && m_glue->isDirectoryRequested())
        requestDirectory();
}

void SNCPythonClient::appClientReceiveDirectory(QStringList directory)
{
    QMutexLocker lock (&m_lock);

    m_directory = directory;
}

void SNCPythonClient::appClientConnected()
{
    m_glue->setConnected(true);
}

void SNCPythonClient::appClientClosed()
{
    m_glue->setConnected(false);

    QMutexLocker lock (&m_lock);
    m_directory.clear();
    m_receivedData.clear();
}

void SNCPythonClient::appClientReceiveMulticast(int servicePort, SNC_EHEAD *message, int length)
{
    QMutexLocker lock(&m_lock);

    while (servicePort >= m_receivedData.count())
        m_receivedData.append(QQueue<QByteArray>());

    m_receivedData[servicePort].append(QByteArray((const char *)(message + 1), length));
    if (m_receivedData[servicePort].count() > 5)            // stop queue getting too long
        m_receivedData[servicePort].dequeue();
    clientSendMulticastAck(servicePort);
    free(message);
}

void SNCPythonClient::appClientReceiveE2E(int servicePort, SNC_EHEAD *message, int length)
{
    QMutexLocker lock(&m_lock);

    while (servicePort >= m_receivedData.count())
        m_receivedData.append(QQueue<QByteArray>());

    m_receivedData[servicePort].append(QByteArray((const char *)(message + 1), length));
    if (m_receivedData[servicePort].count() > 5)            // stop queue getting too long
        m_receivedData[servicePort].dequeue();
    free(message);
}

void SNCPythonClient::clientSendAVData(int servicePort, QByteArray video, QByteArray audio)
{
    if (clientIsServiceActive(servicePort) && clientClearToSend(servicePort)) {
        QImage img((unsigned char *)video.data(), m_avParams.videoWidth, m_avParams.videoHeight, QImage::Format_RGB888);
        QByteArray jpegArray;
        QBuffer buffer(&jpegArray);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "JPEG", 70);
        clientSendJpegAVData(servicePort, jpegArray, audio);
    }
}

void SNCPythonClient::clientSendJpegAVData(int servicePort, QByteArray video, QByteArray audio)
{
    if (clientIsServiceActive(servicePort) && clientClearToSend(servicePort)) {
        SNC_EHEAD *multiCast = clientBuildMessage(servicePort, sizeof(SNC_RECORD_AVMUX)
                                                  + video.size() + audio.size());
        SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
        SNCUtils::avmuxHeaderInit(avHead, &m_avParams, SNC_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, video.size(), audio.size());
        SNCUtils::convertInt64ToUC8(QDateTime::currentMSecsSinceEpoch(), avHead->recordHeader.timestamp);

        unsigned char *ptr = (unsigned char *)(avHead + 1);

        if (video.size() > 0) {
            memcpy(ptr, video.data(), video.size());
            ptr += video.size();
        }

        if (audio.size() > 0) {
            memcpy(ptr, audio.data(), audio.size());
        }

        int length = sizeof(SNC_RECORD_AVMUX) + video.size() + audio.size();
        clientSendMessage(servicePort, multiCast, length, SNCLINK_MEDPRI);
    }
}

void SNCPythonClient::clientSendVideoData(int servicePort, qint64 timestamp, QByteArray video, int width, int height, int rate)
{
    if (clientIsServiceActive(servicePort) && clientClearToSend(servicePort)) {
        SNC_EHEAD *multiCast = clientBuildMessage(servicePort, sizeof(SNC_RECORD_AVMUX)
                                                  + video.size());
        SNC_RECORD_AVMUX *avHead = (SNC_RECORD_AVMUX *)(multiCast + 1);
        SNCUtils::avmuxHeaderInit(avHead, &m_avParams, SNC_RECORDHEADER_PARAM_NORMAL, m_recordIndex++, 0, video.size(), 0);
        SNCUtils::convertInt64ToUC8(timestamp, avHead->recordHeader.timestamp);
        SNCUtils::convertIntToUC2(width, avHead->videoWidth);
        SNCUtils::convertIntToUC2(height, avHead->videoHeight);
        SNCUtils::convertIntToUC2(rate, avHead->videoFramerate);

        unsigned char *ptr = (unsigned char *)(avHead + 1);

        if (video.size() > 0) {
            memcpy(ptr, video.data(), video.size());
            ptr += video.size();
        }

        int length = sizeof(SNC_RECORD_AVMUX) + video.size();
        clientSendMessage(servicePort, multiCast, length, SNCLINK_MEDPRI);
    }
}

void SNCPythonClient::clientSendSensorData(int servicePort, QByteArray data)
{
    if (clientIsServiceActive(servicePort) && clientClearToSend(servicePort)) {

        SNC_EHEAD *multiCast = clientBuildMessage(servicePort, sizeof(SNC_RECORD_HEADER) + data.size());
        SNC_RECORD_HEADER *header = (SNC_RECORD_HEADER *)(multiCast + 1);
        SNCUtils::convertIntToUC2(SNC_RECORD_TYPE_SENSOR, header->type);
        SNCUtils::convertIntToUC2(0,header->subType);
        SNCUtils::convertIntToUC2(sizeof(SNC_RECORD_HEADER), header->headerLength);
        SNCUtils::convertIntToUC2(0, header->param);
        SNCUtils::convertIntToUC4(m_recordIndex++, header->recordIndex);
        SNCUtils::setTimestamp(header->timestamp);

        memcpy(header + 1, data.data(), data.size());
        clientSendMessage(servicePort, multiCast, sizeof(SNC_RECORD_HEADER) + data.size(), SNCLINK_MEDPRI);
    }
}

void SNCPythonClient::clientSendMulticastData(int servicePort, QByteArray data)
{
    if (clientIsServiceActive(servicePort) && clientClearToSend(servicePort)) {
        SNC_EHEAD *multiCast = clientBuildMessage(servicePort, data.size());
        memcpy(multiCast + 1, data.data(), data.size());
        clientSendMessage(servicePort, multiCast, data.size(), SNCLINK_MEDPRI);
    }
}

void SNCPythonClient::clientSendE2EData(int servicePort, QByteArray data)
{
    if (clientIsServiceActive(servicePort)) {
        SNC_EHEAD *multiCast = clientBuildMessage(servicePort, data.size());
        memcpy(multiCast + 1, data.data(), data.size());
        clientSendMessage(servicePort, multiCast, data.size(), SNCLINK_MEDPRI);
    }
}

char *SNCPythonClient::lookupSources(const char *sourceName, int serviceType)
{
    SNCDirectoryEntry de;
    QString servicePath;
    QString streamName;
    QString streamSource;
    static char availableSources[SNC_MAX_DELENGTH];
    QStringList services;
    QMutexLocker lock(&m_lock);

    availableSources[0] = 0;

    for (int i = 0; i < m_directory.count(); i++) {
        de.setLine(m_directory.at(i));

        if (!de.isValid())
            continue;

        if (serviceType == SERVICETYPE_MULTICAST)
            services = de.multicastServices();
        else
            services = de.e2eServices();

        for (int i = 0; i < services.count(); i++) {
            servicePath = de.appName() + SNC_SERVICEPATH_SEP + services.at(i);

            SNCUtils::removeStreamNameFromPath(servicePath, streamSource, streamName);

            if (streamName == sourceName) {
                strcat(availableSources, qPrintable(streamSource));
                strcat(availableSources, "\n");
            }
        }
    }
    return availableSources;
}

bool SNCPythonClient::getAVData(int servicePort, qint64 *timestamp, unsigned char **videoData, int *videoLength,
                    unsigned char **audioData, int *audioLength)
{
    unsigned char *rxVideoData, *rxAudioData;
    int rxMuxLength, rxVideoLength, rxAudioLength;

    *videoData = *audioData = NULL;
    *videoLength = *audioLength = 0;

    QMutexLocker lock(&m_lock);

    if (servicePort >= m_receivedData.count())
        return false;                                       // no queue for this service port yet

    if (m_receivedData[servicePort].empty())
        return false;                                       // no data in the queue

    QByteArray data = m_receivedData[servicePort].dequeue();

    SNC_RECORD_AVMUX *avmuxHeader = (SNC_RECORD_AVMUX *)data.data();

    int recordType = SNCUtils::convertUC2ToUInt(avmuxHeader->recordHeader.type);

    if (recordType != SNC_RECORD_TYPE_AVMUX)
        return false;                                       // incorrect record type

    if (!SNCUtils::avmuxHeaderValidate(avmuxHeader, data.length(),
                    NULL, rxMuxLength, &rxVideoData, rxVideoLength, &rxAudioData, rxAudioLength))
        return false;                                       // something wrong with data

    *timestamp = SNCUtils::convertUC8ToInt64(avmuxHeader->recordHeader.timestamp);

    if (rxVideoLength != 0) {
        // there is video data present

        if ((rxVideoLength < 0) || (rxVideoLength > 300000))
            return false;                               // illegal size

        *videoData = (unsigned char *)malloc(rxVideoLength);
        memcpy(*videoData, rxVideoData, rxVideoLength);
        *videoLength = rxVideoLength;
    }
    if (rxAudioLength != 0) {
        // there is audio data present

        if ((rxAudioLength < 0) || (rxAudioLength > 300000)) {
            if (*videoData != NULL)
                free(*videoData);
            *videoData = NULL;
            return false;                               // illegal size
        }

        *audioData = (unsigned char *)malloc(rxAudioLength);
        memcpy(*audioData, rxAudioData, rxAudioLength);
        *audioLength = rxAudioLength;
    }
    return true;
}

bool SNCPythonClient::getVideoData(int servicePort, qint64 *timestamp, unsigned char **videoData, int *videoLength,
                    int *width, int *height, int *rate)
{
    unsigned char *rxVideoData, *rxAudioData;
    int rxMuxLength, rxVideoLength, rxAudioLength;

    *videoData = NULL;
    *videoLength = 0;

    QMutexLocker lock(&m_lock);

    if (servicePort >= m_receivedData.count())
        return false;                                       // no queue for this service port yet

    if (m_receivedData[servicePort].empty())
        return false;                                       // no data in the queue

    QByteArray data = m_receivedData[servicePort].dequeue();

    SNC_RECORD_AVMUX *avmuxHeader = (SNC_RECORD_AVMUX *)data.data();

    int recordType = SNCUtils::convertUC2ToUInt(avmuxHeader->recordHeader.type);

    if (recordType != SNC_RECORD_TYPE_AVMUX)
        return false;                                       // incorrect record type

    if (!SNCUtils::avmuxHeaderValidate(avmuxHeader, data.length(),
                    NULL, rxMuxLength, &rxVideoData, rxVideoLength, &rxAudioData, rxAudioLength))
        return false;                                       // something wrong with data

    *timestamp = SNCUtils::convertUC8ToInt64(avmuxHeader->recordHeader.timestamp);

    if (rxVideoLength != 0) {
        // there is video data present

        if ((rxVideoLength < 0) || (rxVideoLength > SNC_MESSAGE_MAX))
            return false;                               // illegal size

        *videoData = (unsigned char *)malloc(rxVideoLength);
        memcpy(*videoData, rxVideoData, rxVideoLength);
        *videoLength = rxVideoLength;
        *width = SNCUtils::convertUC2ToInt(avmuxHeader->videoWidth);
        *height = SNCUtils::convertUC2ToInt(avmuxHeader->videoHeight);
        *rate = SNCUtils::convertUC2ToInt(avmuxHeader->videoFramerate);
//        SNCUtils::logError("Client", QString("%1").arg(*width));
    }

    return true;
}

bool SNCPythonClient::getSensorData(int servicePort, qint64 *timestamp, unsigned char **jsonData, int *jsonLength)
{
    *jsonData = NULL;
    *jsonLength = 0;

    QMutexLocker lock(&m_lock);

    if (servicePort >= m_receivedData.count())
        return false;                                       // no queue for this service port yet

    if (m_receivedData[servicePort].empty())
        return false;                                       // no data in the queue

    QByteArray rxData = m_receivedData[servicePort].dequeue();

    SNC_RECORD_HEADER *header = (SNC_RECORD_HEADER *)rxData.data();

    int recordType = SNCUtils::convertUC2ToUInt(header->type);

    if (recordType != SNC_RECORD_TYPE_SENSOR)
        return false;                                       // incorrect record type

    *timestamp = SNCUtils::convertUC8ToInt64(header->timestamp);

    int dataLength = rxData.length() - sizeof(SNC_RECORD_HEADER);
    if (dataLength <= 0)
        return false;

    *jsonData = (unsigned char *)malloc(dataLength);
    memcpy(*jsonData, rxData.data() + sizeof(SNC_RECORD_HEADER), dataLength);
    *jsonLength = dataLength;

    return true;
}

bool SNCPythonClient::getGenericData(int servicePort, unsigned char **data, int *dataLength)
{
    *data = NULL;
    *dataLength = 0;

    QMutexLocker lock(&m_lock);

    if (servicePort >= m_receivedData.count())
        return false;                                       // no queue for this service port yet

    if (m_receivedData[servicePort].empty())
        return false;                                       // no data in the queue

    QByteArray rxData = m_receivedData[servicePort].dequeue();

    *data = (unsigned char *)malloc(rxData.size());
    memcpy(*data, rxData.data(), rxData.size());
    *dataLength = rxData.size();

    return true;
}


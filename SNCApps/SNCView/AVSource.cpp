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

#include "AVSource.h"
#include "SNCUtils.h"

AVSource::AVSource(QString servicePath)
{
    QString streamSource;

    m_name = servicePath;
    m_isSensor = servicePath.endsWith("sensor");
    SNCUtils::removeStreamNameFromPath(servicePath, streamSource, m_streamName);

    m_decoder = NULL;
    m_servicePort = -1;
    m_audioEnabled = false;
    m_lastUpdate = 0;
    m_refCount = 0;
    m_stats = new DisplayStatsData();
    connect(this, SIGNAL(updateStats(int)), m_stats, SLOT(updateBytes(int)));
}

AVSource::~AVSource()
{
    stopBackgroundProcessing();

    if (m_stats) {
        delete m_stats;
        m_stats = NULL;
    }
}

QString AVSource::name() const
{
    return m_name;
}

QString AVSource::streamName() const
{
    return m_streamName;
}

int AVSource::servicePort() const
{
    return m_servicePort;
}

void AVSource::setServicePort(int port)
{
    m_servicePort = port;

    if (port == -1) {
        stopBackgroundProcessing();
        return;
    }
    else if (!m_decoder) {
        m_decoder = new AVMuxDecode();

        connect(this, SIGNAL(newAVMuxData(QByteArray)), m_decoder, SLOT(newAVMuxData(QByteArray)));

        connect(m_decoder, SIGNAL(newImage(QImage, qint64)),
            this, SLOT(newImage(QImage, qint64)));

        connect(m_decoder, SIGNAL(newAudioSamples(QByteArray, qint64, int, int, int)),
            this, SLOT(newAudioSamples(QByteArray, qint64, int, int, int)));

        m_decoder->resumeThread();
    }
}

void AVSource::setSensorData(QByteArray sensorData)
{
    m_sensorData = sensorData;
    m_lastUpdate = SNCUtils::clock();
    emit updateStats(sensorData.size());
}

qint64 AVSource::lastUpdate() const
{
    return m_lastUpdate;
}

void AVSource::setLastUpdate(qint64 timestamp)
{
    m_lastUpdate = timestamp;
}

QImage AVSource::image()
{
    return m_image;
}

qint64 AVSource::imageTimestamp()
{
    return m_imageTimestamp;
}

void AVSource::stopBackgroundProcessing()
{
    if (m_decoder) {
        disconnect(this, SIGNAL(newAVMuxData(QByteArray)), m_decoder, SLOT(newAVMuxData(QByteArray)));

        disconnect(m_decoder, SIGNAL(newImage(QImage, qint64)),
            this, SLOT(newImage(QImage, qint64)));

        disconnect(m_decoder, SIGNAL(newAudioSamples(QByteArray, qint64, int, int, int)),
            this, SLOT(newAudioSamples(QByteArray, qint64, int, int, int)));

        m_decoder->exitThread();
        m_decoder = NULL;
    }
}

void AVSource::enableAudio(bool enable)
{
    m_audioEnabled = enable;
}

bool AVSource::audioEnabled() const
{
    return m_audioEnabled;
}

// feed new raw data to the decoder, called from the SNC client thread
void AVSource::setAVMuxData(QByteArray data)
{
    emit updateStats(data.size());
    emit newAVMuxData(data);
}

// signal from the decoder, processed image
void AVSource::newImage(QImage image, qint64 timestamp)
{
    if (!image.isNull()) {
        m_image = image;
        m_imageTimestamp = timestamp;
    }
    m_lastUpdate = SNCUtils::clock();
}

// signal from the decoder, processed sound
void AVSource::newAudioSamples(QByteArray data, qint64, int rate, int channels, int size)
{
    if (m_audioEnabled)
        emit newAudio(data, rate, channels, size);
}

DisplayStatsData* AVSource::stats()
{
    return m_stats;
}

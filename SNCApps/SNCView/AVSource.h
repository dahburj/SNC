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

#ifndef AVSOURCE_H
#define AVSOURCE_H

#ifdef USING_GSTREAMER
#include "AVMuxDecodeGS.h"
#else
#include "AVMuxDecode.h"
#endif
#include "DisplayStatsData.h"

#include "QImage"

class AVSource : public QObject
{
    Q_OBJECT

public:
    AVSource(QString streamName);
    ~AVSource();

    QString name() const;
    QString streamName() const;
    bool isSensor() { return m_isSensor; }

    int servicePort() const;
    void setServicePort(int port);

    qint64 lastUpdate() const;
    void setLastUpdate(qint64 timestamp);

    QImage image();
    qint64 imageTimestamp();

    void setAVMuxData(QByteArray rawData);
    void stopBackgroundProcessing();

    void setSensorData(QByteArray(sensorData));
    QByteArray getSensorData() { return m_sensorData; }

    void enableAudio(bool enable);
    bool audioEnabled() const;

    DisplayStatsData *stats();

    int getRefCount() { return m_refCount; }
    void incRefCount() { m_refCount++; }
    void decRefCount() { m_refCount--; }

signals:
    void newAudio(QByteArray data, int rate, int channels, int size);
    void newAVMuxData(QByteArray data);
    void updateStats(int bytes);

public slots:
    void newImage(QImage image, qint64 timestamp);
    void newAudioSamples(QByteArray data, qint64 timestamp, int rate, int channels, int size);

private:
    QString m_name;
    bool m_isSensor;
    int m_servicePort;
    QString m_streamName;

    qint64 m_lastUpdate;

    QImage m_image;
    qint64 m_imageTimestamp;

    QByteArray m_sensorData;

    bool m_audioEnabled;

    AVMuxDecode *m_decoder;

    int m_refCount;

    int m_statsTimer;
    DisplayStatsData *m_stats;
};

#endif // AVSOURCE_H

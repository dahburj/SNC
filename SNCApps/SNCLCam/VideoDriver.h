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

#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

#include <QSize>
#include <QSettings>
#include <qimage.h>

#include <linux/videodev2.h>

#include "SNCThread.h"

#define HUFFMAN_TABLE_SIZE 420
#define AVI_HEADER_SIZE 37

class VideoDriver : public SNCThread
{
    Q_OBJECT

public:
    VideoDriver();
    virtual ~VideoDriver();

    bool deviceExists();
    bool isDeviceOpen();

    QSize getImageSize();

public slots:
    void newCamera();

signals:
    void pixelFormat(quint32 format);
    void videoFormat(int width, int height, int frameRate);
    void newJpeg(QByteArray);
    void cameraState(QString state);

protected:
    void initThread();
    void finishThread();
    void timerEvent(QTimerEvent *event);

private:
    bool openDevice();
    void closeDevice();
    bool readFrame();
    void loadSettings();

    void queryAvailableFormats();
    void queryAvailableSizes();
    void queryAvailableRates();

    bool choosePixelFormat();
    bool chooseFrameSize();
    bool chooseFrameRate();

    bool setImageFormat();
    bool setFrameRate();
    bool allocMmapBuffers();
    void freeMmapBuffers();
    bool queueV4LBuffer(quint32 index);
    bool streamOn();
    void streamOff();
    bool handleFrame();
    bool handleJpeg(quint32 index, quint32 size);
    bool handleYUYV(quint32 index, quint32 size);
    QImage YUYV2RGB(quint32 index);
    int xioctl(int request, void *arg);
    void dumpRaw(QByteArray frame);

    static bool frameSizeLessThan(const QSize &a, const QSize &b);
    static bool frameRateLessThan(const QSize &a, const QSize &b);

    quint32 m_preferredFormat;
    int m_preferredWidth;
    int m_preferredHeight;
    qreal m_preferredFrameRate;

    quint32 m_format;
    int m_width;
    int m_height;
    qreal m_frameRate;
    int m_frameRateIndex;

    quint32 m_pixelFormat;
    int m_fd;
    QString m_cameraName;
    quint32 m_consecutiveBadFrames;
    quint32 m_mmBuffLen;
    QList<char *> m_mmBuff;
    uchar *m_rgbBuff;
    quint32 m_jpegSize;
    static const unsigned char huffmanTable[HUFFMAN_TABLE_SIZE];
    static const unsigned char jpegAviHeader[AVI_HEADER_SIZE];
    int m_frameCount;
    QList<quint32> m_formatList;
    QList<QSize> m_sizeList;
    QList<QSize> m_rateList;

    int m_timer;
    int m_state;
    int m_ticks;
};

#endif // VIDEODRIVER_H


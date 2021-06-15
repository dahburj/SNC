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

#ifndef CAMERASTATUS_H
#define CAMERASTATUS_H

#include "Dialog.h"

//  SNCJSON defs

#define CAMERASTATUS_DIALOG_NAME              "cameraStatus"
#define CAMERASTATUS_DIALOG_DESC              "Camera status"

class CameraStatus : public Dialog
{
    Q_OBJECT

public:
    CameraStatus();
    virtual ~CameraStatus();

public slots:
    void videoFormat(int width, int height, int frameRate);
    void cameraState(QString state);
    void newHighRateVideoData(int bytes);
    void newLowRateVideoData(int bytes);

    void audioState(QString state);
    void audioFormat(int sampleRate, int channels, int sampleSize);
    void newHighRateAudioData(int bytes);
    void newLowRateAudioData(int bytes);

protected:
    void getDialog(QJsonObject& newConfig);
    void timerEvent(QTimerEvent *event);

private:
    QString m_videoState;
    int m_videoWidth;
    int m_videoHeight;
    int m_videoRate;

    int m_videoHighRateFramesPerSecond;
    qint64 m_videoHighRateBytesSent;
    int m_videoHighRateByteRate;

    int m_videoHighRateByteTemp;
    int m_videoHighRateRateTemp;

    int m_videoLowRateFramesPerSecond;
    qint64 m_videoLowRateBytesSent;
    int m_videoLowRateByteRate;

    int m_videoLowRateByteTemp;
    int m_videoLowRateRateTemp;

    QString m_audioState;
    int m_audioRate;
    int m_audioChannels;
    int m_audioSampleSize;

    int m_audioHighRateFramesPerSecond;
    qint64 m_audioHighRateBytesSent;
    int m_audioHighRateByteRate;

    int m_audioHighRateByteTemp;
    int m_audioHighRateRateTemp;

    int m_audioLowRateFramesPerSecond;
    qint64 m_audioLowRateBytesSent;
    int m_audioLowRateByteRate;

    int m_audioLowRateByteTemp;
    int m_audioLowRateRateTemp;

    int m_timerID;
};

#endif // CAMERASTATUS_H

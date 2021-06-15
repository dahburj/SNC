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

#include "CameraStatus.h"
#include "SNCUtils.h"

CameraStatus::CameraStatus() : Dialog(CAMERASTATUS_DIALOG_NAME, CAMERASTATUS_DIALOG_DESC)
{
    m_videoHighRateBytesSent = m_videoHighRateByteRate = 0;
    m_videoLowRateBytesSent = m_videoLowRateByteRate = 0;
    m_videoHighRateByteTemp = m_videoHighRateRateTemp = 0;
    m_videoLowRateByteTemp = m_videoLowRateRateTemp = 0;

    m_videoWidth = m_videoHeight = m_videoRate = m_videoHighRateFramesPerSecond = 0;

    m_audioHighRateBytesSent = m_audioHighRateByteRate = 0;
    m_audioLowRateBytesSent = m_audioLowRateByteRate = 0;
    m_audioHighRateByteTemp = m_audioHighRateRateTemp = 0;
    m_audioLowRateByteTemp = m_audioLowRateRateTemp = 0;

    m_audioChannels = m_audioRate = m_audioSampleSize = 0;

    m_timerID = startTimer(1000);
}

CameraStatus::~CameraStatus()
{
    killTimer(m_timerID);
}

void CameraStatus::timerEvent(QTimerEvent * /* event */)
{
    m_videoHighRateByteRate = m_videoHighRateByteTemp;
    m_videoHighRateByteTemp = 0;

    m_videoHighRateFramesPerSecond = m_videoHighRateRateTemp;
    m_videoHighRateRateTemp = 0;

    m_videoLowRateByteRate = m_videoLowRateByteTemp;
    m_videoLowRateByteTemp = 0;

    m_videoLowRateFramesPerSecond= m_videoLowRateRateTemp;
    m_videoLowRateRateTemp = 0;

    m_audioHighRateByteRate = m_audioHighRateByteTemp;
    m_audioHighRateByteTemp = 0;

    m_audioHighRateFramesPerSecond = m_audioHighRateRateTemp;
    m_audioHighRateRateTemp = 0;

    m_audioLowRateByteRate = m_audioLowRateByteTemp;
    m_audioLowRateByteTemp = 0;

    m_audioLowRateFramesPerSecond = m_audioLowRateRateTemp;
    m_audioLowRateRateTemp = 0;

}

void CameraStatus::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createGraphicsStringVar("Video:", "font-size: 18px;"));
    addVar(createInfoStringVar("State", m_videoState));
    addVar(createInfoStringVar("Width", QString::number(m_videoWidth)));
    addVar(createInfoStringVar("Height", QString::number(m_videoHeight)));
    addVar(createInfoStringVar("Rate (fps)", QString::number(m_videoRate)));
    addVar(createInfoStringVar("High rate fps", QString::number(m_videoHighRateFramesPerSecond)));
    addVar(createInfoStringVar("High rate bytes sent", QString::number(m_videoHighRateBytesSent)));
    addVar(createInfoStringVar("High Rate byte rate", QString::number(m_videoHighRateByteRate)));
    addVar(createInfoStringVar("Low rate fps", QString::number(m_videoLowRateFramesPerSecond)));
    addVar(createInfoStringVar("Low rate bytes sent", QString::number(m_videoLowRateBytesSent)));
    addVar(createInfoStringVar("Low Rate byte rate", QString::number(m_videoLowRateByteRate)));

    addVar(createGraphicsLineVar());

    addVar(createGraphicsStringVar("Audio:", "font-size: 18px;"));
    addVar(createInfoStringVar("State:", m_audioState));
    addVar(createInfoStringVar("Sample rate", QString::number(m_audioRate)));
    addVar(createInfoStringVar("Channels", QString::number(m_audioChannels)));
    addVar(createInfoStringVar("Sample size", QString::number(m_audioSampleSize)));
    addVar(createInfoStringVar("High rate pps", QString::number(m_audioHighRateFramesPerSecond)));
    addVar(createInfoStringVar("High rate bytes sent", QString::number(m_audioHighRateBytesSent)));
    addVar(createInfoStringVar("High rate byte rate", QString::number(m_audioHighRateByteRate)));
    addVar(createInfoStringVar("Low rate pps", QString::number(m_audioLowRateFramesPerSecond)));
    addVar(createInfoStringVar("Low rate bytes sent", QString::number(m_audioLowRateBytesSent)));
    addVar(createInfoStringVar("Low rate byte rate", QString::number(m_audioLowRateByteRate)));

    return dialog(newDialog, true);
}

void CameraStatus::videoFormat(int width, int height, int frameRate)
{
    m_videoWidth = width;
    m_videoHeight = height;
    m_videoRate = frameRate;
}

void CameraStatus::newHighRateVideoData(int bytes)
{
    m_videoHighRateBytesSent += bytes;
    m_videoHighRateByteTemp += bytes;
    m_videoHighRateRateTemp++;
}

void CameraStatus::newLowRateVideoData(int bytes)
{
    m_videoLowRateBytesSent += bytes;
    m_videoLowRateByteTemp += bytes;
    m_videoLowRateRateTemp++;
}

void CameraStatus::cameraState(QString state)
{
    m_videoState = state;
}

void CameraStatus::audioState(QString state)
{
    m_audioState = state;
}

void CameraStatus::newHighRateAudioData(int bytes)
{
    m_audioHighRateBytesSent += bytes;
    m_audioHighRateByteTemp += bytes;
    m_audioHighRateRateTemp++;
}

void CameraStatus::newLowRateAudioData(int bytes)
{
    m_audioLowRateBytesSent += bytes;
    m_audioLowRateByteTemp += bytes;
    m_audioLowRateRateTemp++;
}

void CameraStatus::audioFormat(int sampleRate, int channels, int sampleSize)
{
    m_audioRate = sampleRate;
    m_audioChannels = channels;
    m_audioSampleSize = sampleSize;
}


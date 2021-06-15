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

#include "SNCLCam.h"

#include "CameraDlg.h"
#include "AudioDlg.h"
#include "MotionDlg.h"
#include "StreamsDlg.h"
#include "CameraClient.h"
#include "VideoDriver.h"
#include "AudioDriver.h"
#include "CameraStatus.h"

#define TAG "SNCLCam"

SNCLCam::SNCLCam(QObject *parent, bool daemonMode) : MainConsole(parent, daemonMode)
{
    m_cameraClient = new CameraClient(this);
    m_cameraClient->resumeThread();

    m_camera = new VideoDriver();

    connect(m_camera, SIGNAL(videoFormat(int,int,int)), m_cameraClient, SLOT(videoFormat(int,int,int)));
    connect(m_camera, SIGNAL(newJpeg(QByteArray)), m_cameraClient, SLOT(newJpeg(QByteArray)), Qt::DirectConnection);

    m_audio = new AudioDriver();
    connect(m_audio, SIGNAL(newAudio(QByteArray)), m_cameraClient, SLOT(newAudio(QByteArray)), Qt::DirectConnection);
    connect(m_audio, SIGNAL(audioFormat(int, int, int)), m_cameraClient, SLOT(audioFormat(int, int, int)), Qt::QueuedConnection);

    addStandardDialogs();

    AudioDlg *audioDlg = new AudioDlg();
    connect(audioDlg, SIGNAL(newAudioSrc()), m_audio, SLOT(newAudioSrc()));
    SNCJSON::addConfigDialog(audioDlg);

    CameraDlg *cameraDlg = new CameraDlg();
    connect(cameraDlg, SIGNAL(newCamera()), m_camera, SLOT(newCamera()));
    SNCJSON::addConfigDialog(cameraDlg);

    MotionDlg *motionDlg = new MotionDlg();
    connect(motionDlg, SIGNAL(newStream()), m_cameraClient, SLOT(newStream()));
    SNCJSON::addConfigDialog(motionDlg);

    StreamsDlg *streamsDlg = new StreamsDlg();
    connect(streamsDlg, SIGNAL(newStream()), m_cameraClient, SLOT(newStream()));
    SNCJSON::addConfigDialog(streamsDlg);

    CameraStatus *status = new CameraStatus();
    connect(m_camera, SIGNAL(cameraState(QString)), status, SLOT(cameraState(QString)));
    connect(m_camera, SIGNAL(videoFormat(int,int,int)), status, SLOT(videoFormat(int,int,int)));
    connect(m_cameraClient, SIGNAL(newHighRateVideoData(int)), status, SLOT(newHighRateVideoData(int)));
    connect(m_cameraClient, SIGNAL(newLowRateVideoData(int)), status, SLOT(newLowRateVideoData(int)));
    connect(m_audio, SIGNAL(audioState(QString)), status, SLOT(audioState(QString)));
    connect(m_audio, SIGNAL(audioFormat(int,int,int)), status, SLOT(audioFormat(int,int,int)));
    connect(m_cameraClient, SIGNAL(newHighRateAudioData(int)), status, SLOT(newHighRateAudioData(int)));
    connect(m_cameraClient, SIGNAL(newLowRateAudioData(int)), status, SLOT(newLowRateAudioData(int)));
    SNCJSON::addInfoDialog(status);

    m_camera->resumeThread();
    m_audio->resumeThread();

    startServices();
}

void SNCLCam::appExit()
{
    m_camera->exitThread();
    m_camera = NULL;
    m_audio->exitThread();
    m_audio = NULL;
    m_cameraClient->exitThread();
    m_cameraClient = NULL;
}

void SNCLCam::showHelp()
{
    printf("\nOptions are:\n\n");
    printf("  h - Show help\n");
    printf("  s - Show status\n");
    printf("  x - Exit\n");
}

void SNCLCam::showStatus()
{
    printf("\nStatus: %s\n", qPrintable(m_cameraClient->getLinkState()));
}

void SNCLCam::processInput(char c)
{
    switch (c) {
        case 'H':
            showHelp();
            break;

        case 'S':
            showStatus();
            break;
    }
}

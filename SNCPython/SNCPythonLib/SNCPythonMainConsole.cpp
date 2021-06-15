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

#include "SNCPythonMainConsole.h"
#include "SNCUtils.h"
#include "SNCPythonClient.h"
#include "SNCPythonGlue.h"
#include "SNCPythonVidCap.h"

SNCPythonMainConsole::SNCPythonMainConsole(QObject *parent, SNCPythonGlue *glue)
    : QThread(parent), SNCPythonMain(glue)
{
    connect(this, SIGNAL(clientSendAVData(int,QByteArray,QByteArray)),
                         m_client, SLOT(clientSendAVData(int,QByteArray,QByteArray)));

    connect(this, SIGNAL(clientSendJpegAVData(int,QByteArray,QByteArray)),
                         m_client, SLOT(clientSendJpegAVData(int,QByteArray,QByteArray)));

    connect(this, SIGNAL(clientSendVideoData(int,qint64, QByteArray,int,int,int)),
                         m_client, SLOT(clientSendVideoData(int,qint64,QByteArray,int,int,int)));

    connect(this, SIGNAL(clientSendSensorData(int,QByteArray)),
                         m_client, SLOT(clientSendSensorData(int,QByteArray)));

    connect(this, SIGNAL(clientSendMulticastData(int,QByteArray)),
                         m_client, SLOT(clientSendMulticastData(int,QByteArray)));

    connect(this, SIGNAL(clientSendE2EData(int,QByteArray)),
                         m_client, SLOT(clientSendE2EData(int,QByteArray)));

    start();
}

void SNCPythonMainConsole::addVidCapSignal(SNCPythonVidCap *vidCap)
{
    connect(vidCap, SIGNAL(newFrame(int,QByteArray,bool,int,int,int)), this, SLOT(newFrame(int,QByteArray,bool,int,int,int)));
}

void SNCPythonMainConsole::newFrame(int cameraNum, QByteArray frame, bool jpeg, int width, int height, int rate)
{
    addFrameToQueue(cameraNum, frame, jpeg, width, height, rate);
}


void SNCPythonMainConsole::run()
{
    while(!m_mustExit) {
        msleep(10);
    }

    m_client->exitThread();
    SNCUtils::SNCAppExit();
    QCoreApplication::exit();
}

bool SNCPythonMainConsole::sendAVData(int servicePort, unsigned char *videoData, int videoLength,
                    unsigned char *audioData, int audioLength)
{
    if (m_mustExit)
        return false;
    emit clientSendAVData(servicePort, QByteArray((const char *)videoData, videoLength), QByteArray((const char *)audioData, audioLength));
    return true;
}

bool SNCPythonMainConsole::sendJpegAVData(int servicePort, unsigned char *videoData, int videoLength,
                    unsigned char *audioData, int audioLength)
{
    if (m_mustExit)
        return false;

    emit clientSendJpegAVData(servicePort, QByteArray((const char *)videoData, videoLength), QByteArray((const char *)audioData, audioLength));
    return true;
}

bool SNCPythonMainConsole::sendVideoData(int servicePort, long long timestamp, unsigned char *videoData, int videoLength,
                    int width, int height, int rate)
{
    if (m_mustExit)
        return false;

    emit clientSendVideoData(servicePort, timestamp, QByteArray((const char *)videoData, videoLength), width, height, rate);
    return true;
}

bool SNCPythonMainConsole::sendSensorData(int servicePort, unsigned char *data, int dataLength)
{
    if (m_mustExit)
        return false;
    emit clientSendSensorData(servicePort, QByteArray((const char *)data, dataLength));
    return true;
}

bool SNCPythonMainConsole::sendMulticastData(int servicePort, unsigned char *data, int dataLength)
{
    if (m_mustExit)
        return false;
    emit clientSendMulticastData(servicePort, QByteArray((const char *)data, dataLength));
    return true;
}

bool SNCPythonMainConsole::sendE2EData(int servicePort, unsigned char *data, int dataLength)
{
    if (m_mustExit)
        return false;
    emit clientSendE2EData(servicePort, QByteArray((const char *)data, dataLength));
    return true;
}


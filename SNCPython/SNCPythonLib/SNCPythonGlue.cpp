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

#include "SNCPythonGlue.h"
#include "SNCPythonMainConsole.h"
#include "SNCPythonMainWindow.h"
#include "SNCPythonClient.h"

#include "SNCUtils.h"

#include <qmutex.h>

QMutex lockSNCPython;

SNCPythonGlue::SNCPythonGlue()
{
    m_main = NULL;
    m_connected = false;
    m_directoryRequested = false;
}

SNCPythonGlue::~SNCPythonGlue()
{
    if (m_main != NULL)
        stopLib();
}

void SNCPythonGlue::startLib(const char *productType, int& argc, char **argv, bool showWindow)
{
    m_argc = argc;
    m_argv = argv;

    m_daemonMode = SNCUtils::checkDaemonModeFlag(m_argc, m_argv);

    if (SNCUtils::checkConsoleModeFlag(m_argc, m_argv)) {
        QCoreApplication a(m_argc, m_argv);
        SNCUtils::loadStandardSettings(productType, a.arguments());
        m_main = new SNCPythonMainConsole(&a, this);
        a.exec();
    } else {
        QApplication a(m_argc, m_argv);
        SNCUtils::loadStandardSettings(productType, a.arguments());
        m_main = new SNCPythonMainWindow(this);
        if (showWindow)
            ((SNCPythonMainWindow *)m_main)->show();
        a.exec();
    }
 }

void SNCPythonGlue::stopLib()
{
    m_main->stopRunning();
    m_main = NULL;
}

void SNCPythonGlue::setWindowTitle(char *title)
{
    m_main->setWindowTitle(title);
}

void SNCPythonGlue::displayImage(unsigned char *image, int length,
                                    int width, int height, char *timestamp)
{
    m_main->displayImage(QByteArray((const char *)image, length), width, height, timestamp);
}

void SNCPythonGlue::displayJpegImage(unsigned char *image, int length, char *timestamp)
{
    m_main->displayJpegImage(QByteArray((const char *)image, length), timestamp);
}

char *SNCPythonGlue::getAppName()
{
    static char name[256];

    strcpy(name, qPrintable(SNCUtils::getAppName()));
    return name;
}

void SNCPythonGlue::setConnected(bool state)
{
    QMutexLocker lock(&lockSNCPython);
    m_connected = state;
}

bool SNCPythonGlue::isConnected()
{
    QMutexLocker lock(&lockSNCPython);
    return m_connected;
}

bool SNCPythonGlue::requestDirectory()
{
    QMutexLocker lock(&lockSNCPython);

    if (!m_connected)
        return false;
    m_directoryRequested = true;
    return true;
}

bool SNCPythonGlue::isDirectoryRequested()
{
    QMutexLocker lock(&lockSNCPython);

    bool ret = m_directoryRequested;
    m_directoryRequested = false;
    return ret;
}

char *SNCPythonGlue::lookupMulticastSources(const char *sourceName)
{
    return m_main->getClient()->lookupSources(sourceName, SERVICETYPE_MULTICAST);
}

int SNCPythonGlue::addMulticastSourceService(const char *servicePath)
{
    return m_main->getClient()->clientAddService(servicePath, SERVICETYPE_MULTICAST, true);
}

int SNCPythonGlue::addMulticastSinkService(const char *servicePath)
{
    return m_main->getClient()->clientAddService(servicePath, SERVICETYPE_MULTICAST, false);
}

char *SNCPythonGlue::lookupE2ESources(const char *sourceType)
{
    return m_main->getClient()->lookupSources(sourceType, SERVICETYPE_E2E);
}

int SNCPythonGlue::addE2ESourceService(const char *servicePath)
{
    return m_main->getClient()->clientAddService(servicePath, SERVICETYPE_E2E, true);
}

int SNCPythonGlue::addE2ESinkService(const char *servicePath)
{
    return m_main->getClient()->clientAddService(servicePath, SERVICETYPE_E2E, false);
}

bool SNCPythonGlue::removeService(int servicePort)
{
    return m_main->getClient()->clientRemoveService(servicePort);
}

bool SNCPythonGlue::isClearToSend(int servicePort)
{
    return m_main->getClient()->clientClearToSend(servicePort);
}

bool SNCPythonGlue::isServiceActive(int servicePort)
{
    return m_main->getClient()->clientIsServiceActive(servicePort);
}

bool SNCPythonGlue::sendAVData(int servicePort, unsigned char *videoData, int videoLength,
                    unsigned char *audioData, int audioLength)
{
    return m_main->sendAVData(servicePort, videoData, videoLength, audioData, audioLength);
}

bool SNCPythonGlue::sendJpegAVData(int servicePort, unsigned char *videoData, int videoLength,
                    unsigned char *audioData, int audioLength)
{
    return m_main->sendJpegAVData(servicePort, videoData, videoLength, audioData, audioLength);
}

bool SNCPythonGlue::getAVData(int servicePort, long long *timestamp, unsigned char **videoData, int *videoLength,
                    unsigned char **audioData, int *audioLength)
{
    return m_main->getClient()->getAVData(servicePort, timestamp, videoData, videoLength, audioData, audioLength);
}

bool SNCPythonGlue::getVideoData(int servicePort, long long *timestamp, unsigned char **videoData, int *videoLength,
                    int *width, int *height, int *rate)
{
    return m_main->getClient()->getVideoData(servicePort, timestamp, videoData, videoLength, width, height, rate);
}

bool SNCPythonGlue::sendVideoData(int servicePort, long long timestamp, unsigned char *videoData, int videoLength,
                    int width, int height, int rate)
{
    return m_main->sendVideoData(servicePort, timestamp, videoData, videoLength, width, height, rate);
}

void SNCPythonGlue::setVideoParams(int width, int height, int rate)
{
    m_main->getClient()->setVideoParams(width, height, rate);
}

void SNCPythonGlue::setAudioParams(int rate, int size, int channels)
{
    m_main->getClient()->setVideoParams(rate, size, channels);
}

bool SNCPythonGlue::getSensorData(int servicePort, long long *timestamp, unsigned char **jsonData, int *jsonLength)
{
    return m_main->getClient()->getSensorData(servicePort, timestamp, jsonData, jsonLength);
}

bool SNCPythonGlue::sendSensorData(int servicePort, unsigned char *data, int dataLength)
{
    return m_main->sendSensorData(servicePort, data, dataLength);
}

bool SNCPythonGlue::sendMulticastData(int servicePort, unsigned char *data, int dataLength)
{
    return m_main->sendMulticastData(servicePort, data, dataLength);
}

bool SNCPythonGlue::sendE2EData(int servicePort, unsigned char *data, int dataLength)
{
    return m_main->sendE2EData(servicePort, data, dataLength);
}

bool SNCPythonGlue::getMulticastData(int servicePort, unsigned char **data, int *dataLength)
{
    return m_main->getClient()->getGenericData(servicePort, data, dataLength);
}

bool SNCPythonGlue::getE2EData(int servicePort, unsigned char **data, int *dataLength)
{
    return m_main->getClient()->getGenericData(servicePort, data, dataLength);
}

bool SNCPythonGlue::vidCapOpen(int cameraNum, int width, int height, int rate)
{
    return m_main->vidCapOpen(cameraNum, width, height, rate);
}

bool SNCPythonGlue::vidCapClose(int cameraNum)
{
    return m_main->vidCapClose(cameraNum);
}

bool SNCPythonGlue::vidCapGetFrame(int cameraNum, unsigned char** frame, int& length, bool& jpeg,
                                         int& width, int& height, int& rate)
{
    QByteArray qframe;

    *frame = NULL;

    if (!m_main->vidCapGetFrame(cameraNum, qframe, jpeg, width, height, rate))
        return false;
    *frame = (unsigned char *)malloc(qframe.length());
    memcpy(*frame, qframe.data(), qframe.length());
    length = qframe.length();
    return true;
}


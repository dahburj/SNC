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

#include "ViewClient.h"
#include "SNCUtils.h"
#include "SNCView.h"

#define TAG "ViewClient"

#define	VIEWCLIENT_BACKGROUND_INTERVAL (SNC_CLOCKS_PER_SEC/100)


ViewClient::ViewClient() : SNCCFSClient(VIEWCLIENT_BACKGROUND_INTERVAL, TAG)
{
}

void ViewClient::appClientInit()
{
    CFSClientInit();
}


void ViewClient::appClientBackground()
{
    CFSClientBackground();
}

void ViewClient::appClientConnected()
{
    emit clientConnected();
}

void ViewClient::appClientClosed()
{
    emit clientClosed();
}


void ViewClient::requestDir()
{
    requestDirectory();
}

void ViewClient::appClientReceiveDirectory(QStringList directory)
{
    emit dirResponse(directory);
}

void ViewClient::addService(AVSource *avSource)
{
    if (avSource->servicePort() != -1)
        return;

    SNCUtils::logInfo(TAG, QString("Adding service %1").arg(avSource->name()));
    int servicePort = clientAddService(avSource->name(), SERVICETYPE_MULTICAST, false, false);

    if (servicePort >= 0) {
        clientSetServiceDataPointer(servicePort, (void *)avSource);
        avSource->setServicePort(servicePort);
        SNCUtils::logInfo(TAG, QString("Added service %1 on port %2").arg(avSource->name()).arg(servicePort));
    }
}

void ViewClient::removeService(AVSource *avSource)
{
    SNCUtils::logInfo(TAG, QString("Removing service %1 on port %2").arg(avSource->name()).arg(avSource->servicePort()));
    clientRemoveService(avSource->servicePort());
    avSource->setServicePort(-1);
}

void ViewClient::enableService(AVSource *avSource)
{
    if (avSource->servicePort() == -1)
        return;

    SNCUtils::logInfo(TAG, QString("Enabling service %1 on port %2").arg(avSource->name()).arg(avSource->servicePort()));
    clientEnableService(avSource->servicePort());
}

void ViewClient::disableService(AVSource *avSource)
{
    SNCUtils::logInfo(TAG, QString("Disabling service %1 on port %2").arg(avSource->name()).arg(avSource->servicePort()));
    clientDisableService(avSource->servicePort());
}


void ViewClient::appClientReceiveMulticast(int servicePort, SNC_EHEAD *multiCast, int len)
{
    AVSource *avSource = reinterpret_cast<AVSource *>(clientGetServiceDataPointer(servicePort));

    if (avSource) {
        if ((avSource->streamName() == SNC_STREAMNAME_AVMUX) || (avSource->streamName() == SNC_STREAMNAME_AVMUXLR)) {
            SNC_RECORD_AVMUX *avmuxHeader = reinterpret_cast<SNC_RECORD_AVMUX *>(multiCast + 1);
            int recordType = SNCUtils::convertUC2ToUInt(avmuxHeader->recordHeader.type);

            if (recordType == SNC_RECORD_TYPE_AVMUX) {
                avSource->setAVMuxData(QByteArray((const char *)avmuxHeader, len));
                clientSendMulticastAck(servicePort);
            } else {
                qDebug() << "Expecting avmux record, received record type" << recordType;
            }
        } else if (avSource->streamName() == SNC_STREAMNAME_SENSOR) {
            if (len > (int)sizeof(SNC_RECORD_HEADER)) {
                SNC_RECORD_HEADER *sensorHeader = reinterpret_cast<SNC_RECORD_HEADER *>(multiCast + 1);
                avSource->setSensorData(QByteArray((const char *)(sensorHeader + 1), len - sizeof(SNC_RECORD_HEADER)));
                clientSendMulticastAck(servicePort);
            } else {
                SNCUtils::logWarn(TAG, QString("Sensor data received was too short (%1) from %2").arg(len).arg(avSource->name()));
            }
        }
    } else {
        SNCUtils::logWarn(TAG, QString("Multicast received to invalid port %1").arg(servicePort));
    }

    free(multiCast);
}


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

#include <qbuffer.h>

#include "ConfigClient.h"

#define	CONFIGCLIENT_BACKGROUND_INTERVAL (SNC_CLOCKS_PER_SEC/10)


ConfigClient::ConfigClient()
    : SNCEndpoint(CONFIGCLIENT_BACKGROUND_INTERVAL, "ConfigClient")
{
    m_appPort = -1;
    m_controlPort = -1;
}

void ConfigClient::appClientInit()
{
    m_controlPort = clientAddService(SNC_STREAMNAME_MANAGE, SERVICETYPE_E2E, true);
}

void ConfigClient::appClientExit()
{
    if (m_controlPort != -1)
        clientRemoveService(m_controlPort);
    m_controlPort = -1;
    stopApp();
}

void ConfigClient::requestDir()
{
    requestDirectory();
}

void ConfigClient::appClientReceiveDirectory(QStringList directory)
{
    emit dirResponse(directory);
}

void ConfigClient::newApp(QString appPath)
{
    if (m_appPort != -1)
        stopApp();

    QString app = SNCUtils::insertStreamNameInPath(appPath, SNC_STREAMNAME_MANAGE);

    m_appPort = clientAddService(app, SERVICETYPE_E2E, false);
}

void ConfigClient::stopApp()
{
    if (m_appPort != -1)
        clientRemoveService(m_appPort);
    m_appPort = -1;
}

void ConfigClient::appClientReceiveE2E(int servicePort, SNC_EHEAD *header, int len)
{
    if (servicePort == m_appPort) {
        QByteArray data((char *)(header + 1), len);
        QJsonDocument var(QJsonDocument::fromJson(data));
        emit receiveAppData(var.object());
    } else if (servicePort == m_controlPort) {
        QByteArray data((char *)(header + 1), len);
        QJsonDocument var(QJsonDocument::fromJson(data));

        QString remoteUID = SNCUtils::displayUID(&(header->sourceUID));
        int remotePort = SNCUtils::convertUC2ToInt(header->sourcePort);
        emit receiveControlData(var.object(), remoteUID, remotePort);
    }
    free(header);
}

void ConfigClient::sendAppData(QJsonObject json)
{
    if (m_appPort == -1)
        return;

    if (!clientIsConnected())
        return;

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    buffer.write(QJsonDocument(json).toJson());
    SNC_EHEAD *header = clientBuildMessage(m_appPort, data.length());
    if (header == NULL)
        return;
    memcpy((char *)(header + 1), data.data(), data.length());
    clientSendMessage(m_appPort, header, data.length(), SNCLINK_LOWPRI);
 }

void ConfigClient::sendControlData(QJsonObject json, QString remoteUID, int remotePort)
{
    SNC_UID uid;
    SNC_UIDSTR uidstr;

    if (m_controlPort == -1)
        return;

    if (!clientIsConnected())
        return;

    strcpy(uidstr, qPrintable(remoteUID));
    SNCUtils::UIDSTRtoUID(uidstr, &uid);

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    buffer.write(QJsonDocument(json).toJson());
    SNC_EHEAD *header = clientBuildLocalE2EMessage(m_controlPort, &uid, remotePort, data.length());
    if (header == NULL)
        return;
    memcpy((char *)(header + 1), data.data(), data.length());
    clientSendMessage(m_controlPort, header, data.length(), SNCLINK_LOWPRI);
}


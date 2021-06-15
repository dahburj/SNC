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

#ifndef CONFIGCLIENT_H
#define CONFIGCLIENT_H

#include "SNCJSON.h"
#include "SNCEndpoint.h"

class ConfigClient : public SNCEndpoint
{
    Q_OBJECT

public:
    ConfigClient();

public slots:
    void newApp(QString servicePath);
    void stopApp();
    void requestDir();
    void sendAppData(QJsonObject json);
    void sendControlData(QJsonObject json, QString uid, int port);

signals:
    void receiveAppData(QJsonObject json);
    void receiveControlData(QJsonObject json, QString uid, int port);
    void dirResponse(QStringList directory);

protected:
    void appClientInit();
    void appClientExit();
    void appClientReceiveE2E(int servicePort, SNC_EHEAD *header, int len);
    void appClientReceiveDirectory(QStringList);

private:
    int m_appPort;
    int m_controlPort;
};

#endif // CONFIGCLIENT_H


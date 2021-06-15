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

#ifndef VIEWCLIENT_H
#define VIEWCLIENT_H

#include "SNCEndpoint.h"
#include "AVSource.h"
#include "SNCCFSClient.h"

class ViewClient : public SNCCFSClient
{
    Q_OBJECT

public:
    ViewClient();

public slots:
    void addService(AVSource *avSource);
    void removeService(AVSource *avSource);
    void enableService(AVSource *avSource);
    void disableService(AVSource *avSource);
    void requestDir();

signals:
    void clientConnected();
    void clientClosed();
    void dirResponse(QStringList directory);

protected:
    void appClientInit() override;
    void appClientBackground() override;
    void appClientConnected() override;
    void appClientClosed() override;
    void appClientReceiveMulticast(int servicePort, SNC_EHEAD *multiCast, int len) override;
    void appClientReceiveDirectory(QStringList) override;


private:
};

#endif // VIEWCLIENT_H


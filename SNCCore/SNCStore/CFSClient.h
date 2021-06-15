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

#ifndef CFSCLIENT_H
#define CFSCLIENT_H

#include "SNCDefs.h"
#include "SNCEndpoint.h"

#include <qobject.h>

class CFSThread;
class StoreCFS;

class CFSClient : public SNCEndpoint
{
    Q_OBJECT

friend class CFSThread;
friend class StoreCFS;

public:
    CFSClient(QObject *parent);
    ~CFSClient() {}

    CFSThread *getCFSThread();

    void sendMessage(SNC_EHEAD *message, int length);

protected:
    void appClientInit();
    void appClientExit();
    void appClientReceiveE2E(int servicePort, SNC_EHEAD *message, int length);
    void appClientBackground();

    bool appClientProcessThreadMessage(SNCThreadMsg *msg);

    int m_CFSPort;                                          // the local service port assigned to CFS

private:

    QObject *m_parent;
    CFSThread *m_CFSThread;                                 // the worker thread
};

#endif // CFSCLIENT_H



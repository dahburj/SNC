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

#ifndef SNCTUNNEL_H
#define SNCTUNNEL_H

#include "SNCDefs.h"
#include "SNCControl.h"
#include "SNCServer.h"
#include "SNCHello.h"

// SNCTunnel

class SNCTunnel
{

public:
    SNCTunnel(SNCServer *server, SS_COMPONENT *component, SNCHello *helloTask, SNCHELLOENTRY *helloEntry);
    virtual ~SNCTunnel();

    void tunnelBackground();
    void connected();                                       // called when onconnect message received
    void close();                                           // close a tunnel

    bool m_connected;                                       // true if connection active
    bool m_connectInProgress;                               // true if in middle of connecting the SNCLink
    SNCHELLOENTRY m_helloEntry;                             // the hello entry for the other end of this tunnel

protected:

    bool connect();                                         // try to connect to target SNCControl

    SNCHello *m_helloTask;                                  // this is to record SNCServer's Hello task
    SNCServer *m_server;                                    // the server task
    SS_COMPONENT *m_comp;                                   // the component to which this tunnel belongs

    qint64 m_connWait;                                      // timer between connection attempts
};

#endif // SNCTUNNEL_H



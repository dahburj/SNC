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

#include "SNCTunnel.h"
#include "SNCThread.h"
#include "SNCHello.h"
#include "SNCEndpoint.h"

// SNCTunnel

#define	SNCTUNNEL_CONNWAIT                    (5 * SNC_CLOCKS_PER_SEC)    // interval between connection attempts

#define TAG "Tunnel"

SNCTunnel::SNCTunnel(SNCServer *server, SS_COMPONENT *SNCComponent, SNCHello *helloTask, SNCHELLOENTRY *helloEntry)
{
    m_server = server;                                      // the server task
    m_comp = SNCComponent;                                  // the component to which this tunnel belongs
    if (helloEntry != NULL)
        m_helloEntry = *helloEntry;
    m_helloTask = helloTask;
    m_connected = false;
    m_connectInProgress = false;
    m_connWait = SNCUtils::clock() - SNCTUNNEL_CONNWAIT;
}

SNCTunnel::~SNCTunnel()
{
}


bool SNCTunnel::connect()
{
    int	returnValue;
    char str[1024];
    int bufSize = SNC_MESSAGE_MAX * 3;
    qint64 now = SNCUtils::clock();

    m_connectInProgress = false;
    m_connected = false;

    if (!SNCUtils::timerExpired(now, m_connWait, SNCTUNNEL_CONNWAIT))
        return false;                                       // not time yet

    m_connWait = now;

    if (!m_comp->tunnelStatic) {

        //  try to find a SyntroControl in the appropriate state

        if (!m_helloTask->findComponent(&m_helloEntry, m_helloEntry.hello.appName, (char *)COMPTYPE_CONTROL)) {// no SyntroControl
            sprintf(str, "Waiting for SyntroControl %s in this run mode...", m_helloEntry.hello.appName);
            SNCUtils::logDebug(TAG, str);
            return false;
        }
    }
    if (m_comp->sock != NULL){
        delete m_comp->sock;
        delete m_comp->link;
        m_comp->sock = NULL;
        m_comp->link = NULL;
    }

    //	Create a new socket

    int id = m_server->getNextConnectionID();
    if (id == -1)
        return false;

    if (m_comp->tunnelStatic)
        m_comp->sock = new SNCSocket(m_server, id, m_comp->tunnelEncrypt);
    else
        m_comp->sock = new SNCSocket(m_server, id, m_server->m_encryptLocal);
    m_server->setComponentSocket(m_comp, m_comp->sock);
    m_comp->link = new SNCLink(TAG);
    returnValue = m_comp->sock->sockCreate(0, SOCK_STREAM);

    m_comp->sock->sockSetConnectMsg(SNCSERVER_ONCONNECT_MESSAGE);
    m_comp->sock->sockSetCloseMsg(SNCSERVER_ONCLOSE_MESSAGE);
    m_comp->sock->sockSetReceiveMsg(SNCSERVER_ONRECEIVE_MESSAGE);

    m_comp->sock->sockSetReceiveBufSize(bufSize);
    m_comp->sock->sockSetSendBufSize(bufSize);

    if (returnValue == 0)
        return false;

    if (!m_comp->tunnelStatic)
        returnValue = m_comp->sock->sockConnect(m_helloEntry.IPAddr,
        m_comp->tunnelEncrypt ? m_server->m_socketNumberEncrypt : m_server->m_socketNumber);
    else
        returnValue = m_comp->sock->sockConnect(qPrintable(m_comp->tunnelStaticPrimary),
        m_comp->tunnelEncrypt ? m_server->m_staticTunnelSocketNumberEncrypt : m_server->m_staticTunnelSocketNumber);
    m_connectInProgress = true;
    m_comp->lastHeartbeatReceived = SNCUtils::clock();
    return	true;
}

void	SNCTunnel::connected()
{
    m_connected = true;
    m_connectInProgress = false;
    if (!m_comp->tunnelStatic) {
        SNCUtils::logDebug(TAG, QString("Tunnel to %1 connected").arg(m_helloEntry.hello.appName));
    } else {
        SNCUtils::logDebug(TAG, QString("Tunnel %1 connected").arg(m_comp->tunnelStaticName));
    }
    m_comp->lastHeartbeatReceived = SNCUtils::clock();
}

void SNCTunnel::close()
{
    m_connected = false;
    m_connectInProgress = false;
}

void SNCTunnel::tunnelBackground()
{
    if ((!m_connectInProgress) && (!m_connected))
        connect();
}

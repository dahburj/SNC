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

#include "SNCStore.h"
#include "CFSClient.h"
#include "CFSThread.h"

#define TAG "CFSClient"

CFSClient::CFSClient(QObject *parent)
    : SNCEndpoint(SNCCFS_BGND_INTERVAL, TAG),  m_parent(parent)
{
    m_CFSThread = NULL;
}

void CFSClient::appClientInit()
{
    m_CFSPort = clientAddService(SNC_STREAMNAME_CFS, SERVICETYPE_E2E, true);
    m_CFSThread = new CFSThread(this);
    m_CFSThread->resumeThread();
}

void CFSClient::appClientExit()
{
    if (m_CFSThread != NULL)
        m_CFSThread->exitThread();

    m_CFSThread = NULL;
}

void CFSClient::appClientBackground()
{
}

CFSThread *CFSClient::getCFSThread()
{
    return m_CFSThread;
}

void CFSClient::appClientReceiveE2E(int servicePort, SNC_EHEAD *message, int length)
{
    if (servicePort != m_CFSPort) {
        SNCUtils::logWarn(TAG, QString("Message received with incorrect service port %1").arg(servicePort));
        free(message);
        return;
    }

    m_CFSThread->postThreadMessage(SNCSTORE_CFS_MESSAGE, length, message);
}

bool CFSClient::appClientProcessThreadMessage(SNCThreadMsg *msg)
{
    if (msg->message == SNCSTORE_CFS_MESSAGE) {
        clientSendMessage(m_CFSPort, (SNC_EHEAD *)msg->ptrParam, msg->intParam, SNCLINK_MEDHIGHPRI);
        return true;
    }

    return false;
}


void CFSClient::sendMessage(SNC_EHEAD *message, int length)
{
    postThreadMessage(SNCSTORE_CFS_MESSAGE, length, message);
}

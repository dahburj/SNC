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

#include "SNCDefs.h"
#include "SNCLink.h"

//#define SNCLINK_TRACE

#define TAG "SNCLink"


SNCMessageWrapper::SNCMessageWrapper()
{
    m_next = NULL;
    m_cmd = 0;
    m_len = 0;
    m_msg = NULL;
    m_ptr = NULL;
    m_bytesLeft = 0;
}

SNCMessageWrapper::~SNCMessageWrapper()
{
    if (m_msg != NULL) {
        free(m_msg);
        m_msg = NULL;
    }
}

//	Public routines
//

void SNCLink::send(int cmd, int len, int priority, SNC_MESSAGE *SNCMessage)
{
    SNCMessageWrapper *wrapper;

#ifdef SNCLINK_TRACE
    SNCUtils::logDebug(TAG, QString("Send - cmd = %1, len = %2, priority= %3").arg(cmd).arg(len).arg(priority);
#endif

    QMutexLocker locker(&m_TXLock);

    wrapper = new SNCMessageWrapper();
    wrapper->m_len = len;
    wrapper->m_msg = SNCMessage;
    wrapper->m_ptr = (unsigned char *)SNCMessage;
    wrapper->m_bytesLeft = len;

//	set up SNCMESSAGE header

    SNCMessage->cmd = cmd;
    SNCMessage->flags = priority;
    SNCMessage->spare = 0;
    SNCUtils::convertIntToUC4(len, SNCMessage->len);
    computeChecksum(SNCMessage);

    addToTXQueue(wrapper, priority);
}

bool SNCLink::receive(int priority, int *cmd, int *len, SNC_MESSAGE **SNCMessage)
{
    SNCMessageWrapper *wrapper;

    QMutexLocker locker(&m_RXLock);

    if ((wrapper = getRXHead(priority)) != NULL) {
        *cmd = wrapper->m_cmd;
        *len = wrapper->m_len;
        *SNCMessage = wrapper->m_msg;
        wrapper->m_msg = NULL;
#ifdef SNCLINK_TRACE
        SNCUtils::logDebug(TAG, QString("Receive - cmd = %1, len = %2, priority = %3").arg(*cmd).arg(*len).arg(priority);
#endif
        delete wrapper;
        return true;
    }
    return false;
}

int SNCLink::tryReceiving(SNCSocket *sock)
{
    int bytesRead;
    int len;
    SNCMessageWrapper *wrapper;

    QMutexLocker locker(&m_RXLock);

    while (1) {
        if (m_RXSM) {										// still waiting for message header
            bytesRead = sock->sockReceive((unsigned char *)&m_SNCMessage + sizeof(SNC_MESSAGE) - m_RXIPBytesLeft , m_RXIPBytesLeft);
            if (bytesRead <= 0)
                return 0;
            m_RXIPBytesLeft -= bytesRead;
            if (m_RXIPBytesLeft == 0) {						// got complete SNC_MESSAGE header
                if (!checkChecksum(&m_SNCMessage)) {
                    SNCUtils::logError(TAG, QString("Incorrect header cksm"));
                    flushReceive(sock);
                    continue;
                }
#ifdef SNCLINK_TRACE
                TRACE2("Received hdr %d %d", m_SNCMessage.cmd, SNCUtils::convertUC4ToInt(m_SNCMessage.len));
#endif
                m_RXIPPriority = m_SNCMessage.flags & SNCLINK_PRI;
                len = SNCUtils::convertUC4ToInt(m_SNCMessage.len);
                if (m_RXIP[m_RXIPPriority] == NULL) {		// nothing in progress at this priority
                    m_RXIP[m_RXIPPriority] = new SNCMessageWrapper();
                    m_RXIP[m_RXIPPriority]->m_cmd = m_SNCMessage.cmd;
                    m_RXIP[m_RXIPPriority]->m_msg = (SNC_MESSAGE *)malloc(len);
                    if (len > (int)sizeof(SNC_MESSAGE)) {
                        m_RXIP[m_RXIPPriority]->m_ptr = (unsigned char *)m_RXIP[m_RXIPPriority]->m_msg + sizeof(SNC_MESSAGE); // we've already received that
                    }
                }
                wrapper = m_RXIP[m_RXIPPriority];
                memcpy(wrapper->m_msg, &m_SNCMessage, sizeof(SNC_MESSAGE));
                wrapper->m_len = len;
                if (len == sizeof(SNC_MESSAGE)) {		// no message part
                    addToRXQueue(wrapper, m_RXIPPriority);
                    m_RXIP[m_RXIPPriority] = NULL;
                    resetReceive(m_RXIPPriority);
                    continue;
                }
                else
                {											// is a message part
                    if ((m_SNCMessage.cmd < SNCMSG_HEARTBEAT) || (m_SNCMessage.cmd > SNCMSG_MAX)) {
                        SNCUtils::logError(TAG, QString("Illegal cmd %1").arg(m_SNCMessage.cmd));
                        flushReceive(sock);
                        free(wrapper->m_msg);
                        wrapper->m_msg = NULL;
                        resetReceive(m_RXIPPriority);
                        continue;
                    }
                    if (len >= SNC_MESSAGE_MAX) {
                        SNCUtils::logError(TAG, QString("Illegal length message cmd %1, len %2").arg(m_SNCMessage.cmd).arg(len));
                        flushReceive(sock);
                        free(wrapper->m_msg);
                        wrapper->m_msg = NULL;
                        resetReceive(m_RXIPPriority);
                        continue;
                    }
                    wrapper->m_bytesLeft = len - sizeof(SNC_MESSAGE);	// since we've already received that
                    m_RXSM = false;
                }
            }
        } else {											// now waiting for data
            wrapper = m_RXIP[m_RXIPPriority];
            bytesRead = sock->sockReceive(wrapper->m_ptr, wrapper->m_bytesLeft);
            if (bytesRead <= 0)
                return 0;
            wrapper->m_bytesLeft -= bytesRead;
            wrapper->m_ptr += bytesRead;
            if (wrapper->m_bytesLeft == 0) {				// got complete message
                addToRXQueue(m_RXIP[m_RXIPPriority], m_RXIPPriority);
                m_RXIP[m_RXIPPriority] = NULL;
                m_RXSM = true;
                m_RXIPMsgPtr = (unsigned char *)&m_SNCMessage;
                m_RXIPBytesLeft = sizeof(SNC_MESSAGE);
            }
        }
    }
}

int SNCLink::trySending(SNCSocket *sock)
{
    int bytesSent;
    int priority;
    SNCMessageWrapper *wrapper;

    QMutexLocker locker(&m_TXLock);

    if (sock == NULL)
        return 0;

    priority = SNCLINK_HIGHPRI;

    while(1) {
        if (m_TXIP[priority] == NULL) {
            m_TXIP[priority] = getTXHead(priority);

            if (m_TXIP[priority] == NULL) {
                priority++;

                if (priority > SNCLINK_LOWPRI)
                    return 0;								// nothing to do and no error

                continue;
            }
        }

        wrapper = m_TXIP[priority];

        bytesSent = sock->sockSend(wrapper->m_ptr, wrapper->m_bytesLeft);
        if (bytesSent <= 0)
            return 0;										// assume buffer full

        wrapper->m_bytesLeft -= bytesSent;
        wrapper->m_ptr += bytesSent;

        if (wrapper->m_bytesLeft == 0) {					// finished this message
            delete m_TXIP[priority];
            m_TXIP[priority] = NULL;
        }
    }
}


SNCLink::SNCLink(const QString& logTag)
{
    m_logTag = logTag;
    for (int i = 0; i < SNCLINK_PRIORITIES; i++) {
        m_TXHead[i] = NULL;
        m_TXTail[i] = NULL;
        m_RXHead[i] = NULL;
        m_RXTail[i] = NULL;
        m_RXIP[i] = NULL;
        m_TXIP[i] = NULL;
    }

    m_RXSM = true;
    m_RXIPMsgPtr = (unsigned char *)&m_SNCMessage;
    m_RXIPBytesLeft = sizeof(SNC_MESSAGE);
}

SNCLink::~SNCLink(void)
{
    clearTXQueue();
    clearRXQueue();
}


void SNCLink::addToTXQueue(SNCMessageWrapper *wrapper, int priority)
{
    if (m_TXHead[priority] == NULL) {
        m_TXHead[priority] = wrapper;
        m_TXTail[priority] = wrapper;
        wrapper->m_next = NULL;
        return;
    }

    m_TXTail[priority]->m_next = wrapper;
    m_TXTail[priority] = wrapper;
    wrapper->m_next = NULL;
}


void	SNCLink::addToRXQueue(SNCMessageWrapper *wrapper, int priority)
{
    if (m_RXHead[priority] == NULL) {
        m_RXHead[priority] = wrapper;
        m_RXTail[priority] = wrapper;
        wrapper->m_next = NULL;
        return;
    }

    m_RXTail[priority]->m_next = wrapper;
    m_RXTail[priority] = wrapper;
    wrapper->m_next = NULL;
}


SNCMessageWrapper *SNCLink::getTXHead(int priority)
{
    SNCMessageWrapper *wrapper;

    if (m_TXHead[priority] == NULL)
        return NULL;

    wrapper = m_TXHead[priority];
    m_TXHead[priority] = wrapper->m_next;
    return wrapper;
}

SNCMessageWrapper *SNCLink::getRXHead(int priority)
{
    SNCMessageWrapper *wrapper;

    if (m_RXHead[priority] == NULL)
        return NULL;

    wrapper = m_RXHead[priority];
    m_RXHead[priority] = wrapper->m_next;
    return wrapper;
}


void SNCLink::clearTXQueue()
{
    for (int i = 0; i < SNCLINK_PRIORITIES; i++) {
        while (m_TXHead[i] != NULL)
            delete getTXHead(i);

        if (m_TXIP[i] != NULL)
            delete m_TXIP[i];

        m_TXIP[i] = NULL;
    }
}

void SNCLink::flushReceive(SNCSocket *sock)
{
    unsigned char buf[1000];

    while (sock->sockReceive(buf, 1000) > 0)
        ;
}

void SNCLink::resetReceive(int priority)
{
    m_RXSM = true;
    m_RXIPMsgPtr = (unsigned char *)&m_SNCMessage;
    m_RXIPBytesLeft = sizeof(SNC_MESSAGE);

    if (m_RXIP[priority] != NULL) {
        delete m_RXIP[priority];
        m_RXIP[priority] = NULL;
    }
}

void SNCLink::clearRXQueue()
{
    for (int i = 0; i < SNCLINK_PRIORITIES; i++) {
        while (m_RXHead[i] != NULL)
            delete getRXHead(i);

        if (m_RXIP[i] != NULL)
            delete m_RXIP[i];

        m_RXIP[i] = NULL;
        resetReceive(i);
    }
}

void SNCLink::computeChecksum(SNC_MESSAGE *SNCMessage)
{
    unsigned char sum;
    unsigned char *array;

    sum = 0;
    array = (unsigned char *)SNCMessage;

    for (size_t i = 0; i < sizeof(SNC_MESSAGE)-1; i++)
        sum += *array++;

    SNCMessage->cksm = -sum;
}

bool SNCLink::checkChecksum(SNC_MESSAGE *SNCMessage)
{
    unsigned char sum;
    unsigned char *array;

    sum = 0;
    array = (unsigned char *)SNCMessage;

    for (size_t i = 0; i < sizeof(SNC_MESSAGE); i++)
        sum += *array++;

    return sum == 0;
}

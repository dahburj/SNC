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

#ifndef _SNCLINK_H_
#define _SNCLINK_H_

#include <qstring.h>

#include "SNCSocket.h"

//  The internal version of SNCMESSAGE

class SNCMessageWrapper
{
public:
    SNCMessageWrapper(void);
    ~SNCMessageWrapper(void);

    SNC_MESSAGE *m_msg;                                     // message buffer pointer
    int m_len;                                              // total length
    SNCMessageWrapper *m_next;                              // pointer to next in chain
    int m_bytesLeft;                                        // bytes left to be received or sent
    unsigned char *m_ptr;                                   // pointer in m_pMsg while receiving or transmitting

//  for receive

    int m_cmd;                                              // the current command
};


//	The SNCLink class itself

class SNCLink
{
public:
    SNCLink(const QString& logTag);
    ~SNCLink(void);

    void send(int cmd, int len, int priority, SNC_MESSAGE *syntroMessage);
    bool receive(int priority, int *cmd, int *len, SNC_MESSAGE **syntroMessage);

    int tryReceiving(SNCSocket *sock);
    int trySending(SNCSocket *sock);

protected:
    void clearTXQueue();
    void clearRXQueue();
    void resetReceive(int priority);
    void flushReceive(SNCSocket *sock);
    SNCMessageWrapper *getTXHead(int priority);
    SNCMessageWrapper *getRXHead(int priority);
    void addToTXQueue(SNCMessageWrapper *wrapper, int nPri);
    void addToRXQueue(SNCMessageWrapper *wrapper, int nPri);
    void computeChecksum(SNC_MESSAGE *SNCMessage);
    bool checkChecksum(SNC_MESSAGE *SNCMessage);

    SNCMessageWrapper *m_TXHead[SNCLINK_PRIORITIES];        // head of transmit list
    SNCMessageWrapper *m_TXTail[SNCLINK_PRIORITIES];        // tail of transmit list
    SNCMessageWrapper *m_RXHead[SNCLINK_PRIORITIES];        // head of receive list
    SNCMessageWrapper *m_RXTail[SNCLINK_PRIORITIES];        // tail of receive list

    SNCMessageWrapper *m_TXIP[SNCLINK_PRIORITIES];          // in progress TX object
    SNCMessageWrapper *m_RXIP[SNCLINK_PRIORITIES];          // in progress RX object
    bool m_RXSM;                                            // true if waiting for SNCMESSAGE header
    unsigned char *m_RXIPMsgPtr;                            // pointer into message header
    int m_RXIPBytesLeft;
    SNC_MESSAGE m_SNCMessage;                               // for receive
    int m_RXIPPriority;                                     // the current priority being received

    QMutex m_RXLock;
    QMutex m_TXLock;

    QString m_logTag;
};

#endif // _SNCLINK_H_


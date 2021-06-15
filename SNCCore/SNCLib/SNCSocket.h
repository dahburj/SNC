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

#ifndef _SNCSOCKET_H
#define _SNCSOCKET_H

#include "SNCUtils.h"
#include "SNCThread.h"

#ifndef NO_SSL
#include <qsslsocket.h>
#include <qsslcipher.h>
#endif

//	Define the standard socket types if necessary

#ifndef	SOCK_STREAM
#define	SOCK_STREAM		0
#define	SOCK_DGRAM		1
#define	SOCK_SERVER		2
#endif

class TCPServer : public QTcpServer
{
public:
    TCPServer(QObject *parent = 0);
#if !defined(Q_OS_IOS)
    bool usingSSL() { return m_encrypt; }
#else
    bool usingSSL() { return false; }
#endif
protected:
    bool m_encrypt;

    QString m_logTag;
};

#ifndef NO_SSL
class SSLServer : public TCPServer
{
    Q_OBJECT

public:
    SSLServer(QObject *parent = 0);
    virtual ~SSLServer() {};

protected:
#if QT_VERSION < 0x050000
    virtual void incomingConnection(int socket);
#else
    virtual void incomingConnection(qintptr socket);
#endif
protected slots:
    void ready();
    void sslErrors(const QList<QSslError>& errors);
    void peerVerifyError(const QSslError & error);
};
#endif

class SNCSocket : public QObject
{
    Q_OBJECT

public:
    SNCSocket(const QString& logTag);
    SNCSocket(SNCThread *thread, int connectionID, bool encrypt);
    virtual ~SNCSocket();

    void sockSetThread(SNCThread *thread);
    int sockGetConnectionID();								// returns the allocated connection ID
    void sockSetConnectMsg(int msg);
    void sockSetAcceptMsg(int msg);
    void sockSetCloseMsg(int msg);
    void sockSetReceiveMsg(int msg);
    void sockSetSendMsg(int msg);
    bool sockEnableBroadcast(int flag);
    bool sockSetReceiveBufSize(int size);
    bool sockSetSendBufSize(int size);
    int sockSendTo(const void *buf, int bufLen, int hostPort, char *host = NULL);
    int sockReceiveFrom(void *buf, int bufLen, char *IpAddr, unsigned int *port, int flags = 0);
    int sockCreate(int socketPort, int socketType, int flags = 0);
    bool sockConnect(const char *addr, int port);
    bool sockAccept(SNCSocket& sock, char *IpAddr, int *port);
    bool sockClose();
    int sockListen();
    int sockReceive(void *buf, int bufLen);
    int sockSend(void *buf, int bufLen);
    int sockPendingDatagramSize();
#ifndef NO_SSL
    bool usingSSL() { return m_encrypt; }
#else
    bool usingSSL() { return false; }
#endif

public slots:
    void onConnect();
    void onAccept();
    void onClose();
    void onReceive();
    void onSend(qint64 bytes);
    void onError(QAbstractSocket::SocketError socketError);
    void onState(QAbstractSocket::SocketState socketState);
#ifndef NO_SSL
    void sslErrors(const QList<QSslError> & errors);
    void peerVerifyError(const QSslError & error);
#endif

protected:
    SNCThread	*m_ownerThread;
    bool m_encrypt;                                         // if using SSL
    int m_connectionID;
    int m_sockType;											// the socket type
    int m_sockPort;											// the socket port number
    QUdpSocket *m_UDPSocket;
    QTcpSocket *m_TCPSocket;                                // this could be a QSslSocket if SSL in use
    TCPServer *m_server;                                    // This could be SSLServer if SSL in use

    void clearSocket();										// clear up all socket fields
    int m_onConnectMsg;
    int m_onAcceptMsg;
    int m_onCloseMsg;
    int m_onReceiveMsg;
    int m_onSendMsg;

    int m_state;											// last reported socket state

    QString m_logTag;
};

#endif // _SNCSOCKET_H



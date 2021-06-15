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

#include "SNCSocket.h"

// SNCSocket

//  This constructor only used by the SNCHello system

SNCSocket::SNCSocket(const QString& logTag)
{
    m_logTag = logTag;
    clearSocket();
}

//  This is what everyone else uses

SNCSocket::SNCSocket(SNCThread *thread, int connectionID, bool encrypt)
{
    clearSocket();
    m_ownerThread = thread;
    m_connectionID = connectionID;
    m_encrypt = encrypt;
    SNCUtils::logDebug(m_logTag, QString("Allocated new socket with connection ID %1").arg(m_connectionID));
}

void SNCSocket::clearSocket()
{
    m_ownerThread = NULL;
    m_onConnectMsg = -1;
    m_onCloseMsg = -1;
    m_onReceiveMsg = -1;
    m_onAcceptMsg = -1;
    m_onSendMsg = -1;
    m_TCPSocket = NULL;
    m_UDPSocket = NULL;
    m_server = NULL;
    m_state = -1;
}

//	Set nFlags = true for reuseaddr

int	SNCSocket::sockCreate(int nSocketPort, int nSocketType, int nFlags)
{
    bool	ret;

    m_sockType = nSocketType;
    m_sockPort = nSocketPort;
    switch (m_sockType) {
        case SOCK_DGRAM:
            m_UDPSocket = new QUdpSocket(this);
            connect(m_UDPSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
            connect(m_UDPSocket, SIGNAL(stateChanged ( QAbstractSocket::SocketState) ), this, SLOT(onState( QAbstractSocket::SocketState)));
            if (nFlags)
                ret = m_UDPSocket->bind(nSocketPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
            else
                ret = m_UDPSocket->bind(nSocketPort);
            return ret;

        case SOCK_STREAM:
#ifndef NO_SSL
            if (m_encrypt) {
                m_TCPSocket = new QSslSocket(this);
                connect((QSslSocket *)m_TCPSocket, SIGNAL(sslErrors(const QList<QSslError>&)),
                    this, SLOT(sslErrors(const QList<QSslError>&)));
                connect((QSslSocket *)m_TCPSocket, SIGNAL(peerVerifyError(const QSslError&)),
                    this, SLOT(peerVerifyError(const QSslError&)));
                ((QSslSocket *)m_TCPSocket)->setPeerVerifyMode(QSslSocket::VerifyNone);
            } else {
#endif
                m_TCPSocket = new QTcpSocket(this);
                connect(m_TCPSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
#ifndef NO_SSL
            }
#endif
            connect(m_TCPSocket, SIGNAL(stateChanged( QAbstractSocket::SocketState) ), this, SLOT(onState( QAbstractSocket::SocketState)));
            return 1;

        case SOCK_SERVER:
#ifndef NO_SSL
            if (m_encrypt)
                m_server = new SSLServer(this);
            else
#endif
                m_server = new TCPServer(this);
            return 1;

        default:
            SNCUtils::logError(m_logTag, QString("Socket create on illegal type %1").arg(nSocketType));
            return 0;
    }
}

bool SNCSocket::sockConnect(const char *addr, int port)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for create %1").arg(m_sockType));
        return false;
    }
#ifndef NO_SSL
    if (m_encrypt)
        ((QSslSocket *)m_TCPSocket)->connectToHostEncrypted(addr, port);
    else
#endif
        m_TCPSocket->connectToHost(addr, port);
    return true;
}

bool SNCSocket::sockAccept(SNCSocket& sock, char *IpAddr, int *port)
{
    sock.m_TCPSocket = m_server->nextPendingConnection();
    QString v4Addr = sock.m_TCPSocket->peerAddress().toString();
    if (v4Addr.startsWith("::ffff:"))
        v4Addr.remove(0, 7);
    strcpy(IpAddr, (char *)v4Addr.toLocal8Bit().constData());
    *port = (int)sock.m_TCPSocket->peerPort();
    sock.m_sockType = SOCK_STREAM;
    sock.m_ownerThread = m_ownerThread;
    sock.m_state = QAbstractSocket::ConnectedState;
    sock.m_encrypt = m_server->usingSSL();
    return true;
}

bool SNCSocket::sockClose()
{
    switch (m_sockType) {
        case SOCK_DGRAM:
            disconnect(m_UDPSocket, 0, 0, 0);
            m_UDPSocket->close();
            delete m_UDPSocket;
            m_UDPSocket = NULL;
            break;

        case SOCK_STREAM:
            disconnect(m_TCPSocket, 0, 0, 0);
            m_TCPSocket->close();
            delete m_TCPSocket;
            m_TCPSocket = NULL;
            break;

        case SOCK_SERVER:
            disconnect(m_server, 0, 0, 0);
            m_server->close();
            delete m_server;
            m_server = NULL;
            break;
    }
    clearSocket();
    return true;
}

int	SNCSocket::sockListen()
{
    if (m_sockType != SOCK_SERVER) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for listen %1").arg(m_sockType));
        return false;
    }
    return m_server->listen(QHostAddress::Any, m_sockPort);
}


int	SNCSocket::sockReceive(void *lpBuf, int nBufLen)
{
    switch (m_sockType) {
        case SOCK_DGRAM:
            return m_UDPSocket->read((char *)lpBuf, nBufLen);

        case SOCK_STREAM:
            if (m_state != QAbstractSocket::ConnectedState)
                return 0;
            return m_TCPSocket->read((char *)lpBuf, nBufLen);

        default:
            SNCUtils::logError(m_logTag, QString("Incorrect socket type for receive %1").arg(m_sockType));
            return -1;
    }
}

int	SNCSocket::sockSend(void *lpBuf, int nBufLen)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for send %1").arg(m_sockType));
        return false;
    }
    if (m_state != QAbstractSocket::ConnectedState)
        return 0;
    int ret = m_TCPSocket->write((char *)lpBuf, nBufLen);
    m_TCPSocket->flush();
    return ret;
}

bool SNCSocket::sockEnableBroadcast(int)
{
    return true;
}

bool SNCSocket::sockSetReceiveBufSize(int nSize)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetReceiveBufferSize %1").arg(m_sockType));
        return false;
    }
    m_TCPSocket->setReadBufferSize(nSize);
    return true;
}

bool SNCSocket::sockSetSendBufSize(int)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetSendBufferSize %1").arg(m_sockType));
        return false;
    }

//	SNCUtils::logDebug(m_logTag, QString("SetSendBufferSize not implemented"));
    return true;
}


int SNCSocket::sockSendTo(const void *buf, int bufLen, int hostPort, char *host)
{
    int ret;

    if (m_sockType != SOCK_DGRAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SendTo %1").arg(m_sockType));
        return false;
    }
    if (host == NULL)
        ret = m_UDPSocket->writeDatagram((const char *)buf, bufLen, SNCUtils::getMyBroadcastAddress(), hostPort);
    else
        ret = m_UDPSocket->writeDatagram((const char *)buf, bufLen, QHostAddress(host), hostPort);
    m_UDPSocket->flush();
    return ret;
}

int	SNCSocket::sockReceiveFrom(void *buf, int bufLen, char *IpAddr, unsigned int *port, int)
{
    QHostAddress ha;
    quint16 qport;

    if (m_sockType != SOCK_DGRAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for ReceiveFrom %1").arg(m_sockType));
        return false;
    }
    int	nRet = m_UDPSocket->readDatagram((char *)buf, bufLen, &ha, &qport);
    *port = qport;
    strcpy(IpAddr, qPrintable(ha.toString()));
    return nRet;
}

int		SNCSocket::sockPendingDatagramSize()
{
    if (m_sockType != SOCK_DGRAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for PendingDatagramSize %1").arg(m_sockType));
        return false;
    }
    return m_UDPSocket->pendingDatagramSize();
}


SNCSocket::~SNCSocket()
{
    if (m_sockType != -1)
        sockClose();
}

// SNCSocket member functions

int SNCSocket::sockGetConnectionID()
{
    return m_connectionID;
}

void SNCSocket::sockSetThread(SNCThread *thread)
{
    m_ownerThread = thread;
}

void SNCSocket::sockSetConnectMsg(int msg)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetConnectMsg %1").arg(m_sockType));
        return;
    }
    m_onConnectMsg = msg;
    connect(m_TCPSocket, SIGNAL(connected()), this, SLOT(onConnect()));
}


void SNCSocket::sockSetAcceptMsg(int msg)
{
    if (m_sockType != SOCK_SERVER) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetAcceptMsg %1").arg(m_sockType));
        return;
    }
    m_onAcceptMsg = msg;
    connect(m_server, SIGNAL(newConnection()), this, SLOT(onAccept()));
}

void SNCSocket::sockSetCloseMsg(int msg)
{
    if (m_sockType != SOCK_STREAM) {
        SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetCloseMsg %1").arg(m_sockType));
        return;
    }
    m_onCloseMsg = msg;
    connect(m_TCPSocket, SIGNAL(disconnected()), this, SLOT(onClose()));
}

void SNCSocket::sockSetReceiveMsg(int msg)
{
    m_onReceiveMsg = msg;

    switch (m_sockType) {
        case SOCK_DGRAM:
            connect(m_UDPSocket, SIGNAL(readyRead()), this, SLOT(onReceive()));
            break;

        case SOCK_STREAM:
            connect(m_TCPSocket, SIGNAL(readyRead()), this, SLOT(onReceive()), Qt::DirectConnection);
            break;

        default:
            SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetReceiveMsg %1").arg(m_sockType));
            break;
    }
}

void SNCSocket::sockSetSendMsg(int msg)
{
    m_onSendMsg = msg;
    switch (m_sockType)
    {
        case SOCK_DGRAM:
            connect(m_UDPSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onSend(qint64)));
            break;

        case SOCK_STREAM:
            connect(m_TCPSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onSend(qint64)), Qt::DirectConnection);
            break;

        default:
            SNCUtils::logError(m_logTag, QString("Incorrect socket type for SetSendMsg %1").arg(m_sockType));
            break;
    }
}

void SNCSocket::onConnect()
{
    if (m_onConnectMsg != -1)
        m_ownerThread->postThreadMessage(m_onConnectMsg, m_connectionID, NULL);
}

void SNCSocket::onAccept()
{
    if (m_onAcceptMsg != -1)
        m_ownerThread->postThreadMessage(m_onAcceptMsg, m_connectionID, NULL);
}

void SNCSocket::onClose()
{
    if (m_onCloseMsg != -1)
        m_ownerThread->postThreadMessage(m_onCloseMsg, m_connectionID, NULL);
}

void SNCSocket::onReceive()
{
    if (m_onReceiveMsg != -1)
        m_ownerThread->postThreadMessage(m_onReceiveMsg, m_connectionID, NULL);
}

void	SNCSocket::onSend(qint64)
{
    if (m_onSendMsg != -1)
        m_ownerThread->postThreadMessage(m_onSendMsg, m_connectionID, NULL);
}

void SNCSocket::onError(QAbstractSocket::SocketError errnum)
{
    switch (m_sockType) {
        case SOCK_DGRAM:
            if (errnum != QAbstractSocket::AddressInUseError)
                SNCUtils::logDebug(m_logTag, QString("UDP socket error %1").arg(m_UDPSocket->errorString()));

            break;

        case SOCK_STREAM:
            SNCUtils::logDebug(m_logTag, QString("TCP socket error %1").arg(m_TCPSocket->errorString()));
            break;
    }
}

void SNCSocket::onState(QAbstractSocket::SocketState socketState)
{
    switch (m_sockType)
    {
        case SOCK_DGRAM:
            SNCUtils::logDebug(m_logTag, QString("UDP socket state %1").arg(socketState));
            break;

        case SOCK_STREAM:
            SNCUtils::logDebug(m_logTag, QString("TCP socket state %1").arg(socketState));
            break;
    }
    if ((socketState == QAbstractSocket::UnconnectedState) && (m_state < QAbstractSocket::ConnectedState)) {
        SNCUtils::logDebug(m_logTag, QString("onClose generated by onState"));
        onClose();									// no signal generated in this situation
    }
    m_state = socketState;
}

#ifndef NO_SSL
void SNCSocket::peerVerifyError(const QSslError & error)
{
    QString msg = "Peer verify error from " + m_TCPSocket->peerAddress().toString() + ": "
        + error.errorString() /* + "\n" + error.certificate().toText() */ ;
    SNCUtils::logWarn(m_logTag, msg);
}

void SNCSocket::sslErrors(const QList<QSslError> & errors) {

    foreach(QSslError err, errors) {
        QString msg = "SSL handshake error from " + m_TCPSocket->peerAddress().toString()
            + ": " + err.errorString() /* + "\n" + err.certificate().toText() */ ;
        SNCUtils::logWarn(m_logTag, msg);
    }

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());
    sslSocket->ignoreSslErrors();

}
#endif

//----------------------------------------------------------
//
//  TCPServer

TCPServer::TCPServer(QObject *parent) : QTcpServer(parent)
{
    m_encrypt = false;
    m_logTag = "TCPServer";
}

#ifndef NO_SSL
//----------------------------------------------------------
//
//  SSLServer

SSLServer::SSLServer(QObject *parent) : TCPServer(parent)
{
    m_encrypt = true;
    m_logTag = "SSLServer";
}

#if QT_VERSION < 0x050000
void SSLServer::incomingConnection(int socket)
#else
void SSLServer::incomingConnection(qintptr socket)
#endif
{

    Q_UNUSED(socket);

    //
    // Prepare a new server socket for the incoming connection
    //
    QSslSocket *sslSocket = new QSslSocket();

    sslSocket->setProtocol( QSsl::AnyProtocol );
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    sslSocket->setLocalCertificate("./server.crt", QSsl::Pem);
    sslSocket->setPrivateKey("./server.key", QSsl::Rsa, QSsl::Pem, "");
    sslSocket->setProtocol(QSsl::TlsV1_3OrLater);

    if (sslSocket->setSocketDescriptor(socket)) {
       connect(sslSocket, SIGNAL(encrypted()), this, SLOT(ready()));
       connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(sslErrors(const QList<QSslError>&)));
       connect(sslSocket, SIGNAL(peerVerifyError(const QSslError&)), this, SLOT(peerVerifyError(const QSslError&)));
       addPendingConnection(sslSocket);

       sslSocket->startServerEncryption();

    }
    else {
       delete sslSocket;
    }
}

void SSLServer::ready() {

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());

    QSslCipher ciph = sslSocket->sessionCipher();
    QString cipher = QString("%1, %2 (%3/%4)").arg(ciph.authenticationMethod())
                          .arg(ciph.name()).arg(ciph.usedBits()).arg(ciph.supportedBits());

    QString msg = "SSL session from " + sslSocket->peerAddress().toString() + " established with cipher: " + cipher;
    SNCUtils::logDebug(m_logTag, msg);
}

void SSLServer::peerVerifyError(const QSslError & error)
{
    QString msg = "Peer verify error: " + error.errorString() /* + "\n" + error.certificate().toText() */;
    SNCUtils::logWarn(m_logTag, msg);
}

void SSLServer::sslErrors(const QList<QSslError> & errors) {

    foreach(QSslError err, errors) {
        QString msg = "SSL handshake error: " + err.errorString() /* + "\n" + err.certificate().toText() */;
        SNCUtils::logWarn(m_logTag, msg);
    }

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());
    sslSocket->ignoreSslErrors();

}
#endif

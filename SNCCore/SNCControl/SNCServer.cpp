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

#include "SNCControl.h"
#include "SNCTunnel.h"
#include "SNCThread.h"

// SNCServer

#define TAG "SNCServer"

SNCServer::SNCServer() : SNCThread(TAG)
{
    QSettings *settings = SNCUtils::getSettings();

    // process SyntroControl-specific settings

    settings->beginGroup(SNCSERVER_PARAMS_GROUP);

    if (!settings->contains(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET))
        settings->setValue(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET, SNC_SOCKET_LOCAL);

    if (!settings->contains(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET))
        settings->setValue(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET, SNC_PRIMARY_SOCKET_STATICTUNNEL);

    if (!settings->contains(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT))
        settings->setValue(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT, SNC_SOCKET_LOCAL_ENCRYPT);

    if (!settings->contains(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT))
        settings->setValue(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT, SNC_PRIMARY_SOCKET_STATICTUNNEL_ENCRYPT);

    if (!settings->contains(SNCSERVER_PARAMS_HBINTERVAL))
        settings->setValue(SNCSERVER_PARAMS_HBINTERVAL, SNC_HEARTBEAT_INTERVAL);

    if (!settings->contains(SNCSERVER_PARAMS_HBTIMEOUT))
        settings->setValue(SNCSERVER_PARAMS_HBTIMEOUT, SNC_HEARTBEAT_TIMEOUT);

    if (!settings->contains(SNCSERVER_PARAMS_ENCRYPT_LOCAL))
        settings->setValue(SNCSERVER_PARAMS_ENCRYPT_LOCAL, false);

    if (!settings->contains(SNCSERVER_PARAMS_ENCRYPT_STATICTUNNEL_SERVER))
        settings->setValue(SNCSERVER_PARAMS_ENCRYPT_STATICTUNNEL_SERVER, false);

    m_socketNumber = settings->value(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET).toInt();
    m_staticTunnelSocketNumber = settings->value(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET).toInt();

    m_socketNumberEncrypt = settings->value(SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT).toInt();
    m_staticTunnelSocketNumberEncrypt = settings->value(SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT).toInt();

    m_encryptLocal = settings->value(SNCSERVER_PARAMS_ENCRYPT_LOCAL).toBool();
    m_encryptStaticTunnelServer = settings->value(SNCSERVER_PARAMS_ENCRYPT_STATICTUNNEL_SERVER).toBool();

    if (m_encryptLocal || m_encryptStaticTunnelServer) {
        if (!QSslSocket::supportsSsl()) {
            SNCUtils::logWarn(TAG, "Encryption configured but SSL not available. Turning off encryption");
            m_encryptLocal = false;
            m_encryptStaticTunnelServer = false;
        }
    }

    int hbInterval = settings->value(SNCSERVER_PARAMS_HBINTERVAL).toInt();
    m_heartbeatSendInterval =  hbInterval * SNC_CLOCKS_PER_SEC;
    m_heartbeatTimeoutCount = settings->value(SNCSERVER_PARAMS_HBTIMEOUT).toInt();

    int priority = settings->value(SNCSERVER_PARAMS_PRIORITY).toInt();

    settings->endGroup();

    // use some standard settings also

    m_componentData.init(COMPTYPE_CONTROL, hbInterval, priority);

    m_listSyntroLinkSock = NULL;
    m_listStaticTunnelSock = NULL;
    m_hello = NULL;

    delete settings;
}

SNCServer::~SNCServer()
{
}

SNCHello *SNCServer::getHelloObject()
{
    return m_hello;
}

void SNCServer::initThread()
{
    int		i;

    QSettings *settings = SNCUtils::getSettings();

    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++)
        m_components[i].inUse = false;
    for (i = 0; i < SNC_MAX_CONNECTIONIDS; i++)
        m_connectionIDMap[i] = -1;

    loadStaticTunnels(settings);
    loadValidTunnelSources(settings);

    m_nextConnectionID = 0;

    m_multicastManager.m_server = this;
    m_multicastManager.m_myUID = m_componentData.getMyUID();

    m_dirManager.m_server = this;

    m_multicastIn = m_multicastOut = m_E2EIn = m_E2EOut = 0;
    m_multicastInRate = m_multicastOutRate = m_E2EInRate = m_E2EOutRate = 0;
    m_counterStart = SNCUtils::clock();

    m_myUID = m_componentData.getMyUID();
    m_appName = settings->value(SNC_PARAMS_APPNAME).toString();

    m_timer = startTimer(SNCSERVER_INTERVAL);
    m_lastOpenSocketsTime = SNCUtils::clock();

    delete settings;
}


void SNCServer::finishThread()
{
    killTimer(m_timer);

    for (int i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++)
        syCleanup(m_components + i);

    m_dirManager.DMShutdown();
    m_multicastManager.MMShutdown();

    if (m_hello != NULL)
        m_hello->exitThread();

    if (m_listSyntroLinkSock != NULL)
        delete m_listSyntroLinkSock;
    if (m_listStaticTunnelSock != NULL)
        delete m_listStaticTunnelSock;
}

void SNCServer::loadStaticTunnels(QSettings *settings)
{
    int	size = settings->beginReadArray(SNCSERVER_PARAMS_STATIC_TUNNELS);
    settings->endArray();

    if (size == 0) {                                        // set a dummy entry for convenience
        settings->beginWriteArray(SNCSERVER_PARAMS_STATIC_TUNNELS);

        settings->setArrayIndex(0);
        settings->setValue(SNCSERVER_PARAMS_STATIC_NAME, "Tunnel");
        settings->setValue(SNCSERVER_PARAMS_STATIC_DESTIP_PRIMARY, "0.0.0.0");
        settings->setValue(SNCSERVER_PARAMS_STATIC_DESTIP_BACKUP, "");
        settings->setValue(SNCSERVER_PARAMS_STATIC_ENCRYPT, false);

        settings->endArray();
        return;
    }

    // Now create entries for valid static tunnel sources

    settings->beginReadArray(SNCSERVER_PARAMS_STATIC_TUNNELS);
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        QString primary = settings->value(SNCSERVER_PARAMS_STATIC_DESTIP_PRIMARY).toString();
        QString backup = settings->value(SNCSERVER_PARAMS_STATIC_DESTIP_BACKUP).toString();
        if (primary.length() == 0)
            continue;
        if (primary == "0.0.0.0")
            continue;

        SS_COMPONENT *component = getFreeComponent();
        if (component == NULL)
            continue;

        component->inUse = true;
        component->tunnelStatic = true;
        component->tunnelSource = true;
        component->tunnelStaticPrimary = primary;
        component->tunnelStaticBackup = backup;
        component->tunnelStaticName = settings->value(SNCSERVER_PARAMS_STATIC_NAME).toString();
        component->tunnelEncrypt = settings->value(SNCSERVER_PARAMS_STATIC_ENCRYPT).toBool();

        if (component->tunnelEncrypt && !QSslSocket::supportsSsl()) {
            SNCUtils::logWarn(TAG, "Static tunnel uses encryption but SSL not available. Turning off encryption");
            component->tunnelEncrypt = false;
        }

        component->tunnel = new SNCTunnel(this, component, NULL, NULL);
        component->state = ConnWFHeartbeat;
        updateSNCStatus(component);
    }
    settings->endArray();
}

void SNCServer::loadValidTunnelSources(QSettings *settings)
{
    int count = 0;
    QString dummy("0000000000000000");

    settings->beginGroup(SNCSERVER_PARAMS_GROUP);

    int	size = settings->beginReadArray(SNCSERVER_PARAMS_VALID_TUNNEL_SOURCES);
    settings->endArray();

    if (size == 0) {
        settings->beginWriteArray(SNCSERVER_PARAMS_VALID_TUNNEL_SOURCES);
        settings->setArrayIndex(0);
        settings->setValue(SNCSERVER_PARAMS_VALID_TUNNEL_UID, dummy);
        settings->endArray();
        settings->endGroup();
        return;
    }

    settings->beginReadArray(SNCSERVER_PARAMS_VALID_TUNNEL_SOURCES);
    for (int i = 0; i < size; i++) {
        settings->setArrayIndex(i);
        QString uidstr = settings->value(SNCSERVER_PARAMS_VALID_TUNNEL_UID).toString();
        if (uidstr.length() != (sizeof(SNC_UID) * 2))
            continue;
        if (uidstr == dummy)
            continue;

        SNC_UID uid;
        SNCUtils::UIDSTRtoUID((char *)qPrintable(uidstr), &uid);
        m_validTunnelSources.append(uid);
        count++;
    }
    settings->endArray();
    SNCUtils::logInfo(TAG, QString("Loaded %1 valid tunnel sources").arg(count));

    settings->endGroup();
}

bool SNCServer::openSockets()
{
    int ret;

    if ((m_listSyntroLinkSock != NULL) && (m_listStaticTunnelSock != NULL))
        return true;                                        // in normal operating mode

    if ((SNCUtils::clock() - m_lastOpenSocketsTime) < SNCSERVER_SOCKET_RETRY)
        return false;                                       // not time to try again

    m_lastOpenSocketsTime = SNCUtils::clock();

    if (m_listSyntroLinkSock == NULL) {
        m_listSyntroLinkSock = getNewSocket(false);
        if (m_listSyntroLinkSock != NULL) {
            ret = m_listSyntroLinkSock->sockListen();
            if (ret == 0) {
                delete m_listSyntroLinkSock;
                m_listSyntroLinkSock = NULL;
            } else {
                SNCUtils::logInfo(TAG, "Local listener active");
                m_hello = new SNCHello(&m_componentData, TAG);
                m_hello->m_parentThread = this;
                m_hello->m_socketFlags = 1;
                m_hello->resumeThread();
            }
        }
    }

    if (m_listStaticTunnelSock == NULL) {
        m_listStaticTunnelSock = getNewSocket(true);
        if (m_listStaticTunnelSock != NULL) {
            ret = m_listStaticTunnelSock->sockListen();
            if (ret == 0) {
                delete m_listStaticTunnelSock;
                m_listStaticTunnelSock = NULL;
            } else {
                SNCUtils::logInfo(TAG, "Static tunnel listener active");
            }
        }
    }
    return (m_listSyntroLinkSock != NULL) && (m_listStaticTunnelSock != NULL);
}


void SNCServer::setComponentSocket(SS_COMPONENT *SNCComponent, SNCSocket *sock)
{
    SNCComponent->sock = sock;
    SNCComponent->connectionID = sock->sockGetConnectionID(); // record the connection ID

    if (m_connectionIDMap[SNCComponent->connectionID] != -1) {
        SNCUtils::logWarn(TAG, QString("Connection ID %1 allocated to slot %2 but should be free")
            .arg(SNCComponent->connectionID)
            .arg(m_connectionIDMap[SNCComponent->connectionID]));
    }

    m_connectionIDMap[SNCComponent->connectionID] = SNCComponent->index;    // set the map entry
}

SS_COMPONENT *SNCServer::getComponentFromConnectionID(int connectionID)
{
    int componentIndex;
    if ((connectionID < 0) || (connectionID >= SNC_MAX_CONNECTIONIDS)) {
        SNCUtils::logWarn(TAG, QString("Tried to map out of range connection ID %1 to component").arg(connectionID));
        return NULL;
    }
    componentIndex = m_connectionIDMap[connectionID];
    if ((componentIndex < 0) || (componentIndex >= SNC_MAX_CONNECTEDCOMPONENTS)) {
        SNCUtils::logWarn(TAG, QString("Out of range component index %1 in map for connection ID %2").arg(componentIndex).arg(connectionID));
        return NULL;
    }
    return m_components + componentIndex;
}

SNCSocket *SNCServer::getNewSocket(bool staticTunnel)
{
    SNCSocket *sock;
    int	retVal;

    int id = getNextConnectionID();

    if (id == -1)
        return NULL;

    sock = new SNCSocket(this, id, staticTunnel ? m_encryptStaticTunnelServer : m_encryptLocal);
    if (sock == NULL)
        return sock;
    if (!staticTunnel) {
        if (m_encryptLocal)
            retVal = sock->sockCreate(m_socketNumberEncrypt, SOCK_SERVER, 1);
        else
            retVal = sock->sockCreate(m_socketNumber, SOCK_SERVER, 1);
    } else {
        if (m_encryptStaticTunnelServer)
            retVal = sock->sockCreate(m_staticTunnelSocketNumberEncrypt, SOCK_SERVER, 1);   // create with reuse flag set to avoid bind failing on restart
        else
            retVal = sock->sockCreate(m_staticTunnelSocketNumber, SOCK_SERVER, 1);  // create with reuse flag set to avoid bind failing on restart
    }
    if (retVal == 0) {
        delete sock;
        return NULL;
    }
    if (!staticTunnel)
        sock->sockSetAcceptMsg(SNCSERVER_ONACCEPT_MESSAGE);
    else
        sock->sockSetAcceptMsg(SNCSERVER_ONACCEPT_STATICTUNNEL_MESSAGE);
    return sock;
}

int SNCServer::getNextConnectionID()
{
    for (int i = 0; i < SNC_MAX_CONNECTIONIDS; i++) {
        if (++m_nextConnectionID == SNC_MAX_CONNECTIONIDS)
            m_nextConnectionID = 0;

        if (m_connectionIDMap[m_nextConnectionID] == -1)
            return m_nextConnectionID;
    }
    SNCUtils::logError(TAG, "No free connection IDs");
    return -1;                                              // none available
}

//	syConnected - handle connected outgoing calls (tunnel sources)

bool SNCServer::syConnected(SS_COMPONENT *SNCComponent)
{
    if (!SNCComponent->tunnelSource) {
        SNCUtils::logWarn(TAG, "Connected message on component that is not a tunnel source");
        return false;
    }
    if (!SNCComponent->tunnel->m_connectInProgress) {
        SNCUtils::logWarn(TAG, "Connected message on component that is not expecting a connected message");
        return false;
    }
    SNCComponent->tunnel->connected();
    SNCComponent->lastHeartbeatReceived = SNCUtils::clock();
    SNCComponent->lastHeartbeatSent = SNCUtils::clock() - m_heartbeatSendInterval;
    SNCComponent->heartbeatInterval = m_heartbeatSendInterval;
    SNCComponent->state = ConnWFHeartbeat;
    if (SNCComponent->dirManagerConnComp == NULL)
        SNCComponent->dirManagerConnComp = m_dirManager.DMAllocateConnectedComponent(SNCComponent);
    return	true;
}


//	SyAccept - handle incoming calls

bool	SNCServer::syAccept(bool staticTunnel)
{

    char IPStr[SNC_MAX_NONTAG];
    SNC_IPADDR componentIPAddr;
    int componentPort;
    SS_COMPONENT *component;
    SNCSocket *sock;
    int bufSize = SNC_MESSAGE_MAX * 3;
    bool retVal;

    int id = getNextConnectionID();
    if (id == -1)
        return false;

    sock = new SNCSocket(this, id, false);
    if (!staticTunnel)
        retVal = m_listSyntroLinkSock->sockAccept(*sock, IPStr, &componentPort);
    else
        retVal = m_listStaticTunnelSock->sockAccept(*sock, IPStr, &componentPort);
    if (!retVal) {
        delete sock;
        return false;
    }
    sock->sockSetConnectMsg(SNCSERVER_ONCONNECT_MESSAGE);
    sock->sockSetCloseMsg(SNCSERVER_ONCLOSE_MESSAGE);
    sock->sockSetReceiveMsg(SNCSERVER_ONRECEIVE_MESSAGE);
    sock->sockSetSendMsg(SNCSERVER_ONSEND_MESSAGE);
    SNCUtils::logDebug(TAG, QString("Accepted call from %1 port %2").arg(IPStr).arg(componentPort));
    SNCUtils::convertIPStringToIPAddr(IPStr, componentIPAddr);

    component = findComponent(componentIPAddr, componentPort);	// see if know about this client already
    if (component != NULL) {                                // do know about this one
        if (component->sock != NULL) {
            delete component->sock;
            delete component->link;
        }
        memcpy(component->compIPAddr, componentIPAddr, SNC_IPADDR_LEN);
        component->compPort = componentPort;
        component->link = new SNCLink(TAG);
    } else {
        component = getFreeComponent();
        if (component == NULL) {                            // too many components!
            delete sock;
            return false;
        }
        memcpy(component->compIPAddr, componentIPAddr, SNC_IPADDR_LEN);
        component->compPort = componentPort;
        component->link = new SNCLink(TAG);
    }
    component->inUse = true;
    setComponentSocket(component, sock);                    // configure component to use this socket
    sock->sockSetReceiveBufSize(bufSize);
    sock->sockSetSendBufSize(bufSize);

    component->lastHeartbeatReceived = SNCUtils::clock();
    component->heartbeatInterval = m_heartbeatSendInterval; // use this until we get it from received heartbeat
    component->state = ConnWFHeartbeat;
    if (staticTunnel) {
        component->tunnelDest = true;
        component->tunnelStatic = true;
    }
    return	true;
}

//	SyClose - received a close indication on a Syntro connection

void	SNCServer::syClose(SS_COMPONENT *SNCComponent)
{
    SNCUtils::logDebug(TAG, QString("Closing ") + SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID));
    syCleanup(SNCComponent);
    updateSNCStatus(SNCComponent);
    emit DMDisplay(&m_dirManager);
}

void	SNCServer::syCleanup(SS_COMPONENT *SNCComponent)
{
    if (SNCComponent != NULL) {
        if (SNCComponent->inUse) {
            m_dirManager.DMDeleteConnectedComponent(SNCComponent->dirManagerConnComp);
            SNCComponent->dirManagerConnComp = NULL;
            if (SNCComponent->dirEntry != NULL) {
                free(SNCComponent->dirEntry);
                SNCComponent->dirEntry = NULL;
            }
            if (SNCComponent->tunnelSource) {
                SNCComponent->tunnel->close();
            } else {
                SNCComponent->inUse = false;                // only if not tunnel source - tunnel source reuses component
            }
            if (SNCComponent->link != NULL) {
                delete SNCComponent->link;
                SNCComponent->link = NULL;
            }
            if (SNCComponent->sock != NULL) {
                delete SNCComponent->sock;
                if ((SNCComponent->connectionID >= 0) && (SNCComponent->connectionID < SNC_MAX_CONNECTIONIDS))
                    m_connectionIDMap[SNCComponent->connectionID] = -1;
                else
                    SNCUtils::logWarn(TAG, QString("Slot %1 has out of range connection ID %2")
                        .arg(SNCComponent->index).arg(SNCComponent->connectionID));
            }
            SNCComponent->sock = NULL;
            SNCComponent->state = ConnIdle;
        }
    }
}


//	GetFreeComponent - get the next free SS_COMPONENT structure

SS_COMPONENT	*SNCServer::getFreeComponent()
{
    int	i;
    SS_COMPONENT	*component;

    component = m_components;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, component++) {
        if (!component->inUse) {
            component->inUse = false;
            component->tunnelDest = false;
            component->tunnelSource = false;
            component->tunnelStatic = false;
            component->tunnelEncrypt = false;
            component->link = NULL;
            component->sock = NULL;
            component->tunnel = NULL;
            component->dirEntry = NULL;
            component->dirEntryLength = 0;
            component->index = i;
            component->dirManagerConnComp = m_dirManager.DMAllocateConnectedComponent(component);

            component->tempRXByteCount = 0;
            component->tempTXByteCount = 0;
            component->tempRXPacketCount = 0;
            component->tempTXPacketCount = 0;

            component->RXPacketRate = 0;
            component->TXPacketRate = 0;
            component->RXByteRate = 0;
            component->TXByteRate = 0;

            component->RXPacketCount = 0;
            component->TXPacketCount = 0;
            component->RXByteCount = 0;
            component->TXByteCount = 0;

            component->lastStatsTime = SNCUtils::clock();
            return component;
        }
    }
    SNCUtils::logError(TAG, "Too many components in component table");
    return NULL;
}

//	SendSNCMessage allows a message to be sent to a specific Component
//

bool SNCServer::sendSNCMessage(SNC_UID *uid, int cmd, SNC_MESSAGE *message, int length, int priority)
{
    SS_COMPONENT *SNCComponent = m_components;

    for (int i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, SNCComponent++) {
        if (SNCComponent->inUse && (SNCComponent->state >= ConnWFHeartbeat)) {
            if (!SNCUtils::compareUID(uid, &(SNCComponent->heartbeat.hello.componentUID)))
                continue;

            // send over link to component
            if (SNCComponent->link != NULL) {
                SNCUtils::logDebug(TAG, QString("Send to ") + SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID));
                SNCComponent->link->send(cmd, length, priority, (SNC_MESSAGE *)message);
                updateTXStats(SNCComponent, length);
                SNCComponent->link->trySending(SNCComponent->sock);
                return true;
            }
        }
    }

    free(message);
    SNCUtils::logWarn(TAG, QString("Failed sending message to %1").arg(qPrintable(SNCUtils::displayUID(uid))));
    return false;
}

//	processReceivedData - handles data received from SNCLinks
//


void	SNCServer::processReceivedData(SS_COMPONENT *SNCComponent)
{
    int cmd;
    int length;
    SNC_MESSAGE *message;
    int priority;

    QMutexLocker locker(&m_lock);

    if (SNCComponent->link == NULL) {
        SNCUtils::logWarn(TAG, "Received data on socket with no SCL");
        return;
    }
    SNCComponent->link->tryReceiving(SNCComponent->sock);

    for (priority = SNCLINK_HIGHPRI; priority <= SNCLINK_LOWPRI; priority++) {
        if (!SNCComponent->link->receive(priority, &cmd, &length, &message))
            continue;
        if (SNCComponent->state < ConnNormal) {
            if (SNCComponent->tunnelSource) {
                SNCUtils::logDebug(TAG, QString("Received %1 from %2").arg(cmd)
                                   .arg(SNCUtils::displayUID(&SNCComponent->tunnel->m_helloEntry.hello.componentUID)));
            } else {
                SNCUtils::logDebug(TAG, QString("Received %1 from %2 port %3").arg(cmd)
                    .arg(SNCUtils::displayIPAddr(SNCComponent->compIPAddr)).arg(SNCComponent->compPort));
            }
        } else {
            SNCUtils::logDebug(TAG, QString("Received %1 from %2").arg(cmd)
                     .arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID)));
        }
        SNCComponent->tempRXPacketCount++;
        SNCComponent->RXPacketCount++;
        SNCComponent->tempRXByteCount += length;
        SNCComponent->RXByteCount += length;

        processReceivedDataDemux(SNCComponent, cmd, length, message);
    }
}

void SNCServer::processReceivedDataDemux(SS_COMPONENT *SNCComponent, int cmd, int length, SNC_MESSAGE *message)
{
    SNC_HEARTBEAT *heartbeat;
    SNC_SERVICE_LOOKUP *serviceLookup;

    switch (cmd) {
        case SNCMSG_HEARTBEAT:                              // SNC client heartbeat
            if (length < (int)sizeof(SNC_HEARTBEAT)) {
                SNCUtils::logWarn(TAG, QString("Heartbeat was too short %1").arg(length));
                free(message);
                break;
            }
            heartbeat = (SNC_HEARTBEAT *)message;
            heartbeat->hello.appName[SNC_MAX_APPNAME - 1] = 0;  // make sure strings are 0 terminated
            heartbeat->hello.componentType[SNC_MAX_COMPTYPE - 1] = 0;   // make sure strings are 0 terminated

            SNCComponent->lastHeartbeatReceived = SNCUtils::clock();
            if (!SNCComponent->tunnelSource)            // tunnel sources use their configured value
                SNCComponent->heartbeatInterval = SNCUtils::convertUC2ToInt(heartbeat->hello.interval) * SNC_CLOCKS_PER_SEC;    // record advertised heartbeat interval
            memcpy(&(SNCComponent->heartbeat), message, sizeof(SNC_HEARTBEAT));
            if (SNCComponent->state == ConnWFHeartbeat) {   // first time for heartbeat

                //  Only SNCControls can connect via tunnels

                if (SNCComponent->tunnelDest && (strcmp(SNCComponent->heartbeat.hello.componentType, COMPTYPE_CONTROL) != 0)) {
                    SNCUtils::logError(TAG, "Received non-control heartbeat on tunnel connection");
                    free(message);
                    break;
                }

                // check is this is dynamic tunnel dest

                if (!SNCComponent->tunnelSource && !SNCComponent->tunnelStatic &&
                    (strcmp(SNCComponent->heartbeat.hello.componentType, COMPTYPE_CONTROL) == 0))
                    SNCComponent->tunnelDest = true;

                //  Validate tunnel source against local UID list

                int validIndex;
                if (SNCComponent->tunnelStatic && SNCComponent->tunnelDest) {
                    for (validIndex = 0; validIndex < m_validTunnelSources.count(); validIndex++) {
                        if (SNCUtils::compareUID(&m_validTunnelSources[validIndex], &SNCComponent->heartbeat.hello.componentUID))
                            break;
                    }
                    if (validIndex == m_validTunnelSources.count()) {
                        SNCUtils::logError(TAG, QString("Tunnel: failed to validate client %1")
                            .arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID)));
                        free(message);
                        break;
                    }
                }

                SNCComponent->state = ConnNormal;
                SNCUtils::logInfo(TAG, QString("New component %1").arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID)));
            }
            updateSNCStatus(SNCComponent);
            length -= sizeof(SNC_HEARTBEAT);
            if (length > 0)                                 // there must be a DE attached
                setComponentDE((char *)message + sizeof(SNC_HEARTBEAT), length, SNCComponent);
            if (!SNCComponent->tunnelSource && !SNCComponent->tunnelDest)
                sendHeartbeat(SNCComponent);                // need to respond if a normal component
            if (SNCComponent->tunnelDest)
                sendTunnelHeartbeat(SNCComponent);          // send a tunnel heartbeat if it is a tunnel dest
            free(message);
            break;

        case SNCMSG_E2E:
            forwardE2EMessage(message, length);
            break;

        case SNCMSG_MULTICAST_MESSAGE:                  // a multicast message
            forwardMulticastMessage(SNCComponent, cmd, message, length);    // forward on to the interested remotes
            free(message);
            break;

        case SNCMSG_MULTICAST_ACK:                      // message is multicast header
            m_multicastManager.MMProcessMulticastAck((SNC_EHEAD *)message, length);
            free(message);
            break;

        case SNCMSG_SERVICE_LOOKUP_REQUEST:             // a Component has requested a service lookup
            if (length != sizeof(SNC_SERVICE_LOOKUP)) {
                SNCUtils::logWarn(TAG, QString("Wrong size service lookup request %1").arg(length));
                free(message);
                break;
            }
            serviceLookup = (SNC_SERVICE_LOOKUP *)message;
            SNCUtils::logDebug(TAG, QString("Got service lookup for %1, type %2")
                               .arg(serviceLookup->servicePath).arg(serviceLookup->serviceType));
            m_dirManager.DMFindService(&(SNCComponent->heartbeat.hello.componentUID), serviceLookup);
            sendSNCMessage(&(SNCComponent->heartbeat.hello.componentUID),
                        SNCMSG_SERVICE_LOOKUP_RESPONSE, message, length, SNCLINK_MEDHIGHPRI);
            break;

        case SNCMSG_SERVICE_LOOKUP_RESPONSE:
            m_multicastManager.MMProcessLookupResponse((SNC_SERVICE_LOOKUP *)message, length);
            free(message);
            break;

        case SNCMSG_DIRECTORY_REQUEST:
            free(message);                                  // nothing useful in the request itself
            m_dirManager.DMBuildDirectoryMessage(sizeof(SNC_DIRECTORY_RESPONSE), (char **)&message, &length, false);
            sendSNCMessage(&(SNCComponent->heartbeat.hello.componentUID),
                        SNCMSG_DIRECTORY_RESPONSE, message, length, SNCLINK_LOWPRI);
            break;

        default:
            if (message != NULL) {
                SNCUtils::logWarn(TAG, QString("Unrecognized message %1 from %2")
                    .arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID))
                    .arg(cmd));
                free(message);
            }
            break;
    }
}

void SNCServer::setComponentDE(char *dirEntry, int length, SS_COMPONENT *SNCComponent)
{
    dirEntry[length-1] = 0;                                 // make sure 0 terminated!
    if (SNCComponent->dirEntry != NULL) {                   // already have a DE - see if it has changed
        if (length == SNCComponent->dirEntryLength) {       // are same length
            if (memcmp(dirEntry, SNCComponent->dirEntry, length) == 0)
                return;                                     // hasn't changed - do nothing
        }
        free(SNCComponent->dirEntry);                       // has changed
    }
    SNCComponent->dirEntry = (char *)malloc(length);
    SNCComponent->dirEntryLength = length;
    memcpy(SNCComponent->dirEntry, dirEntry, length);       // remember new DE
    SNCComponent->dirManagerConnComp->connectedComponentUID = SNCComponent->heartbeat.hello.componentUID;
    m_dirManager.DMProcessDE(SNCComponent->dirManagerConnComp, dirEntry, length);
    SNCUtils::logDebug(TAG, QString("Updated component ") +  SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID));
    emit DMDisplay(&m_dirManager);
}

void	SNCServer::sendHeartbeat(SS_COMPONENT *SNCComponent)
{
    unsigned char *pMsg;

    if (!SNCComponent->inUse)
        return;
    if (SNCComponent->link == NULL)
        return;
    pMsg = (unsigned char *)malloc(sizeof(SNC_HEARTBEAT));
    SNC_HEARTBEAT hb = m_componentData.getMyHeartbeat();
    memcpy(pMsg, &hb, sizeof(SNC_HEARTBEAT));
    SNCComponent->link->send(SNCMSG_HEARTBEAT, sizeof(SNC_HEARTBEAT), SNCLINK_MEDHIGHPRI, (SNC_MESSAGE *)pMsg);
    updateTXStats(SNCComponent, sizeof(SNC_HEARTBEAT));
    SNCComponent->link->trySending(SNCComponent->sock);
    SNCUtils::logDebug(TAG, QString("Sent response HB to %1 from slot %2")
                .arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID)).arg(SNCComponent->index));
}


void SNCServer::sendTunnelHeartbeat(SS_COMPONENT *SNCComponent)
{
    int messageLength;
    unsigned char *message;
    SNC_HEARTBEAT heartbeat;

    heartbeat = m_componentData.getMyHeartbeat();

    if (!SNCComponent->inUse)
        return;

    if (SNCComponent->link == NULL)
        return;

    if (SNCComponent->tunnelSource) {
        if (SNCComponent->tunnel->m_connected) {
            m_dirManager.DMBuildDirectoryMessage(sizeof(SNC_HEARTBEAT), (char **)&message, &messageLength, true); // generate a customized DE
            memcpy(message, &heartbeat, sizeof(SNC_HEARTBEAT));
            if (SNCComponent->link != NULL) {
                SNCComponent->link->send(SNCMSG_HEARTBEAT, messageLength,
                                SNCLINK_MEDHIGHPRI, (SNC_MESSAGE *)message);
                updateTXStats(SNCComponent, messageLength);
                SNCComponent->link->trySending(SNCComponent->sock);
            }
        }
    } else {
        if ((SNCComponent->state > ConnWFHeartbeat) && SNCComponent->tunnelDest) {
            m_dirManager.DMBuildDirectoryMessage(sizeof(SNC_HEARTBEAT), (char **)&message, &messageLength, true); // generate a customized DE
            memcpy(message, &heartbeat, sizeof(SNC_HEARTBEAT));
            if (SNCComponent->link != NULL) {
                SNCComponent->link->send(SNCMSG_HEARTBEAT, messageLength,
                                SNCLINK_MEDHIGHPRI, (SNC_MESSAGE *)message);
                updateTXStats(SNCComponent, messageLength);
                SNCComponent->link->trySending(SNCComponent->sock);
            }
        }
    }
}


// SNCServer message handlers

void SNCServer::timerEvent(QTimerEvent * /* event */)
{
    SNCBackground();
}

bool SNCServer::processMessage(SNCThreadMsg* msg)
{
    SS_COMPONENT *SNCComponent;

    switch(msg->message) {
        case HELLO_STATUS_CHANGE_MESSAGE:
            switch (msg->intParam) {
                case SNCHELLO_BEACON:
                    processHelloBeacon((SNCHELLO *)msg->ptrParam);
                    break;

                case SNCHELLO_UP:
                    processHelloUp((SNCHELLOENTRY *)msg->ptrParam);
                    break;

                case SNCHELLO_DOWN:
                    processHelloDown((SNCHELLOENTRY *)msg->ptrParam);
                    break;

                default:
                    SNCUtils::logDebug(TAG, QString("Illegal Hello state %1").arg(msg->intParam));
                    break;
            }
            free((void *)msg->ptrParam);
            break;

        case SNCSERVER_ONCONNECT_MESSAGE:
            SNCComponent = getComponentFromConnectionID(msg->intParam);
            if (SNCComponent == NULL) {
                SNCUtils::logWarn(TAG, QString("OnConnect on unmatched socket %1").arg(msg->intParam));
                return true;
            }
            if (!SNCComponent->inUse) {
                SNCUtils::logWarn(TAG, QString("OnConnect on not in use component %1").arg(SNCComponent->index));
                return true;
            }
            syConnected(SNCComponent);
            break;

        case SNCSERVER_ONACCEPT_MESSAGE:
            syAccept(false);
            return true;

        case SNCSERVER_ONACCEPT_STATICTUNNEL_MESSAGE:
            syAccept(true);
            return true;

        case SNCSERVER_ONCLOSE_MESSAGE:
            SNCComponent = getComponentFromConnectionID(msg->intParam);
            if (SNCComponent == NULL) {
                SNCUtils::logWarn(TAG, QString("OnClose on unmatched socket %1").arg(msg->intParam));
                return true;
            }
            if (!SNCComponent->inUse) {
                SNCUtils::logWarn(TAG, QString("OnClose on not in use component %1").arg(SNCComponent->index));
                return true;
            }
            syClose(SNCComponent);
            return true;

        case SNCSERVER_ONRECEIVE_MESSAGE:
            SNCComponent = getComponentFromConnectionID(msg->intParam);
            if (SNCComponent == NULL) {
                SNCUtils::logWarn(TAG, QString("OnReceive on unmatched socket %1").arg(msg->intParam));
                return true;
            }
            if (!SNCComponent->inUse) {
                SNCUtils::logWarn(TAG, QString("OnReceive on not in use component %1").arg(SNCComponent->index));
                return true;
            }
            processReceivedData(SNCComponent);
            return true;

        case SNCSERVER_ONSEND_MESSAGE:
            SNCComponent = getComponentFromConnectionID(msg->intParam);
            if (SNCComponent == NULL) {
                SNCUtils::logWarn(TAG, QString("OnSend on unmatched socket %1").arg(msg->intParam));
                return true;
            }
            if (!SNCComponent->inUse) {
                SNCUtils::logWarn(TAG, QString("OnSend on not in use component %1").arg(SNCComponent->index));
                return true;
            }
            if (SNCComponent->link != NULL)
                SNCComponent->link->trySending(SNCComponent->sock);
            return true;
    }
    return true;
}

void SNCServer::processHelloBeacon(SNCHELLO *hello)
{
    SNCUtils::logDebug(TAG, QString("Sending beacon response to ") + SNCUtils::displayUID(&hello->componentUID));
    m_hello->sendSNCHelloBeaconResponse(hello);
}

void	SNCServer::processHelloUp(SNCHELLOENTRY *helloEntry)
{
    SS_COMPONENT *component;
    SNCHELLO *myHello;
    SNC_HEARTBEAT heartbeat;

    heartbeat = m_componentData.getMyHeartbeat();

    myHello = &(heartbeat.hello);
    if (strcmp(helloEntry->hello.componentType, COMPTYPE_CONTROL) != 0) {	// not a SNCControl so must be a beacon
        return;
    }

    if (SNCUtils::compareUID(&(helloEntry->hello.componentUID), &(myHello->componentUID)))
        return;												// this is me - ignore!

    SNCUtils::logDebug(TAG, QString("Got Hello UP from %1 %2").arg(helloEntry->hello.appName)
                    .arg(SNCUtils::displayIPAddr(helloEntry->hello.IPAddr)));

    if (m_fastUIDLookup.FULLookup(&(helloEntry->hello.componentUID))) {
        // component already in table - should mean that this is one in a different mode
        SNCUtils::logDebug(TAG, QString("Received hello up from " + SNCUtils::displayUID(&helloEntry->hello.componentUID)));
        return;
    }

    if (SNCUtils::UIDHigher(&(helloEntry->hello.componentUID), &(myHello->componentUID)))
        return;												// the new UID is lower than mine - let that device establish the call

    if ((component = getFreeComponent()) == NULL) {
        SNCUtils::logError(TAG, QString("No component table space for %1").arg(SNCUtils::displayUID(&helloEntry->hello.componentUID)));
        return;
    }

    component->tunnelEncrypt = m_encryptLocal;
    component->tunnel = new SNCTunnel(this, component, m_hello, helloEntry);

    component->inUse = true;
    component->tunnelSource = true;

    component->heartbeat.hello = helloEntry->hello;
    component->state = ConnWFHeartbeat;
}

void	SNCServer::processHelloDown(SNCHELLOENTRY *helloEntry)
{
    SS_COMPONENT *component;
    SNCHELLOENTRY foundHelloEntry;
    SNC_HEARTBEAT heartbeat;

    heartbeat = m_componentData.getMyHeartbeat();

    if (strcmp(helloEntry->hello.componentType, COMPTYPE_CONTROL) != 0)
        return;												// not an SNCControl

    if (SNCUtils::compareUID(&(helloEntry->hello.componentUID), &(heartbeat.hello.componentUID)))
        return;												// this is me - ignore!

    SNCUtils::logDebug(TAG, QString("Got Hello DOWN from %1 %2").arg(helloEntry->hello.appName)
                            .arg(SNCUtils::displayIPAddr(helloEntry->hello.IPAddr)));

    if (m_hello->findComponent(&foundHelloEntry, &(helloEntry->hello.componentUID))) {
        // component still in hello table so leave channel (must be a primary/backup pair)
        return;
    }

    if ((component = (SS_COMPONENT *)m_fastUIDLookup.FULLookup(&(helloEntry->hello.componentUID))) == NULL) {
        SNCUtils::logDebug(TAG, QString("Failed to find channel for ") + SNCUtils::displayUID(&helloEntry->hello.componentUID));
        return;
    }

    SNCUtils::logDebug(TAG, QString("Deleting entry for SNCControl %1 %2").arg(helloEntry->hello.appName)
                                    .arg(SNCUtils::displayIPAddr(helloEntry->hello.IPAddr)));

    m_fastUIDLookup.FULDelete(&(helloEntry->hello.componentUID));

    syCleanup(component);

    if (component->tunnel != NULL) {
        delete component->tunnel;
        component->tunnel = NULL;
    }

    component->inUse = false;
    updateSNCStatus(component);
}

void	SNCServer::SNCBackground()
{
    int i;
    SS_COMPONENT *SNCComponent;
    qint64 now;

    if (!openSockets())
        return;												// don't do anything until sockets open

    now = SNCUtils::clock();

    if ((now - m_counterStart) >= SNC_CLOCKS_PER_SEC) {	// time to update rates
        emit serverMulticastUpdate(m_multicastIn, m_multicastInRate, m_multicastOut, m_multicastOutRate);
        emit serverE2EUpdate(m_E2EIn, m_E2EInRate, m_E2EOut, m_E2EOutRate);
        m_counterStart += SNC_CLOCKS_PER_SEC;
        m_multicastInRate = m_multicastOutRate = m_E2EInRate = m_E2EOutRate = 0;
    }
    SNCComponent = m_components;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, SNCComponent++)
    {
        if (SNCComponent->inUse)
        {
            updateSNCData(SNCComponent);
            if (SNCComponent->tunnelSource) {
                SNCComponent->tunnel->tunnelBackground();
                if (SNCComponent->tunnel->m_connected) {
                    if (SNCUtils::timerExpired(now, SNCComponent->lastHeartbeatSent, SNCComponent->heartbeatInterval)) {
                        sendTunnelHeartbeat(SNCComponent);
                        SNCComponent->lastHeartbeatSent = now;
                    }
                    processReceivedData(SNCComponent);
                    SNCComponent->link->trySending(SNCComponent->sock);
                }

                if (SNCComponent->tunnel->m_connectInProgress || SNCComponent->tunnel->m_connected) {
                    if (SNCUtils::timerExpired(now, SNCComponent->lastHeartbeatReceived,	m_heartbeatTimeoutCount * SNCComponent->heartbeatInterval)) {
                        if (!SNCComponent->tunnelStatic) {
                            SNCUtils::logWarn(TAG, QString("Timeout on tunnel source to %1").arg(SNCComponent->tunnel->m_helloEntry.hello.appName));
                        } else {
                            SNCUtils::logWarn(TAG, QString("Timeout on tunnel source ") + SNCComponent->tunnelStaticName);
                        }
                        syCleanup(SNCComponent);
                        updateSNCStatus(SNCComponent);
                    }
                }
            } else {										// if not tunnel source
                if (SNCComponent->link != NULL) {
                    processReceivedData(SNCComponent);
                    SNCComponent->link->trySending(SNCComponent->sock);
                }

                if (SNCUtils::timerExpired(SNCUtils::clock(), SNCComponent->lastHeartbeatReceived, m_heartbeatTimeoutCount * SNCComponent->heartbeatInterval)) {
                    SNCUtils::logWarn(TAG, QString("Timeout on %1").arg(SNCComponent->index));
                    syCleanup(SNCComponent);
                    updateSNCStatus(SNCComponent);
                }
            }
        }
    }
    m_multicastManager.MMBackground();
}

void	SNCServer::forwardE2EMessage(SNC_MESSAGE *SNCMessage, int len)
{
    SNC_EHEAD *ehead;
    SS_COMPONENT *component;

    ehead = (SNC_EHEAD *)SNCMessage;
    if (len < (int)sizeof(SNC_EHEAD)) {
        SNCUtils::logWarn(TAG, QString("Got too small E2E %1").arg(len));
        free(SNCMessage);
        return;
    }
    m_E2EIn++;
    m_E2EInRate++;

    if ((component = (SS_COMPONENT *)m_fastUIDLookup.FULLookup(&(ehead->destUID))) != NULL) {

        if (component->inUse && (component->state >= ConnWFHeartbeat)) {
            if (component->link != NULL) {
                SNCUtils::logDebug(TAG, QString("Send to ") + SNCUtils::displayUID(&component->heartbeat.hello.componentUID));
                component->link->send(SNCMSG_E2E, len, SNCMessage->flags & SNCLINK_PRI, SNCMessage);
                updateTXStats(component, len);
                component->link->trySending(component->sock);
                m_E2EOut++;
                m_E2EOutRate++;
            } else {
                free(SNCMessage);
            }
        }
        return;
    }

//	Not found!

    SNCUtils::logError(TAG, QString("Failed to E2E dest for %1").arg(SNCUtils::displayUID(&ehead->destUID)));
    free(SNCMessage);
}


void	SNCServer::forwardMulticastMessage(SS_COMPONENT *SNCComponent, int cmd, SNC_MESSAGE *message, int length)
{
    if (!SNCComponent->inUse) {
        SNCUtils::logWarn(TAG, QString("ForwardMessage on not in use component %1").arg(SNCUtils::displayUID(&SNCComponent->heartbeat.hello.componentUID)));
        return;												// not in use - hmmm. Should not happen!
    }
    if (length < (int)sizeof(SNC_EHEAD)) {
        SNCUtils::logWarn(TAG, QString("ForwardMessage is too short %1").arg(length));
        return;												// not in use - hmmm. Should not happen!
    }
    m_multicastManager.MMForwardMulticastMessage(cmd, message, length);
}


//	FindComponent - find the component corresponding to the address in the parameter

SS_COMPONENT	*SNCServer::findComponent(SNC_IPADDR compAddr, int compPort)
{
    int	i;

    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++) {
        if (m_components[i].inUse) {
            if ((memcmp(m_components[i].compIPAddr, compAddr, SNC_IPADDR_LEN) == 0) &&
                (m_components[i].compPort == compPort))
                return m_components+i;
        }
    }
    return NULL;
}

void SNCServer::updateSNCStatus(SS_COMPONENT *SNCComponent)
{
    if (receivers(SIGNAL(updateSNCStatusBox(int, QStringList))) == 0)
        return;

    QStringList list;
    QString heartbeatInterval;
    QString linkType;
    QString appName;
    QString componentType;
    QString uid;
    QString ipaddr;
    SNCHELLO *hello = &(SNCComponent->heartbeat.hello);

    if (SNCComponent->inUse && (SNCComponent->state == ConnNormal) && (hello != NULL)) {

        appName = hello->appName;
        componentType = hello->componentType;
        uid = SNCUtils::displayUID(&hello->componentUID);
        ipaddr = SNCUtils::displayIPAddr(hello->IPAddr);
        if (SNCComponent->tunnelSource)					// if I am a tunnel source...
            heartbeatInterval = "Tunnel dest";				// ...indicate other end is a tunnel dest
        else
            heartbeatInterval = QString ("%1") .arg(SNCComponent->heartbeatInterval / SNC_CLOCKS_PER_SEC);

        linkType = "Unknown";

        if (SNCComponent->sock != NULL) {
            if (SNCComponent->tunnelStatic) {
                if (SNCComponent->tunnelSource)
                    linkType = SNCComponent->sock->usingSSL() ? "SSL static tunnel src" : "Static tunnel src";
                else
                    linkType = SNCComponent->sock->usingSSL() ? "SSL static tunnel dst" : "Static tunnel dst";
            } else {
                if (SNCComponent->tunnelSource)
                    linkType = SNCComponent->sock->usingSSL() ? "SSL local tunnel src" : "Local tunnel src";
                else if (SNCComponent->tunnelDest)
                    linkType = SNCComponent->sock->usingSSL() ? "SSL local tunnel dst" : "Local tunnel dst";
                else
                    linkType = SNCComponent->sock->usingSSL() ? "SSL local link" : "Local link";
            }
        }
    } else {
        if (SNCComponent->tunnelStatic)
            appName = SNCComponent->tunnelStaticName;
        else
            appName = "...";
    }
    list.append(appName);
    list.append(componentType);
    list.append(uid);
    list.append(ipaddr);
    list.append(heartbeatInterval);
    list.append(linkType);
    emit updateSNCStatusBox(SNCComponent->index, list);
}


void SNCServer::updateSNCData(SS_COMPONENT *SNCComponent)
{
    if (receivers(SIGNAL(updateSNCDataBox(int, QStringList))) == 0)
        return;

    qint64 now = SNCUtils::clock();

    QStringList list;

    QString RXByteCount;
    QString TXByteCount;
    QString RXByteRate;
    QString TXByteRate;

    qint64 deltaTime = now - SNCComponent->lastStatsTime;

    if (deltaTime <= 0)
        return;

    if (deltaTime < SNCSERVER_STATS_INTERVAL)
        return;												// max rate enforced

    SNCComponent->lastStatsTime = now;

    // time to update rates

    SNCComponent->RXByteRate = (SNCComponent->tempRXByteCount * SNC_CLOCKS_PER_SEC) / deltaTime;
    SNCComponent->TXByteRate = (SNCComponent->tempTXByteCount * SNC_CLOCKS_PER_SEC) / deltaTime;
    SNCComponent->RXPacketRate = (SNCComponent->tempRXPacketCount * SNC_CLOCKS_PER_SEC) / deltaTime;
    SNCComponent->TXPacketRate = (SNCComponent->tempTXPacketCount * SNC_CLOCKS_PER_SEC) / deltaTime;

    SNCComponent->tempRXByteCount = 0;
    SNCComponent->tempTXByteCount = 0;
    SNCComponent->tempRXPacketCount = 0;
    SNCComponent->tempTXPacketCount = 0;

    RXByteCount = QString::number(SNCComponent->RXByteCount);
    TXByteCount = QString::number(SNCComponent->TXByteCount);
    RXByteRate = QString::number(SNCComponent->RXByteRate);
    TXByteRate = QString::number(SNCComponent->TXByteRate);

    list.append(RXByteCount);
    list.append(TXByteCount);
    list.append(RXByteRate);
    list.append(TXByteRate);
    emit updateSNCDataBox(SNCComponent->index, list);
}

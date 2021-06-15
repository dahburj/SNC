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

#ifndef SNCSERVER_H
#define SNCSERVER_H

#include "DirectoryManager.h"
#include "MulticastManager.h"
#include "FastUIDLookup.h"
#include "SNCComponentData.h"
#include "SNCLink.h"

#include <qstringlist.h>

//	SNCServer settings

#define SNCSERVER_PARAMS_GROUP                                  "SNCControl"            // group name for SNCControl-specific settings
#define	SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET                    "LocalSocket"           // port number for local links
#define	SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET             "StaticTunnelSocket"    // port number for static tunnels

#define	SNCSERVER_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT            "LocalSocketEncrypt"    // port number for encrypted local links
#define	SNCSERVER_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT     "StaticTunnelSocketEncrypt" // port number for encrypted static tunnels

#define SNCSERVER_PARAMS_ENCRYPT_LOCAL                          "EncryptLocal"          // true if local connections using SSL
#define SNCSERVER_PARAMS_ENCRYPT_STATICTUNNEL_SERVER            "EncryptStaticTunnelServer" // true if static tunnel server connections using SSL

#define SNCSERVER_PARAMS_HBINTERVAL                             "controlHeartbeatInterval"  // interval between heartbeats in seconds
#define SNCSERVER_PARAMS_HBTIMEOUT                              "controlHeartbeatTimeout"   // heartbeat intervals before timeout

#define SNCSERVER_PARAMS_PRIORITY                               "controlPriority"       // priority of this SNCControl

#define SNCSERVER_PARAMS_VALID_TUNNEL_SOURCES   "ValidTunnelSources"    // UIDs of valid tunnel sources
#define SNCSERVER_PARAMS_VALID_TUNNEL_UID       "ValidTunnelUID"        // the array entry

//  Static tunnel settings defs

#define SNCSERVER_PARAMS_STATIC_TUNNELS         "StaticTunnels" // the group name
#define SNCSERVER_PARAMS_STATIC_NAME            "StaticName"    // name of the tunnel
#define SNCSERVER_PARAMS_STATIC_DESTIP_PRIMARY  "StaticPrimary" // the primary destination IP address
#define SNCSERVER_PARAMS_STATIC_DESTIP_BACKUP   "StaticBackup"  // the backup IP port
#define SNCSERVER_PARAMS_STATIC_ENCRYPT         "StaticEncrypt" // true if use SSL

//  SNCServer Thread Messages

#define SNCSERVER_ONCONNECT_MESSAGE             (SNC_MSTART+0)
#define SNCSERVER_ONACCEPT_MESSAGE              (SNC_MSTART+1)
#define SNCSERVER_ONCLOSE_MESSAGE               (SNC_MSTART+2)
#define SNCSERVER_ONRECEIVE_MESSAGE             (SNC_MSTART+3)
#define SNCSERVER_ONSEND_MESSAGE                (SNC_MSTART+4)
#define SNCSERVER_ONACCEPT_STATICTUNNEL_MESSAGE (SNC_MSTART+5)

#define SNCSERVER_SOCKET_RETRY                  (2 * SNC_CLOCKS_PER_SEC)
#define SNCSERVER_STATS_INTERVAL                (2 * SNC_CLOCKS_PER_SEC)

class SNCTunnel;


enum ConnState
{
    ConnIdle,                                               // if not doing anything
    ConnWFHeartbeat,                                        // if waiting for first heartbeat
    ConnNormal,                                             // the normal state
};

//  SS_COMPONENT is the data structure used to hold information about a connection to a component

typedef struct
{
    bool inUse;                                             // true if being used
    bool tunnelSource;                                      // if the source end of a SNCControl tunnel
    bool tunnelDest;                                        // if it is the dest end of a tunnel
    bool tunnelStatic;                                      // if it is a static tunnel
    bool tunnelEncrypt;                                     // is use SSL for tunnel
    QString tunnelStaticName;                               // name of the static tunnel
    QString tunnelStaticPrimary;                            // static tunnel primary IP address
    QString tunnelStaticBackup;                             // static tunnel backup IP address

    SNCThread *widgetThread;                                // the thread pointer if a widget link
    int widgetMessageID;
    int index;                                              // position of entry in array
    SNCLink *link;                                          // the link manager class
    SNC_IPADDR compIPAddr;                                  // the component's IP address
    int compPort;                                           // the component's port
    SNCSocket *sock;                                        // socket for SNCLink connection
    int connectionID;                                       // its connection ID

    // Heartbeat system vars

    SNC_HEARTBEAT heartbeat;                                // a copy of the heartbeat message
    qint64 lastHeartbeatReceived;                           // time last heartbeat was received
    qint64 lastHeartbeatSent;                               // time last heartbeat was send (used by tunnel source)
    qint64 heartbeatInterval;                               // expected receive interval (from heartbeat itself) or send interval if tunnel source
    qint64 heartbeatTimeoutPeriod;                          // if no heartbeats received for this time, time out the SNCLink

    int state;                                              // the connection's state
    SNCTunnel *tunnel;                                      // the tunnel class if it is a tunnel source
    char *dirEntry;                                         // this is the currently in use DE
    int dirEntryLength;                                     // and its length (can't use strlen as may have multiple components)
    DM_CONNECTEDCOMPONENT *dirManagerConnComp;              // this is the directory manager entry for this connection

    quint64 tempRXByteCount;                                // for receive byte rate calculation
    quint64 tempTXByteCount;                                // for transmit byte rate calculation
    quint64 RXByteCount;                                    // receive byte count
    quint64 TXByteCount;                                    // transmit byte count
    quint64 RXByteRate;                                     // receive byte rate
    quint64 TXByteRate;                                     // transmit byte rate

    quint32 tempRXPacketCount;                              // for receive byte rate calculation
    quint32 tempTXPacketCount;                              // for transmit byte rate calculation
    quint32 RXPacketCount;                                  // receive byte count
    quint32 TXPacketCount;                                  // transmit byte count
    quint32 RXPacketRate;                                   // receive byte rate
    quint32 TXPacketRate;                                   // transmit byte rate

    qint64 lastStatsTime;                                   // last time stats were updated

} SS_COMPONENT;

// SNCServer

class SNCServer : public SNCThread
{
    Q_OBJECT

    friend class SNCTunnel;

public:
    SNCServer();
    virtual ~SNCServer();

    SNCHello *getHelloObject();                             // returns the hello object

    int m_socketNumber;                                     // the socket to listen on - defaults to SNC_ACTIVE_SOCKET_LOCAL
    int m_staticTunnelSocketNumber;                         // the socket for static tunnels - defaults to SNC_ACTIVE_SOCKET_TUNNEL
    int m_socketNumberEncrypt;                              // the SSL socket to listen on - defaults to SNC_ACTIVE_SOCKET_LOCAL_ENCRYPT
    int m_staticTunnelSocketNumberEncrypt;                  // the SSL socket for static tunnels - defaults to SNC_ACTIVE_SOCKET_TUNNEL_ENCRYPT

    bool m_netLogEnabled;                                   // true if network logging enabled
    QString m_appName;                                      // this SNCControl's app name

    SNC_UID m_myUID;                                        // my UID - used for local service processing

    SS_COMPONENT m_components[SNC_MAX_CONNECTEDCOMPONENTS]; // the Component array

    DirectoryManager m_dirManager;                          // the directory manager object
    MulticastManager m_multicastManager;                    // the multicast manager object
    FastUIDLookup m_fastUIDLookup;                          // the fast UID lookup object

    bool sendSNCMessage(SNC_UID *uid, int cmd, SNC_MESSAGE *message, int length, int priority);
    void setComponentSocket(SS_COMPONENT *SNCComponent, SNCSocket *sock); // allocate a socket to this component

    qint64 m_multicastIn;                                   // total multicast in count
    unsigned m_multicastInRate;                             // rate accumulator
    qint64 m_multicastOut;                                  // total multicast out count
    unsigned m_multicastOutRate;                            // rate accumulator

    SNCComponentData m_componentData;

//------------------------------------------------------------------

signals:
    void DMDisplay(DirectoryManager *dirManager);
    void updateSNCStatusBox(int, QStringList);
    void updateSNCDataBox(int, QStringList);
    void serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);
    void serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);

protected:

    void initThread();
    void finishThread();
    void timerEvent(QTimerEvent *event);
    void loadStaticTunnels(QSettings *settings);
    void loadValidTunnelSources(QSettings *settings);
    bool processMessage(SNCThreadMsg* msg);
    bool openSockets();                                     // open the sockets SNCControl needs
    int getNextConnectionID();                              // gets the next free connection ID

    bool syConnected(SS_COMPONENT *SNCComponent);
    bool syAccept(bool staticTunnel);
    void syClose(SS_COMPONENT *SNCComponent);
    SNCSocket *getNewSocket(bool staticTunnel);
    SS_COMPONENT *getFreeComponent();
    void processHelloBeacon(SNCHELLO *hello);
    void processHelloUp(SNCHELLOENTRY *helloEntry);
    void processHelloDown(SNCHELLOENTRY *helloEntry);
    void SNCBackground();
    void sendHeartbeat(SS_COMPONENT *SNCComponent);
    void sendTunnelHeartbeat(SS_COMPONENT *SNCComponent);
    void setupComponentStatus();


    void setComponentDE(char *pDE, int nLen, SS_COMPONENT *pComp);
    void syCleanup(SS_COMPONENT *pSC);
    void updateSNCStatus(SS_COMPONENT *SNCComponent);
    void updateSNCData(SS_COMPONENT *SNCComponent);


//  forwardE2EMessage - forward an endpoint to endpoint message

    void forwardE2EMessage(SNC_MESSAGE *message, int length);

//  forwardMulticastMessage - forwards a multicastmessage to the registered remote Components

    void forwardMulticastMessage(SS_COMPONENT *SNCComponent, int cmd, SNC_MESSAGE *message, int length);

//  findComponent - maps an identifier to a component

    SS_COMPONENT *findComponent(SNC_IPADDR compAddr, int compPort);

    SNCSocket *m_listSyntroLinkSock;                        // local listener socket

    SNCSocket *m_listStaticTunnelSock;                      // static tunnel listener socket

    QMutex m_lock;
    SNCHello *m_hello;

    SS_COMPONENT *getComponentFromConnectionID(int connectionID); // uses m_connectioIDMap to get a component pointer
    void processReceivedData(SS_COMPONENT *SNCComponent);
    void processReceivedDataDemux(SS_COMPONENT *SNCComponent, int cmd, int length, SNC_MESSAGE *message);
    qint64 m_heartbeatSendInterval;                         // the initial interval for apps and the send interval for tunnel sources
    int m_heartbeatTimeoutCount;                            // number of heartbeat periods before SNCLink timed out

    int m_connectionIDMap[SNC_MAX_CONNECTIONIDS];           // maps connection IDs to component index
    int m_nextConnectionID;                                 // used to allocate unique IDs to socket connections

    qint64 m_E2EIn;                                         // total multicast in count
    unsigned m_E2EInRate;                                   // rate accumulator
    qint64 m_E2EOut;                                        // total multicast out count
    unsigned m_E2EOutRate;                                  // rate accumulator
    qint64 m_counterStart;                                  // rate counter start time

    bool m_encryptLocal;                                    // if use SSL for local connections
    bool m_encryptStaticTunnelServer;                       // if use SSL for tunnel service

    qint64 m_lastOpenSocketsTime;                           // last time open sockets failed

    QList<SNC_UID> m_validTunnelSources;                    // list of valid UIDs that can be tunnel sources

private:

    inline void updateTXStats(SS_COMPONENT *SNCComponent, int length) {
                SNCComponent->tempTXPacketCount++;
                SNCComponent->TXPacketCount++;
                SNCComponent->tempTXByteCount += length;
                SNCComponent->TXByteCount += length;
    }

    int m_timer;

};

#endif // SNCSERVER_H

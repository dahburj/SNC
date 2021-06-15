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

#ifndef _SNCHELLO_H_
#define _SNCHELLO_H_

#include "SNCUtils.h"
#include "SNCThread.h"

#include <qobject.h>
#include <qmutex.h>

#define SNCHELLO_CONTROL_SNCHELLO_INTERVAL	2000				// send hello every two seconds if control
#define	SNCHELLO_INTERVAL		(2 * SNC_CLOCKS_PER_SEC)		// send a hello every 2 seconds
#define	SNCHELLO_LIFETIME		(4 * SNCHELLO_INTERVAL)			// SNCHELLO entry timeout = 4 retries

//--------------------------------------------------------------------------------
//	The SNCHello data structure

#define	INSTANCE_CONTROL			0						// the instance for SNCControl

#define	INSTANCE_DYNAMIC			-1						// for a non core app, this means that instance is dynamic
#define	INSTANCE_COMPONENT			2						// instance of first component

#define	SOCKET_SNCHELLO				8040					// socket to use is this plus the instance

#define	SNCHELLO_SYNC0		0xff								// the sync bytes
#define	SNCHELLO_SYNC1		0xa5
#define	SNCHELLO_SYNC2		0x5a
#define	SNCHELLO_SYNC3		0x11

#define	SNCHELLO_SYNC_LEN	4									// 4 bytes in sync

#define	SNCHELLO_BEACON	2									// a beacon event (SNCControl only)
#define	SNCHELLO_UP		1									// state in hello state message
#define	SNCHELLO_DOWN		0									// as above

class SNCComponentData;

typedef struct
{
    unsigned char helloSync[SNCHELLO_SYNC_LEN];				// a 4 byte code to check synchronisation (SNCHELLO_SYNC)
    SNC_IPADDR IPAddr;									// device IP address
    SNC_UID componentUID;								// Component UID
    SNC_APPNAME appName;									// the app name of the sender
    SNC_COMPTYPE componentType;							// the component type of the sender
    unsigned char priority;									// priority of SNCControl
    unsigned char unused1;									// was operating mode
    SNC_UC2 interval;									// heartbeat send interval
} SNCHELLO;

//	SNC_HEARTBEAT is the type sent on the SNCLink. It is the hello but with the SNC_MESSAGE header

typedef struct
{
    SNC_MESSAGE syntroMessage;							// the message header
    SNCHELLO hello;											// the hello itself
} SNC_HEARTBEAT;


typedef struct
{
    bool inUse;												// true if entry being used
    SNCHELLO hello;											// received hello
    char IPAddr[SNC_IPSTR_LEN];							// a convenient string form of address
    qint64 lastSNCHello;										// time of last hello
} SNCHELLOENTRY;


class SNCSocket;

class SNCHello : public SNCThread
{
    Q_OBJECT

public:
    SNCHello(SNCComponentData *data, const QString& logTag);
    ~SNCHello();

    void sendSNCHelloBeacon();                                  // sends a hello to SNCControls to elicit a response
    void sendSNCHelloBeaconResponse(SNCHELLO *hello);           // responds to a beacon
    char *getSNCHelloEntry(int index);                          // gets a formatted string version of the hello entry
    void copySNCHelloEntry(int index, SNCHELLOENTRY *dest);     // gets a copy of the SNCHELLOENTRY data structure at index

    //--------------------------------------------
    //	These variables should be changed on startup to get the desired behaviour

    SNCThread *m_parentThread;                              // this is set with the parent thred's ID if messages are required

    int m_socketFlags;										// set to 1 for use socket number

    int m_priority;											// priority to be advertised

    //--------------------------------------------

    //	The lock should always be used when processing the SNCHelloArray

    SNCHELLOENTRY m_helloArray[SNC_MAX_CONNECTEDCOMPONENTS];
    QMutex m_lock;											// for access to the hello array


    void initThread();
    bool processMessage(SNCThreadMsg* msg);
    bool findComponent(SNCHELLOENTRY *foundSNCHelloEntry, SNC_UID *UID);
    bool findComponent(SNCHELLOENTRY *foundSNCHelloEntry, char *appName, char *componentType);
    bool findBestControl(SNCHELLOENTRY *foundSNCHelloEntry);

signals:
    void helloDisplayEvent(SNCHello *hello);

protected:
    void timerEvent(QTimerEvent *event);
    void processSNCHello();
    void deleteEntry(SNCHELLOENTRY *pH);
    bool matchSNCHello(SNCHELLO *a, SNCHELLO *b);
    void processTimers();

    SNCHELLO m_RXSNCHello;
    char m_IpAddr[SNC_MAX_NONTAG];
    unsigned int m_port;
    SNCSocket *m_helloSocket;
    SNCComponentData *m_componentData;
    bool m_control;
    qint64 m_controlTimer;

private:
    int m_timer;
    QString m_logTag;
};

#endif // _SNCHELLO_H_

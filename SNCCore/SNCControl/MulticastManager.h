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

#ifndef MULTICASTMANAGER_H
#define MULTICASTMANAGER_H

#include "SNCDefs.h"

#include <qobject.h>
#include <qmutex.h>

#define SNCSERVER_MAX_MMAPS		100000                      // max simultaneous multicast registrations

#define MM_REFRESH_INTERVAL		(SNC_CLOCKS_PER_SEC * 5)    // multicast refresh interval

//  MM_REGISTEREDCOMPONENT is used to record who has requested multicast data

typedef struct _REGISTEREDCOMPONENT
{
    SNC_UID registeredUID;                                  // the registered UID
    int port;                                               // the registered port to send data to
    unsigned char sendSeq;                                  // the next send sequence number
    unsigned char lastAckSeq;                               // last received ack sequence number
    qint64 lastSendTime;                                    // in order to timeout the WFAck condition
    struct _REGISTEREDCOMPONENT	*next;                      // so they can be linked together
} MM_REGISTEREDCOMPONENT;

//  MM_MMAP records info about a multicast service

typedef struct
{
    bool valid;                                             // true if the entry is in use
    int index;                                              // index of entry
    SNC_UID sourceUID;                                      // the original UID (i.e. where message came from)
    SNC_UID prevHopUID;                                     // previous hop UID (which may be different if via tunnel(s))
    MM_REGISTEREDCOMPONENT *head;                           // head of the registered component list
    SNC_SERVICE_LOOKUP serviceLookup;                       // the lookup structure
    bool registered;                                        // true if successfully registered for a service
    qint64 lookupSent;                                      // time last lookup was sent
    qint64 lastLookupRefresh;                               // last time a subscriber refreshed its lookup
} MM_MMAP;

class SNCServer;

class MulticastManager : public QObject
{
    Q_OBJECT
public:

//  Construction

    MulticastManager(void);
    ~MulticastManager(void);
    SNCServer *m_server;                                    // this must be set up by SNCServer

//  MMShutdown - must be called before deleting object

    void MMShutdown();

//  MMAllocateMap allocates a map for a new service - the name of service and port are the params.

    MM_MMAP	*MMAllocateMMap(SNC_UID *previousHopUID, SNC_UID *sourceUID, const char *componentName, const char *serviceName, int port);

//  MMFreeMap deallocates a map when the service no longer exists

    void MMFreeMMap(MM_MMAP *pM);					// frees a multicast map entry

//  MMAddRegistered adds a new registration for a service

    bool MMAddRegistered(MM_MMAP *multicastMap, SNC_UID *UID, int port);

//  MMCheckRegistered checks to see if an endpoint is already registered for a service

    bool MMCheckRegistered(MM_MMAP *multicastMap, SNC_UID *UID, int port);

//  MMDeleteRegistered - deletes all multicast mapentries for specified UID if nPort = -1
//  else just ones that match the nPort

    void MMDeleteRegistered(SNC_UID *UID, int port);

//  MMForwardMulticastMessage forwards a message to all registered endpoints

    void MMForwardMulticastMessage(int cmd, SNC_MESSAGE *message, int len);

//  MMProcessMulticastAck - handles an ack from a multicast sink

    void MMProcessMulticastAck(SNC_EHEAD *ehead, int len);

//  MMProcessLookupResponse - handles lookup responses

    void MMProcessLookupResponse(SNC_SERVICE_LOOKUP *serviceLookup, int len);

//  MMBackground - must be called once per second

    void MMBackground();

//  Access to m_MMap should only be made while locked

    MM_MMAP	m_multicastMap[SNCSERVER_MAX_MMAPS];            // the multicast map array
    int m_multicastMapSize;                                 // size of the array actually used
    QMutex m_lock;
    SNC_UID m_myUID;

signals:
    void MMDisplay();
    void MMNewEntry(int index);
    void MMDeleteEntry(int index);
    void MMRegistrationChanged(int index);

protected:
    void sendLookupRequest(MM_MMAP *multicastMap, bool rightNow = false);   // sends a multicast service lookup request
    qint64 m_lastBackground;                                // keeps track of interval between backgrounds

};
#endif // MULTICASTMANAGER_H

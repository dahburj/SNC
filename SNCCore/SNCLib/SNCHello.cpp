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

#include "SNCHello.h"
#include "SNCUtils.h"
#include "SNCSocket.h"
#include "SNCComponentData.h"

// SNCHello

SNCHello::SNCHello(SNCComponentData *data, const QString& logTag) : SNCThread(logTag)
{
    int	i;
    SNCHELLOENTRY	*helloEntry;

    m_componentData = data;
    m_logTag = logTag;
    helloEntry = m_helloArray;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++)
        helloEntry->inUse = false;
    m_parentThread = NULL;                                  // default to do not send messages to parent thread
    m_helloSocket = NULL;
    m_socketFlags = 0;                                      // set to 1 for reuse socket port
}

SNCHello::~SNCHello()
{
    m_parentThread = NULL;
    killTimer(m_timer);
    delete m_helloSocket;
    m_helloSocket = NULL;
}

void SNCHello::initThread()
{
    // behavior changes if Control rather than any other component
    m_control = strcmp(m_componentData->getMyComponentType(), COMPTYPE_CONTROL) == 0;

    if (m_componentData->getMySNCHelloSocket() != NULL) {
        m_helloSocket = m_componentData->getMySNCHelloSocket();
    } else {
        m_helloSocket = new SNCSocket(m_logTag);
        if (m_helloSocket->sockCreate(SOCKET_SNCHELLO + m_componentData->getMyInstance(), SOCK_DGRAM, m_socketFlags) == 0)
        {
           SNCUtils::logError(m_logTag, QString("Failed to open socket on instance %1").arg(m_componentData->getMyInstance()));
            return;
        }
    }
    m_helloSocket->sockSetThread(this);
    m_helloSocket->sockSetReceiveMsg(HELLO_ONRECEIVE_MESSAGE);
    m_helloSocket->sockEnableBroadcast(1);

    m_timer = startTimer(100);
    m_controlTimer = SNCUtils::clock();
}


//  getSNCHelloEntry - returns an entry as a formatted string
//
//  Returns NULL if entry is not in use.
//  Not very re-entrant!

char *SNCHello::getSNCHelloEntry(int index)
{
    static char buf[200];
    SNCHELLOENTRY	*helloEntry;

    if ((index < 0) || (index >= SNC_MAX_CONNECTEDCOMPONENTS)) {
        SNCUtils::logWarn(m_logTag, QString("getSNCHelloEntry with out of range index %1").arg(index));
        return NULL;
    }

    QMutexLocker locker(&m_lock);

    helloEntry = m_helloArray + index;
    if (!helloEntry->inUse)
        return NULL;

    sprintf(buf, "%-20s %-20s %d.%d.%d.%d",
            helloEntry->hello.componentType,
            qPrintable(SNCUtils::displayUID(&helloEntry->hello.componentUID)),
            helloEntry->hello.IPAddr[0], helloEntry->hello.IPAddr[1], helloEntry->hello.IPAddr[2],
                helloEntry->hello.IPAddr[3]);

    return buf;
}

//  CopySNCHelloEntry - returns an copy of a SNCHello entry
//

void SNCHello::copySNCHelloEntry(int index, SNCHELLOENTRY *dest)
{
    if ((index < 0) || (index >= SNC_MAX_CONNECTEDCOMPONENTS)) {
        SNCUtils::logWarn(m_logTag, QString("copySNCHelloEntry with out of range index %1").arg(index));
        return;
    }

    m_lock.lock();
    *dest = m_helloArray[index];
    m_lock.unlock();
}


bool SNCHello::findComponent(SNCHELLOENTRY *foundSNCHelloEntry, SNC_UID *UID)
{
    int i;
    SNCHELLOENTRY *helloEntry;

    QMutexLocker locker(&m_lock);

    helloEntry = m_helloArray;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
        if (!helloEntry->inUse)
            continue;
        if (!SNCUtils::compareUID(&(helloEntry->hello.componentUID), UID))
            continue;

//  found it!!
        memcpy(foundSNCHelloEntry, helloEntry, sizeof(SNCHELLOENTRY));
        return true;
    }
    return false;                                           // not found
}

bool SNCHello::findComponent(SNCHELLOENTRY *foundSNCHelloEntry, char *appName, char *componentType)
{
    int i;
    SNCHELLOENTRY *helloEntry;

    QMutexLocker locker(&m_lock);

    helloEntry = m_helloArray;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
        if (!helloEntry->inUse)
            continue;
        if (strlen(appName) > 0) {
            if (strcmp(appName, (char *)(helloEntry->hello.appName)) != 0)
                continue;                                   // node names don't match
        }
        if (strlen(componentType) > 0) {
            if (strcmp(componentType, (char *)(helloEntry->hello.componentType)) != 0)
                continue;                                   // node names don't match
        }

// found it!!
        memcpy(foundSNCHelloEntry, helloEntry, sizeof(SNCHELLOENTRY));
        return true;
    }
    return false;                                           // not found
}


bool SNCHello::findBestControl(SNCHELLOENTRY *foundSNCHelloEntry)
{
    int i;
    int highestPriority = 0;
    int bestControl = -1;
    SNCHELLOENTRY *helloEntry = NULL;

    QMutexLocker locker(&m_lock);

    helloEntry = m_helloArray;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
        if (!helloEntry->inUse)
            continue;
        if (strcmp(COMPTYPE_CONTROL, (char *)(helloEntry->hello.componentType)) != 0)
            continue;                                       // not a SNCControl

        if ((helloEntry->hello.priority > highestPriority) || (bestControl == -1)) {
            highestPriority = helloEntry->hello.priority;
            bestControl = i;
        }
    }

    if (bestControl == -1)
        return false;                                       // no SNCControls found

// found it!!
    memcpy(foundSNCHelloEntry, m_helloArray + bestControl, sizeof(SNCHELLOENTRY));
    return true;
}


void SNCHello::processSNCHello()
{
    int	i, free, instance;
    SNCHELLOENTRY	*helloEntry;

    if ((m_RXSNCHello.helloSync[0] != SNCHELLO_SYNC0) || (m_RXSNCHello.helloSync[1] != SNCHELLO_SYNC1) ||
            (m_RXSNCHello.helloSync[2] != SNCHELLO_SYNC2) || (m_RXSNCHello.helloSync[3] != SNCHELLO_SYNC3)) {
        SNCUtils::logWarn(m_logTag, QString("Received invalid SNCHello sync"));
        return;                                             // invalid sync
    }

    // check it's from correct subnet - must be the same

    if (!SNCUtils::isInMySubnet(m_RXSNCHello.IPAddr))
        return;                                             // just ignore if not from my subnet

    m_RXSNCHello.appName[SNC_MAX_APPNAME-1] = 0;            // make sure zero terminated
    m_RXSNCHello.componentType[SNC_MAX_COMPTYPE-1] = 0;     // make sure zero terminated
    instance = SNCUtils::convertUC2ToInt(m_RXSNCHello.componentUID.instance);   // get the instance from where the hello came
    free = -1;

    // Only put the entry in the table if the hello came from a SNCControl

    if (instance == INSTANCE_CONTROL) {
        // See if we already have this in the table

        QMutexLocker locker(&m_lock);
        helloEntry = m_helloArray;
        for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
            if (!helloEntry->inUse) {
                if (free == -1)
                    free = i;                               // first free entry - might need later
                continue;                                   // unused entry
            }
            if (matchSNCHello(&(helloEntry->hello), &m_RXSNCHello)) {
                helloEntry->lastSNCHello = SNCUtils::clock();       // up date last received time
                memcpy(&(helloEntry->hello), &m_RXSNCHello, sizeof(SNCHELLO)); // copy information just in case
                sprintf(helloEntry->IPAddr, "%d.%d.%d.%d",
                m_RXSNCHello.IPAddr[0], m_RXSNCHello.IPAddr[1], m_RXSNCHello.IPAddr[2], m_RXSNCHello.IPAddr[3]);
                return;
            }
        }
        if (i == SNC_MAX_CONNECTEDCOMPONENTS){
            if (free == -1) {                               // too many components!
                SNCUtils::logError(m_logTag, QString("Too many components in SNCHello table %1").arg(i));
                return;
            }
            helloEntry = m_helloArray+free;                 // setup new entry
            helloEntry->inUse = true;
            helloEntry->lastSNCHello = SNCUtils::clock();   // up date last received time
            memcpy(&(helloEntry->hello), &m_RXSNCHello, sizeof(SNCHELLO));
            sprintf(helloEntry->IPAddr, "%d.%d.%d.%d",
            m_RXSNCHello.IPAddr[0], m_RXSNCHello.IPAddr[1], m_RXSNCHello.IPAddr[2], m_RXSNCHello.IPAddr[3]);
            SNCUtils::logDebug(m_logTag, QString("SNCHello added %1, %2").arg(SNCUtils::displayUID(&helloEntry->hello.componentUID))
                .arg(helloEntry->hello.appName));
            emit helloDisplayEvent(this);
        }

        SNCHELLOENTRY	*messageSNCHelloEntry;
        if (m_parentThread != NULL) {
            messageSNCHelloEntry = (SNCHELLOENTRY *)malloc(sizeof(SNCHELLOENTRY));
            memcpy(messageSNCHelloEntry, helloEntry, sizeof(SNCHELLOENTRY));
            m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, SNCHELLO_UP, messageSNCHelloEntry);
        }
    } else {
        SNCHELLO	*hello;
        if ((m_parentThread != NULL) && m_control)
        {
            hello = (SNCHELLO *)malloc(sizeof(SNCHELLO));
            memcpy(hello, &m_RXSNCHello, sizeof(SNCHELLO));
            m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, SNCHELLO_BEACON, hello);
        }
    }
}

bool SNCHello::matchSNCHello(SNCHELLO *a, SNCHELLO *b)
{
    return SNCUtils::compareUID(&(a->componentUID), &(b->componentUID));
}


void SNCHello::processTimers()
{
    int	i;
    SNCHELLOENTRY *helloEntry;
    qint64 now = SNCUtils::clock();

    if (m_control && SNCUtils::timerExpired(now, m_controlTimer, SNCHELLO_CONTROL_SNCHELLO_INTERVAL)) {
        m_controlTimer = now;
        sendSNCHelloBeacon();
    }

    QMutexLocker locker(&m_lock);
    helloEntry = m_helloArray;
    for (i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
        if (!helloEntry->inUse)
            continue;                                       // not in use

        if (SNCUtils::timerExpired(now, helloEntry->lastSNCHello, SNCHELLO_LIFETIME)) {	// entry has timed out

            SNCHELLOENTRY	*messageSNCHelloEntry;

            if (m_parentThread != NULL) {
                messageSNCHelloEntry = (SNCHELLOENTRY *)malloc(sizeof(SNCHELLOENTRY));
                memcpy(messageSNCHelloEntry, helloEntry, sizeof(SNCHELLOENTRY));
                m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, SNCHELLO_DOWN, messageSNCHelloEntry);
            }
//			TRACE1("SNCHello deleted %s", helloEntry->IPAddr);
            helloEntry->inUse = false;
            emit helloDisplayEvent(this);
        }
    }
}

void SNCHello::sendSNCHelloBeacon()
{
    if (m_helloSocket == NULL)
        return;                                             // called too soon?
    SNCHELLO hello = m_componentData->getMyHeartbeat().hello;
    m_helloSocket->sockSendTo(&(hello), sizeof(SNCHELLO), SOCKET_SNCHELLO + INSTANCE_CONTROL, NULL);
}

void SNCHello::sendSNCHelloBeaconResponse(SNCHELLO *reqSNCHello)
{
    int instance;

    SNCHELLO hello = m_componentData->getMyHeartbeat().hello;

    instance = SNCUtils::convertUC2ToInt(reqSNCHello->componentUID.instance);
    m_helloSocket->sockSendTo(&(hello), sizeof(SNCHELLO), SOCKET_SNCHELLO + instance, NULL);
}

void SNCHello::deleteEntry(SNCHELLOENTRY *helloEntry)
{
    helloEntry->inUse = false;
    emit helloDisplayEvent(this);
}

// SNCHello message handlers

bool SNCHello::processMessage(SNCThreadMsg* msg)
{
    switch(msg->message) {
        case HELLO_ONRECEIVE_MESSAGE:
            while (m_helloSocket->sockPendingDatagramSize() != -1) {
                m_helloSocket->sockReceiveFrom(&m_RXSNCHello, sizeof(SNCHELLO), m_IpAddr, &m_port);
                processSNCHello();
            }
            return true;
    }
    return false;
}

void SNCHello::timerEvent(QTimerEvent *)
{
    while (m_helloSocket->sockPendingDatagramSize() != -1) {
        m_helloSocket->sockReceiveFrom(&m_RXSNCHello, sizeof(SNCHELLO), m_IpAddr, &m_port);
        processSNCHello();
    }

    processTimers();
}


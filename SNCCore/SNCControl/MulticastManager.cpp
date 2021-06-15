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

#include "MulticastManager.h"
#include "SNCControl.h"
#include "SNCServer.h"

#define TAG "MulticastManager"

MulticastManager::MulticastManager(void)
{
    int		i;

    for (i = 0; i < SNCSERVER_MAX_MMAPS; i++) {
        memset(m_multicastMap+i, 0, sizeof(MM_MMAP));
        m_multicastMap[i].index = i;
        m_multicastMap[i].valid = false;
        m_multicastMap[i].head = NULL;
    }
    m_multicastMapSize = 0;
    m_lastBackground = SNCUtils::clock();
}

MulticastManager::~MulticastManager(void)
{
}


//-----------------------------------------------------------------------------------
//
//  Public functions. All of these are protected by a lock to ensure consistency
//  in a multitasking environment. Protected routines do not use the lock to avoid
//  potential deadlocks.

void MulticastManager::MMShutdown()
{
    int i;

    for (i = 0; i < SNCSERVER_MAX_MMAPS; i++)
        MMFreeMMap(m_multicastMap+i);
}

bool MulticastManager::MMAddRegistered(MM_MMAP *multicastMap, SNC_UID *UID, int port)
{
    MM_REGISTEREDCOMPONENT *registeredComponent;

    QMutexLocker locker(&m_lock);
    if (!multicastMap->valid) {
        SNCUtils::logError(TAG, "Invalid MMAP referenced in AddRegistered");
        return false;
    }

    //  build REGISTEREDCOMPONENT for new registration

    registeredComponent = (MM_REGISTEREDCOMPONENT *)malloc(sizeof(MM_REGISTEREDCOMPONENT));
    registeredComponent->sendSeq = 0;
    registeredComponent->lastAckSeq = 0;
    memcpy(&(registeredComponent->registeredUID), UID, sizeof(SNC_UID));
    registeredComponent->port = port;

    //  Now safe to link in the new one

    registeredComponent->next = multicastMap->head;
    multicastMap->head = registeredComponent;
    multicastMap->lastLookupRefresh = SNCUtils::clock();    // don't time it out straightaway
    sendLookupRequest(multicastMap, true);                  // make sure there's a lookup request for the service
    emit MMRegistrationChanged(multicastMap->index);
    return true;
}


bool MulticastManager::MMCheckRegistered(MM_MMAP *multicastMap, SNC_UID *UID, int port)
{
    MM_REGISTEREDCOMPONENT	*registeredComponent;

    QMutexLocker locker(&m_lock);

    if (!multicastMap->valid) {
        SNCUtils::logError(TAG, "Invalid MMAP referenced in CheckRegistered");
        return false;
    }
    registeredComponent = multicastMap->head;
    while (registeredComponent != NULL) {
        if (SNCUtils::compareUID(UID, &(registeredComponent->registeredUID)) && (registeredComponent->port == port)) {
            multicastMap->lastLookupRefresh = SNCUtils::clock();// somebody still wants it
            return true;                                    // it is there
        }
        registeredComponent = registeredComponent->next;
    }
    return false;                                           // not found
}

void MulticastManager::MMDeleteRegistered(SNC_UID *UID, int port)
{
    MM_REGISTEREDCOMPONENT *registeredComponent, *previousRegisteredComponent, *deletedRegisteredComponent;
    int i;
    MM_MMAP *multicastMap;

    QMutexLocker locker(&m_lock);
    multicastMap = m_multicastMap;

    for (i = 0; i < m_multicastMapSize; i++, multicastMap++) {
        if (!multicastMap->valid)
            continue;
        registeredComponent = multicastMap->head;
        previousRegisteredComponent = NULL;
        while (registeredComponent != NULL) {
            if (SNCUtils::compareUID(UID, &(registeredComponent->registeredUID))) {
                if ((port == -1) || (port == registeredComponent->port)) {  // this is a matched entry
                    SNCUtils::logDebug(TAG, QString("Deleting multicast registration on %1 port %2 for %3")
                        .arg(SNCUtils::displayUID(&registeredComponent->registeredUID)).arg(registeredComponent->port)
                            .arg(SNCUtils::displayUID(UID)));
                    if (previousRegisteredComponent == NULL) {  // its the head of the list
                        multicastMap->head = registeredComponent->next;
                    } else {                                // not the head so relink around
                        previousRegisteredComponent->next = registeredComponent->next;
                    }
                    deletedRegisteredComponent = registeredComponent;
                    registeredComponent = registeredComponent->next;
                    free(deletedRegisteredComponent);
                    emit MMRegistrationChanged(multicastMap->index);
                    continue;
                }
            }
            previousRegisteredComponent = registeredComponent;
            registeredComponent = registeredComponent->next;
        }
    }
}


void MulticastManager::MMForwardMulticastMessage(int cmd, SNC_MESSAGE *message, int len)
{
    MM_REGISTEREDCOMPONENT *registeredComponent;
    SNC_EHEAD *inEhead, *outEhead, *ackEhead;
    unsigned char *msgCopy;
    int multicastMapIndex;
    MM_MMAP *multicastMap;

    QMutexLocker locker (&m_lock);
    inEhead = (SNC_EHEAD *)message;
    multicastMapIndex = SNCUtils::convertUC2ToUInt(inEhead->destPort);  // get the dest port number (i.e. my slot number)
    if (multicastMapIndex >= m_multicastMapSize) {
        SNCUtils::logWarn(TAG, QString("Multicast message with illegal DPort %1").arg(multicastMapIndex));
        return;
    }
    multicastMap = m_multicastMap + multicastMapIndex;      // get pointer to entry
    if (!multicastMap->valid) {
        SNCUtils::logWarn(TAG, QString("Multicast message on not in use map slot %1").arg(multicastMap->index));
        return;                                             // not in use - hmmm. Should not happen!
    }
    if (len < (int)sizeof(SNC_EHEAD)) {
        SNCUtils::logWarn(TAG, QString("Multicast message is too short %1").arg(len));
        return;
    }
    if (!SNCUtils::compareUID(&(multicastMap->sourceUID), &(inEhead->sourceUID))) {
        SNCUtils::logWarn(TAG, QString("UID %1 of incoming multicast didn't match UID of slot %2")
            .arg(SNCUtils::displayUID(&inEhead->sourceUID)).arg(SNCUtils::displayUID(&multicastMap->sourceUID)));

        return;
    }
    qint64 now = SNCUtils::clock();
    m_server->m_multicastIn++;
    m_server->m_multicastInRate++;

    registeredComponent = multicastMap->head;
    while (registeredComponent != NULL) {
        if (!SNCUtils::isSendOK(registeredComponent->sendSeq, registeredComponent->lastAckSeq)) {   // see if we have timed out waiting for ack
            if (!SNCUtils::timerExpired(now, registeredComponent->lastSendTime, EXCHANGE_TIMEOUT)){
                registeredComponent = registeredComponent->next;
                continue;                                   // not yet long enough to declare a timeout
            } else {
                registeredComponent->lastAckSeq = registeredComponent->sendSeq;
                SNCUtils::logWarn(TAG, QString("WFAck timeout on %1").arg(SNCUtils::displayUID(&registeredComponent->registeredUID)));
            }
        }
        msgCopy = (unsigned char *)malloc(len);
        memcpy(msgCopy, message, len);
        outEhead = (SNC_EHEAD *)msgCopy;
        SNCUtils::convertIntToUC2(registeredComponent->port, outEhead->destPort);// this is the receiver's service index that was requested
        SNCUtils::convertIntToUC2(multicastMapIndex, outEhead->sourcePort); // this is my slot number (needed for the ack)
        outEhead->destUID = registeredComponent->registeredUID;
        outEhead->sourceUID = multicastMap->sourceUID;
        outEhead->seq = registeredComponent->sendSeq;
        registeredComponent->sendSeq++;
        SNCUtils::logDebug(TAG, QString("Forwarding mcast from component %1 to %2")
                .arg(SNCUtils::displayUID(&outEhead->sourceUID)).arg(SNCUtils::displayUID(&registeredComponent->registeredUID)));
        m_server->sendSNCMessage(&(registeredComponent->registeredUID), cmd, (SNC_MESSAGE *)msgCopy,
                    len, SNCLINK_LOWPRI);
        m_server->m_multicastOut++;
        m_server->m_multicastOutRate++;
        registeredComponent->lastSendTime = now;
        registeredComponent = registeredComponent->next;
    }

    // send an ACK unless the recipient is us
    if (!SNCUtils::compareUID(&m_myUID, &multicastMap->sourceUID)) {
        ackEhead = (SNC_EHEAD *)malloc(sizeof(SNC_EHEAD));
        ackEhead->sourceUID = m_myUID;
        ackEhead->destUID = multicastMap->sourceUID;
        SNCUtils::copyUC2(ackEhead->sourcePort, inEhead->destPort);
        SNCUtils::copyUC2(ackEhead->destPort, inEhead->sourcePort);
        ackEhead->seq = inEhead->seq + 1;

        if (!m_server->sendSNCMessage(&(multicastMap->prevHopUID), SNCMSG_MULTICAST_ACK,
                (SNC_MESSAGE *)ackEhead, sizeof(SNC_EHEAD), SNCLINK_MEDHIGHPRI)) {
            SNCUtils::logWarn(TAG, QString("Failed mcast ack to %1").arg(SNCUtils::displayUID(&multicastMap->prevHopUID)));
        }
    }
}


void	MulticastManager::MMProcessMulticastAck(SNC_EHEAD *ehead, int len)
{
    MM_MMAP *multicastMap;
    MM_REGISTEREDCOMPONENT *registeredComponent;
    int slot;

    if (len < (int)sizeof(SNC_EHEAD)) {
        SNCUtils::logWarn(TAG, QString("Multicast ack is too short %1").arg(len));
        return;
    }

    slot = SNCUtils::convertUC2ToUInt(ehead->destPort);     // get the port number
    if (slot >= m_multicastMapSize) {
        SNCUtils::logWarn(TAG, QString("Invalid dest port %1 for multicast ack from %2").arg(slot).arg(SNCUtils::displayUID(&ehead->sourceUID)));
        return;
    }
    QMutexLocker locker(&m_lock);

    multicastMap = m_multicastMap + slot;
    if (!multicastMap->valid)
        return;                                             // probably disconnected or something
    if (!SNCUtils::compareUID(&(multicastMap->sourceUID), &(ehead->destUID))) {
        SNCUtils::logWarn(TAG, QString("Multicast ack dest %1 doesn't match source %2")
            .arg(SNCUtils::displayUID(&ehead->destUID))
            .arg(SNCUtils::displayUID(&multicastMap->sourceUID)));

        return;
    }

    registeredComponent = multicastMap->head;
    while (registeredComponent != NULL) {
        if (SNCUtils::compareUID(&(ehead->sourceUID), &(registeredComponent->registeredUID)) &&
                    (SNCUtils::convertUC2ToInt(ehead->sourcePort) == registeredComponent->port)) {
            SNCUtils::logDebug(TAG, QString("Matched ack from remote component %1 port %2")
                    .arg(SNCUtils::displayUID(&ehead->sourceUID)).arg(registeredComponent->port));
            registeredComponent->lastAckSeq = ehead->seq;
            return;
        }
        registeredComponent = registeredComponent->next;
    }

    SNCUtils::logWarn(TAG, QString("Failed to match ack from %1 port %2").arg(SNCUtils::displayUID(&ehead->sourceUID))
        .arg(SNCUtils::convertUC2ToInt(ehead->sourcePort)));
}



MM_MMAP	*MulticastManager::MMAllocateMMap(SNC_UID *prevHopUID, SNC_UID *sourceUID,
                                const char *componentName, const char *serviceName, int port)
{
    int i;
    MM_MMAP *multicastMap;

    QMutexLocker locker(&m_lock);
    multicastMap = m_multicastMap;
    for (i = 0; i < SNCSERVER_MAX_MMAPS; i++, multicastMap++) {
        if (!multicastMap->valid)
            break;
    }
    if (i == SNCSERVER_MAX_MMAPS) {
        SNCUtils::logError(TAG, "No more multicast maps");
        return NULL;
    }
    if (i >= m_multicastMapSize)
        m_multicastMapSize = i+1;                           // update the in use map array size
    multicastMap->head = NULL;
    multicastMap->valid = true;
    multicastMap->prevHopUID = *prevHopUID;                 // this is the previous hop UID for the service (i.e. where the data comes from)
    multicastMap->sourceUID = *sourceUID;                   // this is the original source of the stream
    sprintf(multicastMap->serviceLookup.servicePath, "%s%c%s", componentName, SNC_SERVICEPATH_SEP, serviceName);
    SNCUtils::convertIntToUC2(port, multicastMap->serviceLookup.remotePort);// this is the target port (the service port)
    SNCUtils::convertIntToUC2(i, multicastMap->serviceLookup.localPort);    // this is the index into the MMap array
    multicastMap->serviceLookup.response = SERVICE_LOOKUP_FAIL;// indicate lookup response not valid
    multicastMap->serviceLookup.serviceType = SERVICETYPE_MULTICAST;// indicate multicast
    multicastMap->registered = false;                       // indicate not registered
    multicastMap->lookupSent = SNCUtils::clock();           // not important until something registered on it
    SNCUtils::logDebug(TAG, QString("Added %1 from slot %2 to multicast table in slot %3").arg(serviceName).arg(port).arg(i));
    emit MMNewEntry(i);
    return multicastMap;
}

void MulticastManager::MMFreeMMap(MM_MMAP *multicastMap)
{
    MM_REGISTEREDCOMPONENT	*registeredComponent;

    QMutexLocker locker(&m_lock);
    if (!multicastMap->valid)
        return;
    emit MMDeleteEntry(multicastMap->index);
    multicastMap->valid = false;
    while (multicastMap->head != NULL) {
        registeredComponent = multicastMap->head;
        SNCUtils::logDebug(TAG, QString("Freeing MMap %1, component %2 port %3")
            .arg(multicastMap->serviceLookup.servicePath)
            .arg(SNCUtils::displayUID(&registeredComponent->registeredUID)).arg(registeredComponent->port));
        multicastMap->head = registeredComponent->next;
        free(registeredComponent);
    }
}


void MulticastManager::MMProcessLookupResponse(SNC_SERVICE_LOOKUP *serviceLookup, int len)
{
    int index;
    MM_MMAP *multicastMap;

    if (len != sizeof(SNC_SERVICE_LOOKUP)) {
        SNCUtils::logWarn(TAG, QString("Lookup response too short %1").arg(len));
        return;
    }
    index = SNCUtils::convertUC2ToUInt(serviceLookup->localPort);   // get the local port
    if (index >= m_multicastMapSize) {
        SNCUtils::logWarn(TAG, QString("Lookup response from %1 port %2 to incorrect local port %3")
            .arg(SNCUtils::displayUID(&serviceLookup->lookupUID))
            .arg(SNCUtils::convertUC2ToInt(serviceLookup->remotePort))
            .arg(SNCUtils::convertUC2ToInt(serviceLookup->localPort)));

        return;
    }

    QMutexLocker locker(&m_lock);
    multicastMap = m_multicastMap + index;
    if (!multicastMap->valid) {
        SNCUtils::logWarn(TAG, QString("Lookup response to unused local mmap from %1").arg(serviceLookup->servicePath));
        return;
    }

    if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {   // the endpoint is not there!
        if (multicastMap->serviceLookup.response == SERVICE_LOOKUP_SUCCEED) {	// was ok but went away
            SNCUtils::logDebug(TAG, QString("Service %1 no longer available").arg(multicastMap->serviceLookup.servicePath));
        } else {
            SNCUtils::logDebug(TAG, QString("Service %s unavailable").arg(multicastMap->serviceLookup.servicePath));
        }
        multicastMap->serviceLookup.response = SERVICE_LOOKUP_FAIL;	// indicate that the entry is invalid
        multicastMap->registered = false;                   // and we can't be registere
        return;
    }
    if (multicastMap->serviceLookup.response == SERVICE_LOOKUP_SUCCEED) {	// we had a previously valid entry
        if ((SNCUtils::convertUC4ToInt(serviceLookup->ID) == SNCUtils::convertUC4ToInt(multicastMap->serviceLookup.ID))
            && (SNCUtils::compareUID(&(serviceLookup->lookupUID), &(multicastMap->serviceLookup.lookupUID)))
            && (SNCUtils::convertUC2ToInt(serviceLookup->remotePort) == SNCUtils::convertUC2ToInt(multicastMap->serviceLookup.remotePort))) {
                SNCUtils::logDebug(TAG, QString("Reconfirmed %1").arg(serviceLookup->servicePath));
        }
    } else {
//	If we get here, something changed
        SNCUtils::logDebug(TAG, QString("Service %1 mapped to %2 port %3").arg(serviceLookup->servicePath)
            .arg(SNCUtils::displayUID(&serviceLookup->lookupUID)).arg(SNCUtils::convertUC2ToInt(serviceLookup->remotePort)));
        multicastMap->serviceLookup = *serviceLookup;       // record data
    }
    multicastMap->registered = true;
    emit MMDisplay();
}


void MulticastManager::MMBackground()
{
    MM_MMAP *multicastMap;
    int index;

    qint64 now = SNCUtils::clock();

    if (!SNCUtils::timerExpired(now, m_lastBackground, SNC_CLOCKS_PER_SEC))
        return;

    m_lastBackground = now;
    emit MMDisplay();
    QMutexLocker locker(&m_lock);
    multicastMap = m_multicastMap;
    for (index = 0; index < m_multicastMapSize; index++, multicastMap++) {
        if (!multicastMap->valid)
            continue;
        if (SNCUtils::compareUID(&(multicastMap->sourceUID), &m_myUID))
            continue;                                       // don't do anything more for SyntroCOntrol services
        if (multicastMap->head == NULL)
            continue;                                       // no registrations so don't refresh
        // Note - this timer check is only if there's a stuck registration for some reason - it
        // stops continual data transfer when nobody really wants it. Hopefully someone will time the
        // stuck registration out!
        if (SNCUtils::timerExpired(SNCUtils::clock(), multicastMap->lastLookupRefresh, MULTICAST_REFRESH_TIMEOUT)) {
            SNCUtils::logDebug(TAG, QString("Too long since last incoming lookup request on %1 local port %2")
                    .arg(SNCUtils::displayUID(&multicastMap->sourceUID)).arg(index));
            continue;                                       // don't send a lookup request as nobody interested
        }
        sendLookupRequest(multicastMap);
    }
}


//----------------------------------------------------------------------------


void MulticastManager::sendLookupRequest(MM_MMAP *multicastMap, bool rightNow)
{
    SNC_SERVICE_LOOKUP *serviceLookup;
    SNC_SERVICE_ACTIVATE *serviceActivate;

    // no messages to ourself
    if (SNCUtils::compareUID(&m_myUID, &multicastMap->prevHopUID))
        return;

    qint64 now = SNCUtils::clock();
    if (!rightNow && !SNCUtils::timerExpired(now, multicastMap->lookupSent, SERVICE_LOOKUP_INTERVAL))
        return;											// too early to send again

    if (SNCUtils::convertUC2ToInt(multicastMap->prevHopUID.instance) < INSTANCE_COMPONENT) {
        serviceLookup = (SNC_SERVICE_LOOKUP *)malloc(sizeof(SNC_SERVICE_LOOKUP));
        *serviceLookup = multicastMap->serviceLookup;
        SNCUtils::logDebug(TAG, QString("Sending lookup request for %1 from port %2").arg(serviceLookup->servicePath)
                           .arg(SNCUtils::convertUC2ToUInt(serviceLookup->localPort)));

        if (!m_server->sendSNCMessage(&(multicastMap->prevHopUID), SNCMSG_SERVICE_LOOKUP_REQUEST,
                (SNC_MESSAGE *)serviceLookup, sizeof(SNC_SERVICE_LOOKUP), SNCLINK_MEDHIGHPRI)) {
            SNCUtils::logWarn(TAG, QString("Failed sending lookup request to %1").arg(SNCUtils::displayUID(&multicastMap->prevHopUID)));
        }
    }
    else {
        SNCUtils::logDebug(TAG, QString("Sending service activate for %1 from port %2")
                .arg(multicastMap->serviceLookup.servicePath).arg(SNCUtils::convertUC2ToUInt(multicastMap->serviceLookup.localPort)));
        serviceActivate = (SNC_SERVICE_ACTIVATE *)malloc(sizeof(SNC_SERVICE_ACTIVATE));
        SNCUtils::copyUC2(serviceActivate->endpointPort, multicastMap->serviceLookup.remotePort);
        SNCUtils::copyUC2(serviceActivate->componentIndex, multicastMap->serviceLookup.componentIndex);
        SNCUtils::copyUC2(serviceActivate->SNCControlPort, multicastMap->serviceLookup.localPort);
        serviceActivate->response = multicastMap->serviceLookup.response;
        m_server->sendSNCMessage(&(multicastMap->prevHopUID),
                SNCMSG_SERVICE_ACTIVATE, (SNC_MESSAGE *)serviceActivate, sizeof(SNC_SERVICE_ACTIVATE), SNCLINK_MEDHIGHPRI);
    }
    multicastMap->lookupSent = now;
}




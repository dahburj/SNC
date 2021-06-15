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

#include "SNCEndpoint.h"
#include "SNCUtils.h"
#include "SNCSocket.h"

//#define ENDPOINT_TRACE
//#define CFS_TRACE

#define TAG   "SNCEndpoint"

//----------------------------------------------------------
//
//	Client level functions

bool SNCEndpoint::clientLoadServices(bool enabled, int *serviceCount, int *serviceStart)
{
    QSettings *settings = SNCUtils::getSettings();
    int count = settings->beginReadArray(SNC_PARAMS_CLIENT_SERVICES);

    int serviceType;
    bool local;
    bool first = true;
    int port;

    for (int i = 0; i < count; i++) {
        settings->setArrayIndex(i);
        local = settings->value(SNC_PARAMS_CLIENT_SERVICE_LOCATION).toString() == SNC_PARAMS_SERVICE_LOCATION_LOCAL;
        QString serviceTypeString = settings->value(SNC_PARAMS_CLIENT_SERVICE_TYPE).toString();
        if (serviceTypeString == SNC_PARAMS_SERVICE_TYPE_MULTICAST) {
            serviceType = SERVICETYPE_MULTICAST;
        } else if (serviceTypeString == SNC_PARAMS_SERVICE_TYPE_E2E) {
            serviceType = SERVICETYPE_E2E;
        }
        else {
            SNCUtils::logWarn(TAG, QString("Unrecognized type for service %1").arg(settings->value(SNC_PARAMS_CLIENT_SERVICE_NAME).toString()));
            serviceType = SERVICETYPE_E2E;			// fall back to this
        }
        port = clientAddService(settings->value(SNC_PARAMS_CLIENT_SERVICE_NAME).toString(), serviceType, local, enabled);
        if (first) {
            *serviceStart = port;
            first = false;
        }
    }
    settings->endArray();
    *serviceCount = count;

    delete settings;
    return true;
}

int SNCEndpoint::clientAddService(QString servicePath, int serviceType, bool local, bool enabled)
{
    int servicePort;
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    service = m_serviceInfo;
    for (servicePort = 0; servicePort < SNC_MAX_SERVICESPERCOMPONENT; servicePort++, service++) {
        if (!service->inUse)
            break;
    }
    if (servicePort == SNC_MAX_SERVICESPERCOMPONENT) {
        SNCUtils::logError(TAG, QString("Service info array full"));
        return -1;
    }
    if (servicePath.length() > SNC_MAX_SERVPATH - 1) {
        SNCUtils::logError(TAG, QString("Service path too long (%1) %2").arg(servicePath.length()).arg(servicePath));
        return -1;
    }
    service->inUse = true;
    service->enabled = enabled;
    service->local = local;
    service->removingService = false;
    service->serviceType = serviceType;
    strcpy(service->servicePath, qPrintable(servicePath));
    service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;
    service->serviceData = -1;
    service->serviceDataPointer = NULL;
    if (!local) {
        strcpy(service->serviceLookup.servicePath, qPrintable(servicePath));
        service->serviceLookup.serviceType = serviceType;
        service->state = SNC_REMOTE_SERVICE_STATE_LOOK;	// flag for immediate lookup request
        service->tLastLookup = SNCUtils::clock();
    }
    buildDE();
    forceDE();
    return servicePort;
}

bool SNCEndpoint::clientEnableService(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Request to enable service on out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Attempt to enable service on not in use port %1").arg(servicePort));
        return false;
    }
    if (service->removingService) {
        SNCUtils::logWarn(TAG, QString("Attempt to enable service on port %1 that is being removed").arg(servicePort));
        return false;
    }
    service->enabled = true;
    if (service->local) {
        service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;
        buildDE();
        forceDE();
        return true;
    } else {
        service->state = SNC_REMOTE_SERVICE_STATE_LOOK;
        service->tLastLookup = SNCUtils::clock();
        return true;
    }
}

bool	SNCEndpoint::clientIsServiceActive(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to status service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to status service on not in use port %1").arg(servicePort));
        return false;
    }
    if (!service->enabled)
        return false;
    if (service->local) {
        return service->state == SNC_LOCAL_SERVICE_STATE_ACTIVE;
    } else {
        return service->state == SNC_REMOTE_SERVICE_STATE_REGISTERED;
    }
}


bool	SNCEndpoint::clientIsServiceEnabled(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get enable status service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get enable status service on not in use port %1").arg(servicePort));
        return false;
    }
    if (service->local) {
        return service->enabled;
    } else {
        if (!service->enabled)
            return false;
        if (service->state == SNC_REMOTE_SERVICE_STATE_REMOVE)
            return false;
        if (service->state == SNC_REMOTE_SERVICE_STATE_REMOVING)
            return false;
        return true;
    }
}

bool SNCEndpoint::clientDisableService(int servicePort)
{
    SNC_SERVICE_INFO *service;

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Request to disable service out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Attempt to disable service on not in use port %1").arg(servicePort));
        return false;
    }
    if (service->local) {
        forceDE();
        service->enabled = false;
        return true;
    } else {
        switch (service->state) {
            case SNC_REMOTE_SERVICE_STATE_REMOVE:
            case SNC_REMOTE_SERVICE_STATE_REMOVING:
                SNCUtils::logWarn(TAG, QString("Attempt to disable service on port %1 that's already being disabled").arg(servicePort));
                return false;

            case SNC_REMOTE_SERVICE_STATE_LOOK:
                if (service->removingService) {
                    service->inUse = false;					// being removed
                    service->removingService = false;
                }
                service->enabled = false;
                return true;

            case SNC_REMOTE_SERVICE_STATE_LOOKING:
            case SNC_REMOTE_SERVICE_STATE_REGISTERED:
                service->state = SNC_REMOTE_SERVICE_STATE_REMOVE; // indicate we want to remove whatever happens
                return true;

            default:
                SNCUtils::logDebug(TAG, QString("Disable service on port %1 with state %2").arg(servicePort).arg(service->state));
                service->enabled = false;					// just disable then
                if (service->removingService) {
                    service->inUse = false;
                    service->removingService = false;
                }
                return false;
        }
    }
}

bool SNCEndpoint::clientRemoveService(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to remove service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to remove a service on not in use port %1").arg(servicePort));
        return false;
    }
    if (!service->enabled) {
        service->inUse = false;								// if not enabled, just mark as not in use
        return true;
    }
    if (service->local) {
        service->enabled = false;
        service->inUse = false;
        buildDE();
        forceDE();
        return true;
    } else {
        service->removingService = true;
        locker.unlock();									// deadlock if don't do this!
        return clientDisableService(servicePort);
    }
}

QString SNCEndpoint::clientGetServicePath(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
        return "";
    }
    service = m_serviceInfo + servicePort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on disabled port %1").arg(servicePort));
        return "";
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
        return "";
    }
    return QString(service->servicePath);
}

int SNCEndpoint::clientGetServiceType(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
        return -1;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
        return -1;
    }
    return service->serviceType;
}

bool SNCEndpoint::clientIsServiceLocal(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
        return false;
    }
    return service->local;
}

int	SNCEndpoint::clientGetServiceDestPort(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
        return -1;
    }
    service = m_serviceInfo + servicePort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on disabled port %1").arg(servicePort));
        return -1;
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
        return -1;
    }
    if (service->local) {
        if (service->state != SNC_LOCAL_SERVICE_STATE_ACTIVE) {
            SNCUtils::logWarn(TAG, QString("Tried to get dest port for inactive service port %1").arg(servicePort));
            return -1;
        }
        return service->destPort;
    } else {
        if (service->state != SNC_REMOTE_SERVICE_STATE_REGISTERED) {
            SNCUtils::logWarn(TAG, QString("Tried to get dest port for inactive service port %1 in state %2").arg(servicePort).arg(service->state));
            return -1;
        }
        return SNCUtils::convertUC2ToInt(service->serviceLookup.remotePort);
    }
}

int SNCEndpoint::clientGetRemoteServiceState(int servicePort)
{
    SNC_SERVICE_INFO *remoteService;

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Request for service state on out of range port %1").arg(servicePort));
        return SNC_REMOTE_SERVICE_STATE_NOTINUSE;
    }
    remoteService = m_serviceInfo + servicePort;

    if (!remoteService->inUse) {
        return SNC_REMOTE_SERVICE_STATE_NOTINUSE;
    }
    if (remoteService->local) {
        return SNC_REMOTE_SERVICE_STATE_NOTINUSE;
    }
    if (!remoteService->enabled) {
        return SNC_REMOTE_SERVICE_STATE_NOTINUSE;
    }
    return remoteService->state;
}

SNC_UID	*SNCEndpoint::clientGetRemoteServiceUID(int servicePort)
{
    SNC_SERVICE_INFO *remoteService;

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("GetRemoteServiceUID on out of range port %1").arg(servicePort));
        return NULL;
    }
    remoteService = m_serviceInfo + servicePort;
    if (!remoteService->inUse) {
        SNCUtils::logWarn(TAG, QString("GetRemoteServiceUID on not in use port %1").arg(servicePort));
        return NULL;
    }
    if (remoteService->local) {
        SNCUtils::logWarn(TAG, QString("GetRemoteServiceUID on port %1 that is a local service port").arg(servicePort));
        return NULL;
    }
    if (remoteService->state != SNC_REMOTE_SERVICE_STATE_REGISTERED) {
        SNCUtils::logWarn(TAG, QString("GetRemoteServiceUID on port %1 in state %2").arg(servicePort).arg(remoteService->state));
        return NULL;
    }

    return &remoteService->serviceLookup.lookupUID;
}

bool SNCEndpoint::clientIsConnected()
{
    return m_connected;
}

bool SNCEndpoint::clientSetServiceData(int servicePort, int value)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to set data value for service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to set data value on not in use port %1").arg(servicePort));
        return false;
    }
    service->serviceData = value;
    return true;
}

int SNCEndpoint::clientGetServiceData(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value for service in out of range port %1").arg(servicePort));
        return -1;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value on not in use port %1").arg(servicePort));
        return -1;
    }
    return service->serviceData;
}

bool SNCEndpoint::clientSetServiceDataPointer(int servicePort, void *value)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to set data value for service in out of range port %1").arg(servicePort));
        return false;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to set data value on not in use port %1").arg(servicePort));
        return false;
    }
    service->serviceDataPointer = value;
    return true;
}

void *SNCEndpoint::clientGetServiceDataPointer(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value for service in out of range port %1").arg(servicePort));
        return NULL;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value on not in use port %1").arg(servicePort));
        return NULL;
    }
    return service->serviceDataPointer;
}

qint64 SNCEndpoint::clientGetLastSendTime(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value for service in out of range port %1").arg(servicePort));
        return -1;
    }
    service = m_serviceInfo + servicePort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value on not in use port %1").arg(servicePort));
        return -1;
    }
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("Tried to get data value on disabled port %1").arg(servicePort));
        return -1;
    }
    return service->lastSendTime;
}


//----------------------------------------------------------
//
//	send side functions that the app client can call

bool SNCEndpoint::clientClearToSend(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("clientClearToSend with illegal port %1").arg(servicePort));
        return false;
    }

    service = m_serviceInfo + servicePort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("Tried to get clear to send on disabled port %1").arg(servicePort));
        return false;
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Tried to get clear to send on not in use port %1").arg(servicePort));
        return false;
    }

    // within the send/ack window ?
    if (SNCUtils::isSendOK(service->nextSendSeqNo, service->lastReceivedAck)) {
        return true;
    }

    // if we haven't timed out, wait some more
    if (!SNCUtils::timerExpired(SNCUtils::clock(),service->lastSendTime, SNCENDPOINT_MULTICAST_TIMEOUT)) {
        return false;
    }

    // we timed out, reset our sequence numbers
    service->lastReceivedAck = service->nextSendSeqNo;
    return true;
}


SNC_EHEAD *SNCEndpoint::clientBuildMessage(int servicePort, int length)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);
    SNC_EHEAD *message;

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("clientBuildMessage with illegal port %1").arg(servicePort));
        return NULL;
    }

    service = m_serviceInfo + servicePort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("clientBuildMessage on disabled port %1").arg(servicePort));
        return NULL;
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("clientBuildMessage on not in use port %1").arg(servicePort));
        return NULL;
    }

    locker.unlock();
    if (service->serviceType == SERVICETYPE_MULTICAST) {
        int destPort =  clientGetServiceDestPort(servicePort);
        if (destPort == -1) {
            SNCUtils::logDebug(TAG, QString("clientBuildMessage on not in use dest port from local port %1").arg(servicePort));
            return NULL;
        }
        message = SNCUtils::createEHEAD(&(m_UID),
                    servicePort,
                    &(m_UID),
                    destPort,
                    0,
                    length);
    } else {
        if (service->local) {
            SNCUtils::logWarn(TAG, QString("clientBuildMessage on local service port %1").arg(servicePort));
            return NULL;
        }
        int destPort =  clientGetServiceDestPort(servicePort);
        if (destPort == -1) {
            SNCUtils::logDebug(TAG, QString("clientBuildMessage on not in use dest port from local port %1").arg(servicePort));
            return NULL;
        }

        message = SNCUtils::createEHEAD(&(m_UID),
                    servicePort,
                    clientGetRemoteServiceUID(servicePort),
                    clientGetServiceDestPort(servicePort),
                    0,
                    length);
    }
    return message;
}

SNC_EHEAD *SNCEndpoint::clientBuildLocalE2EMessage(int clientPort, SNC_UID *destUID, int destPort, int length)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);
    SNC_EHEAD *message;

    if ((clientPort < 0) || (clientPort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("clientBuildLocalE2EMessage with illegal port %1").arg(clientPort));
        return NULL;
    }

    service = m_serviceInfo + clientPort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("clientBuildLocalE2EMessage on disabled port %1").arg(clientPort));
        return NULL;
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("clientBuildLocalE2EMessage on not in use port %1").arg(clientPort));
        return NULL;
    }
    if (!service->local) {
        SNCUtils::logWarn(TAG, QString("clientBuildLocalE2EMessage on remote service port %1").arg(clientPort));
        return NULL;
    }
    if (service->serviceType != SERVICETYPE_E2E) {
        SNCUtils::logWarn(TAG, QString("clientBuildLocalE2EMessage on port %1 that's not E2E").arg(clientPort));
        return NULL;
    }

    message = SNCUtils::createEHEAD(&(m_UID),
                    clientPort,
                    destUID,
                    destPort,
                    0,
                    length);

    return message;
}

bool SNCEndpoint::clientSendMessage(int servicePort, SNC_EHEAD *message, int length, int priority)
{
    SNC_SERVICE_INFO *service;

    if (!message || length < 1) {
        SNCUtils::logWarn(TAG, QString("clientSendMessage called with invalid parameters"));
        return false;
    }

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("clientSendMessage with illegal port %1").arg(servicePort));
        free(message);
        return false;
    }

    service = m_serviceInfo + servicePort;
    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("clientSendMessage on disabled port %1").arg(servicePort));
        free(message);
        return false;
    }
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("clientSendMessage on not in use port %1").arg(servicePort));
        free(message);
        return false;
    }

    if (service->serviceType == SERVICETYPE_MULTICAST) {
        if (!service->local) {
            SNCUtils::logWarn(TAG, QString("Tried to send multicast message on remote service port %1").arg(servicePort));
            free(message);
            return false;
        }
        if (service->state != SNC_LOCAL_SERVICE_STATE_ACTIVE) {
            SNCUtils::logWarn(TAG, QString("Tried to send multicast message on inactive port %1").arg(servicePort));
            free(message);
            return false;
        }
        message->seq = service->nextSendSeqNo++;
        sendSNCMessage(SNCMSG_MULTICAST_MESSAGE, (SNC_MESSAGE *)message, sizeof(SNC_EHEAD) + length, priority);
    } else {
        if (!service->local && (service->state != SNC_REMOTE_SERVICE_STATE_REGISTERED)) {
            SNCUtils::logWarn(TAG, QString("Tried to send E2E message on remote service without successful lookup on port %1").arg(servicePort));
            free(message);
            return false;
        }
        sendSNCMessage(SNCMSG_E2E, (SNC_MESSAGE *)message, sizeof(SNC_EHEAD) + length, priority);
    }
    service->lastSendTime = SNCUtils::clock();
    return true;
}

bool SNCEndpoint::clientSendMulticastAck(int servicePort)
{
    SNC_SERVICE_INFO *service;

    QMutexLocker locker(&m_serviceLock);

    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("clientSendMulticastAck with illegal port %1").arg(servicePort));
        return false;
    }

    service = m_serviceInfo + servicePort;

    if (!service->enabled) {
        SNCUtils::logWarn(TAG, QString("clientSendMulticastAck on disabled port %1").arg(servicePort));
        return false;
    }

    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("clientSendMulticastAck on not in use port %1").arg(servicePort));
        return false;
    }

    if (service->local) {
        SNCUtils::logWarn(TAG, QString("clientSendMulticastAck on local service port %1").arg(servicePort));
        return false;
    }

    if (service->serviceType != SERVICETYPE_MULTICAST) {
        SNCUtils::logWarn(TAG, QString("clientSendMulticastAck on service port %1 that isn't multicast").arg(servicePort));
        return false;
    }

    sendMulticastAck(servicePort, service->lastReceivedSeqNo + 1);

    return true;
}


//----------------------------------------------------------
//
//	Default functions for appClient overrides

void SNCEndpoint::appClientHeartbeat(SNC_HEARTBEAT *heartbeat, int)
{
    free(heartbeat);
}

void SNCEndpoint::appClientReceiveMulticast(int servicePort, SNC_EHEAD *message, int)
{
    SNCUtils::logWarn(TAG, QString("Unexpected multicast reported by SNCEndpoint on port %1").arg(servicePort));
    free(message);
}


void SNCEndpoint::appClientReceiveMulticastAck(int, SNC_EHEAD *message, int)
{
    free(message);
}

void SNCEndpoint::appClientReceiveE2E(int servicePort, SNC_EHEAD *message, int)
{
    SNCUtils::logWarn(TAG, QString("Unexpected E2E reported by SNCEndpoint on port %1").arg(servicePort));
    free(message);
}

void SNCEndpoint::appClientReceiveDirectory(QStringList)
{
    SNCUtils::logWarn(TAG, QString("Unexpected directory response reported by SNCEndpoint"));
}


//----------------------------------------------------------

SNCEndpoint::SNCEndpoint(qint64 backgroundInterval, const char *compType)
                        : SNCThread(TAG)
{

    m_background = SNCUtils::clock();
    m_compType = compType;
    m_DETimer = m_background;
    m_sentDE = false;
    m_connected = false;
    m_connectInProgress = false;
    m_beaconDelay = false;
    m_backgroundInterval = backgroundInterval;

    QSettings *settings = SNCUtils::getSettings();

    m_configHeartbeatInterval = settings->value(SNC_PARAMS_HBINTERVAL, SNC_HEARTBEAT_INTERVAL).toInt();
    m_configHeartbeatTimeout = settings->value(SNC_PARAMS_HBTIMEOUT, SNC_HEARTBEAT_TIMEOUT).toInt();

    delete settings;
}

SNCEndpoint::~SNCEndpoint(void)
{

}

void SNCEndpoint::finishThread()
{
    killTimer(m_timer);

    appClientExit();

    SNCClose();

    m_hello->exitThread();
}

void SNCEndpoint::initThread()
{

    m_state = "...";
    m_sock = NULL;
    m_SNCLink = NULL;
    m_hello = NULL;

    QSettings *settings = SNCUtils::getSettings();

    m_componentData.init(qPrintable(m_compType), m_configHeartbeatInterval);
    m_UID = m_componentData.getMyUID();

    serviceInit();
    CFSInit();
    m_controlRevert = settings->value(SNC_PARAMS_CONTROLREVERT).toBool();
    m_encryptLink = settings->value(SNC_PARAMS_ENCRYPT_LINK).toBool();

    m_useTunnel = settings->value(SNC_PARAMS_USE_TUNNEL).toBool();
    m_tunnelAddr = settings->value(SNC_PARAMS_TUNNEL_ADDR).toString();
    m_tunnelPort = settings->value(SNC_PARAMS_TUNNEL_PORT).toInt();

    m_heartbeatSendInterval = m_configHeartbeatInterval * SNC_CLOCKS_PER_SEC;
    m_heartbeatTimeoutPeriod = m_heartbeatSendInterval * m_configHeartbeatTimeout;

    int size = settings->beginReadArray(SNC_PARAMS_CONTROL_NAMES);

    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++)
        m_controlName[control][0] = 0;

    for (int control = 0; (control < SNCENDPOINT_MAX_SNCCONTROLS) && (control < size); control++) {
        settings->setArrayIndex(control);
        strcpy(m_controlName[control], qPrintable(settings->value(SNC_PARAMS_CONTROL_NAME).toString()));
    }

    settings->endArray();

    m_hello = new SNCHello(&m_componentData, TAG);
    m_hello->resumeThread();
    m_componentData.getMySNCHelloSocket()->moveToThread(m_hello->thread());

    m_connWait = SNCUtils::clock();
    appClientInit();

    m_timer = startTimer(m_backgroundInterval);

    delete settings;
}

void SNCEndpoint::setHeartbeatTimers(int interval, int timeout)
{
    m_configHeartbeatInterval = interval;
    m_configHeartbeatTimeout = timeout;
}

void SNCEndpoint::timerEvent(QTimerEvent *)
{
    endpointBackground();
    appClientBackground();
}

bool SNCEndpoint::processMessage(SNCThreadMsg* msg)
{
    if (appClientProcessThreadMessage(msg))
            return true;

    return endpointSocketMessage(msg);
}

void SNCEndpoint::updateState(QString msg)
{
    SNCUtils::logInfo(TAG, msg);
    m_state = msg;
}

QString SNCEndpoint::getLinkState()
{
    return m_state;
}

void SNCEndpoint::forceDE()
{
    m_lastHeartbeatSent = SNCUtils::clock() - m_heartbeatSendInterval;
    m_DETimer = SNCUtils::clock() - SNCENDPOINT_DE_INTERVAL;
}

void SNCEndpoint::endpointBackground()
{
    const char *DE;
    SNC_HEARTBEAT *heartbeat;
    int len;
    SNCHELLOENTRY helloEntry;
    int control;

    if ((!m_connectInProgress) && (!m_connected))
        SNCConnect();
    if (!m_connected)
        return;
    if (m_SNCLink == NULL)
        // in process of shutting down the connection
        return;
    m_SNCLink->tryReceiving(m_sock);
    processReceivedData();
    m_SNCLink->trySending(m_sock);

    qint64 now = SNCUtils::clock();

//	Do heartbeat and DE background processing

    if (SNCUtils::timerExpired(now, m_lastHeartbeatSent, m_heartbeatSendInterval)) {
        if (SNCUtils::timerExpired(now, m_DETimer, SNCENDPOINT_DE_INTERVAL)) {	// time to send a DE
            m_DETimer = now;
            DE = m_componentData.getMyDE();						// get a copy of the DE
            len = (int)strlen(DE)+1;
            heartbeat = (SNC_HEARTBEAT *)malloc(sizeof(SNC_HEARTBEAT) + len);
            *heartbeat = m_componentData.getMyHeartbeat();
            memcpy(heartbeat+1, DE, len);
            sendSNCMessage(SNCMSG_HEARTBEAT, (SNC_MESSAGE *)heartbeat, sizeof(SNC_HEARTBEAT) + len, SNCLINK_MEDHIGHPRI);
        } else {									// just the heartbeat
            heartbeat = (SNC_HEARTBEAT *)malloc(sizeof(SNC_HEARTBEAT));
            *heartbeat = m_componentData.getMyHeartbeat();
            sendSNCMessage(SNCMSG_HEARTBEAT, (SNC_MESSAGE *)heartbeat, sizeof(SNC_HEARTBEAT), SNCLINK_MEDHIGHPRI);
        }
        m_lastHeartbeatSent = now;
    }

    if (SNCUtils::timerExpired(now, m_lastHeartbeatReceived, m_heartbeatTimeoutPeriod)) {
        // channel has timed out
        SNCUtils::logWarn(TAG, QString("SNCLink timeout"));
        SNCClose();
        endpointClosed();
        updateState("Connection has timed out");
    }

    CFSBackground();

    if (!SNCUtils::timerExpired(now, m_background, SNCENDPOINT_BACKGROUND_INTERVAL))
        return;									// not time yet

    m_background = now;

    serviceBackground();

    if (m_controlRevert) {
        // send revert beacon if necessary

        if (SNCUtils::timerExpired(now, m_lastReversionBeacon, SNCENDPOINT_REVERSION_BEACON_INTERVAL)) {
            m_hello->sendSNCHelloBeacon();
            m_lastReversionBeacon = now;
        }

        for (control = 0; control < m_controlIndex; control++) {
            if (m_hello->findComponent(&helloEntry, m_controlName[control], (char *)COMPTYPE_CONTROL)) {
                // there is a new and better SNCControl
                SNCUtils::logInfo(TAG, QString("SNCControl list reversion"));
                SNCClose();
                endpointClosed();
                updateState("Higher priority SNCControl available");
                break;
            }
        }

        if ((control == m_controlIndex) && m_priorityMode) {
            if (m_hello->findBestControl(&helloEntry)) {
                if (m_helloEntry.hello.priority < helloEntry.hello.priority) {
                    // there is a new and better SNCControl
                    SNCUtils::logInfo(TAG, QString("SNCControl priority reversion"));
                    SNCClose();
                    endpointClosed();
                    updateState("Higher priority SNCControl available");
                }
            }
        }
    }
}

bool SNCEndpoint::endpointSocketMessage(SNCThreadMsg *msg)
{
    switch(msg->message) {
        case SNCENDPOINT_ONCONNECT_MESSAGE:
            m_connected = true;
            m_connectInProgress = false;
            m_gotHeartbeat = false;
            m_lastHeartbeatReceived = m_lastHeartbeatSent = m_lastReversionBeacon = SNCUtils::clock();
            SNCUtils::logInfo(TAG, QString("SNCLink connected"));
            forceDE();
            endpointConnected();
            if (m_useTunnel)
                updateState(QString("Connected to %1:%2 %3").arg(m_tunnelAddr).arg(m_tunnelPort)
                    .arg(m_encryptLink ? "using SSL" : "unencrypted"));
            else
                updateState(QString("Connected to %1 in %2 mode %3").arg(m_helloEntry.hello.appName)
                    .arg(m_priorityMode ? "priority" : "list").arg(m_encryptLink ? "using SSL" : "unencrypted"));
            return true;

        case SNCENDPOINT_ONCLOSE_MESSAGE:
            if (m_connected)
                updateState("SNCControl connection closed");
            m_connected = false;
            m_connectInProgress = false;
            m_beaconDelay = false;
            m_connWait = SNCUtils::clock();
            delete m_sock;
            m_sock = NULL;
            delete m_SNCLink;
            m_SNCLink = NULL;
            linkCloseCleanup();
            endpointClosed();
            return true;

        case SNCENDPOINT_ONRECEIVE_MESSAGE:
            if (m_sock != NULL) {
#ifdef ENDPOINT_TRACE
                SNCUtils::logDebug(TAG, "SNCEndpoint data received");
#endif
                m_SNCLink->tryReceiving(m_sock);
                processReceivedData();
                return true;
            }
            SNCUtils::logDebug(TAG, "Onreceived but didn't process");
            return true;

        case SNCENDPOINT_ONSEND_MESSAGE:
            if (m_sock != NULL) {
                m_SNCLink->trySending(m_sock);
                return true;
            }
            return true;
    }

    return false;
}


void SNCEndpoint::processReceivedData()
{
    int	cmd;
    int	len;
    SNC_MESSAGE *SNCMessage;
    int	priority;

    QMutexLocker locker(&m_RXLock);

    if (m_SNCLink == NULL)
        return;
    if (m_sock == NULL)
        return;

    for (priority = SNCLINK_HIGHPRI; priority <= SNCLINK_LOWPRI; priority++) {
        while(m_SNCLink->receive(priority, &cmd, &len, &SNCMessage)) {
            processReceivedDataDemux(cmd, len, SNCMessage);
        }
    }
}

void SNCEndpoint::processReceivedDataDemux(int cmd, int len, SNC_MESSAGE*SNCMessage)
{
    SNC_HEARTBEAT *heartbeat;
    SNC_EHEAD *ehead;
    int destPort;
    qint64 now = SNCUtils::clock();

#ifdef ENDPOINT_TRACE
    TRACE2("Processing received data %d %d", cmd, len);
#endif
    switch (cmd) {
        case SNCMSG_HEARTBEAT:						// Heartbeat received
            if (len < (int)sizeof(SNC_HEARTBEAT)) {
                SNCUtils::logError(TAG, QString("Incorrect length heartbeat received %1").arg(len));
                free(SNCMessage);
                break;
            }
            heartbeat = (SNC_HEARTBEAT *)SNCMessage;
            m_gotHeartbeat = true;
            m_lastHeartbeatReceived = now;
            endpointHeartbeat(heartbeat, len);
            break;

        case SNCMSG_MULTICAST_MESSAGE:
            if (len < (int)sizeof(SNC_EHEAD)) {
                SNCUtils::logWarn(TAG, QString("Multicast size error %1").arg(len));
                free(SNCMessage);
                break;
            }

            len -= sizeof(SNC_EHEAD);
            ehead = (SNC_EHEAD *)SNCMessage;
            destPort = SNCUtils::convertUC2ToInt(ehead->destPort);

            if ((destPort < 0) || (destPort >= SNC_MAX_SERVICESPERCOMPONENT)) {
                SNCUtils::logWarn(TAG, QString("Multicast message with out of range port number %1").arg(destPort));
                free(SNCMessage);
                break;
            }

            if (len < (int)sizeof(SNC_RECORD_HEADER)) {
                SNCUtils::logWarn(TAG, QString("Invalid record, too short - length %1 on port %2").arg(len).arg(destPort));
                free(SNCMessage);
                break;
            }

            if (len > SNC_MESSAGE_MAX) {
                SNCUtils::logWarn(TAG, QString("Record too long - length %1 on port %2").arg(len).arg(destPort));
                free(SNCMessage);
                break;
            }

            processMulticast(ehead, len, destPort);
            break;

        case SNCMSG_MULTICAST_ACK:
            if (len < (int)sizeof(SNC_EHEAD)) {
                SNCUtils::logWarn(TAG, QString("Multicast ack size error %1").arg(len));
                free(SNCMessage);
                break;
            }

            len -= sizeof(SNC_EHEAD);
            ehead = (SNC_EHEAD *)SNCMessage;
            destPort = SNCUtils::convertUC2ToInt(ehead->destPort);

            if ((destPort < 0) || (destPort >= SNC_MAX_SERVICESPERCOMPONENT)) {
                SNCUtils::logWarn(TAG, QString("Multicast ack message with out of range port number %1").arg(destPort));
                free(SNCMessage);
                break;
            }

            processMulticastAck(ehead, len, destPort);
            break;

        case SNCMSG_SERVICE_ACTIVATE:
            if (len != (int)sizeof(SNC_SERVICE_ACTIVATE)) {
                SNCUtils::logWarn(TAG, QString("Service activate size error %1").arg(len));
                free(SNCMessage);
                break;
            }
            processServiceActivate((SNC_SERVICE_ACTIVATE *)(SNCMessage));
            free(SNCMessage);
            break;

        case SNCMSG_SERVICE_LOOKUP_RESPONSE:
            if (len != (int)sizeof(SNC_SERVICE_LOOKUP)) {
                SNCUtils::logWarn(TAG, QString("Service lookup size error %1").arg(len));
                free(SNCMessage);
                break;
            }
            processLookupResponse((SNC_SERVICE_LOOKUP *)(SNCMessage));
            free(SNCMessage);
            break;

        case SNCMSG_DIRECTORY_RESPONSE:
            processDirectoryResponse((SNC_DIRECTORY_RESPONSE *)SNCMessage, len);
            free(SNCMessage);
            break;

        case SNCMSG_E2E:
            if (len < (int)sizeof(SNC_EHEAD)) {
                SNCUtils::logWarn(TAG, QString("E2E size error %1").arg(len));
                free(SNCMessage);
                break;
            }
            len -= sizeof(SNC_EHEAD);
            ehead = (SNC_EHEAD *)SNCMessage;
            destPort = SNCUtils::convertUC2ToInt(ehead->destPort);

            if ((destPort < 0) || (destPort >= SNC_MAX_SERVICESPERCOMPONENT)) {
                SNCUtils::logWarn(TAG, QString("E2E message with out of range port number %1").arg(destPort));
                free(SNCMessage);
                break;
            }

            if (len > SNC_MESSAGE_MAX) {
                SNCUtils::logWarn(TAG, QString("E2E message too long - length %1 on port %2").arg(len).arg(destPort));
                free(SNCMessage);
                break;
            }
            if (!CFSProcessMessage(ehead, len, destPort))
                processE2E(ehead, len, destPort);
            break;

        default:
            SNCUtils::logWarn(TAG, QString("Unexpected message %1").arg(cmd));
            free(SNCMessage);
            break;
    }
}


void SNCEndpoint::sendMulticastAck(int servicePort, int seq)
{
    SNC_EHEAD	*ehead;
    SNC_UID uid;

    uid = m_componentData.getMyHeartbeat().hello.componentUID;

    ehead = (SNC_EHEAD *)malloc(sizeof(SNC_EHEAD));
    SNCUtils::copyUC2(ehead->destPort, m_serviceInfo[servicePort].serviceLookup.remotePort);
    SNCUtils::copyUC2(ehead->sourcePort, m_serviceInfo[servicePort].serviceLookup.localPort);
    memcpy(&(ehead->destUID), &(m_serviceInfo[servicePort].serviceLookup.lookupUID), sizeof(SNC_UID));
    memcpy(&(ehead->sourceUID), &uid, sizeof(SNC_UID));
    ehead->seq = seq;
    sendSNCMessage(SNCMSG_MULTICAST_ACK, (SNC_MESSAGE *)ehead, sizeof(SNC_EHEAD), SNCLINK_HIGHPRI);
}


void SNCEndpoint::sendE2EAck(SNC_EHEAD *originalEhead)
{
    SNC_EHEAD *ehead;

    ehead = (SNC_EHEAD *)malloc(sizeof(SNC_EHEAD));
    SNCUtils::copyUC2(ehead->sourcePort, originalEhead->destPort);
    SNCUtils::copyUC2(ehead->destPort, originalEhead->sourcePort);
    memcpy(&(ehead->sourceUID), &(originalEhead->destUID), sizeof(SNC_UID));
    memcpy(&(ehead->destUID), &(originalEhead->sourceUID), sizeof(SNC_UID));
    ehead->seq = originalEhead->seq;
    sendSNCMessage(SNCMSG_E2E, (SNC_MESSAGE *)ehead, sizeof(SNC_EHEAD), SNCLINK_HIGHPRI);
}


void SNCEndpoint::requestDirectory()
{
    SNC_MESSAGE	*message;

    message = (SNC_MESSAGE *)malloc(sizeof(SNC_MESSAGE));
    sendSNCMessage(SNCMSG_DIRECTORY_REQUEST, message, sizeof(SNC_MESSAGE), SNCLINK_LOWPRI);
}

void SNCEndpoint::serviceBackground()
{
    SNC_SERVICE_INFO *service;
    int servicePort;
    qint64 now;

    QMutexLocker locker(&m_serviceLock);
    now = SNCUtils::clock();
    service = m_serviceInfo;
    for (servicePort = 0; servicePort < SNC_MAX_SERVICESPERCOMPONENT; servicePort++, service++) {
        if (!service->inUse)
            continue;										// not being used
        if (!service->enabled)
            continue;
        if (service->local) {								// local service background
            if (service->state != SNC_LOCAL_SERVICE_STATE_ACTIVE)
                continue;									// no current activation anyway

            if (SNCUtils::timerExpired(now, service->tLastLookup, SERVICE_REFRESH_TIMEOUT)) {
                service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;	// indicate inactive and no data should be sent
                SNCUtils::logDebug(TAG, QString("Timed out activation on local service %1 port %2)").arg(service->servicePath).arg(servicePort));
            }
        } else {											// remote service background
            switch (service->state) {
                case SNC_REMOTE_SERVICE_STATE_LOOK:			// need to start looking up
                    service->serviceLookup.response = SERVICE_LOOKUP_FAIL; // indicate we know nothing
                    sendRemoteServiceLookup(service);
                    service->state = SNC_REMOTE_SERVICE_STATE_LOOKING;
                    break;

                case SNC_REMOTE_SERVICE_STATE_LOOKING:
                    if (SNCUtils::timerExpired(now, service->tLastLookup, SERVICE_LOOKUP_INTERVAL))
                        sendRemoteServiceLookup(service);	// try again
                    break;

                case SNC_REMOTE_SERVICE_STATE_REGISTERED:
                    if (SNCUtils::timerExpired(now, service->tLastLookupResponse, SERVICE_REFRESH_TIMEOUT)) {
                        SNCUtils::logWarn(TAG, QString("Refresh timeout on service %1 port %2")
                            .arg(service->serviceLookup.servicePath).arg(servicePort));
                        service->state = SNC_REMOTE_SERVICE_STATE_LOOK;	// go back to looking
                        break;
                    }
                    if (SNCUtils::timerExpired(now, service->tLastLookup, SERVICE_REFRESH_INTERVAL))
                        sendRemoteServiceLookup(service);	// do a refresh
                    break;

                case SNC_REMOTE_SERVICE_STATE_REMOVE:
                    service->serviceLookup.response = SERVICE_LOOKUP_REMOVE; // request the remove
                    sendRemoteServiceLookup(service);
                    service->state = SNC_REMOTE_SERVICE_STATE_REMOVING;
                    service->closingRetries = 0;
                    break;

                case SNC_REMOTE_SERVICE_STATE_REMOVING:
                    if (SNCUtils::timerExpired(now, service->tLastLookup, SERVICE_REMOVING_INTERVAL)) {
                        if (++service->closingRetries == SERVICE_REMOVING_MAX_RETRIES) {
                            SNCUtils::logWarn(TAG, QString("Timed out attempt to remove remote registration for service %1 on port %2")
                                .arg(service->serviceLookup.servicePath).arg(servicePort));
                            if (service->removingService) {
                                service->inUse = false;
                                service->removingService = false;
                            }
                            service->enabled = false;
                            break;
                        }
                        sendRemoteServiceLookup(service);
                        break;
                    }
                        break;

                default:
                    SNCUtils::logWarn(TAG, QString("Illegal state %1 on remote service port %2").arg(service->state).arg(servicePort));
                    service->state = SNC_REMOTE_SERVICE_STATE_LOOK;	// go back to looking
                    break;
            }
        }
    }
}


//----------------------------------------------------------
//
//	Service processing

void SNCEndpoint::serviceInit()
{
    SNC_SERVICE_INFO	*service = m_serviceInfo;

    for (int i = 0; i < SNC_MAX_SERVICESPERCOMPONENT; i++, service++) {
        service->inUse = false;
        service->local = true;
        service->enabled = false;
        service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;
        service->serviceLookup.response = SERVICE_LOOKUP_FAIL;// indicate lookup response not valid
        service->serviceLookup.servicePath[0] = 0;			 // no service path
        service->serviceData = -1;
        service->serviceDataPointer = NULL;
        SNCUtils::convertIntToUC2(i, service->serviceLookup.localPort); // this is my local port index

        service->lastReceivedSeqNo = -1;
        service->nextSendSeqNo = 0;
        service->lastReceivedAck = 0;
        service->lastSendTime = 0;
    }
}


void SNCEndpoint::processServiceActivate(SNC_SERVICE_ACTIVATE *serviceActivate)
{
    int servicePort;
    SNC_SERVICE_INFO	*serviceInfo;

    servicePort = SNCUtils::convertUC2ToInt(serviceActivate->endpointPort);
    if ((servicePort < 0) || (servicePort >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Received service activate on out of range port %1").arg(servicePort));
        return;
    }
    serviceInfo = m_serviceInfo + servicePort;

    if (!serviceInfo->inUse) {
        SNCUtils::logWarn(TAG, QString("Received service activate on not in use port %1").arg(servicePort));
        return;
    }
    if (!serviceInfo->local) {
        SNCUtils::logWarn(TAG, QString("Received service activate on remote service port %1").arg(servicePort));
        return;
    }
    if (!serviceInfo->enabled) {
        return;
    }
    serviceInfo->destPort = SNCUtils::convertUC2ToUInt(serviceActivate->SNCControlPort);	// record the other end's port number
    serviceInfo->state = SNC_LOCAL_SERVICE_STATE_ACTIVE;
    serviceInfo->tLastLookup = SNCUtils::clock();
#ifdef ENDPOINT_TRACE
    TRACE2("Received service activate for port %d to SNCControl port %d", servicePort, serviceInfo->destPort);
#endif
}


void SNCEndpoint::sendRemoteServiceLookup(SNC_SERVICE_INFO *remoteService)
{
    SNC_SERVICE_LOOKUP *serviceLookup;

    if (remoteService->local) {
        SNCUtils::logWarn(TAG, QString("send remote service lookup on local service port"));
        return;
    }

    serviceLookup = (SNC_SERVICE_LOOKUP *)malloc(sizeof(SNC_SERVICE_LOOKUP));
    *serviceLookup = remoteService->serviceLookup;
#ifdef ENDPOINT_TRACE
    TRACE2("Sending request for %s on local port %d", serviceLookup->servicePath, SNCUtils::convertUC2ToUInt(serviceLookup->localPort));
#endif
    sendSNCMessage(SNCMSG_SERVICE_LOOKUP_REQUEST, (SNC_MESSAGE *)serviceLookup, sizeof(SNC_SERVICE_LOOKUP), SNCLINK_MEDHIGHPRI);
    remoteService->tLastLookup = SNCUtils::clock();
}


//	processLookupResponse handles the response to a lookup request,
//	recording the result as required.

void SNCEndpoint::processLookupResponse(SNC_SERVICE_LOOKUP *serviceLookup)
{
    int index;
    SNC_SERVICE_INFO *remoteService;

    index = SNCUtils::convertUC2ToInt(serviceLookup->localPort);		// get the local port
    if ((index < 0) || (index >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Lookup response from %1 port %2 to incorrect local port %3")
                    .arg(SNCUtils::displayUID(&serviceLookup->lookupUID))
                    .arg(SNCUtils::convertUC2ToInt(serviceLookup->remotePort))
                    .arg(SNCUtils::convertUC2ToInt(serviceLookup->localPort)));
        return;
    }

    remoteService = m_serviceInfo + index;
    if (!remoteService->inUse) {
        SNCUtils::logWarn(TAG, QString("Lookup response on not in use port %1").arg(index));
        return;
    }
    if (remoteService->local) {
        SNCUtils::logWarn(TAG, QString("Lookup response on local service port %1").arg(index));
        return;
    }
    if (!remoteService->enabled) {
        return;
    }
    switch (remoteService->state) {
        case SNC_REMOTE_SERVICE_STATE_REMOVE:			// we're waiting to remove so just ignore
            break;

        case SNC_REMOTE_SERVICE_STATE_REMOVING:
            if (serviceLookup->response == SERVICE_LOOKUP_REMOVE) {// removal has been confirmed
                if (remoteService->removingService) {
                    remoteService->inUse = false;				// all done
                    remoteService->removingService = false;
                }
                remoteService->enabled = false;
            }
            break;

        case SNC_REMOTE_SERVICE_STATE_LOOKING:
            if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {	// the service is not there
#ifdef ENDPOINT_TRACE
                TRACE1("Remote service %s unavailable", remoteService->serviceLookup.servicePath);
#else
                ;
#endif
            } else if (serviceLookup->response == SERVICE_LOOKUP_SUCCEED) { // the service is there
#ifdef ENDPOINT_TRACE
                TRACE3("Service %s mapped to %s port %d",
                                serviceLookup->servicePath,
                                SNCUtils::displayUID(&(serviceLookup->lookupUID)),
                                SNCUtils::convertUC2ToInt(serviceLookup->remotePort));
#endif
                // got a good response - record the data and change state to registered

                remoteService->serviceLookup.lookupUID = serviceLookup->lookupUID;
                memcpy(remoteService->serviceLookup.remotePort, serviceLookup->remotePort, sizeof(SNC_UC2));
                memcpy(remoteService->serviceLookup.componentIndex, serviceLookup->componentIndex, sizeof(SNC_UC2));
                memcpy(remoteService->serviceLookup.ID, serviceLookup->ID, sizeof(SNC_UC4));
                remoteService->state = SNC_REMOTE_SERVICE_STATE_REGISTERED; // this is now active
                remoteService->tLastLookupResponse = SNCUtils::clock();		// reset the timeout timer
                remoteService->serviceLookup.response = SERVICE_LOOKUP_SUCCEED;
                remoteService->destPort = SNCUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort);
            }
            break;

        case SNC_REMOTE_SERVICE_STATE_REGISTERED:
            if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {	// the service has gone away
#ifdef ENDPOINT_TRACE
                TRACE1("Remote service %s has gone unavailable", remoteService->serviceLookup.servicePath);
#endif
                remoteService->state = SNC_REMOTE_SERVICE_STATE_LOOK;	// start looking again
            } else if (serviceLookup->response == SERVICE_LOOKUP_SUCCEED) { // the service is still there
                remoteService->tLastLookupResponse = SNCUtils::clock();		// reset the timeout timer
                if ((SNCUtils::convertUC4ToInt(serviceLookup->ID) == SNCUtils::convertUC4ToInt(remoteService->serviceLookup.ID))
                    && (SNCUtils::compareUID(&(serviceLookup->lookupUID), &(remoteService->serviceLookup.lookupUID)))
                    && (SNCUtils::convertUC2ToInt(serviceLookup->remotePort) == SNCUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort))
                    && (SNCUtils::convertUC2ToInt(serviceLookup->componentIndex) == SNCUtils::convertUC2ToInt(remoteService->serviceLookup.componentIndex))) {
#ifdef ENDPOINT_TRACE
                        TRACE1("Reconfirmed %s", serviceLookup->servicePath);
#else
                        ;
#endif
                } else {
#ifdef ENDPOINT_TRACE
                    TRACE3("Service %s remapped to %s port %d",
                                serviceLookup->servicePath,
                                SNCUtils::displayUID(&(serviceLookup->lookupUID)),
                                SNCUtils::convertUC2ToInt(serviceLookup->remotePort));
#endif
                    // got a changed response - record the data and carry on

                    remoteService->serviceLookup.lookupUID = serviceLookup->lookupUID;
                    memcpy(remoteService->serviceLookup.remotePort, serviceLookup->remotePort, sizeof(SNC_UC2));
                    memcpy(remoteService->serviceLookup.componentIndex, serviceLookup->componentIndex, sizeof(SNC_UC2));
                    memcpy(remoteService->serviceLookup.ID, serviceLookup->ID, sizeof(SNC_UC4));
                    remoteService->serviceLookup.response = SERVICE_LOOKUP_SUCCEED;
                    remoteService->destPort = SNCUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort);
                }
            }
            break;

        default:
            SNCUtils::logWarn(TAG, QString("Unexpected state %1 on remote service port %2").arg(remoteService->state).arg(index));
            remoteService->state = SNC_REMOTE_SERVICE_STATE_LOOKING;	// go back to looking
            if (remoteService->removingService) {
                remoteService->inUse = false;
                remoteService->removingService = false;
                remoteService->enabled = false;
            }
            break;
    }
}

void SNCEndpoint::processDirectoryResponse(SNC_DIRECTORY_RESPONSE *directoryResponse, int len)
{
    QStringList dirList;

    QByteArray data(reinterpret_cast<char *>(directoryResponse + 1), len - sizeof(SNC_DIRECTORY_RESPONSE));

    QList<QByteArray> list = data.split(0);

    for (int i = 0; i < list.count(); i++) {
        if (list.at(i).length() > 0)
            dirList << list.at(i);
    }

    appClientReceiveDirectory(dirList);
}

void SNCEndpoint::buildDE()
{
    int servicePort;
    int highestServicePort;
    SNC_SERVICE_INFO *service;

    m_componentData.DESetup();

//	Find highest service number in use in order to keep directory small

    service = m_serviceInfo;
    highestServicePort = 0;
    for (servicePort = 0; servicePort < SNC_MAX_SERVICESPERCOMPONENT; servicePort ++, service++) {
        if (service->inUse && service->enabled)
            highestServicePort = servicePort;
    }

    service = m_serviceInfo;
    for (servicePort = 0; servicePort <= highestServicePort; servicePort ++, service++) {
        if (!service->inUse || !service->enabled) {
            m_componentData.DEAddValue(DETAG_NOSERVICE, "");
            continue;
        }
        if (service->local) {
            if (service->serviceType == SERVICETYPE_MULTICAST)
                m_componentData.DEAddValue(DETAG_MSERVICE, service->servicePath);
            else
                m_componentData.DEAddValue(DETAG_ESERVICE, service->servicePath);
        } else {
            m_componentData.DEAddValue(DETAG_NOSERVICE, "");
        }
    }

    m_componentData.DEComplete();
}

//----------------------------------------------------------


void SNCEndpoint::linkCloseCleanup()
{
    SNC_SERVICE_INFO *service = m_serviceInfo;

    for (int i = 0; i < SNC_MAX_SERVICESPERCOMPONENT; i++, service++) {
        if (!service->inUse)
            continue;

        if (!service->enabled)
            continue;

        if (service->local)
            service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;
        else
            service->state = SNC_REMOTE_SERVICE_STATE_LOOK;
    }
}

bool SNCEndpoint::SNCConnect()
{
    int	returnValue;
    int size = SNC_MESSAGE_MAX * 3;
    int control;

    m_connectInProgress = false;
    m_connected = false;

    qint64 now = SNCUtils::clock();

    if (!SNCUtils::timerExpired(now, m_connWait, SNCENDPOINT_CONNWAIT))
        return false;				// not time yet

    m_connWait = now;

    if (!m_useTunnel) {
        // check if need to send beacon rather than check for SNCControls

        if (!m_beaconDelay) {
            m_hello->sendSNCHelloBeacon();						// request SNCControl hellos
            m_beaconDelay = true;
            return false;
        }

        m_beaconDelay = false;									// beacon delay has now expired

        for (control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
            if (m_controlName[control][0] == 0) {
                if (m_hello->findBestControl(&m_helloEntry)) {
                    m_controlIndex = control;
                    m_priorityMode = true;
                    break;
                }
            } else {
                if (m_hello->findComponent(&m_helloEntry, m_controlName[control], (char *)COMPTYPE_CONTROL)) {
                    m_controlIndex = control;
                    m_priorityMode = false;
                    break;
                }
            }
        }

        if (control == SNCENDPOINT_MAX_SNCCONTROLS) {
            // failed to find any usable SNCControl
            if (strlen(m_controlName[0]) == 0)
                updateState("Waiting for any SNCControl");
            else
                updateState(QString("Waiting for valid SNCControl"));

            return false;
        }
    }

    if (m_sock != NULL) {
        delete m_sock;
        delete m_SNCLink;
    }

    m_sock = new SNCSocket(this, 0, m_encryptLink);
    m_SNCLink = new SNCLink(TAG);

    returnValue = m_sock->sockCreate(0, SOCK_STREAM);

    if (returnValue == 0)
        return false;

    m_sock->sockSetConnectMsg(SNCENDPOINT_ONCONNECT_MESSAGE);
    m_sock->sockSetCloseMsg(SNCENDPOINT_ONCLOSE_MESSAGE);
    m_sock->sockSetReceiveMsg(SNCENDPOINT_ONRECEIVE_MESSAGE);
    m_sock->sockSetReceiveBufSize(size);

    if (m_useTunnel) {
        returnValue = m_sock->sockConnect(qPrintable(m_tunnelAddr), m_tunnelPort);
    } else {
        if (m_encryptLink)
            returnValue = m_sock->sockConnect(m_helloEntry.IPAddr, SNC_SOCKET_LOCAL_ENCRYPT);
        else
            returnValue = m_sock->sockConnect(m_helloEntry.IPAddr, SNC_SOCKET_LOCAL);
    }
    if (!returnValue) {
        SNCUtils::logDebug(TAG, "Connect attempt failed");
        return false;
    }
    QString str;
    m_connectInProgress = true;

    if (m_useTunnel)
        str = QString("Trying tunnel to %1:%2 %3").arg(m_tunnelAddr)
                .arg(m_tunnelPort).arg(m_encryptLink ? "using SSL" : "unencrypted");
    else
        str = QString("Trying connect to %1 %2").arg(m_helloEntry.IPAddr).arg(m_encryptLink ? "using SSL" : "unencrypted");
    updateState(str);
    return	true;
}

void SNCEndpoint::SNCClose()
{
    m_connected = false;
    m_connectInProgress = false;
    m_beaconDelay = false;

    if (m_sock != NULL) {
        delete m_sock;
        m_sock = NULL;
    }

    if (m_SNCLink != NULL) {
        delete m_SNCLink;
        m_SNCLink = NULL;
    }

    updateState("Connection closed");
}

bool SNCEndpoint::sendSNCMessage(int cmd, SNC_MESSAGE *SNCMessage, int len, int priority)
{
    if (!m_connected) {

        // can't send as not connected
        free(SNCMessage);
        return false;
    }

    m_SNCLink->send(cmd, len, priority, SNCMessage);
    m_SNCLink->trySending(m_sock);
    return true;
}


SNCHello *SNCEndpoint::getHelloObject()
{
    return m_hello;
}

//----------------------------------------------------------
//	These intermediate functions call appClient functions after pre-processing and checks


void SNCEndpoint::endpointConnected()
{
    SNC_SERVICE_INFO *service;

    m_connected = true;

    service = m_serviceInfo;

    for (int i = 0; i < SNC_MAX_SERVICESPERCOMPONENT; i++, service++) {
        if (!service->inUse)
            continue;

        if (service->local)
            service->state = SNC_LOCAL_SERVICE_STATE_INACTIVE;
        else
            service->state = SNC_REMOTE_SERVICE_STATE_LOOK;

        service->lastReceivedSeqNo = -1;
        service->nextSendSeqNo = 0;
        service->lastReceivedAck = 0;
        service->lastSendTime = SNCUtils::clock();
    }

    appClientConnected();
}


void SNCEndpoint::endpointClosed()
{
    m_connected = false;
    m_connectInProgress = false;
    m_beaconDelay = false;
    appClientClosed();
}


void SNCEndpoint::endpointHeartbeat(SNC_HEARTBEAT *heartbeat, int length)
{
    m_connected = true;
    appClientHeartbeat(heartbeat, length);
}


void SNCEndpoint::processMulticast(SNC_EHEAD *message, int length, int destPort)
{
    SNC_SERVICE_INFO *service;

    service = m_serviceInfo + destPort;

    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Nulticast data received on not in use port %1").arg(destPort));
        free(message);
        return;
    }

    if (service->serviceType != SERVICETYPE_MULTICAST) {
        SNCUtils::logWarn(TAG, QString("Multicast data received on port %1 that is not a multicast service port").arg(destPort));
        free(message);
        return;
    }

    if (service->lastReceivedSeqNo == message->seq)
        SNCUtils::logWarn(TAG, QString("Received duplicate multicast seq number %1  source %2  dest %3")
            .arg(message->seq)
            .arg(SNCUtils::displayUID(&message->sourceUID))
            .arg(SNCUtils::displayUID(&message->destUID)));

    service->lastReceivedSeqNo = message->seq;

    appClientReceiveMulticast(destPort, message, length);
}

void SNCEndpoint::processMulticastAck(SNC_EHEAD *message, int length, int destPort)
{
    SNC_SERVICE_INFO *service;

    service = m_serviceInfo + destPort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Nulticast data received on not in use port %1").arg(destPort));
        free(message);
        return;
    }
    if (service->serviceType != SERVICETYPE_MULTICAST) {
        SNCUtils::logWarn(TAG, QString("Multicast data received on port %1 that is not a multicast service port").arg(destPort));
        free(message);
        return;
    }

    service->lastReceivedAck = message->seq;

    appClientReceiveMulticastAck(destPort, message, length);
}


void SNCEndpoint::processE2E(SNC_EHEAD *message, int length, int destPort)
{
    SNC_SERVICE_INFO *service;

    service = m_serviceInfo + destPort;
    if (!service->inUse) {
        SNCUtils::logWarn(TAG, QString("Multicast data received on not in use port %1").arg(destPort));
        free(message);
        return;
    }
    if (service->serviceType != SERVICETYPE_E2E) {
        SNCUtils::logWarn(TAG, QString("E2E data received on port %1 that is not an E2E service port").arg(destPort));
        free(message);
        return;
    }

    appClientReceiveE2E(destPort, message, length);
}


//-------------------------------------------------------------------------------------------
//	SNCCFS API functions
//


void SNCEndpoint::CFSInit()
{
    int i;
    SNCCFS_CLIENT_EPINFO *EP;

    EP = cfsEPInfo;

    for (i = 0; i < SNC_MAX_SERVICESPERCOMPONENT; i++, EP++) {
        EP->inUse = false;								// this meansEP is not in use for SNCCFS
    }
}

/*!
    Allows a service port (returned from clientAddService or clientLoadServices) \a serviceEP to be
    designated as a Cloud File System endpoint. SNCEndpoint will trap message received from the
    SNCLink destined for a CFS endpoint and generate app client callbacks in response instead
    of passing up the raw messages.
*/

void SNCEndpoint::CFSAddEP(int serviceEP)
{
    SNCCFS_CLIENT_EPINFO *EP;
    int	i;

    if ((serviceEP < 0) || (serviceEP >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to add out of range EP %1 for SNCCFS processing").arg(serviceEP));
        return;
    }
    EP = cfsEPInfo + serviceEP;
    EP->dirInProgress = false;
    EP->inUse = true;
    for (i = 0; i < SNCCFS_MAX_CLIENT_FILES; i++) {
        EP->cfsFile[i].inUse = false;
        EP->cfsFile[i].clientHandle = i;
    }
}

/*!
    Removes the CFS designation from the service port \a serviceEP.
*/

void SNCEndpoint::CFSDeleteEP(int serviceEP)
{
    SNCCFS_CLIENT_EPINFO *EP;

    if ((serviceEP < 0) || (serviceEP >= SNC_MAX_SERVICESPERCOMPONENT)) {
        SNCUtils::logWarn(TAG, QString("Tried to delete out of range EP %1 for SNCCFS processing").arg(serviceEP));
        return;
    }
    EP = cfsEPInfo + serviceEP;
    EP->inUse = false;
}

/*!
    CFSDir can be called by the client app to request a directory of files from the SNCCFS associated with
    service port \a serviceEP. Only one request can be outstanding at a time. The function returns true if
    the request was sent, false if an error occurred and indicates that the request was not sent. If the
    function returns true, a call to CFSDirResponse will occur, either when a response is received or a timeout occurs.
*/

bool SNCEndpoint::CFSDir(int serviceEP, int cfsDirParam)
{
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    if (cfsEPInfo[serviceEP].dirInProgress) {
        return false;
    }
    requestE2E = CFSBuildRequest(serviceEP, 0);
    if (requestE2E == NULL) {
        return false;
    }
    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);			// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_DIR_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(cfsDirParam, requestHdr->cfsParam);
    sendSNCMessage(SNCMSG_E2E, (SNC_MESSAGE *)requestE2E, sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER), SNCCFS_E2E_PRIORITY);
    cfsEPInfo[serviceEP].dirReqTime = SNCUtils::clock();
    cfsEPInfo[serviceEP].dirInProgress = true;
    return true;
}

int SNCEndpoint::CFSOpenStructuredFile(int serviceEP, QString filePath)
{
    return CFSOpen(serviceEP, filePath, SNCCFS_TYPE_STRUCTURED_FILE, 0);
}

int SNCEndpoint::CFSOpenRawFile(int serviceEP, QString filePath, int blockSize)
{
    // max blockSize is legacy
    if (blockSize < 1 || blockSize > 0xffff) {
        SNCUtils::logError(TAG, QString("CFSOpenRawFile: Invalid blockSize %1").arg(blockSize));
        return -1;
    }

    return CFSOpen(serviceEP, filePath, SNCCFS_TYPE_RAW_FILE, blockSize);
}

int SNCEndpoint::CFSOpen(int serviceEP, QString filePath, int cfsMode, int blockSize)
{
    int handle;

    // Find a free stream slot
    for (handle = 0; handle < SNCCFS_MAX_CLIENT_FILES; handle++) {
        if (!cfsEPInfo[serviceEP].cfsFile[handle].inUse)
            break;
    }

    if (handle == SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logError(TAG, QString("Too many files open"));
        return -1;
    }

    SNC_EHEAD *requestE2E = CFSBuildRequest(serviceEP, filePath.size() + 1);

    if (requestE2E == NULL) {
        SNCUtils::logError(TAG, QString("CFSOpen on unavailable service %1 port %2").arg(filePath).arg(serviceEP));
        return -1;
    }

    SNC_CFS_FILE *scf = cfsEPInfo[serviceEP].cfsFile + handle;

    scf->inUse = true;
    scf->open = false;
    scf->readInProgress = false;
    scf->writeInProgress = false;
    scf->queryInProgress = false;
    scf->fetchQueryInProgress = false;
    scf->cancelQueryInProgress = false;
    scf->closeInProgress = false;
    scf->structured = filePath.endsWith(SNC_RECORD_SRF_RECORD_DOTEXT);

    SNC_CFSHEADER *requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);

    SNCUtils::convertIntToUC2(SNCCFS_TYPE_OPEN_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(cfsMode, requestHdr->cfsParam);
    SNCUtils::convertIntToUC4(blockSize, requestHdr->cfsIndex);

    char *pData = reinterpret_cast<char *>(requestHdr + 1);

    strcpy(pData, qPrintable(filePath));

    int totalLength = sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER) + (int)strlen(pData) + 1;

    sendSNCMessage(SNCMSG_E2E, (SNC_MESSAGE *)requestE2E, totalLength, SNCCFS_E2E_PRIORITY);

    scf->openReqTime = SNCUtils::clock();
    scf->openInProgress = true;

    return handle;
}

/*!
    CFSClose can be called to close an open file with handle \a handle on service port \a serviceEP.
    The function will return true if the close was issued and there will be a subsequent call to
    CFSClose() Response when a response is received or a timeout occurs. Returning false means
    that an error occurred, the close was not sent and there will not be a call to CFSCloseResponse().
    The client app in this case should assume that the file is closed.
*/

bool SNCEndpoint::CFSClose(int serviceEP, int handle)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    EP = cfsEPInfo + serviceEP;
    if (!EP->inUse) {
        SNCUtils::logWarn(TAG, QString("CFSClose attempted on not in use port %1").arg(serviceEP));
        return false;												// the endpoint isn't a SNCCFS one!
    }

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("CFSClose attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    scf = EP->cfsFile + handle;
    if (!scf->inUse || !scf->open) {
        SNCUtils::logWarn(TAG, QString("CFSClose attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }

    if (scf->closeInProgress) {
        SNCUtils::logWarn(TAG, QString("CFSClose attempted on already closing handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    requestE2E = CFSBuildRequest(serviceEP, 0);
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSClose attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_CLOSE_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
    sendSNCMessage(SNCMSG_E2E,
        (SNC_MESSAGE *)requestE2E,
        sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER),
        SNCCFS_E2E_PRIORITY);
    scf->closeReqTime = SNCUtils::clock();
    scf->closeInProgress = true;
    return true;
}

/*!
    CFSReadAtIndex can be called to read a record or block(s) starting at record or
    block \a index from the file associated with \a handle on service port \a serviceEP. \a blockCount
    can be used in raw mode to read more than one block. blockCount can be between 1 and 65535.
    This parameter is ignored in structured mode.

    The function returns true if the read request was issued and a call to CFSReadAtIndexResponse()
    will be made or false if the read was not issued and there will not be a subsequent call to CFSReadAtIndexResponse().
*/

bool SNCEndpoint::CFSReadAtIndex(int serviceEP, int handle, unsigned int index, int blockCount)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    EP = cfsEPInfo + serviceEP;
    if (!EP->inUse) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtIndex attempted on not in use port %1").arg(serviceEP));
        return false;													// the endpoint isn't a SNCCFS one!
    }

    if ((handle < 0) || (handle >= SNCCFS_MAX_CLIENT_FILES)) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtIndex attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    scf = EP->cfsFile + handle;
    if (!scf->inUse || !scf->open) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtIndex attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    requestE2E = CFSBuildRequest(serviceEP, 0);
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtIndex attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_READ_INDEX_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
    SNCUtils::convertIntToUC4(index, requestHdr->cfsIndex);
    SNCUtils::convertIntToUC2(blockCount, requestHdr->cfsParam);		// number of blocks to read if flat
    sendSNCMessage(SNCMSG_E2E,
        (SNC_MESSAGE *)requestE2E,
        sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER),
        SNCCFS_E2E_PRIORITY);
    scf->readReqTime = SNCUtils::clock();
    scf->readInProgress = true;
    return true;
}

/*!
    CFSReadAtTime can be called to read a record \a timeOffset from the file associated
    with \a handle on service port \a serviceEP.

    The function returns true if the read request was issued and a call to CFSReadAtIndexResponse()
    will be made or false if the read was not issued and there will not be a subsequent call to CFSReadAtIndexResponse().
*/

bool SNCEndpoint::CFSReadAtTime(int serviceEP, int handle, unsigned int timeOffset, int intervalOrCount, bool isInterval)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    EP = cfsEPInfo + serviceEP;
    if (!EP->inUse) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtTime attempted on not in use port %1").arg(serviceEP));
        return false;													// the endpoint isn't a SNCCFS one!
    }

    if ((handle < 0) || (handle >= SNCCFS_MAX_CLIENT_FILES)) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtTime attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    scf = EP->cfsFile + handle;
    if (!scf->inUse || !scf->open) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtTime attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    requestE2E = CFSBuildRequest(serviceEP, 0);
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSReadAtTime attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(isInterval ? SNCCFS_TYPE_READ_TIME_INTERVAL_REQ : SNCCFS_TYPE_READ_TIME_COUNT_RES, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
    SNCUtils::convertIntToUC4(timeOffset, requestHdr->cfsIndex);
    SNCUtils::convertIntToUC2(intervalOrCount, requestHdr->cfsParam);
    sendSNCMessage(SNCMSG_E2E,
        (SNC_MESSAGE *)requestE2E,
        sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER),
        SNCCFS_E2E_PRIORITY);
    scf->readReqTime = SNCUtils::clock();
    scf->readInProgress = true;
    return true;
}

/*!
    CFSWriteAtIndex can be called to write a record or block(s) starting at record or
    block \a index to the file associated with \a handle on service port \a serviceEP. \a blockCount
    can be used in raw mode to write more than one block. blockCount can be between 1 and 65535.
    This parameter is ignored in structured mode.

    The function returns true if the write request was issued and a call to CFSWriteAtIndexResponse()
    will be made or false if the write was not issued and there will not be a subsequent call to CFSWriteAtIndexResponse().
*/

bool SNCEndpoint::CFSWriteAtIndex(int serviceEP, int handle, unsigned int index, unsigned char *fileData, int length)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    EP = cfsEPInfo + serviceEP;
    if (!EP->inUse) {
        SNCUtils::logWarn(TAG, QString("CFSWriteAtIndex attempted on not in use port %1").arg(serviceEP));
        return false;													// the endpoint isn't a SNCCFS one!
    }

    if ((handle < 0) || (handle >= SNCCFS_MAX_CLIENT_FILES)) {
        SNCUtils::logWarn(TAG, QString("CFSWriteAtIndex attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    scf = EP->cfsFile + handle;
    if (!scf->inUse || !scf->open) {
        SNCUtils::logWarn(TAG, QString("CFSWriteAtIndex attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }
    requestE2E = CFSBuildRequest(serviceEP, length);
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSWriteAtIndex attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
        return false;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_WRITE_INDEX_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
    SNCUtils::convertIntToUC4(index, requestHdr->cfsIndex);
    memcpy((unsigned char *)(requestHdr + 1), fileData, length);
    sendSNCMessage(SNCMSG_E2E,
        (SNC_MESSAGE *)requestE2E,
        sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER) + length,
        SNCCFS_E2E_PRIORITY);
    scf->writeReqTime = SNCUtils::clock();
    scf->writeInProgress = true;
    return true;
}

/*!
    CFSSendDatagram can be called to send arbitrary data to the server.
*/

bool SNCEndpoint::CFSSendDatagram(int serviceEP, const QByteArray& data)
{
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    EP = cfsEPInfo + serviceEP;
    if (!EP->inUse) {
        SNCUtils::logWarn(TAG, QString("CFSSendDatagram attempted on not in use port %1").arg(serviceEP));
        return false;													// the endpoint isn't a SNCCFS one!
    }

    requestE2E = CFSBuildRequest(serviceEP, data.length());
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSSendDatagram attempted on unavailable service port %2").arg(serviceEP));
        return false;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_DATAGRAM, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(0, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(0, requestHdr->cfsStoreHandle);
    SNCUtils::convertIntToUC4(0, requestHdr->cfsIndex);
    memcpy((unsigned char *)(requestHdr + 1), data.data(), data.length());
    sendSNCMessage(SNCMSG_E2E,
        (SNC_MESSAGE *)requestE2E,
        sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER) + data.length(),
        SNCCFS_E2E_PRIORITY);
    return true;
}


SNC_EHEAD *SNCEndpoint::CFSBuildRequest(int serviceEP, int length)
{
    if (!clientIsServiceActive(serviceEP))
        return NULL;										// can't send on port that isn't running!

    SNC_UID uid = m_componentData.getMyUID();

    SNC_EHEAD *requestE2E = SNCUtils::createEHEAD(&uid,
                    serviceEP,
                    clientGetRemoteServiceUID(serviceEP),
                    clientGetServiceDestPort(serviceEP),
                    0,
                    sizeof(SNC_CFSHEADER) + length);

    if (requestE2E) {
        SNC_CFSHEADER *requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E + 1);
        memset(requestHdr, 0, sizeof(SNC_CFSHEADER));
        SNCUtils::convertIntToUC4(length, requestHdr->cfsLength);
    }

    return requestE2E;
}


void SNCEndpoint::CFSBackground()
{
    int i, j;
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_CFS_FILE *scf;

    qint64 now = SNCUtils::clock();

    EP = cfsEPInfo;
    for (i = 0; i < SNC_MAX_SERVICESPERCOMPONENT; i++, EP++) {
        if (EP->inUse) {
            if (EP->dirInProgress) {
                if (SNCUtils::timerExpired(now, EP->dirReqTime, SNCCFS_DIRREQ_TIMEOUT)) {
                    CFSDirResponse(i, SNCCFS_ERROR_REQUEST_TIMEOUT, QStringList());
                    EP->dirInProgress = false;
                    return;
                }
            }
            scf = EP->cfsFile;
            for (j = 0; j < SNCCFS_MAX_CLIENT_FILES; j++, scf++) {
                if (!scf->inUse)
                    continue;
                if (scf->openInProgress) {					// process open timeout
                    if (SNCUtils::timerExpired(now, scf->openReqTime, SNCCFS_OPENREQ_TIMEOUT)) {
                        SNCUtils::logDebug(TAG, QString("Timed out open request on port %1 slot %2").arg(i).arg(j));
                        scf->inUse = false;					// close it down
                        CFSOpenResponse(i, SNCCFS_ERROR_REQUEST_TIMEOUT, j, 0);	// tell client
                    }
                }
                if (scf->open) {
                    if (SNCUtils::timerExpired(now, scf->lastKeepAliveSent, SNCCFS_KEEPALIVE_INTERVAL))
                        CFSSendKeepAlive(i, scf);

                    if (SNCUtils::timerExpired(now, scf->lastKeepAliveReceived, SNCCFS_KEEPALIVE_TIMEOUT))
                        CFSTimeoutKeepAlive(i, scf);
                }
                if (scf->readInProgress) {
                    if (SNCUtils::timerExpired(now, scf->readReqTime, SNCCFS_READREQ_TIMEOUT)) {
                        SNCUtils::logDebug(TAG, QString("Timed out read request on port %1 slot %2").arg(i).arg(j));
                        CFSReadAtIndexResponse(i, j, 0, SNCCFS_ERROR_REQUEST_TIMEOUT, NULL, 0);	// tell client
                        scf->readInProgress = false;
                    }
                }
                if (scf->writeInProgress) {
                    if (SNCUtils::timerExpired(now, scf->writeReqTime, SNCCFS_WRITEREQ_TIMEOUT)) {
                        SNCUtils::logDebug(TAG, QString("Timed out write request on port %1 slot %2").arg(i).arg(j));
                        CFSWriteAtIndexResponse(i, j, 0, SNCCFS_ERROR_REQUEST_TIMEOUT);	// tell client
                        scf->writeInProgress = false;
                    }
                }
                if (scf->closeInProgress) {
                    if (SNCUtils::timerExpired(now, scf->closeReqTime, SNCCFS_CLOSEREQ_TIMEOUT)) {
                        SNCUtils::logDebug(TAG, QString("Timed out close request on port %1 slot %2").arg(i).arg(j));
                        scf->inUse = false;					// close it down
                        CFSCloseResponse(i, SNCCFS_ERROR_REQUEST_TIMEOUT, j);	// tell client
                    }
                }
            }
        }
    }
}


bool SNCEndpoint::CFSProcessMessage(SNC_EHEAD *pE2E, int nLen, int dstPort)
{
    SNCCFS_CLIENT_EPINFO *EP;
    SNC_CFSHEADER	*cfsHdr;

    EP = cfsEPInfo + dstPort;
    if (!EP->inUse)
        return false;										// false indicates message was not trapped as not a SNCCFS port

    if (nLen < (int)sizeof(SNC_CFSHEADER)) {
        SNCUtils::logWarn(TAG, QString("SNCCFS message too short (%1) on port %2").arg(nLen).arg(dstPort));
        free(pE2E);
        return true;										// don't process any further
    }

    cfsHdr = reinterpret_cast<SNC_CFSHEADER *>(pE2E + 1);

    unsigned int cfsType = SNCUtils::convertUC2ToUInt(cfsHdr->cfsType);

    nLen -= sizeof(SNC_CFSHEADER);

    if (nLen != SNCUtils::convertUC4ToInt(cfsHdr->cfsLength)) {
        SNCUtils::logWarn(TAG, QString("SNCCFS message mismatch %1 %2 on port %3").arg(nLen).arg(SNCUtils::convertUC2ToUInt(cfsHdr->cfsLength)).arg(dstPort));
        free(pE2E);
        return true;
    }

    //	Have trapped a SNCCFS message

    switch (cfsType) {
        case SNCCFS_TYPE_DIR_RES:
            CFSProcessDirResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_OPEN_RES:
            CFSProcessOpenResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_CLOSE_RES:
            CFSProcessCloseResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_KEEPALIVE_RES:
            CFSProcessKeepAliveResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_READ_INDEX_RES:
            CFSProcessReadAtIndexResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_READ_TIME_INTERVAL_RES:
        case SNCCFS_TYPE_READ_TIME_COUNT_RES:
            CFSProcessReadAtTimeResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_WRITE_INDEX_RES:
            CFSProcessWriteAtIndexResponse(cfsHdr, dstPort);
            break;

        case SNCCFS_TYPE_DATAGRAM:
            CFSProcessReceiveDatagram(cfsHdr, dstPort);
            break;

        default:
            SNCUtils::logWarn(TAG, QString("Unrecognized SNCCFS message %1 on port %2").arg(SNCUtils::convertUC2ToUInt(cfsHdr->cfsType)).arg(dstPort));
            break;
    }
    free (pE2E);
    return true;											// indicates message was trapped
}


void SNCEndpoint::CFSProcessDirResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    char	*pData;
    QStringList	filePaths;

    pData = reinterpret_cast<char *>(cfsHdr + 1);			// pointer to strings
    filePaths = QString(pData).split(SNCCFS_FILENAME_SEP);		// break up the file paths

    cfsEPInfo[dstPort].dirInProgress = false;				// dir process complete
    CFSDirResponse(dstPort, SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam), filePaths);
}

void SNCEndpoint::CFSDirResponse(int, unsigned int, QStringList)
{
    SNCUtils::logDebug(TAG, QString("Default CFSDirResponse called"));
}


void SNCEndpoint::CFSProcessOpenResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;
    int responseCode;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("Open response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;								// get the file slot pointer
    responseCode = SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
    if (!scf->inUse) {
        SNCUtils::logWarn(TAG, QString("Open response with not in use handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    if (!scf->openInProgress) {
        SNCUtils::logWarn(TAG, QString("Open response with not in progress handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->openInProgress = false;
    if (responseCode == SNCCFS_SUCCESS) {
        scf->open = true;
        scf->storeHandle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsStoreHandle);// record SNCCFS store handle
        scf->lastKeepAliveSent = SNCUtils::clock();
        scf->lastKeepAliveReceived = scf->lastKeepAliveSent;
    } else {
        scf->inUse = false;									// close it down
    }
    CFSOpenResponse(dstPort, responseCode, handle, SNCUtils::convertUC4ToInt(cfsHdr->cfsIndex));
}

void SNCEndpoint::CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int, unsigned int)
{
    SNCUtils::logDebug(TAG, QString("Default CFSOpenResponse called %1 %2").arg(remoteServiceEP).arg(responseCode));
}

void SNCEndpoint::CFSProcessCloseResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;
    int responseCode;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("Close response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;							// get the stream slot pointer
    responseCode = SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
    if (!scf->inUse) {
        SNCUtils::logWarn(TAG, QString("Close response with not in use handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    if (!scf->open) {
        SNCUtils::logWarn(TAG, QString("Close response with not open handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->open = false;
    scf->inUse = false;										// close it down
    CFSCloseResponse(dstPort, responseCode, handle);
}


void SNCEndpoint::CFSCloseResponse(int serviceEP, unsigned int responseCode, int)
{
    SNCUtils::logDebug(TAG, QString("Default CFSCloseResponse called %1 %2").arg(serviceEP).arg(responseCode));
}

void SNCEndpoint::CFSProcessKeepAliveResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("Keep alive response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;							// get the stream slot pointer
    if (!scf->open) {
        SNCUtils::logWarn(TAG, QString("Keep alive response with not open handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->lastKeepAliveReceived = SNCUtils::clock();
#ifdef CFS_TRACE
    TRACE2("Got keep alive response on handle %d port %d", handle, dstPort);
#endif
}


void SNCEndpoint::CFSSendKeepAlive(int serviceEP, SNC_CFS_FILE *scf)
{
    SNC_EHEAD *requestE2E;
    SNC_CFSHEADER *requestHdr;

    scf->lastKeepAliveSent = SNCUtils::clock();
    requestE2E = CFSBuildRequest(serviceEP, 0);
    if (requestE2E == NULL) {
        SNCUtils::logWarn(TAG, QString("CFSSendKeepAlive on unavailable port %1 handle %2").arg(serviceEP).arg(scf->clientHandle));
        return;
    }

    requestHdr = reinterpret_cast<SNC_CFSHEADER *>(requestE2E+1);	// pointer to the new SNCCFS header
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_KEEPALIVE_REQ, requestHdr->cfsType);
    SNCUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
    SNCUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);

    sendSNCMessage(SNCMSG_E2E, (SNC_MESSAGE *)requestE2E, sizeof(SNC_EHEAD) + sizeof(SNC_CFSHEADER), SNCCFS_E2E_PRIORITY);
}

/*!
    \internal
*/

void SNCEndpoint::CFSTimeoutKeepAlive(int serviceEP, SNC_CFS_FILE *scf)
{
    scf->inUse = false;													// flag as not in use
    CFSKeepAliveTimeout(serviceEP, scf->clientHandle);
}

/*!
    This client app override is called is the CFS keep alive system has detected that the connection to
    the CFS server for the active file associated with handle on service port \a serviceEP has broken.
    The client app should handle this situation as though the file has been closed and the \a handle no longer valid.
*/

void SNCEndpoint::CFSKeepAliveTimeout(int serviceEP, int handle)
{
    SNCUtils::logDebug(TAG, QString("Default CFSKeepAliveTimeout called %1 %2").arg(serviceEP).arg(handle));
}

/*!
    \internal
*/

void SNCEndpoint::CFSProcessReadAtIndexResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;
    int responseCode;
    int length;
    unsigned char *fileData;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("ReadAtIndex response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;								// get the file slot pointer
    if (!scf->open) {
        SNCUtils::logWarn(TAG, QString("ReadAtIndex response with not open handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    if (!scf->readInProgress) {
        SNCUtils::logWarn(TAG, QString("ReadAtIndex response but no read in progress on handle %1 port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->readInProgress = false;
    responseCode = SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
#ifdef CFS_TRACE
    TRACE3("Got ReadAtIndex response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif
    if (responseCode == SNCCFS_SUCCESS) {
        length = SNCUtils::convertUC4ToInt(cfsHdr->cfsLength);
        fileData = reinterpret_cast<unsigned char *>(malloc(length));
        memcpy(fileData, cfsHdr + 1, length);				// make a copy of the record to give to the client
        CFSReadAtIndexResponse(dstPort, handle, SNCUtils::convertUC4ToInt(cfsHdr->cfsIndex), responseCode, fileData, length);
    } else {
        CFSReadAtIndexResponse(dstPort, handle,SNCUtils:: convertUC4ToInt(cfsHdr->cfsIndex), responseCode, NULL, 0);
    }
}


/*!
    This client app override is called when a read response for the file associated with handle on service
    port \a serviceEP has been received or else has timed out. \a responseCode indicates the result.
    SNCCFS_SUCCESS indicates that the request was successful and fileData contains the file
    data with length bytes. Any other value means that the read request failed.

    If the request was successful, the memory associated with fileData must be freed at some point by the client app.
*/

void SNCEndpoint::CFSReadAtIndexResponse(int serviceEP, int handle, unsigned int, unsigned int, unsigned char *fileData, int)
{
    SNCUtils::logDebug(TAG, QString("Default CFSReadAtIndexResponse called %1 %2").arg(serviceEP).arg(handle));
    if (fileData != NULL)
        free(fileData);
}

/*!
    \internal
*/

void SNCEndpoint::CFSProcessReadAtTimeResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;
    int responseCode;
    int length;
    unsigned char *fileData;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("ReadAtTime response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;								// get the file slot pointer
    if (!scf->open) {
        SNCUtils::logWarn(TAG, QString("ReadAtTime response with not open handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    if (!scf->readInProgress) {
        SNCUtils::logWarn(TAG, QString("ReadAtTime response but no read in progress on handle %1 port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->readInProgress = false;
    responseCode = SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
#ifdef CFS_TRACE
    TRACE3("Got ReadAtTime response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif
    if (responseCode == SNCCFS_SUCCESS) {
        length = SNCUtils::convertUC4ToInt(cfsHdr->cfsLength);
        fileData = reinterpret_cast<unsigned char *>(malloc(length));
        memcpy(fileData, cfsHdr + 1, length);				// make a copy of the record to give to the client
        CFSReadAtIndexResponse(dstPort, handle, SNCUtils::convertUC4ToInt(cfsHdr->cfsIndex), responseCode, fileData, length);
    } else {
        CFSReadAtTimeResponse(dstPort, handle,SNCUtils:: convertUC4ToInt(cfsHdr->cfsIndex), responseCode, NULL, 0);
    }
}

/*!
    This client app override is called when a read response for the file associated with handle on service
    port \a serviceEP has been received or else has timed out. \a responseCode indicates the result.
    SNCCFS_SUCCESS indicates that the request was successful and fileData contains the file
    data with length bytes. Any other value means that the read request failed.

    If the request was successful, the memory associated with fileData must be freed at some point by the client app.
*/

void SNCEndpoint::CFSReadAtTimeResponse(int serviceEP, int handle, unsigned int, unsigned int, unsigned char *fileData, int)
{
    SNCUtils::logDebug(TAG, QString("Default CFSReadAtTimeResponse called %1 %2").arg(serviceEP).arg(handle));
    if (fileData != NULL)
        free(fileData);
}

/*!
    \internal
*/

void SNCEndpoint::CFSProcessWriteAtIndexResponse(SNC_CFSHEADER *cfsHdr, int dstPort)
{
    SNC_CFS_FILE *scf;
    SNCCFS_CLIENT_EPINFO *EP;
    int handle;
    int responseCode;

    EP = cfsEPInfo + dstPort;
    handle = SNCUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

    if (handle >= SNCCFS_MAX_CLIENT_FILES) {
        SNCUtils::logWarn(TAG, QString("WriteAtIndex response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    scf = EP->cfsFile + handle;							// get the stream slot pointer
    if (!scf->open) {
        SNCUtils::logWarn(TAG, QString("WriteAtIndex response with not open handle %1 on port %2").arg(handle).arg(dstPort));
        return;
    }
    if (!scf->writeInProgress) {
        SNCUtils::logWarn(TAG, QString("WriteAtIndex response but no read in progress on handle %1 port %2").arg(handle).arg(dstPort));
        return;
    }
    scf->writeInProgress = false;
    responseCode = SNCUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
#ifdef CFS_TRACE
    TRACE3("Got WriteAtIndex response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif
    CFSWriteAtIndexResponse(dstPort, handle, SNCUtils::convertUC4ToInt(cfsHdr->cfsIndex), responseCode);
}

/*!
    \internal
*/

void SNCEndpoint::CFSProcessReceiveDatagram(SNC_CFSHEADER *cfsHdr, int dstPort)
{
#ifdef CFS_TRACE
    SNCCFS_CLIENT_EPINFO *EP;

    EP = cfsEPInfo + dstPort;


    TRACE3("Got ReceiveDatagram on port %d", dstPort);
#endif
    CFSReceiveDatagram(dstPort, QByteArray((const char *)(cfsHdr + 1), SNCUtils::convertUC4ToInt(cfsHdr->cfsLength)));
}


/*!
    This client app override is called when a write response for the file associated with handle
    on service port \a serviceEP has been received or else has timed out. \a responseCode indicates the
    result. SNCCFS_SUCCESS indicates that the request was successful. Any other value means that the write request failed.
*/

void SNCEndpoint::CFSWriteAtIndexResponse(int serviceEP, int handle, unsigned int, unsigned int)
{
    SNCUtils::logDebug(TAG, QString("Default CFSWriteAtIndexResponse called %1 %2").arg(serviceEP).arg(handle));
}

/*!
    This client app override is called when a datagram is received
    on service port \a serviceEP has been received. \a data holds the received data.
*/

void SNCEndpoint::CFSReceiveDatagram(int serviceEP, QByteArray /* data */)
{
    SNCUtils::logDebug(TAG, QString("Default CFSReceiveDatagram called %1").arg(serviceEP));
}


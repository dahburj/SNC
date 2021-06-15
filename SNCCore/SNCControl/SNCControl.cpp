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
#include "SNCServer.h"
#include "ControlSetup.h"
#include "LinkStatus.h"

#include "RestartApp.h"
#include "About.h"
#include "DirectoryStatus.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif


SNCControl::SNCControl(QObject *parent, bool daemonMode) : MainConsole(parent, daemonMode)
{
    m_server = new SNCServer();

    SNCJSON::addConfigDialog(new ControlSetup());

    RestartApp *restartApp = new RestartApp();
    connect(restartApp, SIGNAL(restartApp()), this, SLOT(restartApp()), Qt::QueuedConnection);
    SNCJSON::addConfigDialog(restartApp);

    LinkStatus *linkStatus = new LinkStatus();
    SNCJSON::addInfoDialog(linkStatus);
    SNCJSON::addInfoDialog(new DirectoryStatus(m_server));
    SNCJSON::addInfoDialog(new About());

    connect(m_server, SIGNAL(updateSNCStatusBox(int, QStringList)), linkStatus, SLOT(updateSNCStatusBox(int, QStringList)));
    connect(m_server, SIGNAL(updateSNCDataBox(int, QStringList)), linkStatus, SLOT(updateSNCDataBox(int, QStringList)));
    connect(m_server, SIGNAL(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)),
        linkStatus, SLOT(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);
    connect(m_server, SIGNAL(serverE2EUpdate(qint64, unsigned, qint64, unsigned)),
        linkStatus, SLOT(serverE2EUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);

    m_server->resumeThread();

    startServices();
}

void SNCControl::appExit()
{
    m_server->exitThread();
}

void SNCControl::displayComponents()
{
    bool first = true;

    printf("\n\n%-16s %-16s %-16s %-15s\n", "Component", "Name", "UID", "IP address");
    printf("%-16s %-16s %-16s %-15s\n", "---------", "----", "----", "----------");

    SS_COMPONENT *component = m_server->m_components;

    for (int i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, component++) {
        if (!component->inUse)
            continue;

        first = false;

        SNCHELLO *hello = &(component->heartbeat.hello);

        printf("%-16s %-16s %-16s %-15s\n", hello->componentType, hello->appName,
                qPrintable(SNCUtils::displayUID(&hello->componentUID)),
                qPrintable(SNCUtils::displayIPAddr(hello->IPAddr)));
    }

    if (first)
        printf("\nNo active components\n");
}

void SNCControl::displayDirectory()
{
    bool first = true;

    DM_CONNECTEDCOMPONENT *connectedComponent = m_server->m_dirManager.m_directory;

    m_server->m_dirManager.m_lock.lock();

    for (int i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, connectedComponent++) {
        if (!connectedComponent->valid)
            continue;						// a not in use entry

        DM_COMPONENT *component = connectedComponent->componentDE;

        while (component != NULL) {
            if (!first)
                printf("------------------------");

            first = false;

            printf("\n\nApp name: %s\n", component->appName);
            printf("Comp type: %s\n", component->componentType);
            printf("UID: %s\n", component->UIDStr);
            printf("Sequence ID: %d\n", component->sequenceID);

            DM_SERVICE *service = component->services;

            for (int j = 0; j < component->serviceCount; j++, service++) {
                if (!service->valid)
                    continue;					// can this happen?

                if (service->serviceType == SERVICETYPE_NOSERVICE)
                    continue;

                printf("Service: %s, port %d, type %d\n", service->serviceName, service->port, service->serviceType);
            }

            component = component->next;
        }
    }

    m_server->m_dirManager.m_lock.unlock();

    if (first)
        printf("\n\nNo active directories\n");
}

void SNCControl::displayMulticast()
{
    bool first = true;

    QMutexLocker locker(&(m_server->m_multicastManager.m_lock));

    MM_MMAP *multicastMap = m_server->m_multicastManager.m_multicastMap;

    for (int i = 0; i < SNCSERVER_MAX_MMAPS; i++, multicastMap++) {
        if (!multicastMap->valid)
            continue;

        if (!first)
            printf("------------------------");

        first = false;

        printf("\n\nMap: Srv=%s, UID=%s, \n", multicastMap->serviceLookup.servicePath,
                    qPrintable(SNCUtils::displayUID(&multicastMap->sourceUID)));
        printf("     LPort=%d, RPort=%d\n",
                SNCUtils::convertUC2ToUInt(multicastMap->serviceLookup.localPort),
                SNCUtils::convertUC2ToUInt(multicastMap->serviceLookup.remotePort));

        MM_REGISTEREDCOMPONENT *registeredComponent = multicastMap->head;

        while (registeredComponent != NULL) {
            printf("          RC: UID=%s, port=%d, seq=%d, ack=%d\n",
                qPrintable(SNCUtils::displayUID(&registeredComponent->registeredUID)), registeredComponent->port,
                registeredComponent->sendSeq, registeredComponent->lastAckSeq);
            registeredComponent = registeredComponent->next;
        }
    }

    if (first)
        printf("\n\nNo active multicasts\n");
}

void SNCControl::showHelp()
{
    printf("\nOptions are:\n\n");
    printf("  C - Display Components\n");
    printf("  D - Display directory\n");
    printf("  M - Display multicast map table\n");
    printf("  X - Exit\n");
}

void SNCControl::processInput(char c)
{
    switch (c) {
    case 'C':
        displayComponents();
        break;

    case 'D':
        displayDirectory();
        break;

    case 'H':
        showHelp();
        break;

    case 'M':
        displayMulticast();
        break;

    default:
        break;
    }
}


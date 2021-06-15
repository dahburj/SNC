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

#include <qsettings.h>
#include <qstringlist.h>

#include "DirectoryStatus.h"
#include "SNCControl.h"
#include "SNCServer.h"

DirectoryStatus::DirectoryStatus(SNCServer *server)
    : Dialog (SNCCONTROL_DIRECTORYSTATUS_NAME, SNCCONTROL_DIRECTORYSTATUS_DESC)
{
    m_server = server;
}

void DirectoryStatus::getDialog(QJsonObject& newDialog)
{
    QStringList headers;
    QList<int> widths;
    QStringList data;

    headers << "App name" << "Component type" << "Unique ID" << "Sequence ID" << "Services";

    widths << 120 << 120 << 120 << 80 << 400;

    clearDialog();
    getDirectoryStatusTable(data);

    addVar(createInfoTableVar("Directory status", headers, widths, data));
    return dialog(newDialog, true);
}

void DirectoryStatus::getDirectoryStatusTable(QStringList& list)
{
    DM_CONNECTEDCOMPONENT *connectedComponent = m_server->m_dirManager.m_directory;
    m_server->m_dirManager.m_lock.lock();

    list.clear();

    for (int i = 0; i < SNC_MAX_CONNECTEDCOMPONENTS; i++, connectedComponent++) {
        if (!connectedComponent->valid)
            continue;						// a not in use entry

        DM_COMPONENT *component = connectedComponent->componentDE;

        while (component != NULL) {
            list.append(component->appName);
            list.append(component->componentType);
            list.append(component->UIDStr);
            list.append(QString::number(component->sequenceID));

            DM_SERVICE *service = component->services;
            QString serviceString;

            bool first = true;
            for (int j = 0; j < component->serviceCount; j++, service++) {
                if (!service->valid)
                    continue;					// can this happen?

                if (service->serviceType == SERVICETYPE_NOSERVICE)
                    continue;

                if (!first)
                    serviceString += ", ";
                first = false;

                serviceString += QString("[%1, %2, %3]").arg(service->serviceName)
                    .arg(service->port).arg((service->serviceType == SERVICETYPE_E2E) ? "E2E" : "Mcst");
            }
            list.append(serviceString);

            component = component->next;
        }
    }

    m_server->m_dirManager.m_lock.unlock();
}




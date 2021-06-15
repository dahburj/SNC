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

#include "LinkStatus.h"
#include "SNCControl.h"
#include "SNCServer.h"

LinkStatus::LinkStatus() : Dialog (SNCCONTROL_LINKSTATUS_NAME, SNCCONTROL_LINKSTATUS_DESC)
{
    m_mcastIn = m_mcastOut = m_mcastInRate = m_mcastOutRate = 0;
    m_e2eIn = m_e2eOut = m_e2eInRate = m_e2eOutRate = 0;
}

void LinkStatus::getDialog(QJsonObject& newDialog)
{
    QStringList headers;
    QList<int> widths;
    QStringList data;
    QString mcast;
    QString e2e;

    headers << "App name" << "Component type" << "Unique ID" << "IP Address" << "HB interval" << "Link type"
                        << "RX bytes" << "TX bytes" << "RX rate" << "TX rate";

    widths << 120 << 120 << 120 << 100 << 80 << 120 << 100 << 100 << 100 << 100;

    clearDialog();
    getLinkStatusTable(data);
    getMulticastCounts(mcast);
    getE2ECounts(e2e);

    addVar(createInfoTableVar("Link status", headers, widths, data));
    addVar(createGraphicsStringVar(mcast));
    addVar(createGraphicsStringVar(e2e));
    return dialog(newDialog, true);
}

void LinkStatus::updateSNCStatusBox(int index, QStringList update)
{
    updateLinkStatus(index, update, true);
}


void LinkStatus::updateSNCDataBox(int index, QStringList update)
{
    updateLinkStatus(index, update, false);
}

void LinkStatus::updateLinkStatus(int index, QStringList update, bool statusUpdate)
{
    QMutexLocker lock(&m_lock);

    while (index >= m_linkStatusInfo.count()) {
        m_linkStatusInfo.append(LinkStatusInfo());
    }
    if (statusUpdate)
        m_linkStatusInfo[index].status = update;
    else
        m_linkStatusInfo[index].data = update;
}

void LinkStatus::getLinkStatusTable(QStringList& list)
{
    QMutexLocker lock(&m_lock);

    list.clear();

    for (int i = 0; i < m_linkStatusInfo.count(); i++) {
        list.append(m_linkStatusInfo.at(i).status);
        list.append(m_linkStatusInfo.at(i).data);
    }
}

void LinkStatus::serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate)
{
    QMutexLocker lock(&m_lock);

    m_mcastIn = in;
    m_mcastOut = out;
    m_mcastInRate = inRate;
    m_mcastOutRate = outRate;
}

void LinkStatus::serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate)
{
    QMutexLocker lock(&m_lock);

    m_e2eIn = in;
    m_e2eOut = out;
    m_e2eInRate = inRate;
    m_e2eOutRate = outRate;
}

void LinkStatus::getMulticastCounts(QString& counts)
{
    QMutexLocker lock(&m_lock);

    counts = QString("Mcast: in=") + QString::number(m_mcastIn, 'g', 20) +
             QString(" out=") + QString::number(m_mcastOut, 'g', 20) +
             QString("  Mcast rate: in=") + QString::number(m_mcastInRate, 'g', 8) +
             QString(" out=") + QString::number(m_mcastOutRate, 'g', 8);
}

void LinkStatus::getE2ECounts(QString& counts)
{
    QMutexLocker lock(&m_lock);

    counts = QString("E2E: in=") + QString::number(m_e2eIn, 'g', 20) +
             QString(" out=") + QString::number(m_e2eOut, 'g', 20) +
             QString("  E2E rate: in=") + QString::number(m_e2eInRate, 'g', 8) +
             QString(" out=") + QString::number(m_e2eOutRate, 'g', 8);
}



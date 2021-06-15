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

#ifndef LINKSTATUS_H
#define LINKSTATUS_H

#include "SNCUtils.h"

#include "Dialog.h"


class LinkStatusInfo
{
public:
    LinkStatusInfo() { status << "" << "" << "" << "" << "" << ""; data << "" << "" << "" << ""; }

    QStringList status;
    QStringList data;
};

class LinkStatus : public Dialog
{
    Q_OBJECT

public:
    LinkStatus();

public slots:
    void updateSNCStatusBox(int index, QStringList update);
    void updateSNCDataBox(int index, QStringList update);
    void updateLinkStatus(int index, QStringList update, bool statusUpdate);
    void serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);
    void serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);

    void getDialog(QJsonObject& newConfig);
    virtual bool setDialog(const QJsonObject& /* json */) { return true; }

private:
    void getLinkStatusTable(QStringList& list);
    void getMulticastCounts(QString& counts);
    void getE2ECounts(QString& counts);

    QList<LinkStatusInfo> m_linkStatusInfo;

    qint64 m_mcastIn;
    qint64 m_mcastOut;
    int m_mcastInRate;
    int m_mcastOutRate;

    qint64 m_e2eIn;
    qint64 m_e2eOut;
    int m_e2eInRate;
    int m_e2eOutRate;

    QMutex m_lock;
};

#endif // LINKSTATUS_H

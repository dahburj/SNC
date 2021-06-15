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

#ifndef DIRECTORYSTATUS_H
#define DIRECTORYSTATUS_H

#include "SNCUtils.h"

#include "Dialog.h"


class DirectoryStatusInfo
{
public:
    DirectoryStatusInfo() { status << "" << "" << "" << "" << "" << ""; data << "" << "" << "" << ""; }

    QStringList status;
    QStringList data;
};

class SNCServer;

class DirectoryStatus : public Dialog
{
    Q_OBJECT

public:
    DirectoryStatus(SNCServer *server);

    void getDialog(QJsonObject& newConfig);
    virtual bool setDialog(const QJsonObject& /* json */) { return true; }

private:
    void getDirectoryStatusTable(QStringList& list);

    QList<DirectoryStatusInfo> m_directoryStatusInfo;

    SNCServer *m_server;

    QMutex m_lock;
};

#endif // DIRECTORYSTATUS_H

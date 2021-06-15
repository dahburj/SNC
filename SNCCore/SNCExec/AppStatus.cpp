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

#include "AppStatus.h"
#include "SNCExec.h"

AppStatus::AppStatus() : Dialog(SNCEXEC_APPSTATUS_NAME, SNCEXEC_APPSTATUS_DESC)
{
    for (int i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++)
        m_status.append(QStringList() << "Configure" << "no" << "" << "" << "");
}

void AppStatus::getDialog(QJsonObject& newDialog)
{
    QStringList headers;
    QList<int> widths;
    QStringList data;

    QMutexLocker lock(&m_lock);

    headers << "" << "In use" << "App name" << "UID" << "Execution state";

    widths << 80 << 60 << 140 << 140 << 140;

    clearDialog();

    for (int i = 0; i < m_status.count(); i++) {
        data.append(m_status.at(i));
    }

    addVar(createInfoTableVar("Link status", headers, widths, data, SNCEXEC_APPCONFIG_NAME, 0));
    return dialog(newDialog, true);
}

void AppStatus::updateExecStatus(int index, QStringList list)
{
    QMutexLocker lock(&m_lock);

    m_status[index] = list;
}


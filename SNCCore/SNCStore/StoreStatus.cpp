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

#include "StoreStatus.h"
#include "SNCStore.h"
#include "StoreClient.h"

StoreStatus::StoreStatus(StoreClient *storeClient) : Dialog(SNCSTORE_STATUS_NAME, SNCSTORE_STATUS_DESC)
{
    m_storeClient = storeClient;
}

void StoreStatus::getDialog(QJsonObject& newDialog)
{
    QStringList headers;
    QList<int> widths;
    QStringList data;
    StoreStream ss;

    headers << "" << "In use" << "Stream" << "Total recs" << "Total bytes" << "File recs" << "File bytes" << "Current file path";
    widths << 80 << 60 << 140 << 80 << 100 << 80 << 100 << 400;

    for (int i = 0; i < SNCSTORE_MAX_STREAMS; i++) {
        if (!StoreStream::streamIndexInUse(i) || !m_storeClient->getStoreStream(i, &ss)) {
            data.append("Configure");
            data.append("no");
            data.append("");
            data.append("");
            data.append("");
            data.append("");
            data.append("");
            data.append("");
        } else {
            data.append("Configure");
            data.append("yes");
            data.append(ss.streamName());
            data.append(QString::number(ss.rxTotalRecords()));
            data.append(QString::number(ss.rxTotalBytes()));
            data.append(QString::number(ss.rxRecords()));
            data.append(QString::number(ss.rxBytes()));
            data.append(ss.currentFile());
        }
    }

    clearDialog();
    addVar(createInfoTableVar("Store status", headers, widths, data, SNCSTORE_STREAMCONFIG_NAME, 0));
    return dialog(newDialog, true);
}

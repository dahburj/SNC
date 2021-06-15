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

#include "SNCStore.h"
#include "SNCUtils.h"
#include "StoreClient.h"
#include "CFSClient.h"

#include "ConfigurationDlg.h"
#include "StoreStatus.h"
#include "StoreStreamDlg.h"

SNCStore::SNCStore(QObject *parent) : MainConsole(parent)
{
    m_storeClient = new StoreClient(this);
    m_storeClient->resumeThread();

    m_CFSClient = new CFSClient(this);
    m_CFSClient->resumeThread();

    addStandardDialogs();

    SNCJSON::addConfigDialog(new ConfigurationDlg());

    SNCJSON::addInfoDialog(new StoreStatus(m_storeClient));

    StoreStreamDlg *ssd = new StoreStreamDlg();
    connect(ssd, SIGNAL(refreshStreamSource(int)), m_storeClient, SLOT(refreshStreamSource(int)));
    SNCJSON::addToDialogList(ssd);

    startServices();
}

SNCStore::~SNCStore()
{
}

void SNCStore::appExit()
{
    m_storeClient->exitThread();
    m_CFSClient->exitThread();
}

void SNCStore::showHelp()
{
    printf("\n\nOptions are:\n\n");
    printf("  C - Display record and byte counts\n");
    printf("  S - Display status\n");
    printf("  X - Exit\n");
}

void SNCStore::showStatus()
{
    printf("\n\nStore SyntroControl link status is: %s\n", qPrintable(m_storeClient->getLinkState()));
    printf("CFS SyntroControl link status is: %s\n", qPrintable(m_CFSClient->getLinkState()));
}

void SNCStore::showCounts()
{
    printf("\n\n%-15s %-12s %-16s %s", "Service", "RX records", "RX bytes", "Active File");
    printf("\n%-15s %-12s %-16s %s", "-------", "----------", "--------", "------------------------");

    StoreStream ss;

    for (int i = 0; i < SNCSTORE_MAX_STREAMS; i++) {
        if (!m_storeClient->getStoreStream(i, &ss))
            continue;

        printf("\n%-15s %-12s %-16s %s",
            qPrintable(ss.streamName()),
            qPrintable(QString::number(ss.rxTotalRecords())),
            qPrintable(QString::number(ss.rxTotalBytes())),
            qPrintable(ss.currentFile()));
    }
    printf("\n");
}

void SNCStore::processInput(char c)
{
    switch (c) {
    case 'C':
        showCounts();
        break;

    case 'S':
        showStatus();
        break;

    case 'H':
        showHelp();
        break;

    default:
        break;
    }
}


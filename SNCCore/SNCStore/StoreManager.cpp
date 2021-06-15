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

#include "SNCUtils.h"

#include "SNCStore.h"
#include "StoreManager.h"
#include "StoreBlocksRaw.h"
#include "StoreBlocksStructured.h"

#define TAG "StoreManager"

StoreManager::StoreManager(StoreStream *stream)
    : SNCThread(QString(TAG))
{
    m_stream = stream;
    m_store = NULL;
    m_timerID = -1;
}

StoreManager::~StoreManager()
{
    if (m_store) {
        delete m_store;
        m_store = NULL;
    }
}

void StoreManager::initThread()
{
    QMutexLocker lock(&m_workMutex);

    switch (m_stream->storeFormat()) {
    case structuredFileFormat:
        m_store = new StoreBlocksStructured(m_stream);
        break;

    case rawFileFormat:
        m_store = new StoreBlocksRaw(m_stream);
        break;

    default:
        SNCUtils::logWarn(TAG, QString("Unhandled storeFormat requested: %1").arg(m_stream->storeFormat()));
        break;
    }

    if (m_store)
        m_timerID = startTimer(SNC_CLOCKS_PER_SEC);
}

void StoreManager::finishThread()
{
    QMutexLocker lock(&m_workMutex);

    if (m_timerID > 0) {
        killTimer(m_timerID);
        m_timerID = -1;
    }

    if (m_store) {
        delete m_store;
        m_store = NULL;
    }

    m_stream = NULL;
}

void StoreManager::stop()
{
    QMutexLocker lock(&m_workMutex);

    if (m_store) {
        delete m_store;
        m_store = NULL;
    }

    m_stream = NULL;
}

void StoreManager::queueBlock(QByteArray block)
{
    if (m_store)
        m_store->queueBlock(block);

    if (m_stream)
        m_stream->updateStats(block.length());
}

void StoreManager::timerEvent(QTimerEvent *)
{
    QMutexLocker lock(&m_workMutex);

    if (m_store)
        m_store->processQueue();
}

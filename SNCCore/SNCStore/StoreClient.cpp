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
#include "StoreClient.h"
#include "StoreManager.h"

#include "SNCUtils.h"

#define TAG "StoreClient"

StoreClient::StoreClient(QObject *parent)
    : SNCEndpoint(SNCSTORE_BGND_INTERVAL, TAG),  m_parent(parent)
{
    for (int i = 0; i < SNCSTORE_MAX_STREAMS; i++) {
        m_sources[i] = NULL;
        m_storeManagers[i] = NULL;
    }
    loadSettings();
}

void StoreClient::loadSettings()
{
    QSettings *settings = SNCUtils::getSettings();

    if (!settings->contains(SNCSTORE_PARAMS_ROOT_DIRECTORY))
        settings->setValue(SNCSTORE_PARAMS_ROOT_DIRECTORY, "./");

    // Max age of files in days afterwhich they will be deleted
    // A value of 0 turns off the delete behavior
    if (!settings->contains(SNCSTORE_MAXAGE))
        settings->setValue(SNCSTORE_MAXAGE, 0);

    // The SNCStore component can save any type of stream.
    // Here you can list the SNC streams by name that it should look for.
    int	nSize = settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);
    settings->endArray();

    if (nSize == 0) {
        settings->beginWriteArray(SNCSTORE_PARAMS_STREAM_SOURCES);

        for (int index = 0; index < SNCSTORE_MAX_STREAMS; index++) {
            settings->setArrayIndex(index);
            if (index == 0) {
                settings->setValue(SNCSTORE_PARAMS_INUSE, true);
                settings->setValue(SNCSTORE_PARAMS_STREAM_SOURCE, "source/avmux");
            } else {
                settings->setValue(SNCSTORE_PARAMS_INUSE, false);
                settings->setValue(SNCSTORE_PARAMS_STREAM_SOURCE, "");
            }
            settings->setValue(SNCSTORE_PARAMS_FORMAT, SNC_RECORD_STORE_FORMAT_SRF);
            settings->setValue(SNCSTORE_PARAMS_ROTATION_POLICY, SNCSTORE_PARAMS_ROTATION_POLICY_TIME);
            settings->setValue(SNCSTORE_PARAMS_ROTATION_TIME_UNITS, SNCSTORE_PARAMS_ROTATION_TIME_UNITS_DAYS);
            settings->setValue(SNCSTORE_PARAMS_ROTATION_TIME, 1);
            settings->setValue(SNCSTORE_PARAMS_ROTATION_SIZE, 100);
            settings->setValue(SNCSTORE_PARAMS_DELETION_POLICY, SNCSTORE_PARAMS_DELETION_POLICY_COUNT);
            settings->setValue(SNCSTORE_PARAMS_DELETION_TIME_UNITS, SNCSTORE_PARAMS_DELETION_TIME_UNITS_DAYS);
            settings->setValue(SNCSTORE_PARAMS_DELETION_TIME, 2);
            settings->setValue(SNCSTORE_PARAMS_DELETION_COUNT, 5);
            settings->setValue(SNCSTORE_PARAMS_CREATE_SUBFOLDER, true);
        }
        settings->endArray();
    }

    delete settings;

    return;
}

bool StoreClient::getStoreStream(int index, StoreStream *ss)
{
    QMutexLocker lock(&m_lock);

    if (index < 0 || index >= SNCSTORE_MAX_STREAMS)
        return false;

    if (!ss || !m_sources[index])
        return false;

    *ss = *m_sources[index];

    return true;
}

void StoreClient::appClientInit()
{
    for (int index = 0; index < SNCSTORE_MAX_STREAMS; index++)
        refreshStreamSource(index);
}

void StoreClient::appClientExit()
{
    for (int i = 0; i < SNCSTORE_MAX_STREAMS; i++) {
        deleteStreamSource(i);
    }
}

void StoreClient::appClientReceiveMulticast(int servicePort, SNC_EHEAD *message, int len)
{
    int sourceIndex;

    sourceIndex = clientGetServiceData(servicePort);

    if ((sourceIndex >= SNCSTORE_MAX_STREAMS) || (sourceIndex < 0)) {
        SNCUtils::logWarn(TAG, QString("Multicast received to out of range port %1").arg(servicePort));
        free(message);
        return;
    }

    if (m_storeManagers[sourceIndex] != NULL) {
        m_storeManagers[sourceIndex]->queueBlock(QByteArray(reinterpret_cast<char *>(message + 1), len));
        clientSendMulticastAck(servicePort);
    }
    free(message);
}

void StoreClient::refreshStreamSource(int index)
{
    QMutexLocker locker(&m_lock);

    if (!StoreStream::streamIndexValid(index))
        return;

    bool inUse = StoreStream::streamIndexInUse(index);

    if (m_storeManagers[index] != NULL)
        deleteStreamSource(index);

    if (!inUse)
        return;

    m_sources[index] = new StoreStream();
    m_sources[index]->load(index);

    if (!m_sources[index]->folderWritable()) {
        SNCUtils::logError(TAG, QString("Folder is not writable: %1").arg(m_sources[index]->pathOnly()));
        delete m_sources[index];
        m_sources[index] = NULL;
    }

    m_storeManagers[index] = new StoreManager(m_sources[index]);

    m_sources[index]->port = clientAddService(m_sources[index]->streamName(), SERVICETYPE_MULTICAST, false);

    // record index in service entry
    clientSetServiceData(m_sources[index]->port, index);

    m_storeManagers[index]->resumeThread();
}

void StoreClient::deleteStreamSource(int index)
{
    if ((index < 0) || (index >= SNCSTORE_MAX_STREAMS)) {
        SNCUtils::logWarn(TAG, QString("Got deleteStreamSource with out of range index %1").arg(index));
        return;
    }

    if (m_storeManagers[index] == NULL)
        return;

    clientRemoveService(m_sources[index]->port);
    StoreManager *dm = m_storeManagers[index];
    dm->stop();

    // deletes automatically, but don't want to reference it again
    m_storeManagers[index] = NULL;

    dm->exitThread();

    delete m_sources[index];
    m_sources[index] = NULL;
}

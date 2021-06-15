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

#include "CFSThread.h"
#include "DirThread.h"
#include "SNCStore.h"
#include "CFSClient.h"
#include "StoreCFS.h"
#include "StoreCFSRaw.h"
#include "StoreCFSStructured.h"

//#define CFS_THREAD_TRACE
#define TAG "CFSThread"

CFSThread::CFSThread(CFSClient *parent)
    : SNCThread(QString(TAG)), m_parent(parent)
{
}

CFSThread::~CFSThread()
{
}

void CFSThread::initThread()
{
    QSettings *settings = SNCUtils::getSettings();

    m_storePath = settings->value(SNCSTORE_PARAMS_ROOT_DIRECTORY).toString();

    delete settings;

    CFSInit();

    m_DirThread = new DirThread(m_storePath);
    m_DirThread->resumeThread();

    m_timer = startTimer(SNC_CLOCKS_PER_SEC);
}

void CFSThread::finishThread()
{
    killTimer(m_timer);

    if (m_DirThread)
        m_DirThread->exitThread();
}

void CFSThread::timerEvent(QTimerEvent *)
{
    CFSBackground();
}

bool CFSThread::processMessage(SNCThreadMsg *msg)
{
    if (msg->message == SNCSTORE_CFS_MESSAGE)
        CFSProcessMessage(msg);

    return true;
}

void CFSThread::CFSInit()
{
    for (int i = 0; i < STORECFS_MAX_FILES; i++) {
        m_cfsState[i].inUse = false;
        m_cfsState[i].storeHandle = i;
        m_cfsState[i].agent = NULL;
    }
}

void CFSThread::CFSBackground()
{
    qint64 now = SNCUtils::clock();
    STORECFS_STATE *scs = m_cfsState;

    for (int i = 0; i < STORECFS_MAX_FILES; i++, scs++) {
        if (scs->inUse) {
            if (SNCUtils::timerExpired(now, scs->lastKeepalive, SNCCFS_KEEPALIVE_TIMEOUT)) {
#ifdef CFS_THREAD_TRACE
                SNCUtils::logDebug(TAG, QString("Timed out slot %1 connected to %2").arg(i).arg(SNCUtils::displayUID(&scs->clientUID)));
#endif
                scs->inUse = false;

                if (scs->agent) {
                    delete scs->agent;
                    scs->agent = NULL;
                }

                emit newStatus(scs->storeHandle, scs);
            }

            if (SNCUtils::timerExpired(now, scs->lastStatusEmit, STORECFS_STATUS_INTERVAL)) {
                emit newStatus(scs->storeHandle, scs);
                scs->lastStatusEmit = now;
            }
        }
    }
}

void CFSThread::CFSProcessMessage(SNCThreadMsg *msg)
{
    // this is the message itself
    SNC_EHEAD *message = reinterpret_cast<SNC_EHEAD *>(msg->ptrParam);

    // this is the message length (not including the SNC_EHEAD)
    int length = msg->intParam;

    if (length < (int)sizeof(SNC_CFSHEADER)) {
        SNCUtils::logWarn(TAG, QString("CFS message received is too short %1").arg(length));
        free(message);
        return;
    }

    SNC_CFSHEADER *cfsMsg = reinterpret_cast<SNC_CFSHEADER *>(message + 1);

    if (length != (int)(sizeof(SNC_CFSHEADER) + SNCUtils::convertUC4ToInt(cfsMsg->cfsLength))) {
        SNCUtils::logWarn(TAG, QString("CFS received message of length %1 but header said length was %2")
            .arg(length).arg(sizeof(SNC_CFSHEADER) + SNCUtils::convertUC4ToInt(cfsMsg->cfsLength)));

        free (message);
        return;
    }

    int cfsType = SNCUtils::convertUC2ToUInt(cfsMsg->cfsType);

    switch (cfsType) {
    case SNCCFS_TYPE_DIR_REQ:
    case SNCCFS_TYPE_OPEN_REQ:
        break;

    case SNCCFS_TYPE_CLOSE_REQ:
    case SNCCFS_TYPE_KEEPALIVE_REQ:
    case SNCCFS_TYPE_READ_INDEX_REQ:
    case SNCCFS_TYPE_WRITE_INDEX_REQ:
        if (!CFSSanityCheck(message, cfsMsg)) {
            free(message);
            return;
        }

        break;

    default:
        SNCUtils::logWarn(TAG, QString("CFS message received with unrecognized type %1").arg(cfsType));
        free(message);
        return;
    }

    // avoids issues with status display
    QMutexLocker locker(&m_lock);

    switch (cfsType) {
    case SNCCFS_TYPE_DIR_REQ:
        CFSDir(message, cfsMsg);
        break;
    case SNCCFS_TYPE_OPEN_REQ:
        CFSOpen(message, cfsMsg);
        break;
    case SNCCFS_TYPE_CLOSE_REQ:
        CFSClose(message, cfsMsg);
        break;
    case SNCCFS_TYPE_KEEPALIVE_REQ:
        CFSKeepAlive(message, cfsMsg);
        break;
    case SNCCFS_TYPE_READ_INDEX_REQ:
        CFSReadIndex(message, cfsMsg);
        break;
    case SNCCFS_TYPE_WRITE_INDEX_REQ:
        CFSWriteIndex(message, cfsMsg);
        break;
    }
}

void CFSThread::CFSDir(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    QString dirString;

    dirString = m_DirThread->getDirectory();

#ifdef CFS_THREAD_TRACE
    qDebug() << "dirString: " << dirString;
#endif

    // allow for zero at end of the directory
    SNC_EHEAD *responseE2E = CFSBuildResponse(ehead, cfsMsg, dirString.length() + 1);

    SNC_CFSHEADER *responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);

    char *data = reinterpret_cast<char *>(responseHdr + 1);
    strcpy(data, qPrintable(dirString));

    SNCUtils::convertIntToUC2(SNCCFS_TYPE_DIR_RES, responseHdr->cfsType);
    SNCUtils::convertIntToUC2(SNCCFS_SUCCESS, responseHdr->cfsParam);

    int totalLength = sizeof(SNC_CFSHEADER) + dirString.length() + 1;
    m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent directory to %s, length %d", qPrintable(SNCUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

    free(ehead);
}

void CFSThread::CFSOpen(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle;
    unsigned int recordCount = 0;
    char *data = NULL;
    int nameLength = 0;
    int cfsMode = SNCCFS_TYPE_STRUCTURED_FILE;

    STORECFS_STATE *scs = m_cfsState;

    for (handle = 0; handle < STORECFS_MAX_FILES; handle++, scs++) {
        if (!scs->inUse)
            break;
    }

    if (handle >= STORECFS_MAX_FILES) {
        SNCUtils::convertIntToUC2(SNCCFS_ERROR_MAX_STORE_FILES, cfsMsg->cfsParam);
        goto CFSOPEN_DONE;
    }

    nameLength = SNCUtils::convertUC4ToInt(cfsMsg->cfsLength);

    if (nameLength <= 2) {
        SNCUtils::convertIntToUC2(SNCCFS_ERROR_FILE_NOT_FOUND, cfsMsg->cfsParam);
        goto CFSOPEN_DONE;
    }

    data = reinterpret_cast<char *>(cfsMsg + 1);
    data[nameLength - 1] = 0;

    cfsMode = SNCUtils::convertUC2ToInt(cfsMsg->cfsParam);

    scs->agent = factory(cfsMode, m_parent, m_storePath + "/" + data);

    if (!scs->agent->cfsOpen(cfsMsg)) {
        delete scs->agent;
        scs->agent = NULL;
        goto CFSOPEN_DONE;
    }

    // save some client info
    scs->rxBytes = 0;
    scs->txBytes = 0;
    scs->lastKeepalive = SNCUtils::clock();
    scs->lastStatusEmit = scs->lastKeepalive;
    scs->clientUID = ehead->sourceUID;
    scs->clientPort = SNCUtils::convertUC2ToUInt(ehead->sourcePort);
    scs->clientHandle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsClientHandle);

    // use the cfsIndex field to return the number of records in the requested store
    // for raw files this is the number of read blocks
    // NA for databases since we don't know the table
    recordCount = scs->agent->cfsGetRecordCount();
    SNCUtils::convertIntToUC4(recordCount, cfsMsg->cfsIndex);
    qDebug() << "recordCount =" << recordCount;

    // give the client our handle for this connection
    SNCUtils::convertIntToUC2(scs->storeHandle, cfsMsg->cfsStoreHandle);

    SNCUtils::convertIntToUC2(SNCCFS_SUCCESS, cfsMsg->cfsParam);
    scs->inUse = true;

    emit newStatus(handle, scs);

CFSOPEN_DONE:

    SNCUtils::swapEHead(ehead);
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_OPEN_RES, cfsMsg->cfsType);
    SNCUtils::convertIntToUC4(0, cfsMsg->cfsLength);

    int totalLength = sizeof(SNC_CFSHEADER);
    m_parent->sendMessage(ehead, totalLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent open response to %s, response %d", qPrintable(SNCUtils::displayUID(&ehead->sourceUID)),
        SNCUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif
}

void CFSThread::CFSClose(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

    STORECFS_STATE *scs = m_cfsState + handle;

    SNCUtils::swapEHead(ehead);
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_CLOSE_RES, cfsMsg->cfsType);
    SNCUtils::convertIntToUC4(0, cfsMsg->cfsLength);
    SNCUtils::convertIntToUC2(SNCCFS_SUCCESS, cfsMsg->cfsParam);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent close response to %s, response %d",
        qPrintable(SNCUtils::displayUID(&ehead->sourceUID)),
        SNCUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif

    m_parent->sendMessage(ehead, sizeof(SNC_CFSHEADER));

    scs->inUse = false;

    if (scs->agent) {
        scs->agent->cfsClose();
        delete scs->agent;
        scs->agent = NULL;
    }

    emit newStatus(handle, scs);
}

void CFSThread::CFSKeepAlive(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

    STORECFS_STATE *scs = m_cfsState + handle;

    scs->lastKeepalive = SNCUtils::clock();

    SNCUtils::swapEHead(ehead);
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_KEEPALIVE_RES, cfsMsg->cfsType);
    SNCUtils::convertIntToUC4(0, cfsMsg->cfsLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent keep alive response to %s, response %d", qPrintable(SNCUtils::displayUID(&ehead->destUID)),
        SNCUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif

    m_parent->sendMessage(ehead, sizeof(SNC_CFSHEADER));
}

void CFSThread::CFSReadIndex(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

    STORECFS_STATE *scs = m_cfsState + handle;

    int requestedIndex = SNCUtils::convertUC4ToInt(cfsMsg->cfsIndex);

    scs->agent->cfsRead(ehead, cfsMsg, scs, requestedIndex);
}

void CFSThread::CFSWriteIndex(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

    STORECFS_STATE *scs = m_cfsState + handle;

    int requestedIndex = SNCUtils::convertUC4ToInt(cfsMsg->cfsIndex);

    scs->agent->cfsWrite(ehead, cfsMsg, scs, requestedIndex);
}


SNC_EHEAD *CFSThread::CFSBuildResponse(SNC_EHEAD *ehead, SNC_CFSHEADER *, int length)
{
    SNC_EHEAD *responseE2E = m_parent->clientBuildLocalE2EMessage(m_parent->m_CFSPort,
                                                                     &(ehead->sourceUID),
                                                                     SNCUtils::convertUC2ToUInt(ehead->sourcePort),
                                                                     sizeof(SNC_CFSHEADER) + length);

    SNC_CFSHEADER *responseHdr = (SNC_CFSHEADER *)(responseE2E + 1);

    memset(responseHdr, 0, sizeof(SNC_CFSHEADER));

    SNCUtils::convertIntToUC4(length, responseHdr->cfsLength);

    return responseE2E;
}

bool CFSThread::CFSSanityCheck(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg)
{
    int handle = SNCUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

    if (handle >= STORECFS_MAX_FILES) {
        SNCUtils::logWarn(TAG, QString("Request from %1 on out of range handle %2")
            .arg(SNCUtils::displayUID(&ehead->sourceUID)).arg(handle));

        return false;
    }

    STORECFS_STATE *scs = m_cfsState + handle;

    if (!scs->inUse || !scs->agent) {
        SNCUtils::logWarn(TAG, QString("Request from %1 on not in use handle %2")
            .arg(SNCUtils::displayUID(&ehead->sourceUID)).arg(handle));

        return false;
    }

    if (!SNCUtils::compareUID(&(scs->clientUID), &(ehead->sourceUID))) {
        SNCUtils::logWarn(TAG, QString("Request from %1 doesn't match UID on slot %2")
            .arg(SNCUtils::displayUID(&ehead->sourceUID))
            .arg(SNCUtils::displayUID(&scs->clientUID)));

        return false;
    }

    if (scs->clientPort != SNCUtils::convertUC2ToUInt(ehead->sourcePort)) {
        SNCUtils::logWarn(TAG, QString("Request from %1 doesn't match port on slot %2 port %3")
            .arg(SNCUtils::displayUID(&ehead->sourceUID))
            .arg(SNCUtils::displayUID(&scs->clientUID))
            .arg(SNCUtils::convertUC2ToUInt(ehead->sourcePort)));

        return false;
    }

    if (scs->clientHandle != SNCUtils::convertUC2ToInt(cfsMsg->cfsClientHandle)) {
        SNCUtils::logWarn(TAG, QString("Request from %1 doesn't match client handle on slot %2 port %3")
            .arg(SNCUtils::displayUID(&ehead->sourceUID))
            .arg(SNCUtils::displayUID(&scs->clientUID))
            .arg(SNCUtils::convertUC2ToUInt(ehead->sourcePort)));

        return false;
    }

    return true;
}

void CFSThread::CFSReturnError(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, int responseCode)
{
    SNCUtils::swapEHead(ehead);
    SNCUtils::convertIntToUC4(0, cfsMsg->cfsLength);
    SNCUtils::convertIntToUC2(responseCode, cfsMsg->cfsParam);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent error response to %s, response %d", qPrintable(SNCUtils::displayUID(&ehead->destUID)),
        SNCUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif

    m_parent->sendMessage(ehead, sizeof(SNC_CFSHEADER));
}

StoreCFS* CFSThread::factory(int cfsMode, CFSClient *client, QString filePath)
{
    switch (cfsMode) {
    case SNCCFS_TYPE_STRUCTURED_FILE:
        return new StoreCFSStructured(client, filePath);

    case SNCCFS_TYPE_RAW_FILE:
        return new StoreCFSRaw(client, filePath);
    }

    return NULL;
}

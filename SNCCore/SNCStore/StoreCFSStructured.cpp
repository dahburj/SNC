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

#include <qfile.h>

#include "CFSClient.h"
#include "StoreCFSStructured.h"

StoreCFSStructured::StoreCFSStructured(CFSClient *client, QString filePath)
    : StoreCFS(client, filePath)
{
}

StoreCFSStructured::~StoreCFSStructured()
{
}

bool StoreCFSStructured::cfsOpen(SNC_CFSHEADER *)
{
    m_indexPath = m_filePath;
    m_indexPath.truncate(m_indexPath.length() - (int)strlen(SNC_RECORD_SRF_RECORD_DOTEXT));
    m_indexPath += SNC_RECORD_SRF_INDEX_DOTEXT;

    return true;
}

void StoreCFSStructured::cfsRead(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex)
{
    qint64 rpos;
    SNC_STORE_RECORD_HEADER	cHead;
    int responseCode = SNCCFS_SUCCESS;
    int recordLength = 0;
    SNC_CFSHEADER *responseHdr = NULL;
    SNC_EHEAD *responseE2E = NULL;
    qint64 fileLength = 0;
    qint64 now = 0;
    char *data = NULL;

    QFile xf(m_indexPath);
    QFile rf(m_filePath);

    if (!xf.open(QIODevice::ReadOnly)) {
        responseCode = SNCCFS_ERROR_INDEX_FILE_NOT_FOUND;
        goto sendResponse;
    }

    fileLength = xf.size() / STORE_CFS_INDEX_ENTRY_SIZE;

    if (requestedIndex >= fileLength) {
        responseCode = SNCCFS_ERROR_INVALID_RECORD_INDEX;
        goto sendResponse;
    }

    xf.seek(requestedIndex * STORE_CFS_INDEX_ENTRY_SIZE);

    if ((int)xf.read((char *)(&rpos), sizeof(qint64)) != sizeof(qint64)) {
        responseCode = SNCCFS_ERROR_READING_INDEX_FILE;
        goto sendResponse;
    }

    xf.close();

    if (!rf.open(QIODevice::ReadOnly)) {
        responseCode = SNCCFS_ERROR_FILE_NOT_FOUND;
        goto sendResponse;
    }

    if (!rf.seek(rpos))	{
        responseCode = SNCCFS_ERROR_RECORD_SEEK;
        goto sendResponse;
    }

    if (rf.read((char *)&cHead, sizeof (SNC_STORE_RECORD_HEADER)) != sizeof (SNC_STORE_RECORD_HEADER)) {
        responseCode = SNCCFS_ERROR_RECORD_READ;
        goto sendResponse;
    }

    if (strncmp(SYNC_STRINGV0, cHead.sync, SYNC_LENGTH) != 0) {
        responseCode = SNCCFS_ERROR_INVALID_HEADER;
        goto sendResponse;
    }

    recordLength = SNCUtils::convertUC4ToInt(cHead.size);
    scs->txBytes += recordLength;

    now = SNCUtils::clock();

    if (SNCUtils::timerExpired(now, scs->lastStatusEmit, STORECFS_STATUS_INTERVAL)) {
        //emit newStatus(scs->storeHandle, scs);
        scs->lastStatusEmit = now;
    }

    responseE2E = cfsBuildResponse(ehead, recordLength);

    responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);

    data = reinterpret_cast<char *>(responseHdr + 1);

    if (rf.read(data, recordLength) != recordLength) {
        responseCode = SNCCFS_ERROR_RECORD_READ;
        goto sendResponse;
    }

    rf.close();

sendResponse:

    if (responseE2E == NULL) {
        responseE2E = cfsBuildResponse(ehead, 0);
        recordLength = 0;
        responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);
    }

    SNCUtils::convertIntToUC2(SNCCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
    SNCUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
    SNCUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
    memcpy(responseHdr->cfsClientHandle, cfsMsg->cfsClientHandle, sizeof(SNC_UC2));

    int totalLength = sizeof(SNC_CFSHEADER) + recordLength;
    m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent record to %s, length %d", qPrintable(SNCUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

    free(ehead);
}

void StoreCFSStructured::cfsWrite(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex)
{
    SNC_STORE_RECORD_HEADER cHeadV0;
    unsigned int responseCode = SNCCFS_SUCCESS;
    int length = 0;
    SNC_CFSHEADER *responseHdr = NULL;
    SNC_EHEAD *responseE2E = NULL;
    qint64 pos = 0;
    qint64 timestamp;
    SNC_RECORD_HEADER* record;
    int writtenLength;

    QFile xf(m_indexPath);
    QFile rf(m_filePath);

    // delete first if starting at zero
    if (requestedIndex == 0) {
        xf.remove();
        rf.remove();
    }

    if (!xf.open(QIODevice::Append)) {
        responseCode = SNCCFS_ERROR_INDEX_FILE_NOT_FOUND;
        goto sendResponse;
    }

    if (!rf.open(QIODevice::Append)) {
        responseCode = SNCCFS_ERROR_FILE_NOT_FOUND;
        xf.close();
        goto sendResponse;
    }

    length = SNCUtils::convertUC4ToInt(cfsMsg->cfsLength);
    scs->rxBytes += length;

    strncpy(cHeadV0.sync, SYNC_STRINGV0, SYNC_LENGTH);
    SNCUtils::convertIntToUC4(0, cHeadV0.data);
    SNCUtils::convertIntToUC4(length, cHeadV0.size);

    record = reinterpret_cast<SNC_RECORD_HEADER *>(cfsMsg + 1);
    pos = rf.pos();
    timestamp = SNCUtils::convertUC8ToInt64(record->timestamp);

    writtenLength = xf.write((char *)&pos, sizeof(qint64));
    writtenLength += xf.write((char *)&timestamp, sizeof(qint64));
    if (writtenLength != STORE_CFS_INDEX_ENTRY_SIZE) {
        xf.close();
        rf.close();
        responseCode = SNCCFS_ERROR_INDEX_WRITE;
        goto sendResponse;
    }

    xf.close();

    if (rf.write((char *)(&cHeadV0), sizeof(SNC_STORE_RECORD_HEADER)) != sizeof(SNC_STORE_RECORD_HEADER)) {
        rf.close();
        responseCode = SNCCFS_ERROR_WRITE;
        goto sendResponse;
    }

    if (rf.write(reinterpret_cast<char *>(cfsMsg + 1), length) != length) {
        rf.close();
        responseCode = SNCCFS_ERROR_WRITE;
        goto sendResponse;
    }

    rf.close();

sendResponse:

    responseE2E = cfsBuildResponse(ehead, 0);

    responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);

    SNCUtils::convertIntToUC2(SNCCFS_TYPE_WRITE_INDEX_RES, responseHdr->cfsType);
    SNCUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
    SNCUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);

    int totalLength = sizeof(SNC_CFSHEADER);
    m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Wrote record from %s, length %d", qPrintable(SNCUtils::displayUID(&ehead->sourceUID)), length);
#endif

    free(ehead);
}

unsigned int StoreCFSStructured::cfsGetRecordCount()
{
    QFile xf(m_indexPath);

    if (!xf.open(QIODevice::ReadOnly))
        return 0;

    unsigned int length = (unsigned int)(xf.size() / STORE_CFS_INDEX_ENTRY_SIZE);

    xf.close();

    return length;
}

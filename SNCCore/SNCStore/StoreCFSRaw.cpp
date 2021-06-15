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
#include "StoreCFSRaw.h"

StoreCFSRaw::StoreCFSRaw(CFSClient *client, QString filePath)
    : StoreCFS(client, filePath)
{
}

StoreCFSRaw::~StoreCFSRaw()
{
}

bool StoreCFSRaw::cfsOpen(SNC_CFSHEADER *cfsMsg)
{
    m_blockSize = SNCUtils::convertUC4ToInt(cfsMsg->cfsIndex);

    // we are going to use this as a divisor to get the number of blocks
    if (m_blockSize <= 0) {
        SNCUtils::convertIntToUC2(SNCCFS_ERROR_BAD_BLOCKSIZE_REQUEST, cfsMsg->cfsParam);
        return false;
    }

    return true;
}

void StoreCFSRaw::cfsRead(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex)
{
    int responseCode = SNCCFS_SUCCESS;
    int length = 0;
    SNC_CFSHEADER *responseHdr = NULL;
    SNC_EHEAD *responseE2E = NULL;
    qint64 bpos = 0;
    char *fileData = NULL;

    QFile ff(m_filePath);

    if (requestedIndex >= m_recordCount) {
        responseCode = SNCCFS_ERROR_INVALID_RECORD_INDEX;
        goto sendResponse;
    }

    if (!ff.open(QIODevice::ReadOnly)) {
        responseCode = SNCCFS_ERROR_FILE_NOT_FOUND;
        goto sendResponse;
    }

    // byte position in file
    bpos = (qint64)m_blockSize * (qint64)SNCUtils::convertUC4ToInt(cfsMsg->cfsIndex);

    if (!ff.seek(bpos))	{
        responseCode = SNCCFS_ERROR_RECORD_SEEK;
        ff.close();
        goto sendResponse;
    }

    length = m_blockSize * SNCUtils::convertUC2ToInt(cfsMsg->cfsParam);
    scs->txBytes += length;

    if (length == 0)
        goto sendResponse;

    if (length > (SNC_MESSAGE_MAX - (int)sizeof(SNC_EHEAD) - (int)sizeof(SNC_CFSHEADER))) {
        responseCode = SNCCFS_ERROR_TRANSFER_TOO_LONG;
        ff.close();
        goto sendResponse;
    }

    // adjust for EOF if necessary
    if (((qint64)length + bpos) > m_fileLength)
        length = m_fileLength - bpos;

    qDebug() << "Length: " << length;
    responseE2E = cfsBuildResponse(ehead, length);

    responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);

    fileData = reinterpret_cast<char *>(responseHdr + 1);

    if (ff.read(fileData, length) != length) {
        responseCode = SNCCFS_ERROR_READ;
        ff.close();
        goto sendResponse;
    }

    ff.close();

sendResponse:

    if (responseE2E == NULL) {
        responseE2E = cfsBuildResponse(ehead, 0);
        length = 0;
        responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);
    }

    SNCUtils::convertIntToUC2(SNCCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
    SNCUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
    SNCUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
    memcpy(responseHdr->cfsClientHandle, cfsMsg->cfsClientHandle, sizeof(SNC_UC2));

    int totalLength = sizeof(SNC_CFSHEADER) + length;
    m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
    TRACE2("Sent record to %s, length %d", qPrintable(SNCUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

    free(ehead);
}

void StoreCFSRaw::cfsWrite(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex)
{
    int length = 0;
    unsigned int responseCode = SNCCFS_SUCCESS;

    QFile ff(m_filePath);

    // delete first if starting at zero
    if (requestedIndex == 0)
        ff.remove();

    if (!ff.open(QIODevice::Append)) {
        responseCode = SNCCFS_ERROR_FILE_NOT_FOUND;
        ff.close();
        goto sendResponse;
    }

    length = SNCUtils::convertUC4ToInt(cfsMsg->cfsLength);

    scs->rxBytes += length;

    if (ff.write(reinterpret_cast<char *>(cfsMsg + 1), length) != length) {
        ff.close();
        responseCode = SNCCFS_ERROR_WRITE;
        goto sendResponse;
    }

    ff.close();

sendResponse:

    SNC_EHEAD *responseE2E = cfsBuildResponse(ehead, 0);

    SNC_CFSHEADER *responseHdr = reinterpret_cast<SNC_CFSHEADER *>(responseE2E + 1);

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

unsigned int StoreCFSRaw::cfsGetRecordCount()
{
    QFile xf(m_filePath);

    m_recordCount = 0;

    if (m_blockSize != 0) {
        m_fileLength = xf.size();
        qDebug() << "Filelength: " << m_fileLength;
        m_recordCount = (unsigned int)((m_fileLength + (qint64)m_blockSize - 1) / qint64(m_blockSize));
    }

    return m_recordCount;
}

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

#include "CFSClient.h"
#include "SNCCFSDefs.h"
#include "StoreCFS.h"


StoreCFS::StoreCFS(CFSClient *client, QString filePath)
    : m_parent(client), m_filePath(filePath)
{
}

StoreCFS::~StoreCFS()
{
}

bool StoreCFS::cfsOpen(SNC_CFSHEADER *)
{
    return true;
}

void StoreCFS::cfsClose()
{
}

void StoreCFS::cfsRead(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *, unsigned int)
{
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_READ_INDEX_RES, cfsMsg->cfsType);
    cfsReturnError(ehead, cfsMsg, SNCCFS_ERROR_INVALID_REQUEST_TYPE);
}

void StoreCFS::cfsWrite(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *, unsigned int)
{
    SNCUtils::convertIntToUC2(SNCCFS_TYPE_WRITE_INDEX_RES, cfsMsg->cfsType);
    cfsReturnError(ehead, cfsMsg, SNCCFS_ERROR_INVALID_REQUEST_TYPE);
}


unsigned int StoreCFS::cfsGetRecordCount()
{
    return 0;
}

SNC_EHEAD *StoreCFS::cfsBuildResponse(SNC_EHEAD *ehead, int length)
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

void StoreCFS::cfsReturnError(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, int responseCode)
{
    SNCUtils::swapEHead(ehead);
    SNCUtils::convertIntToUC4(0, cfsMsg->cfsLength);
    SNCUtils::convertIntToUC2(responseCode, cfsMsg->cfsParam);

#ifdef CFS_THREAD_TRACE
    SNCUtils::logDebug(TAG, QString("Sent error response to %1, response %2").arg(SNCUtils::displayUID(&ehead->destUID))
        .arg(SNCUtils::convertUC2ToInt(cfsMsg->cfsParam)));
#endif

    m_parent->sendMessage(ehead, sizeof(SNC_CFSHEADER));
}

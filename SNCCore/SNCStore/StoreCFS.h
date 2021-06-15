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

#ifndef STORECFS_H
#define STORECFS_H

#include "CFSThread.h"
#include "SNCDefs.h"

#define STORE_CFS_INDEX_ENTRY_SIZE  (2 * sizeof(qint64))

class CFSClient;

class StoreCFS
{
public:
    StoreCFS(CFSClient *client, QString filePath);
    virtual ~StoreCFS();

    virtual bool cfsOpen(SNC_CFSHEADER *cfsMsg);
    virtual void cfsClose();
    virtual void cfsRead(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex);
    virtual void cfsWrite(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, STORECFS_STATE *scs, unsigned int requestedIndex);

    virtual unsigned int cfsGetRecordCount();

    SNC_EHEAD *cfsBuildResponse(SNC_EHEAD *ehead, int length);

    void cfsReturnError(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, int responseCode);

protected:
    CFSClient *m_parent;
    QString m_filePath;
};

#endif // STORECFS_H

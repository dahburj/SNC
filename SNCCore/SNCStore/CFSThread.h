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

#ifndef CFSTHREAD_H
#define CFSTHREAD_H

#include <qlist.h>

#include "SNCDefs.h"
#include "SNCCFSDefs.h"
#include "SNCStore.h"
#include "SNCThread.h"

#define STORECFS_STATUS_INTERVAL        (SNC_CLOCKS_PER_SEC)// this is the min interval between status emits

#define STORECFS_MAX_FILES              1024                // max files open at one time

#define CFS_TYPE_STRUCTURED             1
#define CFS_TYPE_RAW                    2

//  The StoreCFS state information referenced by a stream handle

class StoreCFS;

typedef struct
{
public:
    bool inUse;                                             // true if this represents an open stream
    int	storeHandle;                                        // the index of this entry
    int clientHandle;                                       // the client's handle
    SNC_UID clientUID;                                      // the uid of the client app
    int clientPort;                                         // the client port
    qint64 lastKeepalive;                                   // last time a keepalive was received
    qint64 rxBytes;                                         // total bytes received for this file
    qint64 txBytes;                                         // total bytes sent for this file
    qint64 lastStatusEmit;                                  // the time that the last status was emitted
    StoreCFS *agent;
} STORECFS_STATE;

class CFSClient;
class DirThread;

class CFSThread : public SNCThread
{
    Q_OBJECT

public:
    CFSThread(CFSClient *parent);
    ~CFSThread();

    QMutex m_lock;
    STORECFS_STATE m_cfsState[STORECFS_MAX_FILES];          // the open file state cache

signals:
    void newStatus(int handle, STORECFS_STATE *CFSState);

protected:
    void initThread();
    bool processMessage(SNCThreadMsg *msg);
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    StoreCFS* factory(int cfsMode, CFSClient *client, QString filePath);

    CFSClient *m_parent;
    QString m_storePath;
    int m_timer;

    DirThread *m_DirThread;

    void CFSInit();
    void CFSBackground();

    void CFSProcessMessage(SNCThreadMsg *msg);

    void CFSDir(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSOpen(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSClose(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSKeepAlive(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSReadIndex(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSWriteIndex(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);

    SNC_EHEAD *CFSBuildResponse(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, int length);
    bool CFSSanityCheck(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg);
    void CFSReturnError(SNC_EHEAD *ehead, SNC_CFSHEADER *cfsMsg, int responseCode);
};

#endif // CFSTHREAD_H


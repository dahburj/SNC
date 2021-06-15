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

#ifndef _SNCCFSCLIENT_H_
#define _SNCCFSCLIENT_H_

#include "SNCEndpoint.h"

#include <qjsonobject.h>

#define	SNCCFSCLIENT_DIR_INTERVAL		(SNC_CLOCKS_PER_SEC * 4)	// directory refresh interval

//	CFS interface states

enum
{
    SNCCFSCLIENT_STATE_IDLE = 0,                        // no open file
    SNCCFSCLIENT_STATE_IXOPENING,                       // opening index file
    SNCCFSCLIENT_STATE_IXOPEN,                          // index file is open
    SNCCFSCLIENT_STATE_IXCLOSING,                       // infex file in process of being closed
    SNCCFSCLIENT_STATE_SFOPENING,                       // opening structured file
    SNCCFSCLIENT_STATE_SFOPEN,                          // structured file is open
    SNCCFSCLIENT_STATE_SFCLOSING                        // structured file in process of being closed
};

class CFSServerInfo
{
public:
    QString servicePath;									// the path of the service to be look up
    int port;												// the port assigned
    QStringList directory;									// the directory from this server
    bool waitingForDirectory;								// indicates request sent
};

class CFSSessionInfo
{
public:
    CFSSessionInfo(int sessionId);
    ~CFSSessionInfo();

    int m_sessionId;
    CFSServerInfo *m_activeCFS;								// the CFS in use
    bool m_isSensor;                                        // true if from a sensor
    QString m_activeFile;									// the name of the active file
    int m_handle;											// the file handle
    int m_state;											// the CFS interface state
    unsigned int m_blockSize;                               // block size for raw file transfer

    QString m_source;                                       // name of source
    QString m_dataFile;                                     // the data file part (.srf)
    QString m_indexFile;                                    // the index file path (.srx)
    unsigned int m_indexFileLength;                         // length of the index file
    unsigned int m_dataFileLength;                          // length of the data file
    unsigned int m_indexIndex;                              // where we are in the index file
    QList<qint64> m_index;                                  // index file timestamps indexed by record index
    QHash<qint64, int> m_indexDir;                          // index directory (timestamp -> record index) each second
    int m_recordIndex;                                      // where we are in the file

    qint64 m_ts;                                            // latest requested timestamp
    qint64 m_dayStart;                                      // epoch at start of day

    QImage m_frame;											// the next frame to be displayed
    QByteArray m_frameCompressed;							// the compressed version

    bool m_readOutstanding;									// if there's a CFS read outstanding

};

class SNCCFSClient : public SNCEndpoint
{
    Q_OBJECT

public:
    SNCCFSClient(qint64 backgroundInterval, const char* tag);
    ~SNCCFSClient();

public slots:
    void newCFSList();
    void getTimestamp(int sessionId, QString source, qint64 ts);
    void openSession(int sessionId);
    void closeSession(int sessionId);

signals:
    void newCFSState(int sessionId, QString state);			// emitted when state changes
    void newDirectory(QStringList directory);				// emitted when a new CFS directory is received
    void newAudio(int sessionId, QByteArray data, int rate, int channels, int size);
    void newImage(int sessionId, QImage frame, qint64 ts);  // emitted when a frame received
    void newJpeg(int sessionId, QByteArray jpeg, qint64 ts);  // emitted when a frame received
    void newSensorData(int sessionId, QJsonObject data, qint64 ts);  // emitted when sensor data received

protected:
    void CFSClientInit();
    void CFSClientBackground();
    void loadCFSStores(QString group, QString src);

    QList <CFSServerInfo *> m_serverInfo;					// server-specific info
    QStringList m_directory;								// the integrated directory

    void CFSDirResponse(int remoteServiceEP, unsigned int responseCode, QStringList filePaths) override;
    void CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int handle, unsigned int fileLength) override;
    void CFSCloseResponse(int remoteServiceEP, unsigned int responseCode, int handle) override;
    void CFSKeepAliveTimeout(int remoteServiceEP, int handle) override;
    void CFSReadAtIndexResponse(int remoteServiceEP, int handle, unsigned int recordIndex,
        unsigned int responseCode, unsigned char *fileData, int length) override;

private:
    qint64	m_lastDirReq;									// when the last directory request was obtained
    QString m_tag;
    QHash<int, CFSSessionInfo *> m_sessions;

    void setCFSStateIdle(CFSSessionInfo *si);
    void emitUpdatedDirectory();
    void createIndexDir(CFSSessionInfo *si);
    int getRecordIndex(CFSSessionInfo *si, qint64 ts);
    bool openSource(CFSSessionInfo *si, qint64 ts);
    void closeSource(CFSSessionInfo *si);
    void openNewSource(CFSSessionInfo *si);
    void videoDecode(CFSSessionInfo *si, SNC_RECORD_HEADER *pSRH, int length);
    void sensorDecode(CFSSessionInfo *si, SNC_RECORD_HEADER *pSRH, int length);

    CFSSessionInfo *lookUpHandle(int handle);
};

#endif // _SNCCFSCLIENT_H_

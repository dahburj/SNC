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


#include "SNCCFSClient.h"

#include <qjsondocument.h>

SNCCFSClient::SNCCFSClient(qint64 backgroundInterval, const char *tag) : SNCEndpoint(backgroundInterval, tag)
{
    m_tag = tag;
}

SNCCFSClient::~SNCCFSClient()
{
}

void SNCCFSClient::CFSClientInit()
{
    newCFSList();
    m_lastDirReq = SNCUtils::clock();
}

void SNCCFSClient::openSession(int sessionId)
{
    if (m_sessions.contains(sessionId))
        closeSession(sessionId);
    CFSSessionInfo *si = new CFSSessionInfo(sessionId);
    m_sessions.insert(sessionId, si);
}

void SNCCFSClient::closeSession(int sessionId)
{
    if (m_sessions.contains(sessionId)) {
        CFSSessionInfo *si = m_sessions[sessionId];
        m_sessions.remove(sessionId);
        delete si;
    } else {
        SNCUtils::logError(m_tag, QString("Close session but nothing found for sessionID %1").arg(sessionId));
    }
}

void SNCCFSClient::getTimestamp(int sessionId, QString source, qint64 ts)
{
    if (!m_sessions.contains(sessionId)) {
        SNCUtils::logError(m_tag, QString("getTimestamp but no session for sessionID %1, source %2").arg(sessionId).arg(source));
        return;
    }
    CFSSessionInfo *si = m_sessions[sessionId];

    if ((si->m_state != SNCCFSCLIENT_STATE_IDLE) && (si->m_state != SNCCFSCLIENT_STATE_SFOPEN))
        return;

    if (si->m_readOutstanding)
        return;

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts);
    dt.setTime(QTime(0, 0));

    if ((source != si->m_source) || (dt.toMSecsSinceEpoch() != si->m_dayStart)) {
        // source or day has changed - need to redo everything
        if (si->m_source == "") {
            // was idle
            si->m_ts = ts;
            si->m_source = source;
            openSource(si, ts);
        } else {
            si->m_ts = ts;
            si->m_source = source;
            closeSource(si);
        }
        return;
    }

    if (si->m_state == SNCCFSCLIENT_STATE_SFOPEN) {
        si->m_recordIndex = getRecordIndex(si, ts);

        if (si->m_recordIndex == -1) {
            SNCUtils::logWarn(m_tag, QString("Failed to get index for timestamp %1").arg(ts));
            return;
        }

        if (!CFSReadAtIndex(si->m_activeCFS->port, si->m_handle, si->m_recordIndex)) {
            SNCUtils::logWarn(m_tag, QString("Read at index %1 for timestamp %2 failed").arg(si->m_recordIndex).arg(ts));
            return;
        }
        si->m_readOutstanding = true;
    }
}

void SNCCFSClient::newCFSList()
{
    CFSServerInfo *serverInfo;
    //	delete any current list

    for (int i = 0; i < m_serverInfo.count(); i++) {
        serverInfo = m_serverInfo.at(i);
        clientRemoveService(serverInfo->port);
        CFSDeleteEP(serverInfo->port);
        delete(m_serverInfo.at(i));
    }

    m_serverInfo.clear();

    //	Set up SNCCFS endpoints

    loadCFSStores(SNC_PARAMS_CFS_STORES, SNC_PARAMS_CFS_STORE);
    for (int i = 0; i < m_serverInfo.count(); i++) {
        serverInfo = m_serverInfo.at(i);
        serverInfo->port = clientAddService(serverInfo->servicePath + "/" + SNC_STREAMNAME_CFS, SERVICETYPE_E2E, false);

        if (serverInfo->port == -1) {
            SNCUtils::logError(m_tag, QString("Failed to add service for %1").arg(serverInfo->servicePath));
            continue;
        }

        CFSAddEP(serverInfo->port);							// indicate to CEndpoint that this uses the SNCCFS API
        serverInfo->waitingForDirectory = false;			// no outstanding requests
    }

}

bool SNCCFSClient::openSource(CFSSessionInfo *si, qint64 ts)
{
    CFSServerInfo *serverInfo;

    if (si->m_state != SNCCFSCLIENT_STATE_IDLE)
        return false;

    si->m_isSensor = si->m_source.endsWith("sensor");

    // must find which server it is on

    serverInfo = NULL;

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts);
    dt.setTime(QTime(0, 0));

    QString fileName = si->m_source;
    fileName.replace('/', "_");
    fileName += QString("/") + dt.toString("yyyyMMdd_") + "0000";

    si->m_dayStart = dt.toMSecsSinceEpoch();

    si->m_dataFile = fileName + SNC_RECORD_SRF_RECORD_DOTEXT;
    si->m_indexFile = fileName + SNC_RECORD_SRF_INDEX_DOTEXT;

    si->m_index.clear();

    for (int server = 0; server < m_serverInfo.size(); server++) {
        QStringList files = m_serverInfo.at(server)->directory;
        for (int file = 0; file < files.count(); file++) {
            if (si->m_indexFile == files.at(file)) {
                serverInfo = m_serverInfo.at(server);
                break;
            }
        }
        if (serverInfo != NULL)
            break;
    }

    if (serverInfo == NULL) {
        SNCUtils::logError(m_tag, QString("Failed to find server for file %1").arg(fileName));
        si->m_source = "";
        return false;
    }

    si->m_activeCFS = serverInfo;

    if ((si->m_handle = CFSOpenRawFile(si->m_activeCFS->port, si->m_indexFile, si->m_blockSize)) == -1) {
        emit newCFSState(si->m_sessionId, "index file open failed - now idle");
        return false;												// open failed
    }

    si->m_activeFile = fileName;
    si->m_state = SNCCFSCLIENT_STATE_IXOPENING;
    si->m_readOutstanding = false;
    emit newCFSState(si->m_sessionId, "opening index file");
    return true;
}

void SNCCFSClient::closeSource(CFSSessionInfo *si)
{
    if (si->m_state == SNCCFSCLIENT_STATE_IXOPEN) {
        if (CFSClose(si->m_activeCFS->port, si->m_handle)) {
           si->m_state = SNCCFSCLIENT_STATE_IXCLOSING;
           emit newCFSState(si->m_sessionId, "index file closing");
        } else {
           si->m_state = SNCCFSCLIENT_STATE_IDLE;
           emit newCFSState(si->m_sessionId, "index file close failed - now idle");
           openSource(si, si->m_ts);
        }
    }

    if (si->m_state == SNCCFSCLIENT_STATE_SFOPEN) {
        if (CFSClose(si->m_activeCFS->port, si->m_handle)) {
           si->m_state = SNCCFSCLIENT_STATE_SFCLOSING;
           emit newCFSState(si->m_sessionId, "data file file closing");
        } else {
           si->m_state = SNCCFSCLIENT_STATE_IDLE;
           emit newCFSState(si->m_sessionId, "Data file close failed - now idle");
           openSource(si, si->m_ts);
        }
    }

    si->m_readOutstanding = false;
}


void SNCCFSClient::CFSClientBackground()
{
    if ((SNCUtils::clock() - m_lastDirReq) > SNCCFSCLIENT_DIR_INTERVAL) {
        m_lastDirReq = SNCUtils::clock();
        for (int i = 0; i < m_serverInfo.count(); i++) {
            if (!(m_serverInfo.at(i)->waitingForDirectory = CFSDir(m_serverInfo.at(i)->port))) {
                SNCUtils::logWarn(m_tag, QString("Failed to get directory from port %1").arg(m_serverInfo.at(i)->port));
                m_serverInfo.at(i)->directory.clear();
                emitUpdatedDirectory();
            }
        }
    }
}

void SNCCFSClient::CFSDirResponse(int remoteServiceEP, unsigned int responseCode, QStringList filePaths)
{
    CFSServerInfo *serverInfo;
    int i;

    serverInfo = NULL;
    for (i = 0; i < m_serverInfo.size(); i++) {
        if (m_serverInfo.at(i)->port == remoteServiceEP) {
            serverInfo = m_serverInfo.at(i);
            break;
        }
    }

    if (serverInfo == NULL) {
        SNCUtils::logWarn(m_tag, QString("Received directory on unmatched port %1").arg(remoteServiceEP));
        return;
    }

    serverInfo->waitingForDirectory = false;

    if (responseCode != SNCCFS_SUCCESS) {
        SNCUtils::logWarn(m_tag, QString("DirResponse - got error code %1 from port %2").arg(responseCode).arg(remoteServiceEP));
        serverInfo->directory.clear();
        emitUpdatedDirectory();
        return;
    }
    serverInfo->directory.clear();
    serverInfo->directory = filePaths;

//    qDebug() << "*** " << serverInfo->servicePath << ", " << filePaths;

    emitUpdatedDirectory();
}

void SNCCFSClient::emitUpdatedDirectory()
{
    m_directory.clear();

    for (int i = 0; i < m_serverInfo.size(); i++) {
        m_directory.append(m_serverInfo.at(i)->directory);
    }
    emit newDirectory(m_directory);							// tell anyone that wants to know
}

void SNCCFSClient::CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int handle, unsigned int fileLength)
{
    CFSSessionInfo *si = lookUpHandle(handle);

    if (si == NULL) {
        SNCUtils::logError(m_tag, QString("CFSOpenResponse - failed to find session for handle %1").arg(handle));
        return;
    }

    if (responseCode != SNCCFS_SUCCESS) {
        SNCUtils::logWarn(m_tag, QString("Open response - got error code %1 from %2").arg(responseCode).arg(si->m_activeFile));
        si->m_state = SNCCFSCLIENT_STATE_IDLE;
        emit newCFSState(si->m_sessionId, "open error - now idle");
        return;
    }
    if (si->m_state == SNCCFSCLIENT_STATE_IXOPENING) {
        si->m_state = SNCCFSCLIENT_STATE_IXOPEN;
        SNCUtils::logDebug(m_tag, QString("Opened %1 on port %2, handle %3").arg(si->m_activeFile).arg(remoteServiceEP).arg(handle));
        si->m_indexFileLength = fileLength;
        si->m_indexIndex = 0;
        CFSReadAtIndex(remoteServiceEP, handle, si->m_indexIndex, 1);
        si->m_readOutstanding = true;
        emit newCFSState(si->m_sessionId, "index file open");
    } else {
        if (si->m_state == SNCCFSCLIENT_STATE_SFOPENING) {
            si->m_state = SNCCFSCLIENT_STATE_SFOPEN;
            emit newCFSState(si->m_sessionId, "data file open");
            si->m_dataFileLength = fileLength;
            SNCUtils::logDebug(m_tag, QString("Opened %1 on port %2, handle %3")
                               .arg(si->m_activeFile).arg(remoteServiceEP).arg(handle));
            getTimestamp(si->m_sessionId, si->m_source, si->m_ts);
        } else {
            SNCUtils::logWarn(m_tag, QString("Open response - incorrect state from %2").arg(si->m_activeFile));
            si->m_state = SNCCFSCLIENT_STATE_IDLE;
            emit newCFSState(si->m_sessionId, "data file open failed - now idle");
            return;
        }
    }
}

void SNCCFSClient::CFSReadAtIndexResponse(int remoteServiceEP, int handle, unsigned int recordIndex,
        unsigned int responseCode, unsigned char *fileData, int length)
{
    CFSSessionInfo *si = lookUpHandle(handle);

    if (si == NULL) {
        SNCUtils::logError(m_tag, QString("CFSReadAtIndexResponse - failed to find session for handle %1").arg(handle));
        return;
    }

    si->m_readOutstanding = false;
    SNCUtils::logDebug(m_tag, "ReadAtIndexResponse");
    if (responseCode != SNCCFS_SUCCESS) {					// close on read failure
//		if (!CFSClose(remoteServiceEP, handle)) {
//			setCFSStateIdle();
//		}
        SNCUtils::logWarn(m_tag, QString("Read at index response - got error code %1 from %2").arg(responseCode).arg(si->m_activeFile));
        return;
    }

    if (si->m_state == SNCCFSCLIENT_STATE_IXOPEN) {
        if (recordIndex != si->m_indexIndex) {
             SNCUtils::logWarn(m_tag, QString("Got read at %1, expected %2").arg(recordIndex).arg(si->m_indexIndex));
             free(fileData);
             return;
        }
        int indexCount = length / sizeof(SNC_CFSINDEX);
        unsigned char *fp = fileData;

        for (int index = 0; index < indexCount; index++) {
            si->m_index.append(*((qint64 *)(fp + 8)));
            fp += sizeof(SNC_CFSINDEX);
        }
        free(fileData);

        if (++si->m_indexIndex == si->m_indexFileLength) {
            // index file transfer complete

            createIndexDir(si);

            if (CFSClose(si->m_activeCFS->port, si->m_handle)) {
                si->m_state = SNCCFSCLIENT_STATE_IXCLOSING;
                emit newCFSState(si->m_sessionId, "index file closing");
            } else {
                emit newCFSState(si->m_sessionId, "index file close error - now idle");
                si->m_state = SNCCFSCLIENT_STATE_IDLE;
            }
        } else {
            if (!CFSReadAtIndex(remoteServiceEP, handle, si->m_indexIndex, 1)) {
                SNCUtils::logWarn(m_tag, QString("Index read at index for record %1 from %2 failed")
                                  .arg(si->m_indexIndex).arg(si->m_activeFile));
                emit newCFSState(si->m_sessionId, QString("Index read at index for record %1 from %2 failed")
                                 .arg(si->m_indexIndex).arg(si->m_activeFile));
            } else {
                si->m_readOutstanding = true;
            }
        }

    } else if (si->m_state == SNCCFSCLIENT_STATE_SFOPEN) {
       if ((int)recordIndex != si->m_recordIndex) {
            SNCUtils::logWarn(m_tag, QString("Got read at %1, expected %2").arg(recordIndex).arg(si->m_recordIndex));
            free(fileData);
            return;
        }
       if (si->m_isSensor) {
            sensorDecode(si, (SNC_RECORD_HEADER *)fileData, length);
       } else {
           videoDecode(si, (SNC_RECORD_HEADER *)fileData, length);
       }
    } else {
        SNCUtils::logWarn(m_tag, QString("Got read at %1 in incorrect state %2").arg(recordIndex).arg(si->m_state));
    }
}

void SNCCFSClient::CFSCloseResponse(int , unsigned int responseCode, int handle)
{
    CFSSessionInfo *si = lookUpHandle(handle);

    if (si == NULL) {
        SNCUtils::logError(m_tag, QString("CFSCloseResponse - failed to find session for handle %1").arg(handle));
        return;
    }

    if (si->m_state == SNCCFSCLIENT_STATE_IXCLOSING) {
        if ((si->m_handle = CFSOpenStructuredFile(si->m_activeCFS->port, si->m_dataFile)) == -1) {
            setCFSStateIdle(si);
            return;												// structured open failed
        }
        si->m_state = SNCCFSCLIENT_STATE_SFOPENING;
        emit newCFSState(si->m_sessionId, "data file opening");
        return;
    }
    setCFSStateIdle(si);
    SNCUtils::logWarn(m_tag, QString("Close response - got response code %1 from %2").arg(responseCode).arg(si->m_activeFile));
    responseCode += 0;										// to keep compiler happy
}


void SNCCFSClient::CFSKeepAliveTimeout(int remoteServiceEP, int handle)
{
    CFSSessionInfo *si = lookUpHandle(handle);

    if (si == NULL) {
        SNCUtils::logError(m_tag, QString("CFSKeepAliveTimeout - failed to find session for handle %1").arg(handle));
        return;
    }

    setCFSStateIdle(si);
    SNCUtils::logWarn(m_tag, QString("Got keep alive timeout on port %1, slot %2").arg(remoteServiceEP).arg(handle));
    remoteServiceEP += 0;									// to keep compiler happy
    handle += 0;											// to keep compiler happy
}

void SNCCFSClient::videoDecode(CFSSessionInfo *si, SNC_RECORD_HEADER *header, int length)
{
    SNC_RECORD_VIDEO *videoRecord;
    SNC_RECORD_AVMUX *avmuxRecord;

    int muxLength;
    int videoLength;
    int audioLength;
    unsigned char *videoPtr;
    unsigned char *audioPtr;

    switch (SNCUtils::convertUC2ToInt(header->type))
    {
        case SNC_RECORD_TYPE_VIDEO:
            if (length < (int)sizeof(SNC_RECORD_VIDEO)) {
                SNCUtils::logWarn(m_tag, QString("Received video record that was too short: ") + QString::number(length));
                break;
            }
            length -= sizeof(SNC_RECORD_VIDEO);
            videoRecord = (SNC_RECORD_VIDEO *)(header);
            videoLength = SNCUtils::convertUC4ToInt(videoRecord->size);

            if (length < videoLength) {
                SNCUtils::logWarn(m_tag, QString("Received video record that was too short: ") + QString::number(length));
                break;
            }

            switch (SNCUtils::convertUC2ToInt(header->subType))
            {
                case SNC_RECORD_TYPE_VIDEO_MJPEG:
                    si->m_frameCompressed.clear();
                    si->m_frameCompressed.insert(0, reinterpret_cast<const char *>(videoRecord + 1), videoLength);
                    qint64 ts = SNCUtils::convertUC8ToInt64(videoRecord->recordHeader.timestamp);
                    emit newJpeg(si->m_sessionId, si->m_frameCompressed, ts);
                    si->m_frame.loadFromData(reinterpret_cast<const unsigned char *>(videoRecord + 1), videoLength, "JPEG");
                    emit newImage(si->m_sessionId, si->m_frame, ts);
                    break;
            }
            break;

        case SNC_RECORD_TYPE_AVMUX:
            avmuxRecord = (SNC_RECORD_AVMUX *)(header);
            if (!SNCUtils::avmuxHeaderValidate(avmuxRecord, length,
                    NULL, muxLength, &videoPtr, videoLength, &audioPtr, audioLength)) {
                SNCUtils::logWarn(m_tag, "avmux record failed validation");
                break;
            }
            length -= sizeof(SNC_RECORD_AVMUX);

            switch (SNCUtils::convertUC2ToInt(header->subType))
            {
                case SNC_RECORD_TYPE_AVMUX_MJPPCM:
                    si->m_frameCompressed.clear();
                    si->m_frameCompressed.insert(0, (const char *)videoPtr, videoLength);
                    qint64 ts = SNCUtils::convertUC8ToInt64(header->timestamp);
                    emit newJpeg(si->m_sessionId, si->m_frameCompressed, ts);
                    si->m_frame.loadFromData(videoPtr, videoLength, "JPEG");
                    emit newImage(si->m_sessionId, si->m_frame, ts);
                    if (audioLength > 0) {
                        emit newAudio(si->m_sessionId, QByteArray((char *)audioPtr, audioLength),
                            SNCUtils::convertUC4ToInt(avmuxRecord->audioSampleRate),
                            SNCUtils::convertUC2ToInt(avmuxRecord->audioChannels),
                            SNCUtils::convertUC2ToInt(avmuxRecord->audioSampleSize));
                    }
                    break;
            }
            break;


    }
    free(header);
}

void SNCCFSClient::sensorDecode(CFSSessionInfo *si, SNC_RECORD_HEADER *header, int length)
{
    if (length < (int)sizeof(SNC_RECORD_HEADER)) {
        SNCUtils::logWarn(m_tag, QString("Received sensor record that was too short: ") + QString::number(length));
        free(header);
        return;
    }
    length -= sizeof(SNC_RECORD_HEADER);
    QJsonDocument doc(QJsonDocument::fromJson(QByteArray((char *)(header + 1), length)));

    QJsonObject json = doc.object();
    qint64 ts = SNCUtils::convertUC8ToInt64(header->timestamp);
    emit newSensorData(si->m_sessionId, json, ts);

    free(header);
}

void SNCCFSClient::setCFSStateIdle(CFSSessionInfo *si)
{
    si->m_state = SNCCFSCLIENT_STATE_IDLE;
    emit newCFSState(si->m_sessionId, "idle");
    if (si->m_source != "") {
        openSource(si, si->m_ts);
        return;
    }
}

void SNCCFSClient::loadCFSStores(QString group, QString src)
{
    CFSServerInfo *serverInfo;
    QSettings *settings = SNCUtils::getSettings();

    int count = settings->beginReadArray(group);

    for (int i = 0; i < count; i++) {
        settings->setArrayIndex(i);
        serverInfo = new CFSServerInfo;
        serverInfo->servicePath = settings->value(src).toString();
        m_serverInfo.append(serverInfo);
    }

    settings->endArray();

    delete settings;
}

void SNCCFSClient::createIndexDir(CFSSessionInfo *si)
{
    si->m_indexDir.clear();

    qint64 currentSecond = -1;

    for (int index = 0; index < si->m_index.length(); index++) {
        qint64 newSecond = (si->m_index[index] - si->m_dayStart) / 1000;
        if (newSecond < 0)
            continue;                                   // should never happen apparently

        if (newSecond > (3600*24)) {
            SNCUtils::logWarn(m_tag, QString("createIndexDir: newSecond (%1) > length of day").arg(newSecond));

        }

        if (newSecond > currentSecond) {
            for (qint64 second = currentSecond+1; second < newSecond; second++) {
                if (index == 0)
                    si->m_indexDir.insert(second, index);
                else
                    si->m_indexDir.insert(second, index - 1);
            }
            si->m_indexDir.insert(newSecond, index);
            currentSecond = newSecond;
        }
    }
}

int SNCCFSClient::getRecordIndex(CFSSessionInfo *si, qint64 ts)
{
    qint64 second = (ts - si->m_dayStart) / 1000;

    if (second < 0) {
        SNCUtils::logWarn(m_tag, QString("timestamp %1 is earlier than midnight %2")
                          .arg(ts).arg(si->m_dayStart));
        return -1;
    }

    if (second >= si->m_indexDir.count()) {
//        SNCUtils::logWarn(m_tag, QString("timestamp %1 is later than last timestamp %2")
//                          .arg(ts).arg(m_index.last()));
        return si->m_index.count() - 1;
    }

    if (!si->m_indexDir.contains(second)) {
        SNCUtils::logWarn(m_tag, QString("timestamp %1 not found in indexDir")
                          .arg(ts));
        return -1;
    }

    int index = si->m_indexDir[second];

    for (; index < si->m_index.count() - 1; index++) {
        if (si->m_index[index+1] > ts) {
            return index;
        }
    }
    return index;
}

CFSSessionInfo *SNCCFSClient::lookUpHandle(int handle)
{
    CFSSessionInfo *si;

    QHashIterator<int, CFSSessionInfo *> session(m_sessions);
    while (session.hasNext()) {
        session.next();
        si = session.value();
        if (si->m_handle == handle) {
//            qDebug() << "Mapped handle " << handle << " to sessionId " << si->m_sessionId;
            return si;
        }
    }
    SNCUtils::logWarn(m_tag, QString("Failed to map handle %1 to a session id").arg(handle));
    return NULL;
}

//-----------------------------------------------------------------------------

CFSSessionInfo::CFSSessionInfo(int sessionId)
{
    m_sessionId = sessionId;
    m_source = "";
    m_readOutstanding = false;
    m_blockSize = sizeof(SNC_CFSINDEX) * 3000;
    m_state = SNCCFSCLIENT_STATE_IDLE;
    m_isSensor = false;
}

CFSSessionInfo::~CFSSessionInfo()
{

}


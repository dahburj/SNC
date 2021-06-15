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

#ifndef STORESTREAM_H
#define STORESTREAM_H

#include <qsettings.h>
#include <qmutex.h>
#include <qdatetime.h>
#include <qqueue.h>
#include <qlabel.h>


enum StoreFileFormat { structuredFileFormat, rawFileFormat };

enum StoreRotationPolicy { timeRotation, sizeRotation, anyRotation };
enum StoreRotationTimeUnits { rotationMinuteUnits, rotationHourUnits, rotationDayUnits };

enum StoreDeletionPolicy { timeDeletion, countDeletion, anyDeletion };
enum StoreDeletionTimeUnits { deletionHourUnits, deletionDayUnits };

class StoreStream
{
public:
    StoreStream();
    StoreStream(const QString& logTag);
    StoreStream(const StoreStream &rhs);

    StoreStream& operator=(const StoreStream &rhs);

    static bool streamIndexValid(int index);
    static bool streamIndexInUse(int index);

    QString pathOnly();
    QString streamName();
    StoreFileFormat storeFormat();
    QString storePath();
    bool createSubFolder();
    StoreRotationPolicy rotationPolicy();
    StoreRotationTimeUnits rotationTimeUnits();
    qint32 rotationTime();
    qint64 rotationSize();

    bool folderWritable();
    bool needRotation();
    void doRotation();

    QString currentFile();
    QString currentFileFullPath();
    QString rawFileFullPath();
    QString srfFileFullPath();
    QString srfIndexFullPath();

    void updateStats(int recordLength);
    qint64 rxTotalRecords();
    qint64 rxTotalBytes();
    qint64 rxRecords();
    qint64 rxBytes();
    void clearStats();

    bool load(QSettings *settings, const QString& rootDirectory);
    bool load(int sIndex);

    int port;

private:
    bool needTimeRotation(QDateTime now);
    bool needSizeRotation();
    bool checkFolderPermissions();

    void checkDeletion(QDateTime now);
    void setCurrentStart(QDateTime now);

    bool m_folderWritable;
    QString m_streamName;
    QString m_filePrefix;
    QString m_folder;
    QString m_storePath;
    QString m_origStorePath;
    bool m_createSubFolder;
    StoreFileFormat m_storeFormat;

    StoreRotationPolicy m_rotationPolicy;
    StoreRotationTimeUnits m_rotationTimeUnits;
    qint32 m_rotationTime;
    qint64 m_rotationSize;

    StoreDeletionPolicy m_deletionPolicy;
    StoreDeletionTimeUnits m_deletionTimeUnits;
    qint32 m_deletionTime;
    qint64 m_deletionCount;

    QString m_currentFile;
    QString m_currentFileFullPath;
    QString m_currentIndexFileFullPath;
    qint32 m_rotationSecs;
    qint32 m_deletionSecs;

    qint32 m_daysToRotation;
    QDate m_today;

    QMutex m_fileMutex;
    QDateTime m_current;

    QMutex m_statMutex;
    qint64 m_rxTotalRecords;
    qint64 m_rxTotalBytes;
    qint64 m_rxRecords;
    qint64 m_rxBytes;

    QString m_logTag;
};

#endif // STORESTREAM


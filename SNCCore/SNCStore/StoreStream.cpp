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

#include "SNCUtils.h"
#include "StoreStream.h"
#include "SNCStore.h"

#include <qfileinfo.h>
#include <qdir.h>

#define MAX_FILE_ROTATION_SIZE 2000									// 2GB

#define DEFAULT_FILE_ROTATION_SIZE 256								// 256MB

#define TAG "StoreStream"

// static function
// This limitation based on static entries in the settings file
// is going away. This is a temporary way to hide it.
bool StoreStream::streamIndexValid(int index)
{
    bool result = true;

    if (index < 0)
        return false;

    QSettings *settings = SNCUtils::getSettings();

    int count = settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);

    if (index >= count)
        result = false;

    settings->endArray();
    delete settings;

    return result;
}

// static function
bool StoreStream::streamIndexInUse(int index)
{
    bool result = false;

    // ambiguous, GIGO, don't pass negative indices
    if (index < 0)
        return false;

    QSettings *settings = SNCUtils::getSettings();

    int count = settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);

    if (index < count) {
        settings->setArrayIndex(index);
        result = settings->value(SNCSTORE_PARAMS_INUSE).toBool();
    }

    settings->endArray();
    delete settings;

    return result;
}

StoreStream::StoreStream()
{
    m_logTag = "StoreStream";
    m_storePath = "./";
    m_createSubFolder = false;
    m_storeFormat = structuredFileFormat;

    m_rotationPolicy = timeRotation;
    m_rotationTimeUnits = rotationDayUnits;
    m_rotationTime = 1;
    m_rotationSize = 128;

    m_deletionPolicy = timeDeletion;
    m_deletionTimeUnits = deletionDayUnits;
    m_deletionTime = 2;
    m_deletionCount = 5;

    setCurrentStart(QDateTime::currentDateTime());

    clearStats();
}

StoreStream::StoreStream(const QString& logTag)
{
    m_logTag = logTag;
    m_storePath = "./";
    m_createSubFolder = false;
    m_storeFormat = structuredFileFormat;

    m_rotationPolicy = timeRotation;
    m_rotationTimeUnits = rotationDayUnits;
    m_rotationTime = 1;
    m_rotationSize = 128;

    m_deletionPolicy = timeDeletion;
    m_deletionTimeUnits = deletionDayUnits;
    m_deletionTime = 2;
    m_deletionCount = 5;

    setCurrentStart(QDateTime::currentDateTime());

    clearStats();
}

StoreStream::StoreStream(const StoreStream &rhs)
{
    *this = rhs;
}

StoreStream& StoreStream::operator=(const StoreStream &rhs)
{
    if (this != &rhs) {
        m_logTag = rhs.m_logTag;
        m_folderWritable = rhs.m_folderWritable;
        m_streamName = rhs.m_streamName;
        m_origStorePath = rhs.m_origStorePath;
        m_storePath = rhs.m_storePath;
        m_createSubFolder = rhs.m_createSubFolder;
        m_storeFormat = rhs.m_storeFormat;

        m_rotationPolicy = rhs.m_rotationPolicy;
        m_rotationTimeUnits = rhs.m_rotationTimeUnits;
        m_rotationTime = rhs.m_rotationTime;
        m_rotationSize = rhs.m_rotationSize;

        m_deletionPolicy = rhs.m_deletionPolicy;
        m_deletionTimeUnits = rhs.m_deletionTimeUnits;
        m_deletionTime = rhs.m_deletionTime;
        m_deletionCount = rhs.m_deletionCount;

        m_currentFile = rhs.m_currentFile;
        m_currentFileFullPath = rhs.m_currentFileFullPath;
        m_currentIndexFileFullPath = rhs.m_currentIndexFileFullPath;
        m_rotationSecs = rhs.m_rotationSecs;

        m_current = rhs.m_current;

        m_rxTotalRecords = rhs.m_rxTotalRecords;
        m_rxTotalBytes = rhs.m_rxTotalBytes;
        m_rxRecords = rhs.m_rxRecords;
        m_rxBytes = rhs.m_rxBytes;
    }

    return *this;
}

QString StoreStream::pathOnly()
{
    return m_storePath;
}

QString StoreStream::streamName()
{
    return m_streamName;
}

StoreFileFormat StoreStream::storeFormat()
{
    return m_storeFormat;
}

QString StoreStream::storePath()
{
    return m_origStorePath;
}

bool StoreStream::createSubFolder()
{
    return m_createSubFolder;
}

StoreRotationPolicy StoreStream::rotationPolicy()
{
    return m_rotationPolicy;
}

StoreRotationTimeUnits StoreStream::rotationTimeUnits()
{
    return m_rotationTimeUnits;
}

qint32 StoreStream::rotationTime()
{
    return m_rotationTime;
}

qint64 StoreStream::rotationSize()
{
    return m_rotationSize;
}

bool StoreStream::needRotation()
{
    QDateTime now = QDateTime::currentDateTime();

    if (needTimeRotation(now) || needSizeRotation())
        return true;

    return false;
}

void StoreStream::doRotation()
{
    QDateTime now = QDateTime::currentDateTime();
    setCurrentStart(now);

    if (m_storeFormat == rawFileFormat) {
        m_fileMutex.lock();
        m_currentFile = QString(m_filePrefix + m_current.toString("yyyyMMdd_hhmm.")
            + SNC_RECORD_FLAT_EXT);

        m_currentFileFullPath = m_storePath + m_currentFile;

        checkDeletion(now);
        m_fileMutex.unlock();
    } else if (m_storeFormat == structuredFileFormat) {
        m_fileMutex.lock();

        m_currentFile = QString(m_filePrefix + m_current.toString("yyyyMMdd_hhmm.")
            + SNC_RECORD_SRF_RECORD_EXT);

        m_currentFileFullPath = m_storePath + m_currentFile;

        m_currentIndexFileFullPath = QString(m_storePath + m_filePrefix
            + m_current.toString("yyyyMMdd_hhmm.") + SNC_RECORD_SRF_INDEX_EXT);

        checkDeletion(now);
        m_fileMutex.unlock();
    }

    m_statMutex.lock();
    m_rxRecords = 0;
    m_rxBytes = 0;
    m_statMutex.unlock();
}

void StoreStream::setCurrentStart(QDateTime now)
{
    if (m_rotationPolicy != timeRotation && m_rotationPolicy != anyRotation) {
        m_current = now;
        return;
    }
    if (m_rotationTimeUnits != rotationDayUnits) {
        m_current = now;
        return;
    }
    m_today = now.date();
    m_current = QDateTime(m_today);							// force time to midnight
    m_daysToRotation = m_rotationTime;						// number of days to next rotation
}

bool StoreStream::needTimeRotation(QDateTime now)
{
    if (m_rotationPolicy != timeRotation && m_rotationPolicy != anyRotation)
        return false;

    if (m_currentFileFullPath.length() == 0)
        return true;

    if (m_rotationTimeUnits != rotationDayUnits) {
        qint32 secsTo = m_current.secsTo(now);

        if (secsTo >= m_rotationSecs) {
            SNCUtils::logDebug(TAG, QString("Switching stream " + m_streamName + " because of time"));
            return true;
        }
    } else {
        if (m_today != QDate::currentDate()) {
            m_today = QDate::currentDate();
            if (--m_daysToRotation <= 0)
                return true;
        }
    }

    return false;
}

bool StoreStream::needSizeRotation()
{
    if (m_rotationPolicy != sizeRotation && m_rotationPolicy != anyRotation)
        return false;

    if (m_currentFileFullPath.length() == 0)
        return true;

    QFileInfo info(m_currentFileFullPath);

    if (info.exists() && info.size() >= m_rotationSize) {
        SNCUtils::logDebug(TAG, QString("Switching stream " + m_streamName + " because of size"));
        return true;
    }

    return false;
}

void StoreStream::checkDeletion(QDateTime now)
{
    QDir dir;

    if (!dir.cd(m_storePath)) {
        SNCUtils::logError(TAG, QString("Failed to open store path %1").arg(m_storePath));
        return;
    }
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Time | QDir::Reversed);

    QStringList filters;

    if (m_storeFormat == rawFileFormat)
        filters << QString("*.") + SNC_RECORD_FLAT_EXT;
    else if (m_storeFormat == structuredFileFormat)
        filters << QString("*.") + SNC_RECORD_SRF_RECORD_EXT;

    dir.setNameFilters(filters);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); i++) {
        bool deleteFile = false;

        QFileInfo fileInfo = list.at(i);
        QString fileName = fileInfo.absoluteFilePath();

        if ((m_deletionPolicy == countDeletion) || (m_deletionPolicy == anyDeletion)) {
            if ((list.size() - i) > m_deletionCount) {
                SNCUtils::logInfo(TAG, QString("Deleting %1 based on count").arg(fileName));
                deleteFile = true;
            }
        }

        if ((m_deletionPolicy == timeDeletion) || (m_deletionPolicy == anyDeletion)) {
            qint32 age = fileInfo.lastModified().secsTo(now);
            if (age >= m_deletionSecs) {
                SNCUtils::logInfo(TAG, QString("Deleting %1 based on age").arg(fileName));
                deleteFile = true;
            }
        }

        if (deleteFile) {
            QFile file;
            file.remove(fileName);
            if (m_storeFormat == structuredFileFormat) {
                fileName.truncate(fileName.length() - QString(SNC_RECORD_SRF_RECORD_EXT).length());
                fileName += SNC_RECORD_SRF_INDEX_EXT;
                file.remove(fileName);
            }
        }
    }
}

QString StoreStream::currentFile()
{
    QMutexLocker lock(&m_fileMutex);

    return m_currentFileFullPath;
}

QString StoreStream::currentFileFullPath()
{
    QMutexLocker lock(&m_fileMutex);

    return m_currentFileFullPath;
}

QString StoreStream::rawFileFullPath()
{
    return currentFileFullPath();
}

QString StoreStream::srfFileFullPath()
{
    return currentFileFullPath();
}

QString StoreStream::srfIndexFullPath()
{
    QMutexLocker lock(&m_fileMutex);

    return m_currentIndexFileFullPath;
}

void StoreStream::updateStats(int recordLength)
{
    QMutexLocker lock(&m_statMutex);

    m_rxTotalRecords++;
    m_rxTotalBytes += recordLength;
    m_rxRecords++;
    m_rxBytes += recordLength;
}

qint64 StoreStream::rxTotalRecords()
{
    QMutexLocker lock(&m_statMutex);

    return m_rxTotalRecords;
}

qint64 StoreStream::rxTotalBytes()
{
    QMutexLocker lock(&m_statMutex);

    return m_rxTotalBytes;
}

qint64 StoreStream::rxRecords()
{
    QMutexLocker lock(&m_statMutex);

    return m_rxRecords;
}

qint64 StoreStream::rxBytes()
{
    QMutexLocker lock(&m_statMutex);

    return m_rxBytes;
}

void StoreStream::clearStats()
{
    QMutexLocker lock(&m_statMutex);

    m_rxTotalRecords = 0;
    m_rxTotalBytes = 0;
    m_rxRecords = 0;
    m_rxBytes = 0;
}

bool StoreStream::folderWritable()
{
    return m_folderWritable;
}

bool StoreStream::checkFolderPermissions()
{
    QFileInfo info(m_storePath);

    if (!info.exists() || !info.isDir())
        return false;

#ifdef Q_WS_WIN
    return true;
#else
    return info.isWritable();
#endif
}

bool StoreStream::load(int sIndex)
{
    QSettings *settings = SNCUtils::getSettings();
    QString rootDirectory = settings->value(SNCSTORE_PARAMS_ROOT_DIRECTORY).toString();

    int count = settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);

    if ((sIndex < 0) || (sIndex >= count)) {
        settings->endArray();
        delete settings;
        return false;
    }

    settings->setArrayIndex(sIndex);

    bool result = load(settings, rootDirectory);

    settings->endArray();

    delete settings;

    return result;
}

// Assumed that settings has already had beginReadArray called
bool StoreStream::load(QSettings *settings, const QString& rootDirectory)
{
    QString str;
    QDir dir;

    m_streamName = settings->value(SNCSTORE_PARAMS_STREAM_SOURCE).toString();
    m_storePath = rootDirectory;

    if (m_storePath.length() == 0)
        m_storePath = "./";

    if (!m_storePath.endsWith('/') && !m_storePath.endsWith('\\'))
         m_storePath += '/';

    m_origStorePath = m_storePath;

    m_createSubFolder = settings->value(SNCSTORE_PARAMS_CREATE_SUBFOLDER).toBool();

    if (m_createSubFolder) {
        m_folder = m_streamName;
        m_folder.replace(":", "_");
        m_folder.replace("/", "_");
        m_storePath += m_folder + '/';
        m_filePrefix = "";
    } else {
        m_filePrefix = m_streamName + "_";
        m_filePrefix.replace(":", "_");
        m_filePrefix.replace("/", "_");
        m_folder = "";
    }

    if (!dir.mkpath(m_storePath)) {
        m_folderWritable = false;
        SNCUtils::logError(TAG, QString("Failed to create folder: %1").arg(m_storePath));
    } else {
        m_folderWritable = checkFolderPermissions();

        if (!m_folderWritable)
            SNCUtils::logError(TAG, QString("Folder is not writable: %1").arg(m_storePath));
    }



    //	Format

    str = settings->value(SNCSTORE_PARAMS_FORMAT).toString().toLower();
    if (str == SNC_RECORD_STORE_FORMAT_SRF)
        m_storeFormat = structuredFileFormat;
    else
        m_storeFormat = rawFileFormat;

    //	Rotation

    str = settings->value(SNCSTORE_PARAMS_ROTATION_POLICY).toString().toLower();

    if (str == SNCSTORE_PARAMS_ROTATION_POLICY_ANY)
        m_rotationPolicy = anyRotation;
    else if (str == SNCSTORE_PARAMS_ROTATION_POLICY_SIZE)
        m_rotationPolicy = sizeRotation;
    else
        m_rotationPolicy = timeRotation;

    if (m_rotationPolicy == timeRotation || m_rotationPolicy == anyRotation) {
        str = settings->value(SNCSTORE_PARAMS_ROTATION_TIME_UNITS).toString().toLower();

        m_rotationTimeUnits = rotationDayUnits;
        if (str == SNCSTORE_PARAMS_ROTATION_TIME_UNITS_HOURS)
            m_rotationTimeUnits = rotationHourUnits;
        if (str == SNCSTORE_PARAMS_ROTATION_TIME_UNITS_MINUTES)
            m_rotationTimeUnits = rotationMinuteUnits;

        m_rotationTime = settings->value(SNCSTORE_PARAMS_ROTATION_TIME).toInt();
        if (m_rotationTime < 0)
            m_rotationTime = 0;
        m_rotationSecs = 0;
        if (m_rotationTimeUnits == rotationHourUnits)
            m_rotationSecs = m_rotationTime * 3600;
        if (m_rotationTimeUnits == rotationMinuteUnits)
            m_rotationSecs = m_rotationTime * 60;
    }

    if (m_rotationPolicy == sizeRotation || m_rotationPolicy == anyRotation) {
        m_rotationSize = settings->value(SNCSTORE_PARAMS_ROTATION_SIZE).toULongLong();
        if (m_rotationSize < 0)
            m_rotationSize = 0;
        else if (m_rotationSize > MAX_FILE_ROTATION_SIZE)
            m_rotationSize = MAX_FILE_ROTATION_SIZE;
        m_rotationSize *= 1000 * 1000;
    }

    //	Deletion

    str = settings->value(SNCSTORE_PARAMS_DELETION_POLICY).toString().toLower();

    if (str == SNCSTORE_PARAMS_DELETION_POLICY_ANY)
        m_deletionPolicy = anyDeletion;
    else if (str == SNCSTORE_PARAMS_DELETION_POLICY_TIME)
        m_deletionPolicy = timeDeletion;
    else
        m_deletionPolicy = countDeletion;

    if (m_deletionPolicy == timeDeletion || m_deletionPolicy == anyDeletion) {
        str = settings->value(SNCSTORE_PARAMS_DELETION_TIME_UNITS).toString().toLower();

        if (str == SNCSTORE_PARAMS_DELETION_TIME_UNITS_HOURS)
            m_deletionTimeUnits = deletionHourUnits;
        else
            m_deletionTimeUnits = deletionDayUnits;
    }

    m_deletionTime = settings->value(SNCSTORE_PARAMS_DELETION_TIME).toInt();

    if (m_deletionTime < 0)
        m_deletionTime = 0;

    if (m_deletionTimeUnits == deletionHourUnits)
        m_deletionSecs = m_deletionTime * 3600;
    else
        m_deletionSecs = m_deletionTime * 3600 * 24;

    if (m_deletionPolicy == countDeletion || m_deletionPolicy == anyDeletion) {
        m_deletionCount = settings->value(SNCSTORE_PARAMS_DELETION_COUNT).toULongLong();

        if (m_deletionCount < 2)
            m_deletionCount = 2;
    }

    return true;
}

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

#include "DirThread.h"
#include "SNCStore.h"
#include "SNCUtils.h"
#define TAG "DirThread"

DirThread::DirThread(const QString& storePath)
    : SNCThread(QString(TAG))
{
    m_storePath = storePath;
}

DirThread::~DirThread()
{
}

void DirThread::initThread()
{
    m_timer = startTimer(SNCCFS_DM_INTERVAL);
    buildDirString();
}

void DirThread::finishThread()
{
    killTimer(m_timer);
}

QString DirThread::getDirectory()
{
    QMutexLocker locker(&m_lock);
    return m_directory;
}

void DirThread::timerEvent(QTimerEvent *)
{
    buildDirString();
}

void DirThread::buildDirString()
{
    QDir dir;
    QString relativePath;

    QMutexLocker locker(&m_lock);

    m_directory = "";
    relativePath = "";

    if (!dir.cd(m_storePath)) {
        SNCUtils::logError(TAG, QString("Failed to open store path %1").arg(m_storePath));
        return;
    }

    dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    processDir(dir, m_directory, "");
}

void DirThread::processDir(QDir dir, QString& dirString, QString relativePath)
{
    QFileInfoList list = dir.entryInfoList();

    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);

        if (fileInfo.isDir()) {
            dir.cd(fileInfo.fileName());

            if (relativePath == "")
                processDir(dir, dirString, fileInfo.fileName());
            else
                processDir(dir, dirString, relativePath + SNC_SERVICEPATH_SEP + fileInfo.fileName());

            dir.cdUp();
        }
        else {
            if (dirString.length() > 0)
                dirString += SNCCFS_FILENAME_SEP;

            if (relativePath == "")
                dirString += fileInfo.fileName();
            else
                dirString += relativePath + SNC_SERVICEPATH_SEP + fileInfo.fileName();
        }
    }
}

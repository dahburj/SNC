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
#include "SNCUtils.h"
#include "StoreBlocksStructured.h"

StoreBlocksStructured::StoreBlocksStructured(StoreStream *stream)
    : StoreStore(stream)
{
}

StoreBlocksStructured::~StoreBlocksStructured()
{
}

void StoreBlocksStructured::processQueue()
{
    if (m_stream->needRotation())
        m_stream->doRotation();

    writeBlocks();
}

void StoreBlocksStructured::writeBlocks()
{
    SNC_RECORD_HEADER *record;
    SNC_STORE_RECORD_HEADER storeRecHeader;
    qint64 pos, headerLength, timestamp;
    QList<qint64> posList;
    QList<qint64> timestampList;

    m_blockMutex.lock();
    int blockCount = m_blocks.count();
    m_blockMutex.unlock();

    if (blockCount < 1)
        return;

    QString dataFilename = m_stream->srfFileFullPath();
    QString indexFilename = m_stream->srfIndexFullPath();

    QFile dataFile(dataFilename);

    if (!dataFile.open(QIODevice::Append))
        return;

    QFile indexFile(indexFilename);

    if (!indexFile.open(QIODevice::Append)) {
        dataFile.close();
        return;
    }

    strncpy(storeRecHeader.sync, SYNC_STRINGV0, SYNC_LENGTH);
    SNCUtils::convertIntToUC4(0, storeRecHeader.data);

    for (int i = 0; i < blockCount; i++) {
        m_blockMutex.lock();
        QByteArray block = m_blocks.dequeue();
        m_blockMutex.unlock();

        if (block.length() < (int)sizeof(SNC_RECORD_HEADER))
            continue;

        SNCUtils::convertIntToUC4(block.size(), storeRecHeader.size);

        record = reinterpret_cast<SNC_RECORD_HEADER *>(block.data());

        dataFile.write((char *)(&storeRecHeader), sizeof(SNC_STORE_RECORD_HEADER));
        headerLength = (qint64)sizeof(SNC_STORE_RECORD_HEADER);

        pos = dataFile.pos();
        pos -= headerLength;

        dataFile.write((char *)record, block.size());
        posList.append(pos);
        timestampList.append(SNCUtils::convertUC8ToInt64(record->timestamp));
    }

    for (int i = 0; i < posList.count(); i++) {
        pos = posList[i];
        timestamp = timestampList[i];
        indexFile.write((char *)&pos, sizeof(qint64));
        indexFile.write((char *)&timestamp, sizeof(qint64));
    }

    indexFile.close();
    dataFile.close();
}

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
#include "StoreBlocksRaw.h"

#define TAG "StoreBlocksRaw"

StoreBlocksRaw::StoreBlocksRaw(StoreStream *stream)
    : StoreStore(stream)
{
}

StoreBlocksRaw::~StoreBlocksRaw()
{
}

void StoreBlocksRaw::processQueue()
{
    if (m_stream->needRotation())
        m_stream->doRotation();

    writeBlocks();
}

void StoreBlocksRaw::writeBlocks()
{
    SNC_RECORD_HEADER *record;
    int headerLen;

    m_blockMutex.lock();
    int blockCount = m_blocks.count();
    m_blockMutex.unlock();

    if (blockCount < 1)
        return;

    QString dataFilename = m_stream->rawFileFullPath();

    QFile rf(dataFilename);

    if (!rf.open(QIODevice::Append)) {
        SNCUtils::logWarn(TAG, QString("StoreBlocksRaw::writeBlocks - Failed opening file %1").arg(dataFilename));
        return;
    }

    for (int i = 0; i < blockCount; i++) {
        m_blockMutex.lock();
        QByteArray block = m_blocks.dequeue();
        m_blockMutex.unlock();

        if (block.length() < (int)sizeof(SNC_RECORD_HEADER))
            continue;

        record = reinterpret_cast<SNC_RECORD_HEADER *>(block.data());
        headerLen = SNCUtils::convertUC2ToInt(record->headerLength);

        if (headerLen < 0 || headerLen > block.size()) {
            SNCUtils::logWarn(TAG, QString("StoreBlocksRaw::writeBlocks - invalid header size %1").arg(headerLen));
            continue;
        }

        rf.write((char *)record + headerLen, block.size() - headerLen);
    }

    rf.close();
}

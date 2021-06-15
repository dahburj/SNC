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

#include "DisplayStatsData.h"

#define STAT_TIMER_INTERVAL 5

DisplayStatsData::DisplayStatsData()
{
    clear();
    m_timer = startTimer(STAT_TIMER_INTERVAL * 1000);
}

int DisplayStatsData::totalRecords() const
{
    return m_totalRecords;
}

qint64 DisplayStatsData::totalBytes() const
{
    return m_totalBytes;
}

qreal DisplayStatsData::recordRate() const
{
    return m_recordRate;
}

qreal DisplayStatsData::byteRate() const
{
    return m_byteRate;
}

void DisplayStatsData::updateBytes(int bytes)
{
    m_totalRecords++;
    m_totalBytes += bytes;
    m_records++;
    m_bytes += bytes;
}

void DisplayStatsData::timerEvent(QTimerEvent *)
{
    updateRates(STAT_TIMER_INTERVAL);
}

void DisplayStatsData::updateRates(int secs)
{
    if (secs > 0) {
        m_recordRate = (qreal)m_records / (qreal)secs;
        m_byteRate = (qreal)m_bytes / (qreal)secs;
    }

    m_records = 0;
    m_bytes = 0;
}

void DisplayStatsData::clear()
{
    m_totalRecords = 0;
    m_totalBytes = 0;
    m_records = 0;
    m_bytes = 0;
    m_recordRate = 0.0;
    m_byteRate = 0.0;
}

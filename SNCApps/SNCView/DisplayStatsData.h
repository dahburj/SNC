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

#ifndef DISPLAYSTATSDATA_H
#define DISPLAYSTATSDATA_H

#include <qstring.h>
#include <qobject.h>

class DisplayStatsData : public QObject
{
    Q_OBJECT

public:
    DisplayStatsData();

    int totalRecords() const;
    qint64 totalBytes() const;
    qreal recordRate() const;
    qreal byteRate() const;

    void clear();

public slots:
    void updateBytes(int bytes);

protected:
    void timerEvent(QTimerEvent *);

private:
    void updateRates(int secs);

    int m_timer;
    int m_totalRecords;
    qint64 m_totalBytes;
    qreal m_recordRate;
    qreal m_byteRate;
    int m_records;
    int m_bytes;
};

#endif // DISPLAYSTATSDATA_H

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

#ifndef TRENDWIDGET_H
#define TRENDWIDGET_H

#include <qwidget.h>
#include <qmutex.h>
#include <qdatetime.h>
#include <qqueue.h>

class TrendData;

class TrendWidget : public QWidget
{
public:
    TrendWidget(TrendData *trendData, QWidget *parent = 0);
    void setTrendData(TrendData *trendData) { m_trendData = trendData; }

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *);
    void closeEvent(QCloseEvent *);

private:
    void drawLine(QPainter *painter);
    void drawLegend(QPainter *painter);

    int scalePlotYValue(double value, int height);
    int scalePlotXValue(int index, int height);

    TrendData *m_trendData;

    int m_timerId;
};

#endif // TRENDWIDGET_H

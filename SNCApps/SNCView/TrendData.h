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

#ifndef TRENDDATA_H
#define TRENDDATA_H

#include <qobject.h>
#include <qmutex.h>
#include <qdatetime.h>
#include <qqueue.h>
#include <qcolor.h>

class TrendData : public QObject
{
public:
    TrendData(const QString& legend, int maxDataPoints,
                double minValue, double maxValue, int timeStep, QColor color, QWidget *parent = 0);
    ~TrendData();

    void updateLegend(const QString& legend) { m_legend = legend; }
    void addDataPoint(double dataPoint, double timestamp);
    void reset();

    QColor& color() { return m_color; }
    double maxValue() { return m_maxValue; }
    double minValue() { return m_minValue; }
    QList<double>& data() { return m_data; }
    int maxDataPoints() { return m_maxDataPoints; }
    int timeStep() { return m_timeStep; }
    qint64 lastStepTime() { return m_lastStepTime; }
    qint64 lastTimestamp() { return m_lastTimestamp; }
    QString& legend() { return m_legend; }

protected:
    void timerEvent(QTimerEvent *event);

private:
    void doTimeStep();

    QWidget *m_parent;

    QList<double> m_data;
    QString m_legend;
    QColor m_color;
    double m_maxValue;
    double m_minValue;
    int m_maxDataPoints;
    int m_timeStep;
    double m_currentData;
    int m_currentDataCount;
    int m_timer;
    qint64 m_lastStepTime;
    qint64 m_lastTimestamp;
};

#endif // TRENDDATA_H

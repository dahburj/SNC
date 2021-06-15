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

#include "TrendData.h"

#include <qwidget.h>

TrendData::TrendData(const QString& legend, int maxDataPoints, double minValue, double maxValue, int timeStep, QColor color, QWidget *parent)
    : QObject()
{
    Q_ASSERT(maxDataPoints > 1);

    m_parent = parent;

    m_legend = legend;
    m_maxDataPoints = maxDataPoints;
    m_maxValue = maxValue;
    m_minValue = minValue;
    m_color = color;
    for (int i = 0; i < maxDataPoints; i++)
        m_data.append(m_minValue);

    m_currentData = 0;
    m_currentDataCount = 0;
    m_timeStep = timeStep;
    m_timer = startTimer(2);
    m_lastStepTime = QDateTime::currentMSecsSinceEpoch();
}

TrendData::~TrendData()
{
    killTimer(m_timer);
}

void TrendData::reset()
{
    m_data.clear();

    for (int i = 0; i < m_maxDataPoints; i++)
        m_data.append(m_minValue);

    m_currentData = 0;
    m_currentDataCount = 0;
    m_lastStepTime = QDateTime::currentMSecsSinceEpoch();
}

void TrendData::timerEvent(QTimerEvent *)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if ((now - m_lastStepTime) >= m_timeStep) {
        doTimeStep();
        m_lastStepTime += m_timeStep;
    }
}

void TrendData::doTimeStep()
{
    Q_ASSERT(m_maxDataPoints == m_data.count());

    if (m_currentDataCount > 0)
        m_data.append(m_currentData / (double)m_currentDataCount);
    else
        m_data.append(m_data[m_maxDataPoints - 1]);
    m_data.takeFirst();

    m_currentData = 0;
    m_currentDataCount = 0;
}

void TrendData::addDataPoint(double dataPoint, double timestamp)
{
    m_currentData += dataPoint;
    m_lastTimestamp = (qint64)(timestamp * 1000);
    m_currentDataCount++;
}


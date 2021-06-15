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

#include "TrendWidget.h"
#include "TrendData.h"
#include <qpainter.h>
#include <qdebug.h>

TrendWidget::TrendWidget(TrendData *trendData, QWidget *parent)
    : QWidget(parent)
{
    m_trendData = trendData;
    m_timerId = startTimer(100);
}


void TrendWidget::timerEvent(QTimerEvent *)
{
    update();
}

void TrendWidget::closeEvent(QCloseEvent *)
{
    killTimer(m_timerId);
}

void TrendWidget::paintEvent(QPaintEvent *)
{
    if (m_trendData == NULL)
        return;

    QPainter painter(this);
    int w = width();
    int h = height();

    // the background
    painter.fillRect(0, 0, w, h, QColor(20, 20, 20));

    QPen oldPen = painter.pen();
    QPen dashPen;
    dashPen.setStyle(Qt::DotLine);
    dashPen.setColor(Qt::lightGray);
    painter.setPen(dashPen);
    painter.drawLine(0, h / 2, w, h / 2);
    painter.drawLine(0, h / 4, w, h / 4);
    painter.drawLine(0, (h * 3) / 4, w, (h * 3) / 4);
    painter.setPen(oldPen);

    drawLine(&painter);
    drawLegend(&painter);
}

void TrendWidget::drawLegend(QPainter *painter)
{
    if (m_trendData->legend() == "")
        return;

    int fontHeight = height() / 8;

    if (fontHeight < 11)
        fontHeight = 11;

    int y = height() / 2;

    painter->setPen(Qt::white);
    painter->setFont(QFont("Courier", fontHeight));
    painter->drawText(10, y, m_trendData->legend());
}

int TrendWidget::scalePlotYValue(double value, int height)
{
    qreal range = m_trendData->maxValue() - m_trendData->minValue();

    int y = height - (int)((qreal)height * ((value - m_trendData->minValue()) / range));

    if (y < 2)
        y = 2;

    if (y >= height)
        y = height - 2;

    return y;
}

int TrendWidget::scalePlotXValue(int index, int width)
{
    return (index * width) / m_trendData->maxDataPoints();
}

void TrendWidget::drawLine(QPainter *painter)
{
    int y1, y2;
    int h = height();
    int w = width();
    qint64 timeInterval = 120000;

    double value = m_trendData->data()[0];
    painter->setPen(m_trendData->color());

    y1 = scalePlotYValue(value, h);

    for (int index = 1; index < m_trendData->maxDataPoints(); index++) {
        value = m_trendData->data()[index];
        y2 = scalePlotYValue(value, h);
        painter->drawLine(scalePlotXValue(index, w), y1, scalePlotXValue(index + 1, w), y2);
        y1 = y2;
    }

    QPen oldPen = painter->pen();
    QPen dashPen;
    dashPen.setStyle(Qt::DotLine);
    dashPen.setColor(Qt::lightGray);
    painter->setPen(dashPen);

    QPen middlePen;
    middlePen.setStyle(Qt::DotLine);
    middlePen.setColor(Qt::lightGray);

    QPen textPen;
    textPen.setStyle(Qt::SolidLine);
    textPen.setColor(Qt::white);

    qint64 currentTime = m_trendData->lastTimestamp() - m_trendData->maxDataPoints() * m_trendData->timeStep();
    qint64 currentMinute = currentTime / timeInterval;

    int fontHeight = height() / 12;
    painter->setFont(QFont("Courier", fontHeight));

    for (int index = 0; index < m_trendData->maxDataPoints(); index++, currentTime += m_trendData->timeStep()) {
        int x = scalePlotXValue(index, w);
        if ((currentTime / timeInterval) != currentMinute) {
            currentMinute = currentTime / timeInterval;
            if ((w > 300)) {
                painter->drawLine(x, 0, x, h - 20);
                painter->setPen(textPen);
                painter->drawText(x, h - 6, QDateTime::fromMSecsSinceEpoch(currentTime).toString("hh:mm"));
                painter->setPen(dashPen);
            } else {
                painter->drawLine(x, 0, x, h);
            }
        }
        if (index == (m_trendData->maxDataPoints() / 2)) {
            painter->setPen(middlePen);
            painter->drawLine(x, 0, x, h);
            painter->setPen(dashPen);
        }
    }

    painter->setPen(oldPen);
}

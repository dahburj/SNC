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

#include "DialWidget.h"
#include <qpainter.h>
#include "math.h"

#define DIALWIDGET_DEG_TO_RAD           (3.14159365 / 180.0)

//  width and height of dial background

#define DIALWIDGET_DIAL_WIDTH           256
#define DIALWIDGET_DIAL_HEIGHT          256
#define DIALWIDGET_NEEDLE_LENGTH        (DIALWIDGET_DIAL_WIDTH / 2 - 20)

// this is the tick angle delta in radians

#define DIALWIDGET_ANGLE_STEP           (24.0 * DIALWIDGET_DEG_TO_RAD)

//  this is the offset of the start of the dial

#define DIALWIDGET_ANGLE_START          (60.0 * DIALWIDGET_DEG_TO_RAD)

//  this is the range of the dial

#define DIALWIDGET_ANGLE_RANGE          (240.0 * DIALWIDGET_DEG_TO_RAD)

DialWidget::DialWidget(int type, double minValue, double maxValue, QWidget *parent)
    : QWidget(parent)
{
    m_first = true;
    m_type = type;
    m_minValue = minValue;
    m_maxValue = maxValue;
    m_value = 0;
}

void DialWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    int w = width();
    int h = height();

    if (w > h)
        w = h;
    else
        h = w;

    if (m_first) {
        switch (m_type) {
            case DIALWIDGET_TYPE_TEMPERATURE:
                m_dial = QImage(":SNCIVView/icons/DialTemperature.png");
            break;

            case DIALWIDGET_TYPE_PRESSURE:
                m_dial = QImage(":SNCIVView/icons/DialPressure.png");
                break;

            case DIALWIDGET_TYPE_HUMIDITY:
                m_dial = QImage(":SNCIVView/icons/DialHumidity.png");
                break;

            case DIALWIDGET_TYPE_LIGHT:
                m_dial = QImage(":SNCIVView/icons/DialLight.png");
                break;

            case DIALWIDGET_TYPE_AIRQUALITY:
                m_dial = QImage(":SNCIVView/icons/DialAirQuality.png");
                break;

            case DIALWIDGET_TYPE_BLANK:
                break;

        }
        m_first = false;
    }

    // construct image from background and needles

    painter.fillRect(painter.window(), QBrush(Qt::white));

    if (m_type != DIALWIDGET_TYPE_BLANK) {
        QPixmap pixmap = QPixmap::fromImage(m_dial);
        QPainter pixPaint(&pixmap);

        QPen newPen;
        newPen.setStyle(Qt::SolidLine);
        newPen.setWidth(5);

        int pos = DIALWIDGET_DIAL_WIDTH / 2;
        int ex, ey;
        double angle;

        newPen.setColor(Qt::red);
        pixPaint.setPen(newPen);

        angle = DIALWIDGET_ANGLE_START + (m_value - m_minValue) /
              (m_maxValue - m_minValue) * DIALWIDGET_ANGLE_RANGE;

        ex = pos - DIALWIDGET_NEEDLE_LENGTH * sin(angle);
        ey = DIALWIDGET_DIAL_HEIGHT / 2 + DIALWIDGET_NEEDLE_LENGTH * cos(angle);
        pixPaint.drawLine(pos, DIALWIDGET_DIAL_HEIGHT / 2, ex, ey);

        QString text;
        pixPaint.setFont(QFont("Courier", DIALWIDGET_DIAL_HEIGHT / 30));
        switch (m_type) {
            case DIALWIDGET_TYPE_TEMPERATURE:
                text = QString("%1 F").arg(QString::number(m_value, 'f', 2));
            break;

            case DIALWIDGET_TYPE_PRESSURE:
                text = QString("%1 hPA").arg(QString::number(m_value, 'f', 2));
                break;

            case DIALWIDGET_TYPE_HUMIDITY:
                text = QString("%1 \%RH").arg(QString::number(m_value, 'f', 2));
                break;

            case DIALWIDGET_TYPE_LIGHT:
                text = QString("%1 lux").arg(QString::number(m_value, 'f', 2));
                break;

            case DIALWIDGET_TYPE_AIRQUALITY:
                text = QString("%1 \%").arg(QString::number(m_value, 'f', 2));
                break;
        }

        pixPaint.drawText(0, DIALWIDGET_DIAL_HEIGHT / 8, DIALWIDGET_DIAL_WIDTH, DIALWIDGET_DIAL_HEIGHT, Qt::AlignCenter | Qt::AlignHCenter, text);

        QPixmap newPixmap = pixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap((width() - w) / 2, (height() - h) / 2, w, h, newPixmap);


    }
}


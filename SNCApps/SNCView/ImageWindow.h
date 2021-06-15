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

#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <qlabel.h>
#include <qdatetime.h>
#include <qboxlayout.h>

#include "AVSource.h"

class TrendWidget;
class TrendData;

class ImageWindow : public QLabel
{
    Q_OBJECT

public:
    ImageWindow(AVSource *avSource, bool showName, bool showDate, bool showTime, QColor textColor, QWidget *parent = 0);
    virtual ~ImageWindow();

    QString sourceName();

    void setShowName(bool enable);
    void setShowDate(bool enable);
    void setShowTime(bool enable);
    void setTextColor(QColor color);

    bool selected();
    void setSelected(bool select);

    TrendData *getTemperatureData() { return m_temperatureData; }
    TrendData *getPressureData() { return m_pressureData; }
    TrendData *getHumidityData() { return m_humidityData; }
    TrendData *getLightData() { return m_lightData; }
    TrendData *getAirQualityData() { return m_airQualityData; }

    QImage m_image;

signals:
    void imageMousePress(QString name);
    void imageDoubleClick(QString name);

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    void newImage(QImage image, qint64 timestamp);
    QRect drawingRect();

    QByteArray m_sensorData;

    AVSource *m_avSource;
    bool m_showName;
    bool m_showDate;
    bool m_showTime;
    QColor m_textColor;
    bool m_selected;
    bool m_idle;
    int m_timer;

    QVBoxLayout *m_sensorGrid;

    TrendData *m_temperatureData;
    TrendData *m_pressureData;
    TrendData *m_humidityData;
    TrendData *m_lightData;
    TrendData *m_airQualityData;

    TrendWidget *m_temperatureWidget;
    TrendWidget *m_pressureWidget;
    TrendWidget *m_humidityWidget;
    TrendWidget *m_lightWidget;
    TrendWidget *m_airQualityWidget;

    QDate m_displayDate;
    QTime m_displayTime;
    qint64 m_lastFrame;
};

#endif // IMAGEWINDOW_H

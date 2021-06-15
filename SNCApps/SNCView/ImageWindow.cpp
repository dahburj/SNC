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

#include <QtGui>
#include "ImageWindow.h"
#include "TrendWidget.h"
#include "TrendData.h"

#include "SNCUtils.h"

#define	IMAGEVIEW_CAMERA_DEADTIME		(15 * SNC_CLOCKS_PER_SEC)


ImageWindow::ImageWindow(AVSource *avSource, bool showName, bool showDate,
                        bool showTime, QColor textColor, QWidget *parent)
    : QLabel(parent)
{
    m_avSource = avSource;
    m_showName = showName;
    m_showDate = showDate;
    m_showTime = showTime;
    m_textColor = textColor;

    m_selected = false;
    m_idle = true;

    m_lastFrame = SNCUtils::clock();
    m_displayDate = QDate::currentDate();
    m_displayTime = QTime::currentTime();

    setAlignment(Qt::AlignCenter);

    setMinimumWidth(160);
    setMinimumHeight(120);

    setMaximumWidth(1920);
    setMaximumHeight(1080);

    if (m_avSource->streamName() == SNC_STREAMNAME_SENSOR) {
        m_sensorGrid = new QVBoxLayout();
        m_sensorGrid->setMargin(20);
        setLayout(m_sensorGrid);

        m_temperatureData = new TrendData("Temperature", 2000, -20, 100, 200, Qt::red, this);
        m_temperatureWidget = new TrendWidget(m_temperatureData, this);
        m_sensorGrid->addWidget(m_temperatureWidget);

        m_lightData = new TrendData("Light", 2000, 0, 500, 200, Qt::cyan, this);
        m_lightWidget = new TrendWidget(m_lightData, this);
        m_sensorGrid->addWidget(m_lightWidget);

        m_pressureData = new TrendData("Pressure", 2000, 500, 1300, 200, Qt::green, this);
        m_pressureWidget = new TrendWidget(m_pressureData, this);
        m_sensorGrid->addWidget(m_pressureWidget);

        m_humidityData = new TrendData("Humidity", 2000, 0, 100, 200, Qt::yellow, this);
        m_humidityWidget = new TrendWidget(m_humidityData, this);
        m_sensorGrid->addWidget(m_humidityWidget);

        m_airQualityData = new TrendData("Air quality", 2000, -1, 100, 200, Qt::magenta, this);
        m_airQualityWidget = new TrendWidget(m_airQualityData, this);
        m_sensorGrid->addWidget(m_airQualityWidget);
    }

    if (m_avSource)
        m_timer = startTimer(30);
}

ImageWindow::~ImageWindow()
{
    killTimer(m_timer);
    m_avSource = NULL;
}

QString ImageWindow::sourceName()
{
    if (m_avSource)
        return m_avSource->name();

    return QString();
}

void ImageWindow::setShowName(bool enable)
{
    m_showName = enable;
    update();
}

void ImageWindow::setShowDate(bool enable)
{
    m_showDate = enable;
    update();
}

void ImageWindow::setShowTime(bool enable)
{
    m_showTime = enable;
    update();
}

void ImageWindow::setTextColor(QColor color)
{
    m_textColor = color;
    update();
}

void ImageWindow::mousePressEvent(QMouseEvent *)
{
    if (m_avSource)
        emit imageMousePress(m_avSource->name());
}

void ImageWindow::mouseDoubleClickEvent(QMouseEvent *)
{
    if (m_avSource)
        emit imageDoubleClick(m_avSource->name());
}

bool ImageWindow::selected()
{
    return m_selected;
}

void ImageWindow::setSelected(bool select)
{
    m_selected = select;
    update();
}

void ImageWindow::newImage(QImage image, qint64 timestamp)
{
    m_lastFrame = SNCUtils::clock();
    if (image.width() == 0)
        return;

    m_idle = false;
    m_image = image;

    QPixmap pixmap = QPixmap::fromImage(image.scaled(size(), Qt::KeepAspectRatio));
    setPixmap(pixmap);

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(timestamp);
    m_displayDate = dt.date();
    m_displayTime = dt.time();

    update();
}

void ImageWindow::timerEvent(QTimerEvent *)
{
    double value;
    double timestamp = 0;

    if (m_avSource && m_lastFrame < m_avSource->lastUpdate()) {
        if (m_avSource->streamName() == SNC_STREAMNAME_SENSOR) {
            m_sensorData = m_avSource->getSensorData();
            QJsonDocument doc(QJsonDocument::fromJson(m_sensorData));

            QJsonObject json = doc.object();

            if (json.contains("timestamp"))
                timestamp = json.value("timestamp").toDouble();

            if (json.contains("temperature")) {
                value = json.value("temperature").toDouble() * 9.0 / 5.0 + 32.0;
                m_temperatureData->updateLegend(QString("Temperature: %1 F").arg(QString::number(value, 'g', 4)));
                m_temperatureData->addDataPoint(value, timestamp);
            }
            if (json.contains("pressure")) {
                value = json.value("pressure").toDouble();
                m_pressureData->updateLegend(QString("Pressure: %1 hPA").arg(QString::number(value, 'g', 4)));
                m_pressureData->addDataPoint(value, timestamp);
            }
            if (json.contains("humidity")) {
                value = json.value("humidity").toDouble();
                m_humidityData->updateLegend(QString("Humidity: %1 \%RH").arg(QString::number(value, 'g', 4)));
                m_humidityData->addDataPoint(value, timestamp);
            }
            if (json.contains("light")) {
                value = json.value("light").toDouble();
                m_lightData->updateLegend(QString("Light: %1 lux").arg(QString::number(value, 'g', 4)));
                m_lightData->addDataPoint(value, timestamp);
            }

            if (json.contains("airquality")) {
                value = json.value("airquality").toDouble();
                m_airQualityData->updateLegend(QString("Air quality: %1\%").arg(QString::number(value, 'g', 4)));
                m_airQualityData->addDataPoint(value, timestamp);
            }

            QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_avSource->lastUpdate());
            m_displayDate = dt.date();
            m_displayTime = dt.time();
            m_lastFrame =  SNCUtils::clock();
            m_idle = false;
            update();

        } else {
            newImage(m_avSource->image(), m_avSource->imageTimestamp());
        }
    } else if (SNCUtils::timerExpired(SNCUtils::clock(), m_lastFrame, IMAGEVIEW_CAMERA_DEADTIME)) {
        m_idle = true;
        update();
    }
}

void ImageWindow::paintEvent(QPaintEvent *event)
 {
    QString timestamp;
    QLabel::paintEvent(event);
    QString timeFormat("hh:mm:ss:zzz");
    QString dateFormat("ddd MMMM d yyyy");

    QPainter painter(this);

    QRect dr = drawingRect();

    if (m_idle) {
        painter.fillRect(dr, Qt::Dense2Pattern);
    } else {
        if (m_avSource->streamName() == SNC_STREAMNAME_SENSOR) {
            painter.fillRect(dr, QColor(10, 10, 10));
        }
    }

    if (m_selected) {
        QPen pen(Qt::green, 3);
        painter.setPen(pen);
        painter.drawRect(dr);
    }

    int fontHeight;

    fontHeight = dr.width() / 30;

    if (fontHeight < 8)
        fontHeight = 8;
    else if (fontHeight > 12)
        fontHeight = 12;

    painter.setPen(m_textColor);

    painter.setFont(QFont("Arial", fontHeight));

    if (m_showName)
        painter.drawText(dr.left() + 4, dr.top() + fontHeight + 6, m_avSource->name());

    if (m_showTime || m_showDate) {
        if (dr.width() < 160) {
            // only room for one, choose time over date
            if (m_showDate && m_showTime)
                timestamp = m_displayTime.toString(timeFormat);
            else if (m_showDate)
                timestamp = m_displayDate.toString(dateFormat);
            else
                timestamp = m_displayTime.toString(timeFormat);
        }
        else if (!m_showDate) {
            timestamp = m_displayTime.toString(timeFormat);
        }
        else if (!m_showTime) {
            timestamp = m_displayDate.toString(dateFormat);
        }
        else {
            timestamp = m_displayDate.toString(dateFormat) + " " + m_displayTime.toString(timeFormat);
        }

        painter.drawText(dr.left() + 4, dr.bottom() - 2, timestamp);

    }
}

// Assumes horizontal and vertical center alignment
QRect ImageWindow::drawingRect()
{
    QRect dr = rect();

    const QPixmap *pm = pixmap();

    if (pm) {
        QRect pmRect = pm->rect();

        int x = (dr.width() - pmRect.width()) / 2;
        int y = (dr.height() - pmRect.height()) / 2;

        dr.adjust(x, y, -x, -y);
    }

    return dr;
}

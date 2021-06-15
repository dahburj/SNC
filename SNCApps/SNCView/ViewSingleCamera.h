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

#ifndef VIEWSINGLECAMERA_H
#define VIEWSINGLECAMERA_H

#include <qdialog.h>
#include <qlabel.h>
#include <qboxlayout.h>
#include <qstatusbar.h>
#include <qjsonobject.h>

#include "AVSource.h"
#include "DVRWidget.h"

class ImageWindow;
class TrendWidget;
class TrendData;
class DialWidget;

class ViewSingleCamera : public QDialog
{
    Q_OBJECT

public:
    ViewSingleCamera(QWidget *parent);

    void setSource(AVSource *avSource, ImageWindow* imageWindow);
    QString sourceName();

public slots:
    void newTimecode(qint64 ts);
    void realtime(bool enable);
    void newHistoricImage(int sessionId, QImage frame, qint64 ts);	// emitted when a frame received
    void newHistoricSensorData(int sessionId, QJsonObject data, qint64 ts); // emitted when historic sensor data received
    void newCFSState(int sessionId, QString state);
    void newDirectory(QStringList directory);				// emitted when a new CFS directory is received

signals:
    void closed();
    void setTimecode(qint64 ts);
    void getTimestamp(int sessionId, QString source, qint64 ts);
    void newFileDays(QList<QDate>);

protected:
    void timerEvent(QTimerEvent *event);
    void closeEvent(QCloseEvent *event);

private:
    void newImage(QImage image, qint64 ts);
    void saveWindowState();
    void restoreWindowState();

    int m_timer;
    AVSource *m_avSource;
    ImageWindow *m_imageWindow;
    qint64 m_lastFrame;
    QLabel *m_cameraView;
    DVRWidget *m_controls;

    qint64 m_ts;

    bool m_sendFileList;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_panelLayout;
    QVBoxLayout *m_trendLayout;
    QHBoxLayout *m_dateTimeLayout;
    QGridLayout *m_dialGridLayout;

    QLabel *m_timestampLabel;

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

    DialWidget *m_dialTemperatureWidget;
    DialWidget *m_dialPressureWidget;
    DialWidget *m_dialHumidityWidget;
    DialWidget *m_dialLightWidget;
    DialWidget *m_dialAirQualityWidget;
    DialWidget *m_dialBlankWidget;

    QLabel *m_cfsStatus;

    QStringList m_filesAvailable;

    bool m_realtime;

};

#endif // VIEWSINGLECAMERA_H

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

#include "ViewSingleCamera.h"
#include "ImageWindow.h"
#include "TrendData.h"
#include "TrendWidget.h"
#include "SNCView.h"
#include "DialWidget.h"

#include "SNCUtils.h"

#define	SNCVIEW_CAMERA_DEADTIME		(10 * SNC_CLOCKS_PER_SEC)

ViewSingleCamera::ViewSingleCamera(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_avSource = NULL;

    m_mainLayout = new QVBoxLayout();
    m_panelLayout = new QHBoxLayout();
    m_trendLayout = new QVBoxLayout();
    m_dateTimeLayout = new QHBoxLayout();
    m_dialGridLayout = new QGridLayout();

    QLabel *label = new QLabel("Displayed timestamp: ");
    m_dateTimeLayout->addWidget(label);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_timestampLabel = new QLabel("...");
    m_timestampLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_timestampLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_dateTimeLayout->addWidget(m_timestampLabel);

    m_cameraView = new QLabel();
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_cameraView->sizePolicy().hasHeightForWidth());
    m_cameraView->setSizePolicy(sizePolicy);
    m_cameraView->setMinimumSize(QSize(320, 240));
    m_cameraView->setAlignment(Qt::AlignTop);

    m_controls = new DVRWidget();

    connect(m_controls, SIGNAL(newTimecode(qint64)), this, SLOT(newTimecode(qint64)));
    connect(m_controls, SIGNAL(realtime(bool)), this, SLOT(realtime(bool)));
    connect(this, SIGNAL(setTimecode(qint64)), m_controls, SLOT(setTimecode(qint64)));
    connect(this, SIGNAL(newFileDays(QList<QDate>)), m_controls, SLOT(newFileDays(QList<QDate>)));

    m_mainLayout->addLayout(m_dateTimeLayout);
    m_mainLayout->addSpacing(20);
    m_mainLayout->addLayout(m_panelLayout);
    m_panelLayout->addItem(m_trendLayout);
    m_panelLayout->addItem(m_dialGridLayout);
    m_panelLayout->addWidget(m_cameraView);
    m_panelLayout->addWidget(m_controls);
    setLayout(m_mainLayout);

    m_temperatureData = NULL;
    m_temperatureWidget = new TrendWidget(NULL, this);
    m_trendLayout->addWidget(m_temperatureWidget);

    m_lightData = NULL;
    m_lightWidget = new TrendWidget(NULL, this);
    m_trendLayout->addWidget(m_lightWidget);

    m_pressureData = NULL;
    m_pressureWidget = new TrendWidget(NULL, this);
    m_trendLayout->addWidget(m_pressureWidget);

    m_humidityData = NULL;
    m_humidityWidget = new TrendWidget(NULL, this);
    m_trendLayout->addWidget(m_humidityWidget);

    m_airQualityData = NULL;
    m_airQualityWidget = new TrendWidget(NULL, this);
    m_trendLayout->addWidget(m_airQualityWidget);

    m_dialTemperatureWidget = new DialWidget(DIALWIDGET_TYPE_TEMPERATURE, -40, 160, this);
    m_dialGridLayout->addWidget(m_dialTemperatureWidget, 0, 0);

    m_dialLightWidget = new DialWidget(DIALWIDGET_TYPE_LIGHT, 0, 2000, this);
    m_dialGridLayout->addWidget(m_dialLightWidget, 0, 1);

    m_dialPressureWidget = new DialWidget(DIALWIDGET_TYPE_PRESSURE, 500, 1300, this);
    m_dialGridLayout->addWidget(m_dialPressureWidget, 0, 2);

    m_dialHumidityWidget = new DialWidget(DIALWIDGET_TYPE_HUMIDITY, 0, 100, this);
    m_dialGridLayout->addWidget(m_dialHumidityWidget, 1, 0);

    m_dialAirQualityWidget = new DialWidget(DIALWIDGET_TYPE_AIRQUALITY, 0, 100, this);
    m_dialGridLayout->addWidget(m_dialAirQualityWidget, 1, 1);

    m_dialBlankWidget = new DialWidget(DIALWIDGET_TYPE_BLANK, 0, 0, this);
    m_dialGridLayout->addWidget(m_dialBlankWidget, 1, 2);

    QStatusBar *statusBar = new QStatusBar();
    m_cfsStatus = new QLabel(this);
    m_cfsStatus->setAlignment(Qt::AlignLeft);
    statusBar->addWidget(m_cfsStatus, 1);
    m_mainLayout->addWidget(statusBar);

    m_lastFrame = -1;
    m_timer = 0;

    m_sendFileList = false;
    m_ts = QDateTime::currentMSecsSinceEpoch();

    restoreWindowState();
}

void ViewSingleCamera::newTimecode(qint64 ts)
{
    m_ts = ts;
    emit getTimestamp(0, m_avSource->name(), ts);
}

void ViewSingleCamera::realtime(bool enable)
{
    m_realtime = enable;

    if ((m_avSource != NULL) && (m_avSource->isSensor()))
    {
        m_dialTemperatureWidget->setVisible(!enable);
        m_dialLightWidget->setVisible(!enable);
        m_dialPressureWidget->setVisible(!enable);
        m_dialHumidityWidget->setVisible(!enable);
        m_dialAirQualityWidget->setVisible(!enable);
        m_dialBlankWidget->setVisible(!enable);

        m_temperatureWidget->setVisible(enable);
        m_lightWidget->setVisible(enable);
        m_pressureWidget->setVisible(enable);
        m_humidityWidget->setVisible(enable);
        m_airQualityWidget->setVisible(enable);
    }
}

void ViewSingleCamera::setSource(AVSource *avSource, ImageWindow *imageWindow)
{
    m_avSource = avSource;
    m_imageWindow = imageWindow;
    m_controls->enableHistory(false);
    m_sendFileList = true;
    realtime(true);

    if (m_avSource) {
        setWindowTitle(m_avSource->name());

        if (m_avSource->streamName() == SNC_STREAMNAME_SENSOR) {
            m_temperatureWidget->setTrendData(m_imageWindow->getTemperatureData());
            m_pressureWidget->setTrendData(m_imageWindow->getPressureData());
            m_humidityWidget->setTrendData(m_imageWindow->getHumidityData());
            m_lightWidget->setTrendData(m_imageWindow->getLightData());
            m_airQualityWidget->setTrendData(m_imageWindow->getAirQualityData());
            m_cameraView->setHidden(true);
            if (m_timer) {
                killTimer(m_timer);
                m_timer = 0;
            }

            m_timer = startTimer(500);
        } else {

            m_temperatureWidget->setTrendData(NULL);
            m_pressureWidget->setTrendData(NULL);
            m_humidityWidget->setTrendData(NULL);
            m_lightWidget->setTrendData(NULL);
            m_airQualityWidget->setTrendData(NULL);
            m_cameraView->setHidden(false);

            newImage(m_avSource->image(), m_avSource->imageTimestamp());

            if (m_timer) {
                killTimer(m_timer);
                m_timer = 0;
            }

            m_timer = startTimer(30);
        }
        emit getTimestamp(0, m_avSource->name(), QDateTime::currentMSecsSinceEpoch());
    } else {
        m_lastFrame = -1;
        m_cameraView->setText("No Image");

        if (m_timer) {
            killTimer(m_timer);
            m_timer = 0;
        }

        // is this what we want?
        setWindowTitle("");
    }
}

QString ViewSingleCamera::sourceName()
{
    if (m_avSource)
        return m_avSource->name();

    return QString();
}

void ViewSingleCamera::closeEvent(QCloseEvent *)
{
    saveWindowState();

    if (m_timer)
        killTimer(m_timer);
    m_timer = 0;

    emit closed();
}


void ViewSingleCamera::newHistoricImage(int /* sessionId */, QImage image, qint64 ts)
{
    if (m_realtime)
        return;

    m_timestampLabel->setText(QDateTime::fromMSecsSinceEpoch(ts).toString("ddd MMMM d yyyy hh:mm:ss:zzz"));

    if (image.width() == 0)
        return;
    else
        m_cameraView->setPixmap(QPixmap::fromImage(image.scaled(m_cameraView->size(), Qt::KeepAspectRatio)));

    update();
}

void ViewSingleCamera::newHistoricSensorData(int /* sessionId */, QJsonObject json, qint64 ts)
{
    double value;

    if (m_realtime)
        return;

    m_timestampLabel->setText(QDateTime::fromMSecsSinceEpoch(ts).toString("ddd MMMM d yyyy hh:mm:ss:zzz"));

    if (json.contains("temperature")) {
        value = json.value("temperature").toDouble() * 9.0 / 5.0 + 32.0;
        m_dialTemperatureWidget->setValue(value);
        m_dialTemperatureWidget->update();
    }
    if (json.contains("light")) {
        value = json.value("light").toDouble();
        m_dialLightWidget->setValue(value);
        m_dialLightWidget->update();
    }
    if (json.contains("pressure")) {
        value = json.value("pressure").toDouble();
        m_dialPressureWidget->setValue(value);
        m_dialPressureWidget->update();
    }
    if (json.contains("humidity")) {
        value = json.value("humidity").toDouble();
        m_dialHumidityWidget->setValue(value);
        m_dialHumidityWidget->update();
    }
    if (json.contains("airquality")) {
        value = json.value("airquality").toDouble();
        m_dialAirQualityWidget->setValue(value);
        m_dialAirQualityWidget->update();
    }
}

void ViewSingleCamera::newImage(QImage image, qint64 ts)
{
    if (!m_realtime)
        return;
    m_lastFrame = SNCUtils::clock();

    m_timestampLabel->setText(QDateTime::fromMSecsSinceEpoch(ts).toString("ddd MMMM d yyyy hh:mm:ss:zzz"));

    if (image.width() == 0)
        return;
    else
        m_cameraView->setPixmap(QPixmap::fromImage(image.scaled(m_cameraView->size(), Qt::KeepAspectRatio)));

    update();
}

void ViewSingleCamera::timerEvent(QTimerEvent *)
{
    if (m_avSource && (m_avSource->streamName() == SNC_STREAMNAME_SENSOR)) {
        m_timestampLabel->setText(QDateTime::fromMSecsSinceEpoch(m_avSource->lastUpdate()).toString("ddd MMMM d yyyy hh:mm:ss:zzz"));
        m_timestampLabel->update();
        if (m_realtime) {
            emit setTimecode(m_avSource->lastUpdate());
        }
        return;
    }

    if (m_avSource && m_lastFrame < m_avSource->lastUpdate()) {
        newImage(m_avSource->image(), m_avSource->imageTimestamp());
        if (m_realtime) {
            emit setTimecode(m_avSource->lastUpdate());
        }
    } else if (SNCUtils::timerExpired(SNCUtils::clock(), m_lastFrame, SNCVIEW_CAMERA_DEADTIME)) {
        m_cameraView->setText("No Image");
    }
}

void ViewSingleCamera::saveWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("SingleCameraView");
    settings->setValue("Geometry", saveGeometry());
    settings->endGroup();

    delete settings;
}

void ViewSingleCamera::restoreWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("SingleCameraView");
    restoreGeometry(settings->value("Geometry").toByteArray());
    settings->endGroup();

    delete settings;
}

void ViewSingleCamera::newCFSState(int /* sessionId */, QString state)
{
    m_cfsStatus->setText(QString("CFSState: ") + state);
    qDebug() << "State: " << state;
}

void ViewSingleCamera::newDirectory(QStringList directory)
{
    m_filesAvailable.clear();
    QString currentSource = m_avSource->name();
    currentSource.replace('/', '_');

    for (int i = 0; i < directory.count(); i++) {
        if (!directory.at(i).endsWith(".srf"))
            continue;

        int slashPos = directory.at(i).indexOf('/');
        if (slashPos == -1)
            continue;

        QString source = directory.at(i).left(slashPos);
        if (currentSource == source) {
            QString file = directory.at(i);
            file.chop(QString("_0000.srf").length());
            file = file.right(file.length() - slashPos - 1);
            m_filesAvailable.append(file);
        }
    }
    if (m_filesAvailable.count() == 0) {
        m_controls->enableHistory(false);
        return;
    }

    if (m_sendFileList) {
        m_sendFileList = false;
        QList<QDate> dtl;

        for (int i = 0; i < m_filesAvailable.count(); i++) {
            QDate dt = QDate::fromString(m_filesAvailable.at(i), "yyyyMMdd");
            dtl.append(dt);
        }
        emit newFileDays(dtl);
    }
}


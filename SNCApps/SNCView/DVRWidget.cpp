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

#include "DVRWidget.h"

#include <qboxlayout.h>
#include <qlabel.h>

#define DVRWIDGET_STOP                  0
#define DVRWIDGET_FORWARD               1
#define DVRWIDGET_REVERSE               2
#define DVRWIDGET_FASTFORWARD           3
#define DVRWIDGET_FASTREVERSE           4

#define DVRWIDGET_PLAY_INTERVAL         10

#define DVRWIDGET_DIAL_RANGE            100

DVRWidget::DVRWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_realtimeHistory = new QPushButton("View historic data");
    mainLayout->addWidget(m_realtimeHistory);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    m_realtimeHistory->setSizePolicy(sizePolicy);

    m_realtimeHistory->setEnabled(false);

    connect(m_realtimeHistory, SIGNAL(clicked()), this, SLOT(realtimeHistoryClicked()));

    mainLayout->addSpacing(20);

    QLabel *label = new QLabel("Requested date:");
    mainLayout->addWidget(label);

    m_dateEdit = new QComboBox();
    mainLayout->addWidget(m_dateEdit);
    connect(m_dateEdit, SIGNAL(activated(int)), this, SLOT(dateChanged(int)));
    connect(this, SIGNAL(setCurrentDateText(QString)), m_dateEdit, SLOT(setCurrentText(QString)));
    m_dateEditTimeFormat = "ddd MMMM d yyyy";

    mainLayout->addSpacing(20);

    label = new QLabel("Requested time:");
    mainLayout->addWidget(label);

    m_timeEdit = new QDateTimeEdit(QDateTime::currentDateTime().time());
    m_timeEdit->setDisplayFormat("hh:mm:ss:zzz");
    m_timeEdit->setCalendarPopup(true);
    mainLayout->addWidget(m_timeEdit);
    connect(m_timeEdit, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(timeChanged(QDateTime)));

    mainLayout->addSpacing(20);

    QHBoxLayout *dialLayout = new QHBoxLayout();
    dialLayout->addStretch();
    m_dial = new QDial();
    m_dial->setRange(0, DVRWIDGET_DIAL_RANGE);
    m_dial->setNotchesVisible(true);
    m_dial->setWrapping(true);
    m_dial->setFixedSize(300, 300);
    dialLayout->addWidget(m_dial, 1);
    dialLayout->addStretch();
    mainLayout->addLayout(dialLayout);

    mainLayout->addSpacing(20);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_stopButton = new QPushButton(QIcon(":SNCIVView/icons/Stop.png"), "");
    m_stopButton->setMinimumSize(60, 35);
    m_playForwardButton = new QPushButton(QIcon(":SNCIVView/icons/Forward.png"), "");
    m_playForwardButton->setMinimumSize(60, 35);
    m_playReverseButton = new QPushButton(QIcon(":SNCIVView/icons/Reverse.png"), "");
    m_playReverseButton->setMinimumSize(60, 35);
    m_playFastForwardButton = new QPushButton(QIcon(":SNCIVView/icons/FastForward.png"), "");
    m_playFastForwardButton->setMinimumSize(60, 35);
    m_playFastReverseButton = new QPushButton(QIcon(":SNCIVView/icons/FastReverse.png"), "");
    m_playFastReverseButton->setMinimumSize(60, 35);

    buttonLayout->addWidget(m_playFastReverseButton);
    buttonLayout->addWidget(m_playReverseButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_playForwardButton);
    buttonLayout->addWidget(m_playFastForwardButton);

    mainLayout->addLayout(buttonLayout);

    m_slider = new QSlider(Qt::Orientation::Horizontal);
    m_slider->setRange(0, 24 * 60);
    mainLayout->addWidget(m_slider);
    connect(m_slider, SIGNAL(sliderMoved(int)), this, SLOT(sliderMoved(int)));

    mainLayout->addStretch();

    setFixedWidth(350);

    m_dialPosition = 0;

    connect(m_dial, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

    enableButtons(false);

    m_signalMapper = new QSignalMapper(this);
    connect(m_stopButton, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
    connect(m_playForwardButton, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
    connect(m_playReverseButton, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
    connect(m_playFastForwardButton, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
    connect(m_playFastReverseButton, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
    m_signalMapper->setMapping(m_stopButton, DVRWIDGET_STOP);
    m_signalMapper->setMapping(m_playForwardButton, DVRWIDGET_FORWARD);
    m_signalMapper->setMapping(m_playReverseButton, DVRWIDGET_REVERSE);
    m_signalMapper->setMapping(m_playFastForwardButton, DVRWIDGET_FASTFORWARD);
    m_signalMapper->setMapping(m_playFastReverseButton, DVRWIDGET_FASTREVERSE);

    connect(m_signalMapper, SIGNAL(mapped(int)), this, SLOT(onControl(int)));

    m_realtime = true;
    m_newTimestampRequested = false;

    m_interval = 0;

    m_timerID = startTimer(DVRWIDGET_PLAY_INTERVAL);
}

void DVRWidget::stop()
{
    if (m_timerID != -1)
        killTimer(m_timerID);
    m_timerID = -1;
    m_newTimestampRequested = false;
}

void DVRWidget::valueChanged(int value)
{
    int delta = (value - m_dialPosition);
    m_dialPosition = value;

    if (delta > DVRWIDGET_DIAL_RANGE / 2)
        delta = DVRWIDGET_DIAL_RANGE - delta;

    if (delta < -DVRWIDGET_DIAL_RANGE / 2)
        delta = DVRWIDGET_DIAL_RANGE + delta;

    timeChange(delta * 5);
}

void DVRWidget::realtimeHistoryClicked()
{
    m_newTimestampRequested = false;
    if (m_realtime) {
        m_realtimeHistory->setText("View live data");
        m_realtime = false;
        enableButtons(true);
    } else {
        m_realtimeHistory->setText("View historic data");
        m_realtime = true;
        m_interval = 0;
        enableButtons(false);
    }
    emit realtime(m_realtime);
}

void DVRWidget::enableButtons(bool enable)
{
    m_playFastReverseButton->setEnabled(enable);
    m_playReverseButton->setEnabled(enable);
    m_stopButton->setEnabled(enable);
    m_playForwardButton->setEnabled(enable);
    m_playFastForwardButton->setEnabled(enable);
    m_dial->setEnabled(enable);
    m_dateEdit->setEnabled(enable);
    m_timeEdit->setEnabled(enable);
    m_slider->setEnabled(enable);
}

void DVRWidget::timerEvent(QTimerEvent *)
{
    if (m_realtime)
        return;

    if (m_interval != 0)
        timeChange(m_interval);

    if (m_newTimestampRequested)
        emit newTimecode(m_ts);
    m_newTimestampRequested = false;
}

void DVRWidget::onControl(int control)
{
    switch (control) {
    case DVRWIDGET_STOP:
        m_interval = 0;
        return;

    case DVRWIDGET_FORWARD:
        m_interval = DVRWIDGET_PLAY_INTERVAL;
        break;

    case DVRWIDGET_REVERSE:
        m_interval = -DVRWIDGET_PLAY_INTERVAL;
        break;

    case DVRWIDGET_FASTFORWARD:
        m_interval = 10 * DVRWIDGET_PLAY_INTERVAL;
        break;

    case DVRWIDGET_FASTREVERSE:
        m_interval = - 10 * DVRWIDGET_PLAY_INTERVAL;
        break;
    }
}

void DVRWidget::dateChanged(int index)
{
    if (m_realtime)
        return;

    if (index == -1)
        return;

    QDateTime dt = QDateTime(m_fileDays.at(index), m_timeEdit->time());
    m_ts = dt.toMSecsSinceEpoch();
    m_newTimestampRequested = true;
}

void DVRWidget::sliderMoved(int value)
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_ts);
    dt.setTime(QTime::fromMSecsSinceStartOfDay(60000 * value));
    m_timeEdit->setTime(dt.time());
    m_ts = dt.toMSecsSinceEpoch();
    updateSlider();
    m_newTimestampRequested = true;
}

void DVRWidget::timeChanged(const QDateTime & /* dateTime */)
{
    if (m_realtime)
        return;

    QDateTime dt = QDateTime(m_fileDays.at(m_dateEdit->currentIndex()), m_timeEdit->time());
    m_ts = dt.toMSecsSinceEpoch();
    updateSlider();
    m_newTimestampRequested = true;
}

void DVRWidget::timeChange(int change)
{
    m_ts += change;

    //  make sure doesn't go past current time

    qint64 curS = QDateTime::currentMSecsSinceEpoch();
    if (curS < m_ts) {
        m_ts = curS;
        return;
    }

    QString dateString = QDateTime::fromMSecsSinceEpoch(m_ts).date().toString(m_dateEditTimeFormat);
    emit setCurrentDateText(dateString);
    m_timeEdit->setTime(QDateTime::fromMSecsSinceEpoch(m_ts).time());
    updateSlider();
    m_newTimestampRequested = true;
}

void DVRWidget::setTimecode(qint64 ts)
{
    m_ts = ts;
    QString dateString = QDateTime::fromMSecsSinceEpoch(m_ts).date().toString(m_dateEditTimeFormat);
    emit setCurrentDateText(dateString);
    m_timeEdit->setTime(QDateTime::fromMSecsSinceEpoch(m_ts).time());
    updateSlider();
}

void DVRWidget::updateSlider()
{
    m_slider->setSliderPosition(m_timeEdit->time().msecsSinceStartOfDay() / 60000);
}

void DVRWidget::enableHistory(bool enable)
{
    m_realtimeHistory->setEnabled(enable);
    if (enable) {

    } else {
        m_realtimeHistory->setText("View historic data");
        m_realtime = true;
        enableButtons(false);
        m_interval = 0;
    }
}

void DVRWidget::newFileDays(QList<QDate> fdl)
{


    m_fileDays = fdl;

    enableHistory(true);

    m_dateEdit->clear();

    for (int i = 0; i < m_fileDays.count(); i++) {
        m_dateEdit->addItem(m_fileDays.at(i).toString(m_dateEditTimeFormat));
    }
    emit setCurrentDateText(m_fileDays.at(m_fileDays.count() - 1).toString(m_dateEditTimeFormat));
}

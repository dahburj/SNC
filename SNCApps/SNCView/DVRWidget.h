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

#ifndef DVRWIDGET_H
#define DVRWIDGET_H

#include <qwidget.h>
#include <qdatetime.h>
#include <qdial.h>
#include <qpushbutton.h>
#include <qsignalmapper.h>
#include <qmutex.h>
#include <qdatetimeedit.h>
#include <qcombobox.h>
#include <qslider.h>

class DVRWidget : public QWidget
{
    Q_OBJECT

public:
    DVRWidget(QWidget *parent = 0);
    void stop();
    void enableHistory(bool enable);

public slots:
    void valueChanged(int value);
    void onControl(int);
    void dateChanged(int index);
    void timeChanged(const QDateTime& dateTime);
    void sliderMoved(int value);
    void realtimeHistoryClicked();
    void setTimecode(qint64 ts);
    void newFileDays(QList<QDate>);

signals:
    void newTimecode(qint64 ts);
    void realtime(bool enable);
    void setCurrentDateText(const QString& text);

protected:
    void timerEvent(QTimerEvent *);

private:
    void timeChange(int change);
    void enableButtons(bool enable);
    void updateSlider();

    QDateTime m_currentTime;

    QDial *m_dial;
    QPushButton *m_stopButton;
    QPushButton *m_playForwardButton;
    QPushButton *m_playReverseButton;
    QPushButton *m_playFastForwardButton;
    QPushButton *m_playFastReverseButton;
    QComboBox *m_dateEdit;
    QString m_dateEditTimeFormat;
    QDateTimeEdit *m_timeEdit;

    QPushButton *m_realtimeHistory;

    QSlider *m_slider;

    QSignalMapper *m_signalMapper;

    int m_dialPosition;

    bool m_realtime = true;

    bool m_newTimestampRequested;

    QMutex m_lock;

    int m_timerID;

    qint64 m_ts;

    int m_interval;

    QList<QDate> m_fileDays;
};

#endif // DVRWIDGET_H

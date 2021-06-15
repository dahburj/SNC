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

#ifndef DIALOGMENU_H
#define DIALOGMENU_H

#include <qdialog.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qpushbutton.h>

#include "SNCJSON.h"

#define DIALOGMENU_WAIT_TICKS       50

class DialogMenu : public QDialog
{
    Q_OBJECT

public:
    DialogMenu(const QString& app, QWidget *parent = 0);
    ~DialogMenu();

public slots:
    void receiveAppData(QJsonObject json);
    void receiveDialogData(QJsonObject json);

private slots:
    void configDialogSelect(int dialogIndex);
    void infoDialogSelect(int dialogIndex);

signals:
    void sendAppData(QJsonObject json);
    void sendDialogData(QJsonObject json);

protected:
    void timerEvent(QTimerEvent *event);

private:
    void layoutWindow();
    void relayoutWindow();
    int m_timer;
    int m_tickCount;
    QLabel *m_label;
    QProgressBar *m_bar;

    QString m_app;

    QJsonArray m_configMenuArray;
    QJsonArray m_infoMenuArray;

    bool m_menuMode;
};

#endif // DIALOGMENU_H

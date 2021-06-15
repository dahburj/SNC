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

#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H

#include <qdialog.h>
#include <qmessagebox.h>
#include <qprogressbar.h>

#include "SNCJSON.h"

#define UPDATEDIALOG_WAIT_TICKS       50

class UpdateDialog : public QDialog
{
    Q_OBJECT

public:
    UpdateDialog(const QString& app, const QString& dialogName, QWidget *parent = 0);
    ~UpdateDialog();

public slots:
    void receiveDialogData(QJsonObject json);

protected:
    void timerEvent(QTimerEvent *event);

private:
    void layoutWindow();
    int m_timer;
    int m_tickCount;
    QProgressBar *m_bar;

    QString m_app;
    QString m_dialogName;
};

#endif // UPDATEDIALOG_H

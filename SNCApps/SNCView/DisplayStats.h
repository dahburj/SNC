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

#ifndef DISPLAYSTATS_H
#define DISPLAYSTATS_H

#include <qdialog.h>
#include <qevent.h>
#include "ui_DisplayStats.h"
#include "AVSource.h"

#define DEFAULT_ROW_HEIGHT 20

class DisplayStats : public QDialog
{
    Q_OBJECT

public:
    DisplayStats(QWidget *parent);
    ~DisplayStats();

    void addSource(AVSource *avSource);
    void removeSource(QString name);

    void deleteAllServices();

protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    void saveDialogState();
    void restoreDialogState();
    QString formatByteTotalForDisplay(qint64 bytes);

    int m_timer;
    QList<AVSource *> m_avSources;

    Ui::CDisplayStats ui;

    QString m_logTag;
};

#endif // DISPLAYSTATS_H

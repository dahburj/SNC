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

#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <qdialog.h>
#include <qpushbutton.h>

#include "Dialog.h"
#include "SNCJSON.h"

class ConfigClient;
class ControlService;

class MainDialog : public QDialog
{
    Q_OBJECT

public:
    MainDialog(QWidget *parent = 0);
    ~MainDialog();

private slots:
    void configDialogSelect(int dialogIndex);
    void infoDialogSelect(int dialogIndex);
    void restartApp();

protected:
    void addStandardDialogs();
    void startServices();

    void timerEvent(QTimerEvent *event);
    void closeEvent(QCloseEvent * event);

    ControlService *m_controlService;
    ConfigClient *m_configClient;

    virtual void appExit() {}

    void setConfigMenuArray(const QJsonArray& menu) { m_configMenuArray = menu; }
    void setInfoMenuArray(const QJsonArray& menu) { m_infoMenuArray = menu; }

private:
    void layoutWindow();

    QJsonArray m_configMenuArray;
    QJsonArray m_infoMenuArray;

    int m_timer;

    bool m_restart;
 };

#endif // MAINDIALOG_H

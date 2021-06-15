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

#ifndef SNCMANAGER_H
#define SNCMANAGER_H

#define	PRODUCT_TYPE "SNCManager"

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qlistwidget.h>
#include <QModelIndex>

#include "ui_SNCManager.h"
#include "ConfigClient.h"

class ControlService;

class SNCManager : public QMainWindow
{
    Q_OBJECT

public:
    SNCManager();

public slots:
    void onAbout();
    void onBasicSetup();
    void dirResponse(QStringList directory);
    void appSelected(const QModelIndex&);

signals:
    void requestDir();
    void newApp(QString AppPath);
    void stopApp();

protected:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    void parseAvailableServices(const QStringList& directory, QStringList& availabelApps);
    void populateApps();
    void layoutWindow();
    void initStatusBar();
    void saveWindowState();
    void restoreWindowState();

    Ui::SNCManagerClass ui;

    ConfigClient *m_client;
    ControlService *m_controlService;
    QStringList m_clientDirectory;

    QLabel *m_controlStatus;

    int m_statusTimer;
    int m_directoryTimer;

    QString m_logTag;


    QStringList m_availableApps;
    QListWidget *m_availableAppList;
};

#endif // SNCMANAGER_H

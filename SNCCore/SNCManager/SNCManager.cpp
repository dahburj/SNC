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

#include <qboxlayout.h>
#include <qdebug.h>

#include "SNCManager.h"
#include "SNCDirectoryEntry.h"

#include "ControlService.h"
#include "BasicSetup.h"
#include "About.h"
#include "GenericDialog.h"
#include "DialogMenu.h"

SNCManager::SNCManager()
    : QMainWindow()
{
    m_logTag = "SNCManager";
    ui.setupUi(this);

    SNCUtils::SNCAppInit();

    layoutWindow();

    m_client = new ConfigClient();
    m_controlService = new ControlService();

    SNCJSON::addConfigDialog(new BasicSetup());
    SNCJSON::addInfoDialog(new About());

    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
    connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));

    connect(m_client, SIGNAL(dirResponse(QStringList)), this, SLOT(dirResponse(QStringList)));
    connect(this, SIGNAL(requestDir()), m_client, SLOT(requestDir()));

    connect(this, SIGNAL(newApp(QString)), m_client, SLOT(newApp(QString)));
    connect(this, SIGNAL(stopApp()), m_client, SLOT(stopApp()));

    connect(m_client, SIGNAL(receiveControlData(QJsonObject, QString, int)),
            m_controlService, SLOT(receiveControlData(QJsonObject, QString, int)));
    connect(m_controlService, SIGNAL(sendControlData(QJsonObject, QString, int)),
            m_client, SLOT(sendControlData(QJsonObject, QString, int)));

    m_client->resumeThread();
    m_controlService->resumeThread();

    m_statusTimer = startTimer(2000);
    m_directoryTimer = startTimer(5000);

    restoreWindowState();
    initStatusBar();

    setWindowTitle(QString("%1 - %2")
        .arg(SNCUtils::getAppType())
        .arg(SNCUtils::getAppName()));

}

void SNCManager::closeEvent(QCloseEvent *)
{
    killTimer(m_statusTimer);
    killTimer(m_directoryTimer);

    if (m_client) {
        m_client->exitThread();
        m_client = NULL;
    }
    if (m_controlService) {
        m_controlService->exitThread();
        m_controlService = NULL;
    }

    saveWindowState();

    SNCUtils::SNCAppExit();
}

void SNCManager::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_directoryTimer) {
        emit requestDir();
    } else {
        m_controlStatus->setText(m_client->getLinkState());
    }
}

void SNCManager::dirResponse(QStringList directory)
{
    if (m_clientDirectory == directory)
        return;

    QStringList availableApps;
    parseAvailableServices(directory, availableApps);
    if (m_availableApps == availableApps)
        return;
    m_availableApps = availableApps;
    populateApps();
}

void SNCManager::parseAvailableServices(const QStringList& directory, QStringList& availableApps)
{
    SNCDirectoryEntry de;
    QString servicePath;
    QString appName;
    QString appSource;

    availableApps.clear();

    for (int i = 0; i < directory.count(); i++) {
        de.setLine(directory.at(i));

        if (!de.isValid())
            continue;

        QStringList services = de.e2eServices();

        for (int i = 0; i < services.count(); i++) {
            servicePath = de.appName() + SNC_SERVICEPATH_SEP + services.at(i);

            SNCUtils::removeStreamNameFromPath(servicePath, appSource, appName);

            if (availableApps.contains(appSource))
                continue;

            if (appName == SNC_STREAMNAME_MANAGE)
                availableApps.append(appSource);
        }
    }
    availableApps.sort();
}

void SNCManager::layoutWindow()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(centralWidget);

    m_availableAppList = new QListWidget();
    m_availableAppList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_availableAppList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    m_availableAppList->setSizePolicy(sizePolicy);
    m_availableAppList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_availableAppList, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(appSelected(const QModelIndex&)));

    vLayout->addWidget(new QLabel("Available Apps - double click to select"));
    vLayout->addWidget(m_availableAppList, 1);

    vLayout->addStretch();

    setCentralWidget(centralWidget);
}

void SNCManager::appSelected(const QModelIndex& index)
{
    QString app;

    if (index.row() >= m_availableApps.count())
        return;
    app = m_availableApps.at(index.row());
    emit newApp(app);
    DialogMenu dlg(app, this);
    connect(&dlg, SIGNAL(sendAppData(QJsonObject)), m_client, SLOT(sendAppData(QJsonObject)));
    connect(m_client, SIGNAL(receiveAppData(QJsonObject)), &dlg, SLOT(receiveAppData(QJsonObject)));
    dlg.exec();
}

void SNCManager::populateApps()
{
    m_availableAppList->clear();
    for (int i = 0; i < m_availableApps.count(); i++)
        m_availableAppList->addItem(new QListWidgetItem(m_availableApps.at(i)));
}

void SNCManager::initStatusBar()
{
    m_controlStatus = new QLabel(this);
    m_controlStatus->setAlignment(Qt::AlignLeft);
    ui.statusBar->addWidget(m_controlStatus, 1);
}

void SNCManager::saveWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("Window");
    settings->setValue("Geometry", saveGeometry());
    settings->setValue("State", saveState());

    settings->endGroup();

    delete settings;
}

void SNCManager::restoreWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("Window");
    restoreGeometry(settings->value("Geometry").toByteArray());
    restoreState(settings->value("State").toByteArray());

    settings->endGroup();

    delete settings;
}

void SNCManager::onAbout()
{
    About about;
    GenericDialog dlg(SNCUtils::getAppName(), &about, QJsonObject(), this);
    dlg.exec();
}

void SNCManager::onBasicSetup()
{
    BasicSetup config;
    GenericDialog dlg(SNCUtils::getAppName(), &config, QJsonObject(), this);
    if (dlg.exec() == QDialog::Accepted) {
        config.setDialog(dlg.getUpdatedDialog());
        config.saveLocalData();
    }
}


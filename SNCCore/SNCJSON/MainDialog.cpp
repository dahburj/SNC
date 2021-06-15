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
#include <qformlayout.h>
#include <qlabel.h>
#include <qsignalmapper.h>
#include <qstylefactory.h>
#include <qprocess.h>
#include <qapplication.h>

#include "MainDialog.h"
#include "GenericDialog.h"
#include "UpdateDialog.h"
#include "SNCUtils.h"
#include "ControlService.h"
#include "ConfigClient.h"

#include "About.h"
#include "BasicSetup.h"
#include "RestartApp.h"

#define TAG "MainDialog"

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_restart = false;
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    SNCUtils::SNCAppInit();

    m_configClient = new ConfigClient();

    m_controlService = new ControlService();
}

void MainDialog::addStandardDialogs()
{
    SNCJSON::addConfigDialog(new BasicSetup());

    RestartApp *restartApp = new RestartApp();
    connect(restartApp, SIGNAL(restartApp()), this, SLOT(restartApp()), Qt::QueuedConnection);
    SNCJSON::addConfigDialog(restartApp);

    SNCJSON::addInfoDialog(new About());
}

void MainDialog::startServices()
{
    m_configMenuArray = SNCJSON::getConfigDialogMenu();
    m_infoMenuArray = SNCJSON::getInfoDialogMenu();

    connect(m_configClient, SIGNAL(receiveControlData(QJsonObject, QString, int)),
            m_controlService, SLOT(receiveControlData(QJsonObject, QString, int)));
    connect(m_controlService, SIGNAL(sendControlData(QJsonObject, QString, int)),
            m_configClient, SLOT(sendControlData(QJsonObject, QString, int)));

    m_configClient->resumeThread();
    m_controlService->resumeThread();

    layoutWindow();
    setWindowTitle(SNCUtils::getAppType() + " - " + SNCUtils::getAppName());

    m_timer = startTimer(1000);                             // initial delay is 1 second
}

MainDialog::~MainDialog()
{
    if (m_timer != -1)
        killTimer(m_timer);
    m_timer = -1;
}

void MainDialog::restartApp()
{
    QCloseEvent event;

    m_restart = true;
    closeEvent(&event);
    QApplication::quit();
}

void MainDialog::closeEvent(QCloseEvent *)
{
    appExit();

    disconnect(m_configClient, SIGNAL(receiveControlData(QJsonObject, QString, int)),
        m_controlService, SLOT(receiveControlData(QJsonObject, QString, int)));
    disconnect(m_controlService, SIGNAL(sendControlData(QJsonObject, QString, int)),
        m_configClient, SLOT(sendControlData(QJsonObject, QString, int)));

    if (m_controlService) {
        m_controlService->exitThread();
        m_controlService = NULL;
    }

    if (m_configClient) {
        m_configClient->exitThread();
        m_configClient = NULL;
    }
    SNCUtils::SNCAppExit();
#ifndef Q_OS_IOS
    if (m_restart) {
        thread()->msleep(1000);
        QStringList args = QApplication::arguments();
        args.removeFirst();
        QProcess::startDetached(QApplication::applicationFilePath(), args);
    }
#endif
    exit(0);
}

void MainDialog::timerEvent(QTimerEvent * /* event */)
{
}

void MainDialog::layoutWindow()
{
    QLabel *label;

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(11, 11, 11, 11);

    QVBoxLayout *configLayout = new QVBoxLayout();
    configLayout->setSpacing(20);
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(20);

    QSignalMapper *configMapper = new QSignalMapper(this);
    QSignalMapper *infoMapper = new QSignalMapper(this);

    //  Add config entries

    label = new QLabel("Configuration options:");
    configLayout->addWidget(label);

    for (int i = 0; i < m_configMenuArray.count(); i++) {
        QPushButton *button = new QPushButton(m_configMenuArray.at(i).toObject().value(SNCJSON_DIALOGMENU_DESC).toString());
        connect(button, SIGNAL(clicked()), configMapper, SLOT(map()));
        configMapper->setMapping(button, i);
        configLayout->addWidget(button);
    }
    connect(configMapper, SIGNAL(mapped(int)), this, SLOT(configDialogSelect(int)));

    configLayout->addStretch(1);

    //  Add info entries

    label = new QLabel("Information options:");
    infoLayout->addWidget(label);

    for (int i = 0; i < m_infoMenuArray.count(); i++) {
        QPushButton *button = new QPushButton(m_infoMenuArray.at(i).toObject().value(SNCJSON_DIALOGMENU_DESC).toString());
        connect(button, SIGNAL(clicked()), infoMapper, SLOT(map()));
        infoMapper->setMapping(button, i);
        infoLayout->addWidget(button);
    }
    connect(infoMapper, SIGNAL(mapped(int)), this, SLOT(infoDialogSelect(int)));
    infoLayout->addStretch(1);

    hLayout->addLayout(configLayout);
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setMinimumWidth(20);
    hLayout->addWidget(line);

    hLayout->addLayout(infoLayout);
    setLayout(hLayout);
}

void MainDialog::configDialogSelect(int dialogIndex)
{
    if (dialogIndex >= m_configMenuArray.count())
        return;
    QString dialogName = m_configMenuArray[dialogIndex].toObject().value(SNCJSON_DIALOGMENU_NAME).toString();
    QString dialogDesc = m_configMenuArray[dialogIndex].toObject().value(SNCJSON_DIALOGMENU_DESC).toString();

    Dialog *configDialog;

    configDialog = SNCJSON::mapNameToDialog(dialogName);
    if (configDialog == NULL)
        return;

    if (!configDialog->isConfigDialog()) {
        SNCUtils::logError(TAG, QString("Got select for info dialog from config menu ") + dialogName);
        return;
    }

    GenericDialog dlg(SNCUtils::getAppName(), configDialog, QJsonObject(), this);

    if (dlg.exec() == QDialog::Accepted) {
        configDialog->setDialog(dlg.getUpdatedDialog());
        configDialog->saveLocalData();
    }
}


void MainDialog::infoDialogSelect(int dialogIndex)
{
    if (dialogIndex >= m_infoMenuArray.count())
        return;
    QString dialogName = m_infoMenuArray[dialogIndex].toObject().value(SNCJSON_DIALOGMENU_NAME).toString();
    QString dialogDesc = m_infoMenuArray[dialogIndex].toObject().value(SNCJSON_DIALOGMENU_DESC).toString();

    Dialog *infoDialog;

    infoDialog = SNCJSON::mapNameToDialog(dialogName);
    if (infoDialog == NULL)
        return;

    if (infoDialog->isConfigDialog()) {
        SNCUtils::logError(TAG, QString("Got select for config dialog from info menu ") + dialogName);
        return;
    }

    GenericDialog dlg(SNCUtils::getAppName(), infoDialog, QJsonObject(), this);

    dlg.exec();
}

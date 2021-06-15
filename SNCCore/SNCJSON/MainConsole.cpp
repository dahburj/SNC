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

#include <qprocess.h>
#include <qapplication.h>

#include "MainConsole.h"
#include "ConfigClient.h"
#include "ControlService.h"
#include "About.h"
#include "BasicSetup.h"
#include "RestartApp.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#endif

volatile bool MainConsole::sigIntReceived = false;

MainConsole::MainConsole(QObject *parent, bool daemonMode)
    : QThread(parent)
{
    m_restart = false;
    m_mustExit = false;
    m_daemonMode = daemonMode;

    SNCUtils::SNCAppInit();

    connect((QCoreApplication *)parent, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    m_configClient = new ConfigClient();
    m_controlService = new ControlService();

}

void MainConsole::addStandardDialogs()
{
    SNCJSON::addConfigDialog(new BasicSetup());

    RestartApp *restartApp = new RestartApp();
    connect(restartApp, SIGNAL(restartApp()), this, SLOT(restartApp()), Qt::QueuedConnection);
    SNCJSON::addConfigDialog(restartApp);

    SNCJSON::addInfoDialog(new About());
}

void MainConsole::startServices()
{
    m_configMenuArray = SNCJSON::getConfigDialogMenu();
    m_infoMenuArray = SNCJSON::getInfoDialogMenu();

    connect(m_configClient, SIGNAL(receiveControlData(QJsonObject, QString, int)),
            m_controlService, SLOT(receiveControlData(QJsonObject, QString, int)));
    connect(m_controlService, SIGNAL(sendControlData(QJsonObject, QString, int)),
            m_configClient, SLOT(sendControlData(QJsonObject, QString, int)));

    m_configClient->resumeThread();
    m_controlService->resumeThread();

#ifndef WIN32
    registerSigHandler();
    if (m_daemonMode)
        SNCUtils::setLogDisplayLevel(SNC_LOG_LEVEL_NONE);
#endif
    start();
}

void MainConsole::stopServices()
{
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
}

void MainConsole::aboutToQuit()
{

    SNCUtils::SNCAppExit();
    for (int i = 0; i < 5; i++) {
        if (wait(1000))
            break;

        printf("Waiting for console thread to exit...\n");
    }
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

void MainConsole::run()
{
    char c;

#ifndef WIN32
    if (m_daemonMode) {
        runDaemon();
        return;
    }

        struct termios	ctty;

        tcgetattr(fileno(stdout), &ctty);
        ctty.c_lflag &= ~(ICANON);
        ctty.c_cc[VMIN] = 1;
        ctty.c_cc[VTIME] = 1;
        tcsetattr(fileno(stdout), TCSANOW, &ctty);

    while (!MainConsole::sigIntReceived && !m_mustExit) {

        printf("\nEnter option: ");

#ifdef WIN32
        c = toupper(_getch());
#else
        fflush(stdout);
        while (!checkForChar()) {
            if (m_mustExit || MainConsole::sigIntReceived)
                break;
        }
        if (m_mustExit || MainConsole::sigIntReceived)
            break;

        c = toupper(getchar());
#endif
        switch (c) {
        case 'X':
            m_mustExit = true;
            break;

        default:
            processInput(c);
        }
    }
#endif
    m_mustExit = true;
    stopServices();
    appExit();
    ((QCoreApplication *)parent())->exit();
}

void MainConsole::restartApp()
{
    m_restart = true;
    m_mustExit = true;
}


#ifndef WIN32
void MainConsole::runDaemon()
{
    while (!MainConsole::sigIntReceived && !m_mustExit)
        msleep(100);

    stopServices();
    appExit();
    ((QCoreApplication *)parent())->exit();
}

void MainConsole::registerSigHandler()
{
    struct sigaction sia;

    bzero(&sia, sizeof sia);
    sia.sa_handler = MainConsole::sigHandler;

    if (sigaction(SIGINT, &sia, NULL) < 0)
        perror("sigaction(SIGINT)");
    if (sigaction(SIGQUIT, &sia, NULL) < 0)
        perror("sigaction(SIGQUIT)");
}

void MainConsole::sigHandler(int)
{
    MainConsole::sigIntReceived = true;
}

bool MainConsole::checkForChar()
{
    struct timeval tv = {0, 100000};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) == 1;
}

#endif


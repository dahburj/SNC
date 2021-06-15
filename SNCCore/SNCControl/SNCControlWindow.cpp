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

#include "SNCControlWindow.h"
#include "SNCUtils.h"
#include "SNCServer.h"
#include "ControlService.h"
#include "ControlSetup.h"
#include "About.h"
#include "LinkStatus.h"
#include "RestartApp.h"
#include "DirectoryStatus.h"

SNCControlWindow::SNCControlWindow() : MainDialog()
{
    m_server = new SNCServer();
    m_server->resumeThread();

    SNCJSON::addConfigDialog(new ControlSetup());

    RestartApp *restartApp = new RestartApp();
    connect(restartApp, SIGNAL(restartApp()), this, SLOT(restartApp()), Qt::QueuedConnection);
    SNCJSON::addConfigDialog(restartApp);

    LinkStatus *linkStatus = new LinkStatus();
    SNCJSON::addInfoDialog(linkStatus);
    SNCJSON::addInfoDialog(new DirectoryStatus(m_server));
    SNCJSON::addInfoDialog(new About());

    startServices();

    connect(m_server, SIGNAL(updateSNCStatusBox(int, QStringList)), linkStatus, SLOT(updateSNCStatusBox(int, QStringList)));
    connect(m_server, SIGNAL(updateSNCDataBox(int, QStringList)), linkStatus, SLOT(updateSNCDataBox(int, QStringList)));
    connect(m_server, SIGNAL(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)),
        linkStatus, SLOT(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);
    connect(m_server, SIGNAL(serverE2EUpdate(qint64, unsigned, qint64, unsigned)),
        linkStatus, SLOT(serverE2EUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);

}

void SNCControlWindow::appExit()
{
    m_server->exitThread();
}

void SNCControlWindow::timerEvent(QTimerEvent *)
{

}


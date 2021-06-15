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

#include "SNCExecWindow.h"
#include "ControlService.h"
#include "ComponentManager.h"

#include "AppStatus.h"
#include "AppConfig.h"

SNCExecWindow::SNCExecWindow() : MainDialog()
{
    m_manager = new ComponentManager();

    addStandardDialogs();

    AppStatus *appStatus = new AppStatus();
    SNCJSON::addInfoDialog(appStatus);

    AppConfig *appConfig = new AppConfig();
    SNCJSON::addToDialogList(appConfig);
    connect(appConfig, SIGNAL(loadComponent(int)), m_manager, SLOT(loadComponent(int)));

    connect(m_manager, SIGNAL(updateExecStatus(int, QStringList)),
        appStatus, SLOT(updateExecStatus(int, QStringList)));

    m_manager->resumeThread();
    startServices();
}

void SNCExecWindow::appExit()
{
    m_manager->exitThread();
}
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

#include "SNCExec.h"
#include "ComponentManager.h"

#include "AppStatus.h"
#include "AppConfig.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif


SNCExec::SNCExec(QObject *parent, bool daemonMode) : MainConsole(parent, daemonMode)
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


void SNCExec::appExit()
{
    m_manager->exitThread();
}

void SNCExec::displayManagedComponents()
{
    ManagerComponent *component;
    int	index;

    printf("\n%-5s %-18s %-17s %-10s\n",
            "Inst", "App name", "UID", "State");
    printf("%-5s %-18s %-17s %-10s\n",
            "----", "--------", "---", "-----");

    index = 0;
    while ((component = m_manager->getComponent(index++)) != NULL) {
        if (component->processState == "unused")
            continue;

        printf("%-5d %-18s %-17s %-10s\n",
            component->instance,
            qPrintable(component->appName),
            qPrintable(SNCUtils::displayUID(&component->UID)),
            qPrintable(component->processState));
    }
}

void SNCExec::showHelp()
{
    printf("\nOptions are:\n\n");
    printf("  H - Display this help page\n");
    printf("  M - Display managed components\n");
    printf("  X - Exit\n");
}

void SNCExec::processInput(char c)
{
    switch (c) {
    case 'H':
        showHelp();
        break;

    case 'M':
        displayManagedComponents();
        break;

    default:
        break;
    }
}


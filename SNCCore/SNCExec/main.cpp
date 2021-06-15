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


#include <QApplication>

#include "SNCExec.h"
#include "SNCExecWindow.h"
#include "SNCUtils.h"


int runGuiApp(int argc, char *argv[]);
int runConsoleApp(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    if (SNCUtils::checkConsoleModeFlag(argc, argv))
        return runConsoleApp(argc, argv);
    else
        return runGuiApp(argc, argv);
}

// look but do not modify argv

int runGuiApp(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SNCUtils::loadStandardSettings(APPTYPE_EXEC, a.arguments());

    SNCExecWindow *w = new SNCExecWindow();

    w->show();

    return a.exec();
}

int runConsoleApp(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    bool daemonMode = SNCUtils::checkDaemonModeFlag(argc, argv);

    SNCUtils::loadStandardSettings(APPTYPE_EXEC, a.arguments());

    SNCExec cc(&a, daemonMode);

    return a.exec();
}


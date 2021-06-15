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

#ifndef SNCEXEC_H
#define SNCEXEC_H

#include "MainConsole.h"
#include "SNCDefs.h"
#include "SNCHello.h"

#define INSTANCE_EXEC   1

//  PARAMS keys

//  Note: The appName is something like "SyntroControl". Extensions like ".exe" for Windows must not be added
//  as SNCExec will automatically add them.

#define SNCEXEC_PARAMS_COMPONENTS        "components"    // the component settings array
#define SNCEXEC_PARAMS_INUSE             "inUse"         // true/false in use flag
#define SNCEXEC_PARAMS_APP_NAME          "appName"       // the Syntro app name
#define SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY "executableDirectory" // the directory containing the Syntro app executable
#define SNCEXEC_PARAMS_WORKING_DIRECTORY "workingDirectory" // the execution directory
#define SNCEXEC_PARAMS_ADAPTOR           "adaptor"       // the network adaptor
#define SNCEXEC_PARAMS_CONSOLE_MODE      "consoleMode"   // true for console mode
#define SNCEXEC_PARAMS_INI_PATH          "iniPath"       // the path name of the .ini file

//  SNCJSON dialog names

#define SNCEXEC_APPSTATUS_NAME           "appStatus"
#define SNCEXEC_APPSTATUS_DESC           "Managed app status"

#define SNCEXEC_APPCONFIG_NAME           "appConfig"
#define SNCEXEC_APPCONFIG_DESC           "Configure managed app"

//  Timer intervals

#define SNCEXEC_INTERVAL                 (SNC_CLOCKS_PER_SEC / 2)   // SNCExec run frequency

#define SNCEXEC_STARTUP_DELAY            (SNCHELLO_INTERVAL * 2)    // enough time to find any apps that shouldn't be running
#define SNCEXEC_HELLO_STARTUP_DELAY      (SNC_CLOCKS_PER_SEC * 5) // how long a component has to send its first hello
#define SNCEXEC_RESTART_INTERVAL         (SNC_CLOCKS_PER_SEC * 5) // at most an attempt every five seconds

class ComponentManager;

class SNCExec : public MainConsole
{
    Q_OBJECT

public:
    SNCExec(QObject *parent, bool daemonMode = false);
    ~SNCExec() {}

signals:
    void loadComponent(int);                                // load specified component

protected:
    void processInput(char c);
    void appExit();

private:
    void displayComponents();
    void displayManagedComponents();
    void showHelp();

    ComponentManager *m_manager;
};

#endif // SNCEXEC_H

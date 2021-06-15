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

#ifndef MAINCONSOLE_H
#define MAINCONSOLE_H

#include <QThread>
#include "SNCJSON.h"

class ControlService;
class ConfigClient;

class MainConsole : public QThread
{
    Q_OBJECT

public:
    MainConsole(QObject *parent, bool daemonMode = false);
    ~MainConsole() {}

public slots:
    void aboutToQuit();
    void restartApp();

protected:
    virtual void appExit() {}
    virtual void processInput(char /* c */) {}

    void run();
    void startServices();
    void addStandardDialogs();

    ControlService *m_controlService;
    ConfigClient *m_configClient;

private:
    void stopServices();
#ifndef WIN32
    bool checkForChar();
    void runDaemon();
    void registerSigHandler();
    static void sigHandler(int sig);
#endif
    static volatile bool sigIntReceived;

    QJsonArray m_configMenuArray;
    QJsonArray m_infoMenuArray;

    bool m_daemonMode;
    bool m_restart;
    bool m_mustExit;

};

#endif // MAINCONSOLE_H

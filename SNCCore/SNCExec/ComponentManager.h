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

#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include <QThread>
#include <QProcess>
#include "SNCComponentData.h"

class ManagerComponent
{
public:

//  params from .ini

    bool inUse;                                             // true if being used
    QString appName;                                        // the Syntro app name
    QString executableDirectory;                            // the directory that contains the Syntro app executable
    QString workingDirectory;                               // the working directory
    QString adaptor;                                        // the network adaptor name
    bool consoleMode;                                       // true if console mode
    QString iniPath;                                        // path to the .ini file

//  internally generated vars

    int instance;                                           // the components instance
    QProcess process;                                       // the actual process
    QString processState;                                   // current state of the process
    qint64 timeStarted;                                     // last time this component was started
    SNC_UID UID;                                            // its UID
};

class ComponentManager : public SNCThread
{
    Q_OBJECT

public:
    ComponentManager();
    ~ComponentManager();

    ManagerComponent *getComponent(int index);              // gets a component object or null if out of range

    QMutex m_lock;                                          // for access to the m_component table

    SNCComponentData m_componentData;

public slots:
    void loadComponent(int index);                          // load specified component

signals:
    void updateExecStatus(int index, QStringList list);

protected:
    void initThread();
    void finishThread();
    void timerEvent(QTimerEvent *event);

private:
    void setDefaults();
    void updateStatus(ManagerComponent *managerComponent);
    int m_timer;

    ManagerComponent *m_components[SNC_MAX_COMPONENTSPERDEVICE]; // the component array
    QString m_extension;                                    // OS-dependent executable extension
    qint64 m_startTime;                                     // used to time the period looking for old apps
    bool m_startMode;                                       // if in start mode

    void killComponents();
    void startModeKillAll();                                // kill all running components at startup
    void findAndKillProcess(QString processName);           // finds and kills processes at startup
    void managerBackground();
    void startComponent(ManagerComponent *component);
};

#endif // COMPONENTMANAGER_H

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

#define TAG "ComponentManager"

ComponentManager::ComponentManager() : SNCThread(TAG)
{

#if defined(Q_OS_WIN64) || defined(Q_OS_WIN32)
    m_extension = ".exe";                                   // Windows needs the extension
#endif

}

ComponentManager::~ComponentManager()
{

}


void ComponentManager::initThread()
{
    setDefaults();
    QSettings *settings = SNCUtils::getSettings();
    m_componentData.init(COMPTYPE_EXEC, settings->value(SNC_PARAMS_HBINTERVAL).toInt());
    delete settings;

    for (int i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        m_components[i] = new ManagerComponent;
        m_components[i]->inUse = false;
        m_components[i]->instance = i;
        m_components[i]->processState = "Unused";
    }

    m_startMode = true;
    m_startTime = SNCUtils::clock();                            // indicate in start mode

    m_timer = startTimer(SNCEXEC_INTERVAL);

    m_components[INSTANCE_EXEC]->inUse = true;
    m_components[INSTANCE_EXEC]->appName = APPTYPE_EXEC;
    m_components[INSTANCE_EXEC]->processState = "Execing";
    m_components[INSTANCE_EXEC]->UID = m_componentData.getMyUID();  // form SNCExec's UID
    SNCUtils::convertIntToUC2(INSTANCE_EXEC, m_components[INSTANCE_EXEC]->UID.instance);
    updateStatus(m_components[INSTANCE_EXEC]);

}

void ComponentManager::finishThread()
{
    killTimer(m_timer);

    killComponents();

    for (int i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        delete m_components[i];
    }
}

void ComponentManager::setDefaults()
{

    QSettings *settings = SNCUtils::getSettings();

    int	size = settings->beginReadArray(SNCEXEC_PARAMS_COMPONENTS);
    settings->endArray();

    if (size == 0) {
        settings->beginWriteArray(SNCEXEC_PARAMS_COMPONENTS);
        settings->setArrayIndex(0);

    // by default, just configure a SyntroControl with default runtime arguments

        settings->setValue(SNCEXEC_PARAMS_INUSE, true);
        settings->setValue(SNCEXEC_PARAMS_APP_NAME, APPTYPE_CONTROL);
#if defined Q_OS_LINUX
        settings->setValue(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY, "/usr/bin");
#elif defined Q_OS_WIN
        settings->setValue(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY, "./");
#else
        settings->setValue(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY, "/Applications");
#endif
        settings->setValue(SNCEXEC_PARAMS_WORKING_DIRECTORY, "");
        settings->setValue(SNCEXEC_PARAMS_ADAPTOR, "");
        settings->setValue(SNCEXEC_PARAMS_INI_PATH, "");
        settings->setValue(SNCEXEC_PARAMS_CONSOLE_MODE, false);

    // and something for SNCExec

        settings->setArrayIndex(1);
        settings->setValue(SNCEXEC_PARAMS_APP_NAME, APPTYPE_EXEC);

        settings->endArray();
    }

    delete settings;

    return;
}


void ComponentManager::killComponents()
{
    int i;

    //	try just terminating all components

    for (i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        if (i == INSTANCE_EXEC)
            continue;
        if (!m_components[i]->inUse)
            continue;
        SNCUtils::logInfo(TAG, QString("Terminating ") + m_components[i]->appName);
        m_components[i]->process.terminate();
    }

    qint64 startTime = SNCUtils::clock();

    // wait for up to five seconds to see if they have all died

    while (!SNCUtils::timerExpired(SNCUtils::clock(), startTime, 2000)) {
        for (i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
            if (i == INSTANCE_EXEC)
                continue;
            if (!m_components[i]->inUse)
                continue;
            SNCUtils::logDebug(TAG, QString("Component %1 state %2").arg(m_components[i]->appName)
                            .arg(m_components[i]->process.waitForFinished(0)));
            if (!m_components[i]->process.waitForFinished(0))
                break;                                      // at least one still running
        }
        if (i == SNC_MAX_COMPONENTSPERDEVICE)
            return;                                         // none are running so exit
        thread()->msleep(100);                              // give the components a chance to terminate and test again
    }

    // kill off anything still running

    for (i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        if (i == INSTANCE_EXEC)
            continue;
        if (!m_components[i]->inUse)
            continue;
        if (!m_components[i]->process.waitForFinished(0)) {
            SNCUtils::logInfo(TAG, QString("Killing ") + m_components[i]->appName);
            m_components[i]->process.kill();
        }
    }
}

void ComponentManager::loadComponent(int index)
{
    ManagerComponent *component;

    QSettings *settings = SNCUtils::getSettings();

    settings->beginReadArray(SNCEXEC_PARAMS_COMPONENTS);
    settings->setArrayIndex(index);
    component = m_components[index];

    //	Instances 0 and 1 are special  so force values if that's what is being dealt with

    if (index == INSTANCE_CONTROL) {
        settings->setValue(SNCEXEC_PARAMS_APP_NAME, APPTYPE_CONTROL);
    }

    if (index == INSTANCE_EXEC) {
        settings->setValue(SNCEXEC_PARAMS_APP_NAME, COMPTYPE_EXEC);
        settings->endArray();
        updateStatus(component);
        delete settings;
        return;
    }

    if (component->inUse) {                                 // if in use, need to check if something critcal has changed
        if (!settings->value(SNCEXEC_PARAMS_INUSE).toBool() ||
            (settings->value(SNCEXEC_PARAMS_APP_NAME).toString() != component->appName) ||
            (settings->value(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY).toString() != component->executableDirectory) ||
            (settings->value(SNCEXEC_PARAMS_WORKING_DIRECTORY).toString() != component->workingDirectory) ||
            (settings->value(SNCEXEC_PARAMS_ADAPTOR).toString() != component->adaptor) ||
            (settings->value(SNCEXEC_PARAMS_INI_PATH).toString() != component->iniPath) ||
            (settings->value(SNCEXEC_PARAMS_CONSOLE_MODE).toBool() != component->consoleMode)) {

        // something critical changed - must kill existing.

            SNCUtils::logInfo(TAG, QString("Killing app %1 by user command").arg(component->appName));
            component->process.terminate();
            thread()->msleep(1000);                         // ugly but it gives the app a chance
            component->process.kill();
            component->inUse = false;
        } else {                                            // everything ok - maybe monitor hellos changed?
            settings->endArray();
            updateStatus(component);                        // make sure display updated
            delete settings;
            return;
        }
    }

    // Safe to read new data in now

    memset(&(component->UID), 0, sizeof(SNC_UID));
    component->inUse = settings->value(SNCEXEC_PARAMS_INUSE, false).toBool();
    component->appName = settings->value(SNCEXEC_PARAMS_APP_NAME).toString();

    component->executableDirectory = settings->value(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY, "").toString();
    component->workingDirectory = settings->value(SNCEXEC_PARAMS_WORKING_DIRECTORY, "").toString();
    component->adaptor = settings->value(SNCEXEC_PARAMS_ADAPTOR, "").toString(); // first adaptor with valid address
    component->iniPath = settings->value(SNCEXEC_PARAMS_INI_PATH, "").toString(); // default .ini in working directory
    component->consoleMode = settings->value(SNCEXEC_PARAMS_CONSOLE_MODE, false).toBool();
    component->timeStarted = SNCUtils::clock() - SNCEXEC_RESTART_INTERVAL; // force immediate attempt

    if (component->workingDirectory.length() == 0)
        component->workingDirectory = component->executableDirectory;

    component->UID = m_componentData.getMyUID();            // form the component's UID
    SNCUtils::convertIntToUC2(component->instance, component->UID.instance);

    if (!settings->value(SNCEXEC_PARAMS_INUSE).toBool() ||
        (settings->value(SNCEXEC_PARAMS_APP_NAME).toString()) == "") {
        settings->endArray();
        component->processState = "unused";
        updateStatus(component);                            // make sure display updated
        delete settings;
        return;
    }

    // If get here, need to start the process

    if (component->iniPath.length() == 0) {
        SNCUtils::logWarn(TAG, QString("Loaded %1 instance %2 with default .ini file").arg(component->appName).arg(component->instance));
    } else {
        SNCUtils::logInfo(TAG, QString("Loaded %1 instance %2, .ini file %3")
            .arg(component->appName).arg(component->instance).arg(component->iniPath));
    }
    component->processState = "Starting...";
    component->inUse = true;
    settings->endArray();
    updateStatus(component);
    delete settings;
}

ManagerComponent *ComponentManager::getComponent(int index)
{
    if (index >= 0 && index < SNC_MAX_COMPONENTSPERDEVICE)
        return m_components[index];

    return NULL;
}

void ComponentManager::managerBackground()
{
    ManagerComponent *component;
    int i;
    qint64 now;

    now = SNCUtils::clock();

    if (m_startMode) {                                      // do special processing in start mode
        if (!SNCUtils::timerExpired(now, m_startTime, SNCEXEC_STARTUP_DELAY))
            return;                                         // do nothing while waiting for timer
        // now exiting start mode - kill anything from this machine generating hellos
        m_startMode = false;
        for (int i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++)
            loadComponent(i);
        startModeKillAll();
        m_components[INSTANCE_EXEC]->processState = "Execing";
        updateStatus(m_components[INSTANCE_EXEC]);

        return;
    }

    for (i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        component = m_components[i];
        if (component->instance == INSTANCE_EXEC)
            continue;                                       // don't process SNCExec's dummy entry
        if (!component->inUse)
            continue;
        m_lock.lock();
        switch (component->process.state()) {
            case QProcess::NotRunning:
                component->processState = "Not running";
                if (SNCUtils::timerExpired(now, component->timeStarted, SNCEXEC_RESTART_INTERVAL))
                    startComponent(component);
                break;

            case QProcess::Starting:
                component->processState = "Starting";
                break;

            case QProcess::Running:
                component->processState = "Running";
                break;
        }
        m_lock.unlock();
        updateStatus(component);
    }
}

void ComponentManager::startComponent(ManagerComponent *component)
{
    QStringList arguments;
    QString argString;
    int i;

    component->process.setWorkingDirectory(component->workingDirectory);

    if (component->consoleMode) {
        arguments << QString("-c");
    }

    if (component->iniPath.length() != 0) {
        arguments << QString("-s") + component->iniPath;
    }

    if (component->adaptor.length() != 0) {
        arguments << QString("-a") + component->adaptor;
    }

    QString executable = component->executableDirectory + "/" + component->appName + m_extension;

    component->process.start(executable, arguments);

    for (i = 0; i < arguments.size(); i++)
        argString += arguments.at(i) + QString(" ");

    SNCUtils::logInfo(TAG, QString("Starting %1 with arguments %2").arg(executable).arg(argString));

    component->timeStarted = SNCUtils::clock();
}


void ComponentManager::startModeKillAll()
{
    int	i;

    //  make sure no instances of apps named in the .ini are running

    for (i = 0; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        if (!m_components[i]->inUse)
            continue;
        if (i == INSTANCE_EXEC)
            continue;                                       // don't kill SNCExec!
        if (m_components[i]->appName.length() == 0)
            continue;
        findAndKillProcess(m_components[i]->appName + m_extension);
    }
}

void ComponentManager::timerEvent(QTimerEvent * /* event */)
{
    managerBackground();
}


void ComponentManager:: updateStatus(ManagerComponent *component)
{
    QMutexLocker locker(&m_lock);

    QStringList list;
    QString configure;
    QString inUse;
    QString appName;
    QString uid;
    QString execState;

    configure = "Configure";
    if (component->inUse) {
        inUse = "yes";
        appName = component->appName;
        uid = SNCUtils::displayUID(&component->UID);
        execState = component->processState;
    } else {
        inUse = "no";
    }
    list.append(configure);
    list.append(inUse);
    list.append(appName);
    list.append(uid);
    list.append(execState);
    emit updateExecStatus(component->instance, list);
}

//----------------------------------------------------------
//
//	Platform specific process killing code

#if defined(Q_OS_WIN32)
#define _MSC_VER 1600
#ifndef WINVER              // Allow use of features specific to Windows XP or later.
#define WINVER 0x0501       // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501 // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS      // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410   // Change this to the appropriate value to target Windows Me or later.
#endif
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <TlHelp32.h>

void ComponentManager::findAndKillProcess(QString processName)
{
    HANDLE processSnapshot = NULL;
    PROCESSENTRY32 pe32 = {0};
    HANDLE hProcess;
    char foundName[1000];

    //  Take a snapshot of all processes in the system.

    processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (processSnapshot == INVALID_HANDLE_VALUE) {
        SNCUtils::logError(TAG, "Failed to get process snapshot handle");
        return;
    }

    //  Fill in the size of the structure before using it.

    pe32.dwSize = sizeof(PROCESSENTRY32);

    //  Walk the snapshot of the processes, and for each process,
    //  display information.

    if (Process32First(processSnapshot, &pe32)) {
        do {
            hProcess = OpenProcess (PROCESS_ALL_ACCESS,
                 FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                wcstombs(foundName, pe32.szExeFile, 1000);
                if (strcmp(qPrintable(processName), foundName) == 0) {  // found it
                    SNCUtils::logInfo(TAG, QString("Start mode killing %1").arg(foundName));
                    TerminateProcess(hProcess, 2);
                }
                CloseHandle (hProcess);
            }
        }
        while (Process32Next(processSnapshot, &pe32));
     }
    CloseHandle (processSnapshot);
}
#elif defined(Q_OS_LINUX)
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

void ComponentManager::findAndKillProcess(QString processName)
{
    DIR 	*d = opendir("/proc");
    FILE 	*fp;
    pid_t 	pid;
    struct 	dirent *de;
    char 	*end;
    char 	cmdl[200], *name;

    while ((de = readdir(d)) != NULL) {
        pid = strtoul(de->d_name, &end, 10);
        if (*end != '\0')
           continue; // skip this dir.
        sprintf( cmdl, "/proc/%d/cmdline", pid);
        fp = fopen(cmdl, "rt");
        if (fp == NULL)
            continue;
        name = fgets(cmdl, (int)sizeof(cmdl), fp);
        fclose(fp);
        if (name == NULL)
            continue;
        if (strstr(cmdl, qPrintable(processName)) != NULL) { // it's a known component
            SNCUtils::logInfo(TAG, QString("Start mode killing  %1").arg(processName));
            kill(pid, SIGKILL);
        }
    }
    closedir(d);
}
#else

void ComponentManager::findAndKillProcess(QString )
{

}
#endif

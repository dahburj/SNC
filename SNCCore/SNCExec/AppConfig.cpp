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

#include "AppConfig.h"
#include "SNCExec.h"
#include "SNCUtils.h"

#define TAG "AppConfig"

AppConfig::AppConfig() : Dialog(SNCEXEC_APPCONFIG_NAME, SNCEXEC_APPCONFIG_DESC)
{
    setConfigDialog(true);
    m_index = 0;
}

void AppConfig::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createConfigBoolVar("inUse", "In use", m_inUse));
    addVar(createConfigAppNameVar("appName", "App name", m_appName));
    addVar(createConfigStringVar("exeDir", "Executable directory", m_executableDirectory));
    addVar(createConfigStringVar("workDir", "Working directory", m_workingDirectory));
    addVar(createConfigStringVar("iniDir", "Settings file", m_iniPath));
    addVar(createConfigSelectionFromListVar("adaptorList", "Ethernet adaptor", m_adaptor, m_adaptorList));
    addVar(createConfigBoolVar("consoleMode", "Console mode", m_consoleMode));
    return dialog(newDialog);
}


bool AppConfig::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "inUse") {
        if (m_inUse != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_inUse = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "appName") {
        if (m_appName != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_appName = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "exeDir") {
        if (m_executableDirectory != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_executableDirectory = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "workDir") {
        if (m_workingDirectory != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_workingDirectory = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "iniDir") {
        if (m_iniPath != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_iniPath = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "adaptorList") {
        if (m_adaptor != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_adaptor = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "consoleMode") {
        if (m_consoleMode != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_consoleMode = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    }
    return changed;
}


void AppConfig::loadLocalData(const QJsonObject& param)
{
    QNetworkInterface cInterface;

    m_index = param.value(SNCJSON_PARAM_INDEX).toInt();
    if ((m_index < 0) || (m_index >= SNC_MAX_COMPONENTSPERDEVICE)) {
        SNCUtils::logError(TAG, QString("loadLocalData for out of range index %1").arg(m_index));
        m_index = 0;
    }

    QSettings *settings = SNCUtils::getSettings();
    settings->beginReadArray(SNCEXEC_PARAMS_COMPONENTS);
    settings->setArrayIndex(m_index);

    m_appName = settings->value(SNCEXEC_PARAMS_APP_NAME).toString();
    m_executableDirectory = settings->value(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY).toString();
    m_workingDirectory = settings->value(SNCEXEC_PARAMS_WORKING_DIRECTORY).toString();
    m_iniPath = settings->value(SNCEXEC_PARAMS_INI_PATH).toString();

    m_inUse = settings->value(SNCEXEC_PARAMS_INUSE).toBool();
    m_consoleMode = settings->value(SNCEXEC_PARAMS_CONSOLE_MODE).toBool();

    m_adaptor = settings->value(SNCEXEC_PARAMS_ADAPTOR).toString();

    m_adaptorList.append("<any>");
    QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
    foreach (cInterface, ani)
        m_adaptorList.append(cInterface.humanReadableName());

    settings->endArray();
    delete settings;
}

void AppConfig::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginWriteArray(SNCEXEC_PARAMS_COMPONENTS);
    settings->setArrayIndex(m_index);

    settings->setValue(SNCEXEC_PARAMS_APP_NAME, m_appName);
    settings->setValue(SNCEXEC_PARAMS_EXECUTABLE_DIRECTORY, m_executableDirectory);
    settings->setValue(SNCEXEC_PARAMS_WORKING_DIRECTORY, m_workingDirectory);

    if (m_adaptor == "<any>")
        settings->setValue(SNCEXEC_PARAMS_ADAPTOR, "");
    else
        settings->setValue(SNCEXEC_PARAMS_ADAPTOR, m_adaptor);

    settings->setValue(SNCEXEC_PARAMS_INI_PATH, m_iniPath);

    settings->setValue(SNCEXEC_PARAMS_INUSE, m_inUse);
    settings->setValue(SNCEXEC_PARAMS_CONSOLE_MODE, m_consoleMode);

    settings->endArray();

    delete settings;

    emit loadComponent(m_index);
}

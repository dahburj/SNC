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

#include <qsettings.h>

#include "ControlSetup.h"
#include "SNCControl.h"
#include "SNCServer.h"

ControlSetup::ControlSetup() : Dialog(SNCCONTROL_CONTROLSETUP_NAME, SNCCONTROL_CONTROLSETUP_DESC)
{
    setConfigDialog(true);
}

bool ControlSetup::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "appName") {
        if (m_appName != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_appName = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
     } else if (name == "priority") {
        int pri = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        if (m_priority != pri) {
            changed = true;
            m_priority = pri;
        }
    } else if (name == "adaptorList") {
        if (m_adaptor != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_adaptor = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "useMACAsUID") {
        if (m_useMACAsUID != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_useMACAsUID = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "uid") {
        if (m_uid != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_uid = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    }
    return changed;
}

void ControlSetup::getDialog(QJsonObject& newDialog)
{
    QNetworkInterface cInterface;
    QStringList adaptorList;

    adaptorList.append("<any>");
    QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
    foreach (cInterface, ani)
        adaptorList.append(cInterface.humanReadableName());

    clearDialog();
    addVar(createConfigAppNameVar("appName", "App name", m_appName));
    addVar(createConfigRangedIntVar("priority", "Priority (0 (lowest) - 255)", m_priority, 0, 255));
    addVar(createConfigSelectionFromListVar("adaptorList", "Ethernet adaptor", m_adaptor, adaptorList));
    addVar(createConfigBoolVar("useMACAsUID", "Use MAC address as UID", m_useMACAsUID));
    addVar(createConfigUIDVar("uid", "Configured UID", m_uid));
    return dialog(newDialog);
}

void ControlSetup::loadLocalData(const QJsonObject& /* param */)
{
    QSettings *settings = SNCUtils::getSettings();

    m_appName = settings->value(SNC_PARAMS_APPNAME).toString();
    m_adaptor = settings->value(SNC_RUNTIME_ADAPTER).toString();
    m_useMACAsUID = settings->value(SNC_PARAMS_UID_USE_MAC).toBool();
    m_uid = settings->value(SNC_PARAMS_UID).toString();

    settings->beginGroup(SNCSERVER_PARAMS_GROUP);
    m_priority = settings->value(SNCSERVER_PARAMS_PRIORITY).toInt();
    settings->endGroup();

    delete settings;
}

void ControlSetup::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->setValue(SNC_PARAMS_APPNAME, m_appName);
    settings->setValue(SNC_PARAMS_UID_USE_MAC, m_useMACAsUID);
    settings->setValue(SNC_PARAMS_UID, m_uid);

    settings->beginGroup(SNCSERVER_PARAMS_GROUP);
    settings->setValue(SNCSERVER_PARAMS_PRIORITY, m_priority);
    settings->endGroup();

    if (m_adaptor == "<any>")
        settings->setValue(SNC_RUNTIME_ADAPTER, "");
    else
        settings->setValue(SNC_RUNTIME_ADAPTER, m_adaptor);

    delete settings;
}


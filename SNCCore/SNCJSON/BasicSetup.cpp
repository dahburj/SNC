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

#include "BasicSetup.h"

BasicSetup::BasicSetup() : Dialog(SNCJSON_DIALOG_NAME_BASICSETUP, SNCJSON_DIALOG_DESC_BASICSETUP)
{
    setConfigDialog(true);
}

bool BasicSetup::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "appName") {
        if (m_appName != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_appName = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "useTunnel") {
        if (m_useTunnel != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_useTunnel = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "tunnelAddr") {
        if (m_tunnelAddr != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_tunnelAddr = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "tunnelPort") {
        if (m_tunnelPort != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_tunnelPort = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "controlPri3") {
        if (m_controlNames[2] != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_controlNames[2] = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "controlPri2") {
        if (m_controlNames[1] != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_controlNames[1] = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "controlPri1") {
        if (m_controlNames[0] != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_controlNames[0] = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "revert") {
        if (m_revert != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_revert = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "adaptorList") {
        if (m_adaptor != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_adaptor = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "encryptLink") {
        if (m_encryptLink != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_encryptLink = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    }
    return changed;
}

void BasicSetup::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createConfigAppNameVar("appName", "App name", m_appName));
    addVar(createGraphicsLineVar());
    addVar(createGraphicsStringVar("Static tunnel:"));
    addVar(createConfigBoolVar("useTunnel", "Use static tunnel", m_useTunnel));
    addVar(createConfigStringVar("tunnelAddr", "Tunnel address", m_tunnelAddr));
    addVar(createConfigRangedIntVar("tunnelPort", "Tunnel port", m_tunnelPort, 0, 65535));
    addVar(createGraphicsLineVar());
    addVar(createGraphicsStringVar("Dynamic link:"));
    addVar(createConfigAppNameVar("controlPri3", "SNCControl (priority 3)", m_controlNames[2]));
    addVar(createConfigAppNameVar("controlPri2", "SNCControl (priority 2)", m_controlNames[1]));
    addVar(createConfigAppNameVar("controlPri1", "SNCControl (priority 1)", m_controlNames[0]));
    addVar(createConfigBoolVar("revert", "Revert to best SNCControl", m_revert));
    addVar(createGraphicsLineVar());
    addVar(createConfigSelectionFromListVar("adaptorList", "Ethernet adaptor", m_adaptor, m_adaptorList));
    addVar(createConfigBoolVar("encryptLink", "Encrypt link", m_encryptLink));
    return dialog(newDialog);
}

void BasicSetup::loadLocalData(const QJsonObject& /* param */)
{
    QNetworkInterface cInterface;

    QSettings *settings = SNCUtils::getSettings();

    m_appName = settings->value(SNC_PARAMS_APPNAME).toString();

    m_useTunnel = settings->value(SNC_PARAMS_USE_TUNNEL).toBool();
    m_tunnelAddr = settings->value(SNC_PARAMS_TUNNEL_ADDR).toString();
    m_tunnelPort = settings->value(SNC_PARAMS_TUNNEL_PORT).toInt();

    settings->beginReadArray(SNC_PARAMS_CONTROL_NAMES);
    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
        settings->setArrayIndex(control);
        m_controlNames[control] = settings->value(SNC_PARAMS_CONTROL_NAME).toString();
    }
    settings->endArray();

    m_revert = settings->value(SNC_PARAMS_CONTROLREVERT).toBool();
    m_encryptLink = settings->value(SNC_PARAMS_ENCRYPT_LINK).toBool();

    m_adaptor = settings->value(SNC_RUNTIME_ADAPTER).toString();
    m_adaptorList.append("<any>");
    QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
    foreach (cInterface, ani)
        m_adaptorList.append(cInterface.humanReadableName());

    delete settings;
}

void BasicSetup::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->setValue(SNC_PARAMS_APPNAME, m_appName);

    settings->setValue(SNC_PARAMS_USE_TUNNEL, m_useTunnel);
    settings->setValue(SNC_PARAMS_TUNNEL_ADDR, m_tunnelAddr);
    settings->setValue(SNC_PARAMS_TUNNEL_PORT, m_tunnelPort);

    settings->beginWriteArray(SNC_PARAMS_CONTROL_NAMES);
    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
        settings->setArrayIndex(control);
        settings->setValue(SNC_PARAMS_CONTROL_NAME, m_controlNames[control]);
    }
    settings->endArray();

    settings->setValue(SNC_PARAMS_CONTROLREVERT, m_revert);
    settings->setValue(SNC_PARAMS_ENCRYPT_LINK, m_encryptLink);

    if (m_adaptor == "<any>")
        settings->setValue(SNC_RUNTIME_ADAPTER, "");
    else
        settings->setValue(SNC_RUNTIME_ADAPTER, m_adaptor);

    delete settings;
}


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

#include "CameraDlg.h"
#include "CameraClient.h"
#include "SNCRTSPCam.h"

#include <qdir.h>

CameraDlg::CameraDlg() : Dialog(SNCRTSPCAM_CONFIGURATION_NAME, SNCRTSPCAM_CONFIGURATION_DESC)
{
    setConfigDialog(true);
}

void CameraDlg::getDialog(QJsonObject& newDialog)
{
    QStringList typeList;
    typeList << RTSP_CAMERA_TYPE_FOSCAM << RTSP_CAMERA_TYPE_SV3CB01 << RTSP_CAMERA_TYPE_SV3CB06;

    clearDialog();
    addVar(createConfigSelectionFromListVar("type", "Camera type", m_type, typeList));
    addVar(createConfigStringVar("ipaddress", "IP address", m_ipAddress));
    addVar(createConfigStringVar("username", "User name", m_username));
    addVar(createConfigPasswordVar("password", "Password", m_password));
    addVar(createConfigIntVar("tcpport", "TCP port", m_tcpPort));
    return dialog(newDialog);
}

bool CameraDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "type") {
        if (m_type != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_type = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "ipaddress") {
        if (m_ipAddress != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_ipAddress = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "username") {
        if (m_username != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_username = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "password") {
        if (m_password != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_password = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "tcpport") {
        if (m_tcpPort != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_tcpPort = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    }

    return changed;
}

void CameraDlg::loadLocalData(const QJsonObject & /* param */)
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

    m_type = settings->value(RTSP_CAMERA_TYPE).toString();
    m_ipAddress = settings->value(RTSP_CAMERA_IPADDRESS).toString();
    m_username = settings->value(RTSP_CAMERA_USERNAME).toString();
    m_password = settings->value(RTSP_CAMERA_PASSWORD).toString();
    m_tcpPort = settings->value(RTSP_CAMERA_TCPPORT).toInt();

    settings->endGroup();
    delete settings;

}

void CameraDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

    settings->setValue(RTSP_CAMERA_TYPE, m_type);
    settings->setValue(RTSP_CAMERA_IPADDRESS, m_ipAddress);
    settings->setValue(RTSP_CAMERA_USERNAME, m_username);
    settings->setValue(RTSP_CAMERA_PASSWORD, m_password);
    settings->setValue(RTSP_CAMERA_TCPPORT, m_tcpPort);
    settings->endGroup();
    delete settings;

    emit newCamera();
}

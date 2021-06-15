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
#include "SNCLCam.h"

#include <qdir.h>

CameraDlg::CameraDlg() : Dialog(SNCLCAM_CONFIGURATION_NAME, SNCLCAM_CONFIGURATION_DESC)
{
    setConfigDialog(true);
}

void CameraDlg::getDialog(QJsonObject& newDialog)
{
    QStringList deviceList = getVideoDeviceList();

    clearDialog();
    addVar(createConfigSelectionFromListVar("device", "Camera device", m_device, deviceList));
    addVar(createConfigRangedIntVar("width", "Frame width", m_width, 120, 3840));
    addVar(createConfigRangedIntVar("height", "Frame height", m_height, 80, 2160));
    addVar(createConfigRangedIntVar("rate", "Frame rate (fps)", m_rate, 1, 100));
    return dialog(newDialog);
}

bool CameraDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "device") {
        if (m_device != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_device = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "width") {
        if (m_width != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_width = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "height") {
        if (m_height != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_height = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "rate") {
        if (m_rate != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_rate = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    }

    return changed;
}

void CameraDlg::loadLocalData(const QJsonObject & /* param */)
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

    m_device = settings->value(CAMERA_CAMERA).toString();
    m_width = settings->value(CAMERA_WIDTH).toInt();
    m_height = settings->value(CAMERA_HEIGHT).toInt();
    m_rate = settings->value(CAMERA_FRAMERATE).toInt();

    settings->endGroup();
    delete settings;

}

void CameraDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERA_GROUP);

    settings->setValue(CAMERA_CAMERA, m_device);
    settings->setValue(CAMERA_WIDTH, m_width);
    settings->setValue(CAMERA_HEIGHT, m_height);
    settings->setValue(CAMERA_FRAMERATE, m_rate);
    settings->endGroup();
    delete settings;

    emit newCamera();
}

QStringList CameraDlg::getVideoDeviceList()
{
    QStringList list;
    QDir dir;
    QStringList nameFilter;

    QSettings *settings = SNCUtils::getSettings();
    settings->beginGroup(CAMERA_GROUP);

    dir.setPath("/dev");

    nameFilter << "video*";

    list = dir.entryList(nameFilter, QDir::System | QDir::Readable | QDir::Writable, QDir::Name);

    settings->endGroup();
    delete settings;

    return list;
}

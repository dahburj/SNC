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

#include "MotionDlg.h"
#include "CameraClient.h"

MotionDlg::MotionDlg() : Dialog(MOTION_DIALOG_NAME, MOTION_DIALOG_DESC)
{
    setConfigDialog(true);
}

void MotionDlg::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createConfigRangedIntVar("motionDelta", "Motion check interval (mS) - 0 disables motion detection",
                                    m_motionDelta, 33, 10000));
    addVar(createConfigRangedIntVar("minDelta", "Min image change delta", m_minDelta, 0, 4000));
    addVar(createConfigRangedIntVar("preroll", "Preroll time (mS)", m_preroll, 200, 10000));
    addVar(createConfigRangedIntVar("postroll", "Postroll time (mS)", m_postroll, 200, 10000));
    return dialog(newDialog);
}

bool MotionDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "motionDelta") {
        if (m_motionDelta != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_motionDelta = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "minDelta") {
        if (m_minDelta != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_minDelta = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "preroll") {
        if (m_preroll != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_preroll = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "postroll") {
        if (m_postroll != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_postroll = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    }

    return changed;
}


void MotionDlg::loadLocalData(const QJsonObject &)
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_MOTION_GROUP);

    m_motionDelta = settings->value(CAMERACLIENT_MOTION_DELTA_INTERVAL).toInt();
    m_minDelta = settings->value(CAMERACLIENT_MOTION_MIN_DELTA).toInt();
    m_preroll = settings->value(CAMERACLIENT_MOTION_PREROLL).toInt();
    m_postroll = settings->value(CAMERACLIENT_MOTION_POSTROLL).toInt();

    settings->endGroup();

    delete settings;
}


void MotionDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_MOTION_GROUP);

    settings->setValue(CAMERACLIENT_MOTION_MIN_DELTA, m_minDelta);
    settings->setValue(CAMERACLIENT_MOTION_DELTA_INTERVAL, m_motionDelta);
    settings->setValue(CAMERACLIENT_MOTION_PREROLL, m_preroll);
    settings->setValue(CAMERACLIENT_MOTION_POSTROLL, m_postroll);

    settings->endGroup();

    delete settings;

    emit newStream();
}

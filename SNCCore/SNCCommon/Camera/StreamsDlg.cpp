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

#include "StreamsDlg.h"
#include "CameraClient.h"

StreamsDlg::StreamsDlg() : Dialog(STREAMS_DIALOG_NAME, STREAMS_DIALOG_DESC)
{
    setConfigDialog(true);
}

void StreamsDlg::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createConfigRangedIntVar("highRateMinInterval", "High rate min interval (mS)", m_highRateMinInterval, 33, 10000));
    addVar(createConfigRangedIntVar("highRateMaxInterval", "High rate max interval (mS)", m_highRateMaxInterval, 10000, 60000));
    addVar(createConfigRangedIntVar("highRateNullInterval", "High rate null interval (mS)", m_highRateNullInterval, 1000, 10000));
    addVar(createConfigBoolVar("generateLowRate", "Generate low rate", m_generateLowRate));
    addVar(createConfigBoolVar("lowRateHalfRes", "Low rate at half resolution", m_lowRateHalfRes));
    addVar(createConfigRangedIntVar("lowRateMinInterval", "Low rate min interval (mS)", m_lowRateMinInterval, 33, 10000));
    addVar(createConfigRangedIntVar("lowRateMaxInterval", "Low rate max interval (mS)", m_lowRateMaxInterval, 10000, 60000));
    addVar(createConfigRangedIntVar("lowRateNullInterval", "Low rate null interval (mS)", m_lowRateNullInterval, 1000, 10000));
    addVar(createConfigBoolVar("generateRaw", "Generate raw jpeg stream", m_generateRaw));
    return dialog(newDialog);
}

bool StreamsDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "highRateMinInterval") {
        if (m_highRateMinInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_highRateMinInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "highRateMaxInterval") {
        if (m_highRateMaxInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_highRateMaxInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "highRateNullInterval") {
        if (m_highRateNullInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_highRateNullInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "generateLowRate") {
        if (m_generateLowRate != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_generateLowRate = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "lowRateHalfRes") {
        if (m_lowRateHalfRes != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_lowRateHalfRes = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "lowRateMinInterval") {
        if (m_lowRateMinInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_lowRateMinInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "lowRateMaxInterval") {
        if (m_lowRateMaxInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_lowRateMaxInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "lowRateNullInterval") {
        if (m_lowRateNullInterval != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_lowRateNullInterval = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "generateRaw") {
        if (m_generateRaw != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_generateRaw = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    }

    return changed;
}

void StreamsDlg::loadLocalData(const QJsonObject & /* param */)
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_STREAM_GROUP);

    m_highRateMinInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL).toInt();
    m_highRateMaxInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL).toInt();
    m_highRateNullInterval = settings->value(CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL).toInt();
    m_generateLowRate = settings->value(CAMERACLIENT_GENERATE_LOWRATE).toBool();
    m_lowRateHalfRes = settings->value(CAMERACLIENT_LOWRATE_HALFRES).toBool();
    m_lowRateMinInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_MININTERVAL).toInt();
    m_lowRateMaxInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL).toInt();
    m_lowRateNullInterval = settings->value(CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL).toInt();
    m_generateRaw = settings->value(CAMERACLIENT_GENERATE_RAW).toBool();

    settings->endGroup();

    delete settings;
}


void StreamsDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(CAMERACLIENT_STREAM_GROUP);

    settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_MININTERVAL, m_highRateMinInterval);
    settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_MAXINTERVAL, m_highRateMaxInterval);
    settings->setValue(CAMERACLIENT_HIGHRATEVIDEO_NULLINTERVAL, m_highRateNullInterval);
    settings->setValue(CAMERACLIENT_GENERATE_LOWRATE, m_generateLowRate);
    settings->setValue(CAMERACLIENT_LOWRATE_HALFRES, m_lowRateHalfRes);
    settings->setValue(CAMERACLIENT_LOWRATEVIDEO_MININTERVAL, m_lowRateMinInterval);
    settings->setValue(CAMERACLIENT_LOWRATEVIDEO_MAXINTERVAL, m_lowRateMaxInterval);
    settings->setValue(CAMERACLIENT_LOWRATEVIDEO_NULLINTERVAL, m_lowRateNullInterval);
    settings->setValue(CAMERACLIENT_GENERATE_RAW, m_generateRaw);

    settings->endGroup();

    delete settings;
    emit newStream();
}

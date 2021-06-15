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

#include "StoreStreamDlg.h"
#include "StoreStream.h"
#include "SNCStore.h"
#include "SNCUtils.h"

#define TAG "StoreStreamDlg"

StoreStreamDlg::StoreStreamDlg() : Dialog(SNCSTORE_STREAMCONFIG_NAME, SNCSTORE_STREAMCONFIG_DESC)
{
    setConfigDialog(true);
}

void StoreStreamDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);
    settings->setArrayIndex(m_index);

    settings->setValue(SNCSTORE_PARAMS_INUSE, m_inUse);
    settings->setValue(SNCSTORE_PARAMS_FORMAT, m_format);
    settings->setValue(SNCSTORE_PARAMS_CREATE_SUBFOLDER, m_subFolder);
    settings->setValue(SNCSTORE_PARAMS_STREAM_SOURCE, m_streamName);

    settings->setValue(SNCSTORE_PARAMS_ROTATION_POLICY, m_rotationPolicy);

    settings->setValue(SNCSTORE_PARAMS_ROTATION_TIME_UNITS, m_rotationTimeUnits);

    settings->setValue(SNCSTORE_PARAMS_ROTATION_TIME, m_rotationTime);
    settings->setValue(SNCSTORE_PARAMS_ROTATION_SIZE, m_rotationSize);

    //	Deletion policy

    settings->setValue(SNCSTORE_PARAMS_DELETION_POLICY, m_deletionPolicy);

    settings->setValue(SNCSTORE_PARAMS_DELETION_TIME_UNITS, m_deletionTimeUnits);

    settings->setValue(SNCSTORE_PARAMS_DELETION_TIME, m_deletionTime);
    settings->setValue(SNCSTORE_PARAMS_DELETION_COUNT, m_deletionSize);

    settings->endArray();

    delete settings;

    emit refreshStreamSource(m_index);
}

void StoreStreamDlg::loadLocalData(const QJsonObject& param)
{
    m_index = param.value(SNCJSON_PARAM_INDEX).toInt();

    QSettings *settings = SNCUtils::getSettings();

    settings->beginReadArray(SNCSTORE_PARAMS_STREAM_SOURCES);
    settings->setArrayIndex(m_index);

    m_inUse = settings->value(SNCSTORE_PARAMS_INUSE).toBool();

    m_streamName = settings->value(SNCSTORE_PARAMS_STREAM_SOURCE).toString();

    m_format = settings->value(SNCSTORE_PARAMS_FORMAT).toString();

    m_subFolder = settings->value(SNCSTORE_PARAMS_CREATE_SUBFOLDER).toBool();

    //	Rotation policy

    m_rotationPolicy = settings->value(SNCSTORE_PARAMS_ROTATION_POLICY).toString();

    m_rotationTimeUnits = settings->value(SNCSTORE_PARAMS_ROTATION_TIME_UNITS).toString();

    m_rotationTime = settings->value(SNCSTORE_PARAMS_ROTATION_TIME).toInt();
    m_rotationSize = settings->value(SNCSTORE_PARAMS_ROTATION_SIZE).toInt();

    //	Deletion policy

    m_deletionPolicy = settings->value(SNCSTORE_PARAMS_DELETION_POLICY).toString();

    m_deletionTimeUnits = settings->value(SNCSTORE_PARAMS_DELETION_TIME_UNITS).toString();

    m_deletionTime = settings->value(SNCSTORE_PARAMS_DELETION_TIME).toInt();
    m_deletionSize = settings->value(SNCSTORE_PARAMS_DELETION_COUNT).toInt();

    settings->endArray();

    delete settings;

}

void StoreStreamDlg::getDialog(QJsonObject& newDialog)
{
    QStringList formatList, rotationPolicyList, rotationTimeUnitsList, deletionPolicyList, deletionTimeUnitsList;
    formatList << SNC_RECORD_STORE_FORMAT_SRF << SNC_RECORD_STORE_FORMAT_RAW;

    rotationPolicyList << SNCSTORE_PARAMS_ROTATION_POLICY_TIME << SNCSTORE_PARAMS_ROTATION_POLICY_SIZE
                          << SNCSTORE_PARAMS_ROTATION_POLICY_ANY;

    rotationTimeUnitsList << SNCSTORE_PARAMS_ROTATION_TIME_UNITS_MINUTES << SNCSTORE_PARAMS_ROTATION_TIME_UNITS_HOURS
                     << SNCSTORE_PARAMS_ROTATION_TIME_UNITS_DAYS;

    deletionPolicyList << SNCSTORE_PARAMS_DELETION_POLICY_TIME << SNCSTORE_PARAMS_DELETION_POLICY_COUNT
                          << SNCSTORE_PARAMS_DELETION_POLICY_ANY;

    deletionTimeUnitsList << SNCSTORE_PARAMS_DELETION_TIME_UNITS_HOURS << SNCSTORE_PARAMS_DELETION_TIME_UNITS_DAYS;

    clearDialog();
    addVar(createConfigBoolVar("inUse", "Enable entry", m_inUse));
    addVar(createConfigSelectionFromListVar("storeFormat", "Store format", m_format, formatList));
    addVar(createConfigServicePathVar("streamPath", "Stream path", m_streamName));
    addVar(createConfigBoolVar("subDir", "Create sub directory", m_subFolder));
    addVar(createConfigSelectionFromListVar("rotPolicy", "Rotation policy", m_rotationPolicy, rotationPolicyList));
    addVar(createConfigSelectionFromListVar("rotTimeUnits", "Rotation time units", m_rotationTimeUnits, rotationTimeUnitsList));
    addVar(createConfigIntVar("rotTime", "Rotation time", m_rotationTime));
    addVar(createConfigIntVar("rotSize", "Rotation size (MB)", m_rotationSize));
    addVar(createConfigSelectionFromListVar("delPolicy", "Deletion policy", m_deletionPolicy, deletionPolicyList));
    addVar(createConfigIntVar("delTime", "Deletion time", m_deletionTime));
    addVar(createConfigIntVar("delCount", "Max files to keep", m_deletionSize));

    return dialog(newDialog);
}


bool StoreStreamDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "inUse") {
        if (m_inUse != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_inUse = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
     } else if (name == "storeFormat") {
        if (m_format != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_format = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "streamPath") {
        if (m_streamName != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_streamName = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "subDir") {
        if (m_subFolder != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_subFolder = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "rotPolicy") {
        if (m_rotationPolicy != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_rotationPolicy = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "rotTimeUnits") {
        if (m_rotationTimeUnits != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_rotationTimeUnits = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "rotTime") {
        if (m_rotationTime != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_rotationTime = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "rotSize") {
        if (m_rotationSize != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_rotationSize = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "delPolicy") {
        if (m_deletionPolicy != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_deletionPolicy = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "delTime") {
        if (m_deletionTime != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_deletionTime = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    } else if (name == "delCount") {
        if (m_deletionSize != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
            changed = true;
            m_deletionSize = var.value(SNCJSON_CONFIG_VAR_VALUE).toInt();
        }
    }

    return changed;
}


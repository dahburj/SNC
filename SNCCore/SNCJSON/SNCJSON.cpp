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

#include "SNCJSON.h"
#include "Dialog.h"
#include "SNCUtils.h"

#define TAG "SNCJSON"

int SNCJSON::staticRecordIndex = 0;
QJsonArray SNCJSON::m_configDialogMenu;
QJsonArray SNCJSON::m_infoDialogMenu;
QList<Dialog *> SNCJSON::m_dialogs;

SNCJSON::SNCJSON()
{
}

void SNCJSON::addArray(const QJsonArray& array, const QString& key)
{
    m_jsonRecord[key] = array;
}

void SNCJSON::addVar(const QJsonObject& var, const QString& key)
{
    m_jsonRecord[key] = var;
}

QJsonObject SNCJSON::completeRequest(const QString& type, const QString& command, const QJsonObject& param)
{
    m_jsonRecord[SNCJSON_RECORD_TYPE] = type;
    m_jsonRecord[SNCJSON_RECORD_COMMAND] = command;
    m_jsonRecord[SNCJSON_RECORD_PARAM] = param;
    m_jsonRecord[SNCJSON_RECORD_INDEX] = staticRecordIndex++;
    return m_jsonRecord;
}

QJsonObject SNCJSON::completeResponse(const QString& type, const QString& command, const QJsonObject& param, int recordIndex)
{
    m_jsonRecord[SNCJSON_RECORD_TYPE] = type;
    m_jsonRecord[SNCJSON_RECORD_COMMAND] = command;
    m_jsonRecord[SNCJSON_RECORD_PARAM] = param;
    m_jsonRecord[SNCJSON_RECORD_INDEX] = recordIndex;
    return m_jsonRecord;
}

void SNCJSON::displayJson(QJsonObject json)
{
    SNCUtils::logDebug(TAG, QJsonDocument(json).toJson());
}

void SNCJSON::addConfigDialog(Dialog *dialog)
{
    QJsonObject dme;

    m_dialogs.append(dialog);
    dme[SNCJSON_DIALOGMENU_NAME] = dialog->getName();
    dme[SNCJSON_DIALOGMENU_DESC] = dialog->getDesc();
    m_configDialogMenu.append(dme);
}

void SNCJSON::addInfoDialog(Dialog *dialog)
{
    QJsonObject dme;

    m_dialogs.append(dialog);
    dme[SNCJSON_DIALOGMENU_NAME] = dialog->getName();
    dme[SNCJSON_DIALOGMENU_DESC] = dialog->getDesc();
    m_infoDialogMenu.append(dme);
}

Dialog *SNCJSON::mapNameToDialog(const QString &name)
{
    for (int i = 0; i < m_dialogs.count(); i++) {
        if (name == m_dialogs.at(i)->getName())
            return m_dialogs.at(i);
    }
    return NULL;
}


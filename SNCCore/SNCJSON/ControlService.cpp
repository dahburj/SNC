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

#include "ControlService.h"
#include "About.h"
#include "BasicSetup.h"

#define TAG "ControlService"

ControlService::ControlService() : SNCThread(TAG)
{
}

void ControlService::initThread()
{
}

void ControlService::finishThread()
{

}


void ControlService::receiveControlData(QJsonObject json, QString remoteUID, int remotePort)
{
    if (!json.contains(SNCJSON_RECORD_TYPE))
        return;

    QString type = json.value(SNCJSON_RECORD_TYPE).toString();

    if (type == SNCJSON_RECORD_TYPE_PING) {
        if (!json.contains(SNCJSON_RECORD_COMMAND))
            return;
        QString command = json.value(SNCJSON_RECORD_COMMAND).toString();

        if (command == SNCJSON_COMMAND_REQUEST) {
            json[SNCJSON_RECORD_COMMAND] = SNCJSON_COMMAND_RESPONSE;
            emit sendControlData(json, remoteUID, remotePort);   // just send it back
        }
    } else if (type == SNCJSON_RECORD_TYPE_DIALOGMENU) {
        if (!json.contains(SNCJSON_RECORD_COMMAND))
            return;
        QString command = json.value(SNCJSON_RECORD_COMMAND).toString();

        if (command == SNCJSON_COMMAND_REQUEST) {
            processDialogMenuRequest(json, remoteUID, remotePort);
        }
    } else if (type == SNCJSON_RECORD_TYPE_DIALOG) {
        if (!json.contains(SNCJSON_RECORD_COMMAND))
            return;
        QString command = json.value(SNCJSON_RECORD_COMMAND).toString();

        if (command == SNCJSON_COMMAND_REQUEST) {
            processDialogRequest(json, remoteUID, remotePort);
        }
    } else if (type == SNCJSON_RECORD_TYPE_DIALOGUPDATE) {
        if (!json.contains(SNCJSON_RECORD_COMMAND))
            return;
        QString command = json.value(SNCJSON_RECORD_COMMAND).toString();

        if (command == SNCJSON_COMMAND_REQUEST) {
            processDialogUpdateRequest(json, remoteUID, remotePort);
        }
    }
}

void ControlService::processDialogMenuRequest(const QJsonObject& request, const QString& remoteUID, int remotePort)
{
    SNCJSON js;
    QJsonObject jo;

    jo[SNCJSON_DIALOGMENU_CONFIG] = SNCJSON::getConfigDialogMenu();
    jo[SNCJSON_DIALOGMENU_INFO] = SNCJSON::getInfoDialogMenu();
    js.addVar(jo, SNCJSON_RECORD_DATA);

    emit sendControlData(js.completeResponse(SNCJSON_RECORD_TYPE_DIALOGMENU,
                                             SNCJSON_COMMAND_RESPONSE,
                                             request[SNCJSON_RECORD_PARAM].toObject(),
                                             (int)request[SNCJSON_RECORD_INDEX].toDouble()),
                         remoteUID, remotePort);
}

void ControlService::processDialogRequest(const QJsonObject& request, const QString& remoteUID, int remotePort)
{
    SNCJSON js;
    Dialog *dialog;

    QJsonObject param = request[SNCJSON_RECORD_PARAM].toObject();
    QString dialogName = param[SNCJSON_PARAM_DIALOG_NAME].toString();

    dialog = SNCJSON::mapNameToDialog(dialogName);

    if (dialog == NULL) {
        SNCUtils::logError(TAG, QString("No match for dialog name ") + dialogName);
        return;
    }

    if (dialog->isConfigDialog()) {
        dialog->loadLocalData(param);
        QJsonObject json;
        dialog->getDialog(json);
        js.addVar(json, SNCJSON_RECORD_DATA);
        emit sendControlData(js.completeResponse(SNCJSON_RECORD_TYPE_DIALOG,
                                                 SNCJSON_COMMAND_RESPONSE,
                                                 param,
                                                 (int)request[SNCJSON_RECORD_INDEX].toDouble()),
                                                 remoteUID, remotePort);
    } else {
        dialog->loadLocalData(param);
        QJsonObject json;
        dialog->getDialog(json);
        js.addVar(json, SNCJSON_RECORD_DATA);
        emit sendControlData(js.completeResponse(SNCJSON_RECORD_TYPE_DIALOG,
                                                 SNCJSON_COMMAND_RESPONSE,
                                                 param,
                                                 (int)request[SNCJSON_RECORD_INDEX].toDouble()),
                                                 remoteUID, remotePort);
    }
}

void ControlService::processDialogUpdateRequest(const QJsonObject& request, const QString& remoteUID, int remotePort)
{
    SNCJSON js;
    Dialog *config;

    QJsonObject param = request[SNCJSON_RECORD_PARAM].toObject();
    QString dialogName = param[SNCJSON_PARAM_DIALOG_NAME].toString();

    config = SNCJSON::mapNameToDialog(dialogName);

    if (config == NULL) {
        SNCUtils::logError(TAG, QString("No match for dialog name ") + dialogName);
        return;
    }

    if (!config->isConfigDialog()) {
        SNCUtils::logError(TAG, QString("Got request to update info dialog ") + dialogName);
        return;
    }
    config->loadLocalData(param);

    if (!request.contains(SNCJSON_RECORD_DATA)) {
        return;
    }

    QJsonObject dialog = request[SNCJSON_RECORD_DATA].toObject();
    if (dialog[SNCJSON_DIALOG_NAME].toString() != dialogName) {
        return;
    }

    if (config->setDialog(dialog))
        config->saveLocalData();

    emit sendControlData(js.completeResponse(SNCJSON_RECORD_TYPE_DIALOGUPDATE,
                                             SNCJSON_COMMAND_RESPONSE,
                                             param,
                                             (int)request[SNCJSON_RECORD_INDEX].toDouble()),
                                             remoteUID, remotePort);
}



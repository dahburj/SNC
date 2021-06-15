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

#ifndef SNCJSON_H
#define SNCJSON_H

#include <qstring.h>
#include <qstringlist.h>
#include "SNCJSONDefs.h"

class Dialog;

class SNCJSON
{
public:
    SNCJSON();
    QJsonObject completeRequest(const QString& type, const QString& command, const QJsonObject& param);
    QJsonObject completeResponse(const QString& type, const QString& command, const QJsonObject& param, int recordIndex);

    void addArray(const QJsonArray& array, const QString& key);

    void addVar(const QJsonObject& var, const QString& key);

    static void displayJson(QJsonObject json);
    static const QJsonArray& getConfigDialogMenu() { return m_configDialogMenu; }
    static const QJsonArray& getInfoDialogMenu() { return m_infoDialogMenu; }

    static void addConfigDialog(Dialog *dialog);
    static void addInfoDialog(Dialog *dialog);
    static void addToDialogList(Dialog *dialog) { m_dialogs.append(dialog); }
    static Dialog *mapNameToDialog(const QString& name);

private:
    QJsonObject m_jsonRecord;

    static int staticRecordIndex;
    static QJsonArray m_configDialogMenu;
    static QJsonArray m_infoDialogMenu;

    static QList<Dialog *> m_dialogs;
};

#endif // SNCJSON_H

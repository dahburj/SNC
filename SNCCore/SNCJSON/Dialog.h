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

#ifndef DIALOG_H
#define DIALOG_H

#include "SNCJSON.h"
#include <qdialog.h>

class Dialog : public QObject
{
    Q_OBJECT

public:
    Dialog(const QString& name, const QString& desc);
    virtual ~Dialog() {}

    bool isConfigDialog() { return m_configDialog; }
    void setConfigDialog(bool configDialog) { m_configDialog = configDialog; }

    const QString& getName() { return m_name; }
    const QString& getDesc() { return m_desc; }

    bool setDialog(const QJsonObject& json );

    //  These four functions should be overridden as required

    virtual void getDialog(QJsonObject& /* json */) {}
    virtual bool setVar(const QString& /* name */, const QJsonObject& /* var */) { return false; } // returns true if caused a change
    virtual void loadLocalData(const QJsonObject& /* param */) {}
    virtual void saveLocalData() {}

protected:
    void clearDialog();
    void dialog(QJsonObject& newConfig, bool updateFlag = false, const QString& cookie = QString());
    void addVar(const QJsonObject& var);

    //  Config vars

    QJsonObject createConfigStringVar(const QString& name, const QString& desc, const QString& value);
    QJsonObject createConfigPasswordVar(const QString& name, const QString& desc, const QString& value);
    QJsonObject createConfigUIDVar(const QString& name, const QString& desc, const QString& value);
    QJsonObject createConfigSelectionFromListVar(const QString& name, const QString& desc, const QString& value, const QStringList& list);
    QJsonObject createConfigMultiStringFromListVar(const QString& name, const QString& desc, const QStringList& value, const QStringList& list);
    QJsonObject createConfigAppNameVar(const QString& name, const QString& desc, const QString& appName);
    QJsonObject createConfigServicePathVar(const QString& name, const QString& desc, const QString& path);
    QJsonObject createConfigBoolVar(const QString& name, const QString& desc, bool value);
    QJsonObject createConfigIntVar(const QString& name, const QString& desc, int value);
    QJsonObject createConfigRangedIntVar(const QString& name, const QString& desc, int value, int minimum, int maxiumum);
    QJsonObject createConfigHexVar(const QString& name, const QString& desc, unsigned int value);
    QJsonObject createConfigRangedHexVar(const QString& name, const QString& desc, unsigned int value, unsigned int minimum, unsigned int maxiumum);
    QJsonObject createConfigButtonVar(const QString& name, const QString& desc, const QString& dialogName);
    QJsonObject createConfigDOWVar(const QString& name, const QString& desc, unsigned char value);

    //  info vars

    QJsonObject createInfoStringVar(const QString& desc, const QString& value);
    QJsonObject createInfoButtonVar(const QString& name, const QString& desc, const QString& dialogName);
    QJsonObject createInfoTableVar(const QString& desc, const QStringList& columnHeaders, const QList<int>& columnWidths,
                                   const QStringList& data, const QString& dialogName = "", int configColumn = -1);

    //  graphics vars

    QJsonObject createGraphicsStringVar(const QString& value, const QString& style = "");
    QJsonObject createGraphicsLineVar();

    QString m_name;
    QString m_desc;

private:
    QJsonArray m_varArray;
    bool m_configDialog;

};

#endif // DIALOG_H

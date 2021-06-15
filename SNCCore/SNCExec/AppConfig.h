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

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "Dialog.h"

class AppConfig : public Dialog
{
    Q_OBJECT

public:
    AppConfig();

    virtual void loadLocalData(const QJsonObject& param);
    virtual void saveLocalData();

    virtual bool setVar(const QString& name, const QJsonObject& var);
    virtual void getDialog(QJsonObject& newConfig);

signals:
    void loadComponent(int);

private:
    QString m_appName;
    QString m_executableDirectory;
    QString m_workingDirectory;
    QString m_iniPath;
    QString m_adaptor;
    bool m_inUse;
    bool m_consoleMode;

    QStringList m_adaptorList;

    int m_index;

};

#endif // APPCONFIG_H

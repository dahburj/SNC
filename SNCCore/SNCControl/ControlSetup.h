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

#ifndef CONTROLSETUP_H
#define CONTROLSETUP_H

#include "SNCUtils.h"

#include "Dialog.h"

class ControlSetup : public Dialog
{
public:
    ControlSetup();

    virtual void loadLocalData(const QJsonObject& param);
    virtual void saveLocalData();

    virtual bool setVar(const QString& name, const QJsonObject& var);
    virtual void getDialog(QJsonObject& newConfig);

    QString m_appName;
    int m_priority;
    QString m_adaptor;
    int m_localSocket;
    int m_encryptLocalSocket;
    int m_staticTunnelSocket;
    int m_encryptStaticTunnelSocket;
    bool m_encryptLocal;
    bool m_encryptStaticTunnelServer;
    int m_heartbeatInterval;
    int m_heartbeatRetries;
    bool m_useMACAsUID;
    QString m_uid;
};

#endif // CONTROLSETUP_H

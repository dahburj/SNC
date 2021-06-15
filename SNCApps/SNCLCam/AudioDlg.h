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

#ifndef AUDIODLG_H
#define AUDIODLG_H

#include "Dialog.h"

class AudioDlg : public Dialog
{
    Q_OBJECT

public:
    AudioDlg();

signals:
    void newAudioSrc();

protected:
    void loadLocalData(const QJsonObject& param);
    void saveLocalData();

    bool setVar(const QString& name, const QJsonObject& var);
    void getDialog(QJsonObject& newConfig);

private:
    QStringList getDeviceList();
    void processDeviceLine(QStringList& list, QString& line);
    bool m_enable;
    QString m_audioSource;
    int m_inputDevice;
    int m_inputCard;
    QString m_channels;
    QString m_sampleRate;
};

#endif // AUDIODLG_H

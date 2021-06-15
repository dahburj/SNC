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

#ifndef AUDIOOUTPUTDLG_H
#define AUDIOOUTPUTDLG_H

#include <QDialog>
#include <qvalidator.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qcheckbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>

//  group for audio related entries

#define	AUDIO_GROUP                   "AudioGroup"

// service enable flag

#define	AUDIO_ENABLE                  "AudioEnable"

// audio source to use

#define	AUDIO_OUTPUT_CARD               "AudioOutputCard"
#define	AUDIO_OUTPUT_DEVICE             "AudioOutputDevice"

#define AUDIO_DEFAULT_DEVICE			"<default device>"
#define AUDIO_DEFAULT_DEVICE_MAC		"Built-in Output"

class AudioOutputDlg : public QDialog
{
    Q_OBJECT

public:
    AudioOutputDlg(QWidget *parent);
    ~AudioOutputDlg();

public slots:
    void onOk();

private:
    void layoutWindow();

    QCheckBox *m_enable;
#if defined(Q_OS_WIN) || defined(Q_OS_OSX)
    QComboBox *m_outputDevice;
#else
    QLineEdit *m_outputDevice;
    QLineEdit *m_outputCard;
#endif
    QDialogButtonBox *m_buttons;
};

#endif // AUDIOOUTPUTDLG_H

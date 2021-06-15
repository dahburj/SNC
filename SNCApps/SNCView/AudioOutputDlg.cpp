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

#include "AudioOutputDlg.h"
#include "SNCUtils.h"

#include <qlist.h>
#include <qboxlayout.h>
#include <qformlayout.h>
#if defined(Q_OS_OSX)
#include <QAudioOutput>
#endif

AudioOutputDlg::AudioOutputDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    setWindowTitle("Audio output configuration");

    layoutWindow();

    connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

AudioOutputDlg::~AudioOutputDlg()
{

}

void AudioOutputDlg::onOk()
{
    bool changed = false;

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    if ((m_enable->checkState() == Qt::Checked) != settings->value(AUDIO_ENABLE).toBool()) {
        settings->setValue(AUDIO_ENABLE, m_enable->checkState() == Qt::Checked);
        changed = true;
    }

#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
    if (m_outputDevice->currentText() != settings->value(AUDIO_OUTPUT_DEVICE).toString()) {
        settings->setValue(AUDIO_OUTPUT_DEVICE, m_outputDevice->currentText());
        changed = true;
    }
#else
    if (m_outputCard->text() != settings->value(AUDIO_OUTPUT_CARD).toString()) {
        settings->setValue(AUDIO_OUTPUT_CARD, m_outputCard->text());
        changed = true;
    }
    if (m_outputDevice->text() != settings->value(AUDIO_OUTPUT_DEVICE).toString()) {
        settings->setValue(AUDIO_OUTPUT_DEVICE, m_outputDevice->text());
        changed = true;
    }
#endif
#endif

    settings->endGroup();

    delete settings;

    if (changed)
        accept();
    else
        reject();
}

void AudioOutputDlg::layoutWindow()
{

    QSettings *settings = SNCUtils::getSettings();

    setModal(true);
    settings->beginGroup(AUDIO_GROUP);

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->setSpacing(20);
    centralLayout->setContentsMargins(11, 11, 11, 11);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(16);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_enable = new QCheckBox(this);
    m_enable->setMinimumWidth(100);
    formLayout->addRow(tr("Enable audio"), m_enable);
    m_enable->setCheckState(settings->value(AUDIO_ENABLE).toBool() ? Qt::Checked : Qt::Unchecked);

#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
    m_outputDevice = new QComboBox(this);
    m_outputDevice->setMaximumWidth(200);
    m_outputDevice->setMinimumWidth(200);
    formLayout->addRow(tr("Audio output device"), m_outputDevice);

    m_outputDevice->addItem(AUDIO_DEFAULT_DEVICE);

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    for (int i = 0; i < devices.size(); i++) {
        m_outputDevice->addItem(devices.at(i).deviceName());
        if (devices.at(i).deviceName() == settings->value(AUDIO_OUTPUT_DEVICE).toString())
            m_outputDevice->setCurrentIndex(i+1);
    }

#else
    m_outputCard = new QLineEdit(this);
    m_outputCard->setMaximumWidth(60);
    formLayout->addRow(tr("Audio output card"), m_outputCard);
    m_outputCard->setText(settings->value(AUDIO_OUTPUT_CARD).toString());
    m_outputCard->setValidator(new QIntValidator(0, 64));

    m_outputDevice = new QLineEdit(this);
    m_outputDevice->setMaximumWidth(60);
    formLayout->addRow(tr("Audio output device"), m_outputDevice);
    m_outputDevice->setText(settings->value(AUDIO_OUTPUT_DEVICE).toString());
    m_outputDevice->setValidator(new QIntValidator(0, 64));

#endif
#endif

    centralLayout->addLayout(formLayout);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_buttons->setCenterButtons(true);

    centralLayout->addWidget(m_buttons);

    settings->endGroup();
    delete settings;
}


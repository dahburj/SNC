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

#include "AudioDlg.h"
#include "SNCLCam.h"
#include "AudioDriver.h"

#include "SNCUtils.h"

AudioDlg::AudioDlg() : Dialog(SNCLCAM_AUDIOCONFIGURATION_NAME, SNCLCAM_AUDIOCONFIGURATION_DESC)
{
    setConfigDialog(true);
}

void AudioDlg::getDialog(QJsonObject& newDialog)
{
    QStringList deviceList = getDeviceList();

    QStringList channelList, rateList;

    channelList << "1" << "2";
    rateList << "8000" << "16000" << "48000";

    clearDialog();
    addVar(createConfigBoolVar("enable", "Enable audio", m_enable));
    addVar(createConfigSelectionFromListVar("audioSource", "Audio Device", m_audioSource, deviceList));
    addVar(createConfigSelectionFromListVar("channels", "Audio channels", m_channels, channelList));
    addVar(createConfigSelectionFromListVar("sampleRate", "Audiosample rate", m_sampleRate, rateList));
    return dialog(newDialog);
}

bool AudioDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "enable") {
        if (m_enable != var.value(SNCJSON_CONFIG_VAR_VALUE).toBool()) {
            changed = true;
            m_enable = var.value(SNCJSON_CONFIG_VAR_VALUE).toBool();
        }
    } else if (name == "audioSource") {
        if (m_audioSource != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_audioSource = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "channels") {
        if (m_channels != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_channels = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    } else if (name == "sampleRate") {
        if (m_sampleRate != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_sampleRate = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    }
    return changed;
}

void AudioDlg::loadLocalData(const QJsonObject & /* param */)
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    m_enable = settings->value(AUDIO_ENABLE).toBool();
    m_audioSource = settings->value(AUDIO_SOURCE).toString();
    m_channels = settings->value(AUDIO_CHANNELS).toString();
    m_sampleRate = settings->value(AUDIO_SAMPLERATE).toString();

    settings->endGroup();
    delete settings;

}

void AudioDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    settings->setValue(AUDIO_ENABLE, m_enable);
    settings->setValue(AUDIO_SOURCE, m_audioSource);
    settings->setValue(AUDIO_CHANNELS, m_channels);
    settings->setValue(AUDIO_SAMPLERATE, m_sampleRate);

    settings->endGroup();
    delete settings;

    emit newAudioSrc();
}

QStringList AudioDlg::getDeviceList()
{
    QStringList list;

    QFile file("/proc/asound/pcm");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return list;

    QTextStream in(&file);

    QString line = in.readLine();

    while (!line.isNull()) {
        processDeviceLine(list, line);
        line = in.readLine();
    }
    return list;
}

void AudioDlg::processDeviceLine(QStringList& list, QString& line)
{
    if (!line.contains("capture"))
        return;

    QStringList fields = line.split(':');

    if (fields.length() < 2)
        return;

    QString entry = fields[0].trimmed() + ": " + fields[1].trimmed();

    list.append(entry);
}

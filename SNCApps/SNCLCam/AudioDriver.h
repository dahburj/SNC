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

#ifndef AUDIODRIVER_H
#define AUDIODRIVER_H

#include <QSize>
#include <QSettings>

#include "SNCThread.h"

//  Size in bits of sample

#define AUDIO_FIXED_SIZE                16

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

//  group for audio related entries

#define	AUDIO_GROUP                     "audioGroup"

// service enable flag

#define	AUDIO_ENABLE                    "audioEnable"

// audio source to use

#define	AUDIO_SOURCE                    "audioSource"

//  parameters to use

#define AUDIO_CHANNELS                  "audioChannels"
#define AUDIO_SAMPLERATE                "audioSampleRate"

class AudioDriver : public SNCThread
{
    Q_OBJECT

public:
    AudioDriver();

public slots:
    void newAudioSrc();

signals:
    void newAudio(QByteArray);
    void audioState(QString);
    void audioFormat(int sampleRate, int channels, int sampleSize);

protected:
    void initThread();
    void timerEvent(QTimerEvent *event);
    void finishThread();

private:
    bool loadSettings();
    bool deviceExists();
    void startCapture();
    void stopCapture();
    bool openDevice();
    void closeDevice();
    int getCard(QString audioDevice);
    int getDevice(QString audioDevice);

    QString m_audioSource;
    int m_audioDevice;
    int m_audioCard;

    snd_pcm_t *m_handle;
    snd_pcm_hw_params_t *m_params;
    unsigned char *m_buffer;

    int m_audioChannels;
    int m_audioSampleRate;
    int m_audioFramesPerBlock;
    int m_bytesPerBlock;

    bool m_enabled;

    int m_state;
    int m_ticks;
    int m_timer;
};

#endif // AUDIODRIVER_H

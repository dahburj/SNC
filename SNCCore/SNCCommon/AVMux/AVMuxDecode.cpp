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

#include "AVMuxDecode.h"
#include "SNCUtils.h"

#include <qbuffer.h>
#include <qbytearray.h>

AVMuxDecode::AVMuxDecode()
    : SNCThread("AVMuxDecode")
{
}

void AVMuxDecode::newAVMuxData(QByteArray data)
{
    if (data.length() < (int)sizeof(SNC_RECORD_AVMUX))
        return;

    SNC_RECORD_AVMUX *avmux = (SNC_RECORD_AVMUX *)data.constData();

    SNCUtils::avmuxHeaderToAVParams(avmux, &m_avParams);

    switch (m_avParams.avmuxSubtype) {
        case SNC_RECORD_TYPE_AVMUX_MJPPCM:
            processMJPPCM(avmux);
            break;

        default:
            printf("Unsupported avmux type %d\n", m_avParams.avmuxSubtype);
            break;
    }
}

void AVMuxDecode::processMJPPCM(SNC_RECORD_AVMUX *avmux)
{
    int videoSize = SNCUtils::convertUC4ToInt(avmux->videoSize);

    unsigned char *ptr = (unsigned char *)(avmux + 1) + SNCUtils::convertUC4ToInt(avmux->muxSize);

    if (videoSize != 0) {
        // there is video data present

        if ((videoSize < 0) || (videoSize > 400000)) {
            printf("Illegal video data size %d\n", videoSize);
            return;
        }

        QImage image;
        image.loadFromData((const uchar *)ptr, videoSize, "JPEG");
        emit newImage(image, SNCUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));

        ptr += videoSize;
    } else {
        if (SNC_RECORDHEADER_PARAM_NOOP == SNCUtils::convertUC2ToInt(avmux->recordHeader.param))
            emit newImage(QImage(), SNCUtils::convertUC8ToInt64(avmux->recordHeader.timestamp));
    }

    int audioSize = SNCUtils::convertUC4ToInt(avmux->audioSize);

    if (audioSize != 0) {
        // there is audio data present

        if ((audioSize < 0) || (audioSize > 300000)) {
            printf("Illegal audio data size %d\n", audioSize);
            return;
        }

        emit newAudioSamples(QByteArray((const char *)ptr, audioSize),
            SNCUtils::convertUC8ToInt64(avmux->recordHeader.timestamp),
            m_avParams.audioSampleRate, m_avParams.audioChannels, m_avParams.audioSampleSize);

        ptr += audioSize;
    }
}

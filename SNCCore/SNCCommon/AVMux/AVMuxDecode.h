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

#ifndef _AVMUXDECODE_H_
#define _AVMUXDECODE_H_

#include "SNCThread.h"
#include "SNCAVDefs.h"

#include <qmutex.h>
#include <qimage.h>

class AVMuxDecode : public SNCThread
{
    Q_OBJECT

public:
    AVMuxDecode();

public slots:
    void newAVMuxData(QByteArray data);

signals:
    void newImage(QImage image, qint64 timestamp);
    void newAudioSamples(QByteArray dataArray, qint64 timestamp, int rate, int channels, int size);

private:
    void processMJPPCM(SNC_RECORD_AVMUX *avmux);
    void processAVData(QByteArray avmuxArray);

    SNC_AVPARAMS m_avParams;
};

#endif // _AVMUXDECODE_H_

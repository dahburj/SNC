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

#ifndef _DIALWIDGET_H_
#define _DIALWIDGET_H_

#include <qwidget.h>
#include <qmutex.h>
#include <qimage.h>

//  dial types

#define DIALWIDGET_TYPE_TEMPERATURE     0
#define DIALWIDGET_TYPE_HUMIDITY        1
#define DIALWIDGET_TYPE_PRESSURE        2
#define DIALWIDGET_TYPE_LIGHT           3
#define DIALWIDGET_TYPE_AIRQUALITY      4
#define DIALWIDGET_TYPE_BLANK           5

class DialWidget : public QWidget
{
    Q_OBJECT

public:
    DialWidget(int type, double minValue, double maxValue, QWidget *parent = 0);

    void setValue(double value) { m_value = value; }

protected:
    void paintEvent(QPaintEvent *event);

private:
    bool m_first;

    int m_type;
    QImage m_dial;
    double m_value;
    double m_minValue;
    double m_maxValue;
};

#endif // _DIALWIDGET_H_

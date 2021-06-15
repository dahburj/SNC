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

#include <qstringlist.h>

#include "SNCDefs.h"
#include "SNCDirectoryEntry.h"

SNCDirectoryEntry::SNCDirectoryEntry(QString dirLine)
{
    m_raw = dirLine;
    parseLine();
}

void SNCDirectoryEntry::setLine(QString dirLine)
{
    m_uid.clear();
    m_name.clear();
    m_type.clear();
    m_multicastServices.clear();
    m_e2eServices.clear();

    m_raw = dirLine;

    parseLine();
}

bool SNCDirectoryEntry::isValid()
{
    if (m_uid.length() > 0 && m_name.length() > 0 && m_type.length() > 0)
        return true;

    return false;
}

QString SNCDirectoryEntry::uid()
{
    return m_uid;
}

QString SNCDirectoryEntry::appName()
{
    return m_name;
}

QString SNCDirectoryEntry::componentType()
{
    return m_type;
}

QStringList SNCDirectoryEntry::multicastServices()
{
    return m_multicastServices;
}

QStringList SNCDirectoryEntry::e2eServices()
{
    return m_e2eServices;
}

void SNCDirectoryEntry::parseLine()
{
    if (m_raw.length() == 0)
        return;

    m_uid = element(DETAG_UID);
    m_name = element(DETAG_APPNAME);
    m_type = element(DETAG_COMPTYPE);
    m_multicastServices = elements(DETAG_MSERVICE);
    m_e2eServices = elements(DETAG_ESERVICE);
}

QString SNCDirectoryEntry::element(QString name)
{
    QString element;

    QString start= QString("<%1>").arg(name);
    QString end = QString("</%1>").arg(name);

    int i = m_raw.indexOf(start);
    int j = m_raw.indexOf(end);

    if (i >= 0 && j > i + start.length())
        element = m_raw.mid(i + start.length(), j - (i + start.length()));

    return element;
}

QStringList SNCDirectoryEntry::elements(QString name)
{
    QStringList elements;
    int pos = 0;

    QString start = QString("<%1>").arg(name);
    QString end = QString("</%1>").arg(name);

    int i = m_raw.indexOf(start, pos);

    while (i >= 0) {
        int j = m_raw.indexOf(end, pos);

        if (j > i + start.length())
            elements << m_raw.mid(i + start.length(), j - (i + start.length()));

        pos = j + end.length();

        i = m_raw.indexOf(start, pos);
    }

    return elements;
}

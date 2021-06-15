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

#include "SNCComponentData.h"
#include <qglobal.h>
#include "SNCUtils.h"
#include "SNCSocket.h"

SNCComponentData::SNCComponentData()
{
    m_mySNCHelloSocket = NULL;
    m_myInstance = -1;
}

SNCComponentData::~SNCComponentData()
{
}

void SNCComponentData::init(const char *compType, int hbInterval, int priority)
{
    SNCHELLO *hello;

    m_logTag = compType;

    hello = &(m_myHeartbeat.hello);
    hello->helloSync[0] = SNCHELLO_SYNC0;
    hello->helloSync[1] = SNCHELLO_SYNC1;
    hello->helloSync[2] = SNCHELLO_SYNC2;
    hello->helloSync[3] = SNCHELLO_SYNC3;

    strncpy(hello->appName, qPrintable(SNCUtils::getAppName()), SNC_MAX_APPNAME - 1);
    strncpy(hello->componentType, compType, SNC_MAX_COMPTYPE - 1);

    strcpy(m_myComponentType, compType);

    // set up instance values and create hello socket using dynamic instance if a normal component
    if (strcmp(compType, COMPTYPE_CONTROL) == 0)
        m_myInstance = INSTANCE_CONTROL;
    else
        createSNCHelloSocket();
    memcpy(hello->IPAddr, SNCUtils::getMyIPAddr(), sizeof(SNC_IPADDR));

    QSettings *settings = SNCUtils::getSettings();
    bool ok;

    if (settings->value(SNC_PARAMS_UID_USE_MAC).toBool()) {
        // use mac address as first 6 bytes
        memcpy(hello->componentUID.macAddr, SNCUtils::getMyMacAddr(), sizeof(SNC_MACADDR));
    } else {
        quint64 uid = settings->value(SNC_PARAMS_UID).toString().toULongLong(&ok, 16);
        hello->componentUID.macAddr[0] = (uid >> 40) & 0xff;
        hello->componentUID.macAddr[1] = (uid >> 32) & 0xff;
        hello->componentUID.macAddr[2] = (uid >> 24) & 0xff;
        hello->componentUID.macAddr[3] = (uid >> 16) & 0xff;
        hello->componentUID.macAddr[4] = (uid >> 8) & 0xff;
        hello->componentUID.macAddr[5] = uid & 0xff;
    }

    delete settings;

    SNCUtils::convertIntToUC2(m_myInstance, hello->componentUID.instance);

    memcpy(&(m_myUID), &(hello->componentUID), sizeof(SNC_UID));

    SNCUtils::convertIntToUC2(hbInterval, hello->interval);

    hello->priority = priority;
    hello->unused1 = 1;

    // generate empty DE
    DESetup();
    DEComplete();

}

void SNCComponentData::DESetup()
{
    m_myDE[0] = 0;
    sprintf(m_myDE + (int)strlen(m_myDE), "<%s>", DETAG_COMP);
    DEAddValue(DETAG_UID, qPrintable(SNCUtils::displayUID(&(m_myUID))));
    DEAddValue(DETAG_APPNAME, (char *)m_myHeartbeat.hello.appName);
    DEAddValue(DETAG_COMPTYPE, (char *)m_myHeartbeat.hello.componentType);
}

void SNCComponentData::DEComplete()
{
    sprintf(m_myDE + (int)strlen(m_myDE), "</%s>", DETAG_COMP);
}

bool SNCComponentData::DEAddValue(QString tag, QString value)
{
    int len = (int)strlen(m_myDE) + (2 * tag.length()) + value.length() + 8;

    if (len >= SNC_MAX_DELENGTH) {
        SNCUtils::logError(m_logTag, QString("DE too long"));
        return false;
    }

    sprintf(m_myDE + (int)strlen(m_myDE), "<%s>%s</%s>", qPrintable(tag), qPrintable(value), qPrintable(tag));

    return true;
}


bool SNCComponentData::createSNCHelloSocket()
{
    int i;

    m_mySNCHelloSocket = new SNCSocket(m_logTag);

    if (!m_mySNCHelloSocket) {
        SNCUtils::logError(m_logTag, QString("Failed to create socket got %s").arg(m_myComponentType));
        return false;
    }
    for (i = INSTANCE_COMPONENT; i < SNC_MAX_COMPONENTSPERDEVICE; i++) {
        if (m_mySNCHelloSocket->sockCreate(SOCKET_SNCHELLO + i, SOCK_DGRAM) != 0) {
            m_myInstance = i;
            return true;
        }
    }

    if (i == SNC_MAX_COMPONENTSPERDEVICE) {
        SNCUtils::logError(m_logTag, QString("Dynamic instance allocation failed for %1").arg(m_myComponentType));
        return false;
    }
    return true;
}

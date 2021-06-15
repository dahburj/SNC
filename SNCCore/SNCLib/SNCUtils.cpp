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

#include "SNCUtils.h"
#include "SNCSocket.h"
#include "SNCEndpoint.h"

#include <qfileinfo.h>
#include <qdir.h>
#include <qhostinfo.h>
#include <qthread.h>
#include <qstandardpaths.h>

QString SNCUtils::m_logTag = "Utils";
QString SNCUtils::m_appName = "appName";
QString SNCUtils::m_appType = "appType";
SNC_IPADDR SNCUtils::m_myIPAddr;
SNC_MACADDR SNCUtils::m_myMacAddr;

QString SNCUtils::m_iniPath;

//	The address info for the adaptor being used

QHostAddress SNCUtils::m_platformBroadcastAddress;
QHostAddress SNCUtils::m_platformSubnetAddress;
QHostAddress SNCUtils::m_platformNetMask;

//  log level

#ifdef QT_NO_DEBUG
int SNCUtils::m_logDisplayLevel = SNC_LOG_LEVEL_WARN;
#else
int SNCUtils::m_logDisplayLevel = SNC_LOG_LEVEL_DEBUG;
#endif

SNC_IPADDR *SNCUtils::getMyIPAddr()
{
    return &m_myIPAddr;
}

SNC_MACADDR *SNCUtils::getMyMacAddr()
{
    return &m_myMacAddr;
}

const char *SNCUtils::SNCLibVersion()
{
    return SNCLIB_VERSION;
}

void SNCUtils::SNCAppInit()
{
    m_logTag = "Utils";
    getMyIPAddress();
}

void SNCUtils::SNCAppExit()
{
}

const QString& SNCUtils::getAppName()
{
    return m_appName;
}

const QString& SNCUtils::getAppType()
{
    return m_appType;
}

bool SNCUtils::checkConsoleModeFlag(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c"))
            return true;
    }

    return false;
}

bool SNCUtils::checkDaemonModeFlag(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d"))
            return true;
    }

    return false;
}

bool SNCUtils::isSendOK(unsigned char seq, unsigned char ack)
{
    return (seq - ack) < SNC_MAX_WINDOW;
}

void SNCUtils::makeUIDSTR(SNC_UIDSTR UIDStr, SNC_MACADDRSTR macAddress, int instance)
{
    sprintf(UIDStr, "%s%04x", macAddress, instance);
}

void SNCUtils::UIDSTRtoUID(SNC_UIDSTR sourceStr, SNC_UID *destUID)
{
    int byteTemp;

    for (int i = 0; i < SNC_MACADDR_LEN; i++) {
        sscanf(sourceStr + i * 2, "%02X", &byteTemp);
        destUID->macAddr[i] = byteTemp;
    }

    sscanf(sourceStr + SNC_MACADDR_LEN * 2, "%04X", &byteTemp);
    convertIntToUC2(byteTemp, destUID->instance);
}

void SNCUtils::UIDtoUIDSTR(SNC_UID *sourceUID, SNC_UIDSTR destStr)
{
    for (int i = 0; i < SNC_MACADDR_LEN; i++)
        sprintf(destStr + i * 2, "%02X", sourceUID->macAddr[i]);

    sprintf(destStr + SNC_MACADDR_LEN * 2, "%04x", convertUC2ToUInt(sourceUID->instance));
}

QString SNCUtils::displayIPAddr(SNC_IPADDR address)
{
    return QString("%1.%2.%3.%4").arg(address[0]).arg(address[1]).arg(address[2]).arg(address[3]);
}

void SNCUtils::convertIPStringToIPAddr(char *IPStr, SNC_IPADDR IPAddr)
{
    int	a[4];

    sscanf(IPStr, "%d.%d.%d.%d", a, a+1, a+2, a+3);

    for (int i = 0; i < 4; i++)
        IPAddr[i] = (unsigned char)a[i];
}

bool SNCUtils::IPZero(SNC_IPADDR addr)
{
    return (addr[0] == 0) && (addr[1] == 0) && (addr[2] == 0) && (addr[3] == 0);
}

bool SNCUtils::IPLoopback(SNC_IPADDR addr)
{
    return (addr[0] == 127) && (addr[1] == 0) && (addr[2] == 0) && (addr[3] == 1);
}


QString SNCUtils::displayUID(SNC_UID *uid)
{
    char temp[20];

    UIDtoUIDSTR(uid, temp);

    return QString(temp);
}

bool SNCUtils::compareUID(SNC_UID *a, SNC_UID *b)
{
    return memcmp(a, b, sizeof(SNC_UID)) == 0;
}


bool SNCUtils::UIDHigher(SNC_UID *a, SNC_UID *b)
{
    return memcmp(a, b, sizeof(SNC_UID)) > 0;
}

void SNCUtils::swapEHead(SNC_EHEAD *ehead)
{
    swapUID(&(ehead->sourceUID), &(ehead->destUID));

    int i = convertUC2ToInt(ehead->sourcePort);

    copyUC2(ehead->sourcePort, ehead->destPort);

    convertIntToUC2(i, ehead->destPort);
}

void SNCUtils::swapUID(SNC_UID *a, SNC_UID *b)
{
    SNC_UID temp;

    memcpy(&temp, a, sizeof(SNC_UID));
    memcpy(a, b, sizeof(SNC_UID));
    memcpy(b, &temp, sizeof(SNC_UID));
}

//	UC2, UC4 and UC8 Conversion routines
//
//	*** Note: 32 bit int assumed ***
//

qint64 SNCUtils::convertUC8ToInt64(SNC_UC8 uc8)
{
    return ((qint64)uc8[0] << 56) | ((qint64)uc8[1] << 48) | ((qint64)uc8[2] << 40) | ((qint64)uc8[3] << 32) |
        ((qint64)uc8[4] << 24) | ((qint64)uc8[5] << 16) | ((qint64)uc8[6] << 8) | ((qint64)uc8[7] << 0);
}

void SNCUtils::convertInt64ToUC8(qint64 val, SNC_UC8 uc8)
{
    uc8[7] = val & 0xff;
    uc8[6] = (val >> 8) & 0xff;
    uc8[5] = (val >> 16) & 0xff;
    uc8[4] = (val >> 24) & 0xff;
    uc8[3] = (val >> 32) & 0xff;
    uc8[2] = (val >> 40) & 0xff;
    uc8[1] = (val >> 48) & 0xff;
    uc8[0] = (val >> 56) & 0xff;
}

int	SNCUtils::convertUC4ToInt(SNC_UC4 uc4)
{
    return ((int)uc4[0] << 24) | ((int)uc4[1] << 16) | ((int)uc4[2] << 8) | uc4[3];
}

void SNCUtils::convertIntToUC4(int val, SNC_UC4 uc4)
{
    uc4[3] = val & 0xff;
    uc4[2] = (val >> 8) & 0xff;
    uc4[1] = (val >> 16) & 0xff;
    uc4[0] = (val >> 24) & 0xff;
}

int	SNCUtils::convertUC2ToInt(SNC_UC2 uc2)
{
    int val = ((int)uc2[0] << 8) | uc2[1];

    if (val & 0x8000)
        val |= 0xffff0000;

    return val;
}

int SNCUtils::convertUC2ToUInt(SNC_UC2 uc2)
{
    return ((int)uc2[0] << 8) | uc2[1];
}

void SNCUtils::convertIntToUC2(int val, SNC_UC2 uc2)
{
    uc2[1] = val & 0xff;
    uc2[0] = (val >> 8) & 0xff;
}

void SNCUtils::copyUC2(SNC_UC2 dst, SNC_UC2 src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

SNC_EHEAD *SNCUtils::createEHEAD(SNC_UID *sourceUID, int sourcePort, SNC_UID *destUID, int destPort,
                                        unsigned char seq, int len)
{
    SNC_EHEAD *ehead;

    ehead = (SNC_EHEAD *)malloc(len + sizeof(SNC_EHEAD));
    memcpy(&(ehead->sourceUID), sourceUID, sizeof(SNC_UID));
    convertIntToUC2(sourcePort, ehead->sourcePort);
    memcpy(&(ehead->destUID), destUID, sizeof(SNC_UID));
    convertIntToUC2(destPort, ehead->destPort);
    ehead->seq = seq;

    return ehead;
}

bool SNCUtils::crackServicePath(QString servicePath, QString& regionName, QString& componentName, QString& serviceName)
{
    regionName.clear();
    componentName.clear();
    serviceName.clear();

    QStringList stringList = servicePath.split(SNC_SERVICEPATH_SEP);

    if (stringList.count() > 3) {
        SNCUtils::logWarn(m_logTag, QString("Service path received has invalid format ") + servicePath);
        return false;
    }
    switch (stringList.count()) {
        case 1:
            serviceName = stringList.at(0);
            break;

        case 2:
            serviceName = stringList.at(1);
            componentName = stringList.at(0);
            break;

        case 3:
            serviceName = stringList.at(2);
            componentName = stringList.at(1);
            regionName = stringList.at(0);
    }
    return true;
}

/*
    This function is normally called from within main.cpp of a SNC app before the main code begins.
    It makes sure that settings common to all applications are set up correctly in the .ini file and
    processes the runtime arguments.

    Supported runtime arguments are:


    -a<adaptor>: adaptor. Can be used to specify a particular network adaptor for use. an example would be
        -aeth0.
    -c: console mode. If present, the app should run in console mode rather than windowed mode.
    -d: daemon mode. If present, the app should run in daemon mode
    -s<path>: settings file path. By default, the app will use the file <apptype>.ini in the working directory.
    The -s option can be used to override that and specify a path to anywhere in the file system.

*/

void SNCUtils::loadStandardSettings(const char *appType, QStringList arglist)
{
    QString args;
    QString adapter;                                        // IP adaptor name
    QSettings *settings = NULL;
    QString appName;

    m_iniPath = "";
    adapter = "";

    for (int i = 1; i < arglist.size(); i++) {              // skip the first string as that is the program name
        QString opt = arglist.at(i);                        // get the param

        if (opt.at(0).toLatin1() != '-')
            continue;                                       // doesn't start with '-'

        char optCode = opt.at(1).toLatin1();                // get the option code
        opt.remove(0, 2);                                   // remove the '-' and the code

        switch (optCode) {
        case 's':
            m_iniPath = opt;
            break;

        case 'a':
            adapter = opt;
            break;

        case 'n':
            appName = opt;
            break;

        case 'c':
        // console app flag, handled elsewhere
            break;

        case 'd':
        // daemon mode flag, handled elsewhere
            break;

        default:
            SNCUtils::logWarn(m_logTag, QString("Unrecognized argument option %1").arg(optCode));
        }
    }

    // user can override the default location for the ini file

    if (m_iniPath.size() == 0) {
#ifndef Q_OS_IOS
        settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "SNC", appType);

        // need this for subsequent opens
        m_iniPath = settings->fileName();
        delete settings;
#else
        m_iniPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + appType + ".ini";
#endif
    }

    if (m_iniPath.size() > 0) {
        // make sure the file specified is usable
        QFileInfo fileInfo(m_iniPath);

        if (fileInfo.exists()) {
            if (!fileInfo.isWritable())
                SNCUtils::logWarn(m_logTag, QString("Ini file is not writable: %1").arg(m_iniPath));

            if (!fileInfo.isReadable())
                SNCUtils::logWarn(m_logTag, QString("Ini file is not readable: %1").arg(m_iniPath));
        } else {
            QFileInfo dirInfo(fileInfo.path());

            if (!dirInfo.exists()) {
                SNCUtils::logWarn(m_logTag, QString("Directory for ini file does not exist: %1").arg(fileInfo.path()));
            } else {
                if (!dirInfo.isWritable())
                    SNCUtils::logWarn(m_logTag, QString("Directory for ini file is not writable: %1").arg(fileInfo.path()));

                if (!dirInfo.isReadable())
                    SNCUtils::logWarn(m_logTag, QString("Directory for ini file is not readable: %1").arg(fileInfo.path()));
            }
        }

    }

    // regardless of how the checks turned out, we don't want to fall back to the
    // default as that might overwrite an existing good default configuration
    // i.e. the common reason for passing an ini file is to start a second instance
    // of the app

    settings = new QSettings(m_iniPath, QSettings::IniFormat);

    // Save settings generated from command line arguments

    settings->setValue(SNC_RUNTIME_PATH, m_iniPath);

    if (adapter.length() > 0) {
        settings->setValue(SNC_RUNTIME_ADAPTER, adapter);
    } else {
        // we don't want to overwrite an existing entry so check first
        adapter = settings->value(SNC_RUNTIME_ADAPTER).toString();

        // but we still want to put a default placeholder
        if (adapter.length() < 1)
            settings->setValue(SNC_RUNTIME_ADAPTER, adapter);
    }

    //  Make sure common settings are present or else generate defaults

    if (appName != "") {
        // user specified new appName
        settings->setValue(SNC_PARAMS_APPNAME, appName);
     } else {
        if (!settings->contains(SNC_PARAMS_APPNAME))
            settings->setValue(SNC_PARAMS_APPNAME, QHostInfo::localHostName());		// use hostname for app name
    }

    m_appName = settings->value(SNC_PARAMS_APPNAME).toString();

    //  Force type to be mine always

    settings->setValue(SNC_PARAMS_APPTYPE, appType);
    m_appType = appType;

    if (!settings->contains(SNC_PARAMS_USE_TUNNEL))
        settings->setValue(SNC_PARAMS_USE_TUNNEL, false);
    if (!settings->contains(SNC_PARAMS_TUNNEL_ADDR))
        settings->setValue(SNC_PARAMS_TUNNEL_ADDR, "");
    if (!settings->contains(SNC_PARAMS_TUNNEL_PORT))
        settings->setValue(SNC_PARAMS_TUNNEL_PORT, SNC_SOCKET_LOCAL);

    //  Add in SNCControl array

    int	size = settings->beginReadArray(SNC_PARAMS_CONTROL_NAMES);
    settings->endArray();

    if (size == 0) {
        settings->beginWriteArray(SNC_PARAMS_CONTROL_NAMES);

        for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
            settings->setArrayIndex(control);
            settings->setValue(SNC_PARAMS_CONTROL_NAME, "");
        }
        settings->endArray();
    }

    if (!settings->contains(SNC_PARAMS_CONTROLREVERT))
        settings->setValue(SNC_PARAMS_CONTROLREVERT, false);

    if (!settings->contains(SNC_PARAMS_ENCRYPT_LINK))
        settings->setValue(SNC_PARAMS_ENCRYPT_LINK, false);

    if (!settings->contains(SNC_PARAMS_HBINTERVAL))
        settings->setValue(SNC_PARAMS_HBINTERVAL, SNC_HEARTBEAT_INTERVAL);

    if (!settings->contains(SNC_PARAMS_HBTIMEOUT))
        settings->setValue(SNC_PARAMS_HBTIMEOUT, SNC_HEARTBEAT_TIMEOUT);

    if (!settings->contains(SNC_PARAMS_UID_USE_MAC))
        settings->setValue(SNC_PARAMS_UID_USE_MAC, true);

    if (!settings->contains(SNC_PARAMS_UID))
        settings->setValue(SNC_PARAMS_UID, "000000000000");

    settings->sync();
    delete settings;
}

QSettings *SNCUtils::getSettings()
{
    return new QSettings(m_iniPath, QSettings::IniFormat);
}

bool SNCUtils::isReservedNameCharacter(char value)
{
    switch (value) {
        case SNCCFS_FILENAME_SEP:
        case SNC_SERVICEPATH_SEP:
        case ' ' :
        case '\\':
            return true;

        default:
            return false;
    }
}

bool SNCUtils::isReservedPathCharacter(char value)
{
    switch (value) {
        case SNCCFS_FILENAME_SEP:
        case ' ' :
        case '\\':
            return true;

        default:
            return false;
    }
}


/*
    This funtion takes \a streamSource, a QString containing the app name of a stream source possibly with
    a concententated type qualifier (eg :lr for low rate video) and inserts the \a streamName at the
    coorrect point and returns the result as a new QString.

    For example "Ubuntu:lr" with a stream name of "video" would result in "Ubuntu/video:lr".

    In many apps, users may specify the app name and type qualifier but the stream name is fixed. This
    function provides a convenient way of integrating the two ready to activate a remote stream for example.
*/

QString SNCUtils::insertStreamNameInPath(const QString& streamSource, const QString& streamName)
{
     int index;
     QString result;

     index = streamSource.indexOf(SNC_STREAM_TYPE_SEP);
     if (index == -1) {
         // there is no extension - just add stream name to the end
         result = streamSource + "/" + streamName;
     } else {
         // there is an extension - insert stream name before extension
         result = streamSource;
         result.insert(index, QString("/") + streamName);
     }
     return result;
}

/*
    This funtion takes \a servicePath, and return a \a streamSource that is the servicePath with
    the stream name removed. The stream name is returned in \a streamName.

    For example "Ubuntu/video:lr" would return a streamSource of "ubuntu:lr" and a streamName of "video".

*/

void SNCUtils::removeStreamNameFromPath(const QString& servicePath,
            QString& streamSource, QString& streamName)
{
     int start, end;

     start = servicePath.indexOf(SNC_SERVICEPATH_SEP);      // find the "/"
     if (start == -1) {                                     // not found
         streamSource = servicePath;
         streamName = "";
         return;
     }
     end = servicePath.indexOf(SNC_STREAM_TYPE_SEP);        // find the ":" if there is one

     if (end == -1) {                                       // there isn't
         streamSource = servicePath.left(start);
         streamName = servicePath;
         streamName.remove(0, start + 1);
         return;
     }

     // We have all parts present

     streamSource = servicePath;
     streamSource.remove(start, end - start);               // take out stream name
     streamName = servicePath;
     streamName.remove(end, streamName.length());           // make sure everything at end removed
     streamName.remove(0, start + 1);
}

QHostAddress SNCUtils::getMyBroadcastAddress()
{
    return m_platformBroadcastAddress;
}

QHostAddress SNCUtils::getMySubnetAddress()
{
    return m_platformSubnetAddress;
}

QHostAddress SNCUtils::getMyNetMask()
{
    return m_platformNetMask;
}

bool SNCUtils::isInMySubnet(SNC_IPADDR IPAddr)
{
    quint32 intaddr;

    if ((IPAddr[0] == 0) && (IPAddr[1] == 0) && (IPAddr[2] == 0) && (IPAddr[3] == 0))
        return true;
    if ((IPAddr[0] == 0xff) && (IPAddr[1] == 0xff) && (IPAddr[2] == 0xff) && (IPAddr[3] == 0xff))
        return true;
    intaddr = (IPAddr[0] << 24) + (IPAddr[1] << 16) + (IPAddr[2] << 8) + IPAddr[3];
    return (intaddr & m_platformNetMask.toIPv4Address()) == m_platformSubnetAddress.toIPv4Address();
}


/*
    Provides a convenient way of checking to see if a timer has expired. \a now is the current
    time. \a start is the start time of the timer, usually
    the value of clock when the timer was started. \a interval is the length of the timer.

    Returns true if the timer has expired, false otherwise. If \a interval is 0, the function always returns false,
    providing an easy way of disabling timers.
*/

bool SNCUtils::timerExpired(qint64 now, qint64 start, qint64 interval)
{
    if (interval == 0)
        return false;										// can never expire
    return ((now - start) >= interval);
}

void SNCUtils::getMyIPAddress()
{
    QNetworkInterface		cInterface;
    QNetworkAddressEntry	cEntry;
    QList<QNetworkAddressEntry>	cList;
    QHostAddress			haddr;
    quint32 intaddr;
    quint32 intmask;
    int addr[SNC_MACADDR_LEN];

    QSettings *settings = SNCUtils::getSettings();

    while (1) {
        QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
        foreach (cInterface, ani) {
            QString name = cInterface.humanReadableName();
            SNCUtils::logDebug(m_logTag, QString("Found IP adaptor %1").arg(name));

            if ((strcmp(qPrintable(name), qPrintable(settings->value(SNC_RUNTIME_ADAPTER).toString())) == 0) ||
                (strlen(qPrintable(settings->value(SNC_RUNTIME_ADAPTER).toString())) == 0)) {
                cList = cInterface.addressEntries();        // check to see if there's an address
                if (cList.size() > 0) {
                    foreach (cEntry, cList) {
                        haddr = cEntry.ip();
                        intaddr = haddr.toIPv4Address();    // get it as four bytes
                        if (intaddr == 0)
                            continue;                       // not real

                        m_myIPAddr[0] = intaddr >> 24;
                        m_myIPAddr[1] = intaddr >> 16;
                        m_myIPAddr[2] = intaddr >> 8;
                        m_myIPAddr[3] = intaddr;
                        if (IPLoopback(m_myIPAddr) || IPZero(m_myIPAddr))
                            continue;
                        QString macaddr = cInterface.hardwareAddress(); // get the MAC address
                        sscanf(macaddr.toLocal8Bit().constData(), "%x:%x:%x:%x:%x:%x",
                            addr, addr+1, addr+2, addr+3, addr+4, addr+5);

                        for (int i = 0; i < SNC_MACADDR_LEN; i++)
                            m_myMacAddr[i] = addr[i];

                        SNCUtils::logInfo(m_logTag, QString("Using IP adaptor %1").arg(displayIPAddr(m_myIPAddr)));

                        m_platformNetMask = cEntry.netmask();
                        intmask = m_platformNetMask.toIPv4Address();
                        intaddr &= intmask;
                        m_platformSubnetAddress = QHostAddress(intaddr);
                        intaddr |= ~intmask;
                        m_platformBroadcastAddress = QHostAddress(intaddr);
                        SNCUtils::logInfo(m_logTag, QString("Subnet = %1, netmask = %2, bcast = %3")
                            .arg(m_platformSubnetAddress.toString())
                            .arg(m_platformNetMask.toString())
                            .arg(m_platformBroadcastAddress.toString()));

                        delete settings;

                        return;
                    }
                }
            }
        }

        SNCUtils::logDebug(m_logTag, QString("Waiting for adapter ") + settings->value(SNC_RUNTIME_ADAPTER).toString());
        QThread::yieldCurrentThread();
    }
    delete settings;
}

/*!
    Sets the current mS resolution timestamp into \a timestamp.
*/

void SNCUtils::setTimestamp(SNC_UC8 timestamp)
{
    convertInt64ToUC8(QDateTime::currentMSecsSinceEpoch(), timestamp);
}

/*!
    Returns the timestamp in \a timestamp as a qint64.
*/

qint64 SNCUtils::getTimestamp(SNC_UC8 timestamp)
{
    return convertUC8ToInt64(timestamp);
}

//  Multimedia functions

void SNCUtils::avmuxHeaderInit(SNC_RECORD_AVMUX *avmuxHead, SNC_AVPARAMS *avParams,
        int param, int recordIndex, int muxSize, int videoSize,int audioSize)
{
    memset(avmuxHead, 0, sizeof(SNC_RECORD_AVMUX));

    // do the generic record header stuff

    convertIntToUC2(SNC_RECORD_TYPE_AVMUX, avmuxHead->recordHeader.type);
    convertIntToUC2(avParams->avmuxSubtype, avmuxHead->recordHeader.subType);
    convertIntToUC2(sizeof(SNC_RECORD_AVMUX), avmuxHead->recordHeader.headerLength);
    convertIntToUC2(param, avmuxHead->recordHeader.param);
    convertIntToUC4(recordIndex, avmuxHead->recordHeader.recordIndex);
    setTimestamp(avmuxHead->recordHeader.timestamp);

    // and the rest

    avmuxHead->videoSubtype = avParams->videoSubtype;
    avmuxHead->audioSubtype = avParams->audioSubtype;
    convertIntToUC4(muxSize, avmuxHead->muxSize);
    convertIntToUC4(videoSize, avmuxHead->videoSize);
    convertIntToUC4(audioSize, avmuxHead->audioSize);

    convertIntToUC2(avParams->videoWidth, avmuxHead->videoWidth);
    convertIntToUC2(avParams->videoHeight, avmuxHead->videoHeight);
    convertIntToUC2(avParams->videoFramerate, avmuxHead->videoFramerate);

    convertIntToUC4(avParams->audioSampleRate, avmuxHead->audioSampleRate);
    convertIntToUC2(avParams->audioChannels, avmuxHead->audioChannels);
    convertIntToUC2(avParams->audioSampleSize, avmuxHead->audioSampleSize);
}

void SNCUtils::avmuxHeaderToAVParams(SNC_RECORD_AVMUX *avmuxHead, SNC_AVPARAMS *avParams)
{
    avParams->avmuxSubtype = convertUC2ToInt(avmuxHead->recordHeader.subType);

    avParams->videoSubtype = avmuxHead->videoSubtype;
    avParams->audioSubtype = avmuxHead->audioSubtype;

    avParams->videoWidth = convertUC2ToInt(avmuxHead->videoWidth);
    avParams->videoHeight = convertUC2ToInt(avmuxHead->videoHeight);
    avParams->videoFramerate = convertUC2ToInt(avmuxHead->videoFramerate);

    avParams->audioSampleRate = convertUC4ToInt(avmuxHead->audioSampleRate);
    avParams->audioChannels = convertUC2ToInt(avmuxHead->audioChannels);
    avParams->audioSampleSize = convertUC2ToInt(avmuxHead->audioSampleSize);
}

bool SNCUtils::avmuxHeaderValidate(SNC_RECORD_AVMUX *avmuxHead, int length,
                unsigned char **muxPtr, int& muxLength,
                unsigned char **videoPtr, int& videoLength,
                unsigned char **audioPtr, int& audioLength)
{
    muxLength = convertUC4ToInt(avmuxHead->muxSize);
    videoLength = convertUC4ToInt(avmuxHead->videoSize);
    audioLength = convertUC4ToInt(avmuxHead->audioSize);

    if (length != ((int)sizeof(SNC_RECORD_AVMUX) + muxLength + videoLength + audioLength))
        return false;

    if (muxPtr != NULL)
        *muxPtr = (unsigned char *)(avmuxHead + 1);

    if (videoPtr != NULL)
        *videoPtr = (unsigned char *)(avmuxHead + 1) + muxLength;

    if (audioPtr != NULL)
        *audioPtr = (unsigned char *)(avmuxHead + 1) + muxLength + videoLength;

    return true;
}



//----------------------------------------------------------
//
//  log functions

void SNCUtils::logDebug(const QString& tag, const QString& msg) { SNCUtils::addLogMessage(tag, msg, SNC_LOG_LEVEL_DEBUG); }
void SNCUtils::logInfo(const QString& tag, const QString& msg) { SNCUtils::addLogMessage(tag, msg, SNC_LOG_LEVEL_INFO); }
void SNCUtils::logWarn(const QString& tag, const QString& msg) { SNCUtils::addLogMessage(tag, msg, SNC_LOG_LEVEL_WARN); }
void SNCUtils::logError(const QString& tag, const QString& msg) { SNCUtils::addLogMessage(tag, msg, SNC_LOG_LEVEL_ERROR); }


void SNCUtils::addLogMessage(const QString& tag, const QString& msg, int level)
{
    if (level < m_logDisplayLevel)
        return;

    QString logMsg(QDateTime::currentDateTime().toString() + " " + tag + "(" + QString::number(level) + "): " + msg);
    qDebug() << qPrintable(logMsg);
}

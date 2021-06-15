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

#ifndef _SNCUTILS_H_
#define _SNCUTILS_H_

#include "SNCDefs.h"
#include "SNCAVDefs.h"
#include "SNCHello.h"

//	Standard Qt includes for SNCLib

#include <qmutex.h>
#include <qabstractsocket.h>
#include <qtcpserver.h>
#include <qudpsocket.h>
#include <qtcpsocket.h>
#include <qthread.h>
#include <qapplication.h>
#include <qstringlist.h>
#include <qqueue.h>
#include <qnetworkinterface.h>
#include <qdatetime.h>
#include <qsettings.h>
#include <qlabel.h>

class SNCSocket;

//		SNC standard settings entries

#define	SNC_RUNTIME_PATH                "runtimePath"       // runtime path
#define	SNC_RUNTIME_ADAPTER             "runtimeAdapter"    // ethernet adapter

//	Standard operating parameter entries

#define	SNC_PARAMS_APPNAME              "appName"           // name of the application
#define	SNC_PARAMS_APPTYPE              "appType"           // type of the application
#define SNC_PARAMS_USE_TUNNEL           "useTunnel"         // if should use static tunnel
#define SNC_PARAMS_TUNNEL_ADDR          "tunnelAddr"        // tunnel address
#define SNC_PARAMS_TUNNEL_PORT          "tunnelPort"        // tunnel port
#define SNC_PARAMS_CONTROLREVERT        "controlRevert"     // true if implement control priority with active connection
#define	SNC_PARAMS_HBINTERVAL           "heartbeatInterval" // time between sent heartbeats
#define	SNC_PARAMS_HBTIMEOUT            "heartbeatTimeout"  // number of hb intervals without hb before timeout
#define SNC_PARAMS_ENCRYPT_LINK         "encryptLink"       // true if use SSL for links
#define SNC_PARAMS_UID_USE_MAC          "UIDUseMAC"         // true if use MAC for UID, false means use configured
#define SNC_PARAMS_UID                  "UID"               // configured UID

#define	SNC_PARAMS_CONTROL_NAMES        "controlNames"      // ordered list of SNCControls as an array
#define	SNC_PARAMS_CONTROL_NAME         "controlName"       // an entry in the array

//	Standard stream entries

#define SNC_PARAMS_STREAM_SOURCES       "streamSources"
#define SNC_PARAMS_STREAM_SOURCE        "source"

//	Standard entries for SNCEndpoint
//
//	"local" services are those published by this client.
//	"remote" services are those to which this client subscribes
//	Each array entry consists of a name, local or remote and type for a service

#define	SNC_PARAMS_CLIENT_SERVICES      "clientServices"    // the array containing the service names, location and types
#define	SNC_PARAMS_CLIENT_SERVICE_NAME  "clientServiceName" // the name of a service
#define	SNC_PARAMS_CLIENT_SERVICE_TYPE  "clientServiceType" // multicast or E2E
#define	SNC_PARAMS_CLIENT_SERVICE_LOCATION "clientServiceLocation" // local or remote

#define SNC_PARAMS_SERVICE_LOCATION_LOCAL  "local"          // local service
#define SNC_PARAMS_SERVICE_LOCATION_REMOTE "remote"         // remote service

#define SNC_PARAMS_SERVICE_TYPE_MULTICAST   "multicast"     // the multicast service type string
#define SNC_PARAMS_SERVICE_TYPE_E2E         "E2E"           // the E2E service type string

//	Standard SNCCFS entries

#define	SNC_PARAMS_CFS_STORES           "CFSStores"         // SNCCFS stores array
#define	SNC_PARAMS_CFS_STORE            "CFSStore"          // the entries

//  Log defines

#define SNC_LOG_LEVEL_DEBUG             0                   // debug level
#define SNC_LOG_LEVEL_INFO              1                   // info level
#define SNC_LOG_LEVEL_WARN              2                   // warn level
#define SNC_LOG_LEVEL_ERROR             3                   // error level

#define SNC_LOG_LEVEL_NONE              4                   // used to display no log messages

class SNCUtils
{
public:
    static const char *SNCLibVersion();                     // returns a string containing the SNCLib version as x.y.z

//  Settings functions

    static void loadStandardSettings(const char *appType, QStringList arglist);
    static QSettings *getSettings();

//  app init and exit

    static void SNCAppInit();
    static void SNCAppExit();

    static const QString& getAppName();
    static const QString& getAppType();

    static SNC_IPADDR *getMyIPAddr();
    static SNC_MACADDR *getMyMacAddr();

    static bool checkConsoleModeFlag(int argc, char *argv[]);   // checks if console mode
    static bool checkDaemonModeFlag(int argc, char *argv[]);
    static bool isSendOK(unsigned char sendSeq, unsigned char ackSeq);
    static SNC_EHEAD *createEHEAD(SNC_UID *sourceUID, int sourcePort,
                    SNC_UID *destUID, int destPort, unsigned char seq, int len);
    static void swapEHead(SNC_EHEAD *ehead);		// swaps UIDs and port numbers
    static bool crackServicePath(QString servicePath, QString &regionName, QString& componentName, QString& serviceName); // breaks a service path into its constituent bits
    static bool timerExpired(qint64 now, qint64 start, qint64 interval);

    static qint64 clock() {	return QDateTime::currentMSecsSinceEpoch();}

//  Log functions

    static void logDebug(const QString& tag, const QString& msg);   // log message at debug level
    static void logInfo(const QString& tag, const QString& msg);    // log message at info level
    static void logWarn(const QString& tag, const QString& msg);    // log message at warn level
    static void logError(const QString& tag, const QString& msg);   // log message at error level
    static void setLogDisplayLevel(int level) { m_logDisplayLevel = level;} // sets the log level to be displayed

//  Service path functions

    static bool isReservedNameCharacter(char value);        // returns true if value is not allowed in names (components of paths)
    static bool isReservedPathCharacter(char value);        // returns true if value is not allowed in paths
    static QString insertStreamNameInPath(const QString& streamSource, const QString& streamName); // adds in a stream name before any extension
    static void removeStreamNameFromPath(const QString& servicePath,
            QString& streamSource, QString& streamName);    // extracts a stream name before any extension

//  IP Address functions

    static void getMyIPAddress();                           // gets my IP address from first or known IP adaptor
    static QString displayIPAddr(SNC_IPADDR addr);          // returns string version of IP
    static void convertIPStringToIPAddr(char *IPStr, SNC_IPADDR IPAddr);	// converts a string IP address to internal
    static bool IPZero(SNC_IPADDR addr);                    // returns true if address all zeroes
    static bool IPLoopback(SNC_IPADDR addr);                // returns true if address is 127.0.0.1

//  UID manipulation functions

    static void UIDSTRtoUID(SNC_UIDSTR sourceStr, SNC_UID *destUID);// converts a string UID to a binary
    static void UIDtoUIDSTR(SNC_UID *sourceUID, SNC_UIDSTR destStr);// converts a binary UID to a string
    static void makeUIDSTR(SNC_UIDSTR UIDStr, SNC_MACADDRSTR macAddress, int instance); // Fills in the UID
    static QString displayUID(SNC_UID *UID);                // return string version of UID
    static bool compareUID(SNC_UID *a, SNC_UID *b);         // return true if UIDs are equal
    static bool UIDHigher(SNC_UID *a, SNC_UID *b);          // returns true if a > b numerically
    static void swapUID(SNC_UID *a, SNC_UID *b);

//	SNC type conversion functions

    static qint64 convertUC8ToInt64(SNC_UC8 uc8);
    static void convertInt64ToUC8(qint64 val, SNC_UC8 uc8);
    static int convertUC4ToInt(SNC_UC4 uc4);
    static void convertIntToUC4(int val, SNC_UC4 uc4);
    static int convertUC2ToInt(SNC_UC2 uc2);
    static int convertUC2ToUInt(SNC_UC2 uc2);
    static void convertIntToUC2(int val, SNC_UC2 uc2);
    static void copyUC2(SNC_UC2 dst, SNC_UC2 src);

//  Multimedia stream functions

    static void avmuxHeaderInit(SNC_RECORD_AVMUX *avmuxHead, SNC_AVPARAMS *avParams,
            int param, int recordIndex, int muxSize, int videoSize,int audioSize);
    static void avmuxHeaderToAVParams(SNC_RECORD_AVMUX *avmuxHead, SNC_AVPARAMS *avParams);
    static bool avmuxHeaderValidate(SNC_RECORD_AVMUX *avmuxHead, int length,
            unsigned char **muxPtr, int& muxLength,
            unsigned char **videoPtr, int& videoLength,
            unsigned char **audioPtr, int& audioLength);

//  SNC timestamp functions

    static void setTimestamp(SNC_UC8 timestamp);
    static qint64 getTimestamp(SNC_UC8 timestamp);

//  Information for the network address in use

    static QHostAddress getMyBroadcastAddress();
    static QHostAddress getMySubnetAddress();
    static QHostAddress getMyNetMask();
    static bool isInMySubnet(SNC_IPADDR IPAddr);

private:
    static QString m_logTag;
    static SNC_IPADDR m_myIPAddr;
    static SNC_MACADDR m_myMacAddr;

    static QString m_iniPath;
    static QString m_appName;
    static QString m_appType;

    //  The address info for the adaptor being used

    static QHostAddress m_platformBroadcastAddress;
    static QHostAddress m_platformSubnetAddress;
    static QHostAddress m_platformNetMask;

    static void addLogMessage(const QString& tag, const QString& msg, int level);
    static int m_logDisplayLevel;
};

#endif //_SNCUTILS_H_

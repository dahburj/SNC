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

#ifndef _SNCSTORE_H_
#define _SNCSTORE_H_

#include "MainConsole.h"
#include "StoreStream.h"
#include "SNCDefs.h"

//  SNCJSON defines

#define SNCSTORE_CONFIGURATION_NAME     "storeConfig"
#define SNCSTORE_CONFIGURATION_DESC     "Configure SNCStore"

#define SNCSTORE_STATUS_NAME            "storeStatus"
#define SNCSTORE_STATUS_DESC            "Store status"

#define SNCSTORE_STREAMCONFIG_NAME      "streamConfig"
#define SNCSTORE_STREAMCONFIG_DESC      "Stream configuration"

#define	SNCSTORE_MAX_STREAMS            (SNC_MAX_SERVICESPERCOMPONENT / 2)

//  SNCStore specific setting keys

#define SNCSTORE_MAXAGE                 "maxAge"

#define SNCSTORE_PARAMS_ROOT_DIRECTORY  "RootDirectory"

#define	SNCSTORE_PARAMS_STREAM_SOURCES  "Streams"
#define	SNCSTORE_PARAMS_INUSE           "inUse"
#define	SNCSTORE_PARAMS_STREAM_SOURCE   "stream"
#define SNCSTORE_PARAMS_CREATE_SUBFOLDER    "createSubFolder"
#define	SNCSTORE_PARAMS_FORMAT          "storeFormat"
#define SNCSTORE_PARAMS_ROTATION_POLICY "rotationPolicy"
#define SNCSTORE_PARAMS_ROTATION_TIME_UNITS "rotationTimeUnits"
#define SNCSTORE_PARAMS_ROTATION_TIME   "rotationTime"
#define SNCSTORE_PARAMS_ROTATION_SIZE   "rotationSize"
#define SNCSTORE_PARAMS_DELETION_POLICY "deletionPolicy"
#define SNCSTORE_PARAMS_DELETION_TIME_UNITS "deletionTimeUnits"
#define SNCSTORE_PARAMS_DELETION_TIME   "deletionTime"
#define SNCSTORE_PARAMS_DELETION_COUNT  "deletionCount"

//  magic strings used in the .ini file

#define SNCSTORE_PARAMS_ROTATION_TIME_UNITS_HOURS   "hours"
#define SNCSTORE_PARAMS_ROTATION_TIME_UNITS_MINUTES "minutes"
#define SNCSTORE_PARAMS_ROTATION_TIME_UNITS_DAYS    "days"
#define SNCSTORE_PARAMS_ROTATION_POLICY_TIME        "time"
#define SNCSTORE_PARAMS_ROTATION_POLICY_SIZE        "size"
#define SNCSTORE_PARAMS_ROTATION_POLICY_ANY         "any"

#define SNCSTORE_PARAMS_DELETION_TIME_UNITS_HOURS   "hours"
#define SNCSTORE_PARAMS_DELETION_TIME_UNITS_DAYS    "days"
#define SNCSTORE_PARAMS_DELETION_POLICY_TIME        "time"
#define SNCSTORE_PARAMS_DELETION_POLICY_COUNT       "count"
#define SNCSTORE_PARAMS_DELETION_POLICY_ANY         "any"

#define SNCSTORE_PARAMS_DELETION_TIME_DEFAULT       "2"
#define SNCSTORE_PARAMS_DELETION_COUNT_DEFAULT      "5"


//  The CFS message

#define	SNCSTORE_CFS_MESSAGE                        (SNC_MSTART + 0)    // the message used to pass CFS messages around


//  Display columns

#define SNCSTORE_COL_CONFIG             0                   // configure entry
#define SNCSTORE_COL_INUSE              1                   // entry in use
#define SNCSTORE_COL_STREAM             2                   // stream name
#define SNCSTORE_COL_TOTALRECS          3                   // total records
#define SNCSTORE_COL_TOTALBYTES         4                   // total bytes
#define SNCSTORE_COL_FILERECS           5                   // file records
#define SNCSTORE_COL_FILEBYTES          6                   // file bytes
#define SNCSTORE_COL_FILE               7                   // name of current file

#define SNCSTORE_COL_COUNT              8                   // number of columns

//  Timer intervals

#define	SNCSTORE_BGND_INTERVAL          (SNC_CLOCKS_PER_SEC/10)
#define	SNCSTORE_DM_INTERVAL            (SNC_CLOCKS_PER_SEC * 10)

#define	SNCCFS_BGND_INTERVAL            (SNC_CLOCKS_PER_SEC / 100)
#define	SNCCFS_DM_INTERVAL              (SNC_CLOCKS_PER_SEC * 10)

class StoreClient;
class CFSClient;

class SNCStore : public MainConsole
{
    Q_OBJECT

public:
    SNCStore(QObject *parent);
    ~SNCStore();

signals:
    void refreshStreamSource(int index);

protected:
    void processInput(char c);
    void appExit();

private:
    void showStatus();
    void showCounts();
    void showHelp();

    StoreClient *m_storeClient;
    CFSClient *m_CFSClient;
};

#endif // _SNCSTORE_H_

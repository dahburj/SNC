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

#ifndef SNCCONTROL_H
#define SNCCONTROL_H

#include "MainConsole.h"

//  Timer intervals

#define	SNCSERVER_INTERVAL              (SNC_CLOCKS_PER_SEC / 100)  // SNCServer runs 100 times per second

#define	EXCHANGE_TIMEOUT				(5 * SNC_CLOCKS_PER_SEC)    // multicast without ack timeout
#define	MULTICAST_REFRESH_TIMEOUT		(3 * SNC_SERVICE_LOOKUP_INTERVAL) // time before stop passing back lookup refreshes

//  SNCJSON dialog names

#define SNCCONTROL_CONTROLSETUP_NAME    "controlSetup"
#define SNCCONTROL_CONTROLSETUP_DESC    "SNCControl setup"

#define SNCCONTROL_LINKSTATUS_NAME      "linkStatus"
#define SNCCONTROL_LINKSTATUS_DESC      "Link status"

#define SNCCONTROL_DIRECTORYSTATUS_NAME "directoryStatus"
#define SNCCONTROL_DIRECTORYSTATUS_DESC "Directory status"

class SNCServer;

class SNCControl : public MainConsole
{
    Q_OBJECT

public:
    SNCControl(QObject *parent, bool daemonMode);
    ~SNCControl() {}

protected:
    void processInput(char c);
    void appExit();

private:
    void displayComponents();
    void displayDirectory();
    void displayMulticast();
    void showHelp();

    SNCServer *m_server;
};

#endif // SNCCONTROL_H

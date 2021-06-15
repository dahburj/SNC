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

#ifndef		_SNCCOMPONENTDATA_H_
#define		_SNCCOMPONENTDATA_H_

#include "SNCDefs.h"
#include "SNCHello.h"

#include <qstring.h>

class SNCSocket;

class SNCComponentData
{
public:
    SNCComponentData();
    virtual ~SNCComponentData();

    // sets up the initial data

    void init(const char *compType, int hbInterval, int priority = 0);

    // returns a pointer to the DE

    inline const char* getMyDE() {return m_myDE;};

    // sets up the initial part of the DE

    void DESetup();

    // adds tag/value pairs to the DE

    bool DEAddValue(QString tag, QString value);

    // called to complete the DE

    void DEComplete();

    // returns the heartbeat

    inline SNC_HEARTBEAT getMyHeartbeat() {return m_myHeartbeat;};

    // returns the component type

    inline const char *getMyComponentType() {return m_myComponentType;};

    // return the component UID

    inline SNC_UID getMyUID() {return m_myUID;};

    // returns the component instance

    inline unsigned char getMyInstance() {return m_myInstance;};

    // return the hello socket pointer

    inline SNCSocket *getMySNCHelloSocket() { return m_mySNCHelloSocket;};


private:
    bool createSNCHelloSocket();

    SNC_HEARTBEAT m_myHeartbeat;
    SNC_COMPTYPE m_myComponentType;
    SNC_UID m_myUID;
    char m_myDE[SNC_MAX_DELENGTH];
    SNCSocket *m_mySNCHelloSocket;
    unsigned char m_myInstance;

    QString m_logTag;
};

#endif		//_SNCCOMPONENTDATA_H_

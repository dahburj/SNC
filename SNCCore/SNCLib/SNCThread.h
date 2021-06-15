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

#ifndef _SNCTHREAD_H_
#define _SNCTHREAD_H_

#include <qthread.h>
#include <qevent.h>

//  Inter-thread message defs

//  SNCEndpoint socket messages

#define	SNCENDPOINT_ONCONNECT_MESSAGE      (1)                 // SNCEndpoint socket messages
#define	SNCENDPOINT_ONCLOSE_MESSAGE        (2)
#define	SNCENDPOINT_ONRECEIVE_MESSAGE      (3)
#define	SNCENDPOINT_ONSEND_MESSAGE         (4)

#define	SNCENDPOINT_MESSAGE_START           SNCENDPOINT_ONCONNECT_MESSAGE   // start of endpoint message range
#define	SNCENDPOINT_MESSAGE_END             SNCENDPOINT_ONSEND_MESSAGE      // end of endpoint message range

//#define	SNCTHREAD_TIMER_MESSAGE		(5)                 // message used by the SNCThread timer
#define	HELLO_ONRECEIVE_MESSAGE			(6)                 // used for received hello messages
#define	HELLO_STATUS_CHANGE_MESSAGE		(7)                 // send to owner when hello status changes for a device

//	This code is the first that can be used by any specific application

#define	SNC_MSTART                      (8)                 // This is where application specific codes start

//	The SNCThreadMsg data structure - this is what is passed in the task's message queue

class SNCThreadMsg : public QEvent
{
public:
    SNCThreadMsg(QEvent::Type nEvent);
    int		message;
    int		intParam;
    void	*ptrParam;
};

class InternalThread : public QThread
{
    Q_OBJECT

public:
    inline void msleep(unsigned long msecs) { QThread::msleep(msecs); }
};

class SNCThread : public QObject
{
    Q_OBJECT

public:
    SNCThread(const QString& threadName);
    virtual ~SNCThread();

    virtual void postThreadMessage(int message, int intParam, void *ptrParam);	// post a message to the thread
    virtual void resumeThread();                            // this must be called to get thread going

    void exitThread();                                      // called to close thread down

    bool isRunning();                                       // returns true if task no exiting

    InternalThread *thread() { return m_thread; }

public slots:
    void internalRunLoop();
    void cleanup();

signals:
    void running();                                         // emitted when everything set up and thread active
    void internalEndThread();                               // this to end thread
    void internalKillThread();                              // tells the QThread to quit

protected:
    virtual	bool processMessage(SNCThreadMsg* msg);
    virtual void initThread();                              // called by resume thread internally
    virtual void finishThread() {}                          // called just before thread is destroyed

    inline void msleep(unsigned long msecs) { thread()->msleep(msecs); }
    bool eventFilter(QObject *obj, QEvent *event);

    int m_event;                                            // the event used for SNC thread message

private:
    QString m_name;                                         // the task name - for debugging mostly
    InternalThread *m_thread;                               // the underlying thread
};

#endif //_SNCTHREAD_H_

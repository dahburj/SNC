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

#include "SNCThread.h"
#include "SNCUtils.h"

SNCThreadMsg::SNCThreadMsg(QEvent::Type nEvent) : QEvent(nEvent)
{
}

SNCThread::SNCThread(const QString& threadName)
{
    m_name = threadName;
    m_event = QEvent::registerEventType();                  // get an event number
}

SNCThread::~SNCThread()
{
}


void SNCThread::resumeThread()
{
    m_thread = new InternalThread;

    moveToThread(m_thread);

    connect(m_thread, SIGNAL(started()), this, SLOT(internalRunLoop()));

    connect(this, SIGNAL(internalEndThread()), this, SLOT(cleanup()));

    connect(this, SIGNAL(internalKillThread()), m_thread, SLOT(quit()));

    connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));

    connect(m_thread, SIGNAL(finished()), this, SLOT(deleteLater()));

    installEventFilter(this);

    m_thread->start();
}

void SNCThread::cleanup()
{
    finishThread();
    emit internalKillThread();
}

void SNCThread::exitThread()
{
    emit internalEndThread();
}

void SNCThread::internalRunLoop()
{
    initThread();
    emit running();
}

void SNCThread::initThread()
{
}

bool SNCThread::processMessage(SNCThreadMsg* msg)
{
    SNCUtils::logDebug(m_name, QString("Message on default PTM - %1").arg(msg->type()));
    return true;
}

void SNCThread::postThreadMessage(int message, int intParam, void *ptrParam)
{
    SNCThreadMsg *msg = new SNCThreadMsg((QEvent::Type)m_event);
    msg->message = message;
    msg->intParam = intParam;
    msg->ptrParam = ptrParam;
    qApp->postEvent(this, msg);
}

bool SNCThread::eventFilter(QObject *obj, QEvent *event)
 {
     if (event->type() == m_event) {
        processMessage((SNCThreadMsg *)event);
        return true;
    }

    //	Just do default processing
    return QObject::eventFilter(obj, event);
 }

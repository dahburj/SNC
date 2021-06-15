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

#ifndef _SNCView_H_
#define _SNCView_H_

#ifdef USING_GSTREAMER
#define	PRODUCT_TYPE "SNCViewGS"
#else
#define	PRODUCT_TYPE "SNCView"
#endif

#include <QtGui>

#include "ui_SNCView.h"
#include "DisplayStats.h"
#include "ViewClient.h"
#include "ImageWindow.h"
#include "ViewSingleCamera.h"

#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
#include <QAudioOutput>
#else
#include <alsa/asoundlib.h>
#endif
#endif

#define SNCVIEW_MAXTABS   8                           // max 8 tabs

class ViewTab
{
public:
    QString m_name;
    QWidget *m_widget;
    QList<AVSource *> m_avSources;
    QList<ImageWindow *> m_windowList;

};

class SNCView : public QMainWindow
{
    Q_OBJECT

public:
    SNCView();

public slots:
    void onStats();
    void onAbout();
    void onBasicSetup();
    void onChooseVideoStreams();
    void onChooseSNCStore();
    void onAudioSetup();
    void onShowName();
    void onShowDate();
    void onShowTime();
    void onTextColor();
    void imageMousePress(QString name);
    void imageDoubleClick(QString name);
    void singleCameraClosed();
    void clientConnected();
    void clientClosed();
    void dirResponse(QStringList directory);
    void currentTabChanged(int index);

    void newAudio(QByteArray data, int rate, int channels, int size);

#if defined(Q_OS_OSX)
    void handleAudioOutStateChanged(QAudio::State);
#endif

signals:
    void requestDir();
    void addService(AVSource *avSource);
    void removeService(AVSource *avSource);
    void enableService(AVSource *avSource);
    void disableService(AVSource *avSource);
    void openSession(int sessionId);
    void closeSession(int sessionId);

protected:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    bool addAVSource(QString name, bool enableEnable = false);
    void removeAVSource(AVSource *avSource);

    void layoutGrid();
    void initStatusBar();
    void initMenus();
    void saveWindowState();
    void restoreWindowState();
    QByteArray convertAudioFormat(const QByteArray& audioData);

    Ui::SNCViewClass ui;

    ViewClient *m_client;
    QStringList m_clientDirectory;

    QList<AVSource *> m_avSourceList;
    QList<AVSource *> m_delayedDeleteList;

    QTabWidget *m_centralWidget;
    ViewTab m_tabs[SNCVIEW_MAXTABS];
    int m_currentTab;

    DisplayStats *m_displayStats;
    QLabel *m_controlStatus;

    int m_statusTimer;
    int m_directoryTimer;

    bool m_showName;
    bool m_showDate;
    bool m_showTime;
    QColor m_textColor;

    ViewSingleCamera *m_singleCamera;
    int m_selectedSource;

#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
    QAudioOutput *m_audioOut;
    QIODevice *m_audioOutDevice;
#else
    snd_pcm_t *m_audioOutHandle;
    bool m_audioOutIsOpen;
    int m_audioOutSampleSize;
#endif
#endif
    void audioOutClose();

    bool audioOutOpen(int rate, int channels, int size);
    bool audioOutWrite(const QByteArray& audioData);
    bool m_audioEnabled;
    int m_audioChannels;
    int m_audioSize;
    int m_audioRate;

    int m_outputRate;
    int m_outputChannels;

    QString m_logTag;
};

#endif // _SNCVIEW_H_

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

#include <qcolordialog.h>

#include "SNCView.h"
#include "AboutDlg.h"
#include "BasicSetupDlg.h"
#include "StreamDialog.h"
#include "AudioOutputDlg.h"
#include "CFSDialog.h"

#define GRID_SPACING 3

SNCView::SNCView()
    : QMainWindow()
{
    m_logTag = "SNCView";
    ui.setupUi(this);

    m_singleCamera = NULL;
    m_selectedSource = -1;

    m_audioSize = -1;
    m_audioRate = -1;
    m_audioChannels = -1;


#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
        m_audioOut = NULL;
        m_audioOutDevice = NULL;
#else
        m_audioOutIsOpen = false;
#endif

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    if (!settings->contains(AUDIO_OUTPUT_DEVICE))

#ifdef Q_OS_OSX
        settings->setValue(AUDIO_OUTPUT_DEVICE, AUDIO_DEFAULT_DEVICE_MAC);
#else
#ifdef Q_OS_LINUX
        settings->setValue(AUDIO_OUTPUT_DEVICE, 0);

    if (!settings->contains(AUDIO_OUTPUT_CARD))
        settings->setValue(AUDIO_OUTPUT_CARD, 0);
#else
        settings->setValue(AUDIO_OUTPUT_DEVICE, AUDIO_DEFAULT_DEVICE);
#endif
#endif

    if (!settings->contains(AUDIO_ENABLE))
        settings->setValue(AUDIO_ENABLE, true);

    m_audioEnabled = settings->value(AUDIO_ENABLE).toBool();

    settings->endGroup();
    delete settings;

#endif

    m_displayStats = new DisplayStats(this);

    SNCUtils::SNCAppInit();
    m_client = new ViewClient();

    connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

    connect(m_client, SIGNAL(clientConnected()), this, SLOT(clientConnected()));
    connect(m_client, SIGNAL(clientClosed()), this, SLOT(clientClosed()));

    connect(m_client, SIGNAL(dirResponse(QStringList)), this, SLOT(dirResponse(QStringList)));
    connect(this, SIGNAL(requestDir()), m_client, SLOT(requestDir()));

    connect(this, SIGNAL(addService(AVSource *)), m_client, SLOT(addService(AVSource *)));
    connect(this, SIGNAL(removeService(AVSource *)), m_client, SLOT(removeService(AVSource *)));
    connect(this, SIGNAL(enableService(AVSource *)), m_client, SLOT(enableService(AVSource *)));
    connect(this, SIGNAL(disableService(AVSource *)), m_client, SLOT(disableService(AVSource *)));

    m_client->resumeThread();

    m_statusTimer = startTimer(2000);
    m_directoryTimer = startTimer(10000);

    m_centralWidget = new QTabWidget();

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        m_tabs[tab].m_name = QString("View %1").arg(tab);
        m_tabs[tab].m_widget = new QWidget();
        m_centralWidget->addTab(m_tabs[tab].m_widget, m_tabs[tab].m_name);
    }
    setCentralWidget(m_centralWidget);

    restoreWindowState();
    initStatusBar();
    initMenus();

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        m_currentTab = tab;
        layoutGrid();
    }

    m_currentTab = -1;
    connect(m_centralWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    m_centralWidget->setCurrentIndex(0);

    setWindowTitle(QString("%1 - %2")
        .arg(SNCUtils::getAppType())
        .arg(SNCUtils::getAppName()));
}


void SNCView::onStats()
{
    m_displayStats->activateWindow();
    m_displayStats->show();
}

void SNCView::closeEvent(QCloseEvent *)
{
    killTimer(m_statusTimer);
    killTimer(m_directoryTimer);

    if (m_singleCamera) {
        disconnect(m_singleCamera, SIGNAL(closed()), this, SLOT(singleCameraClosed()));
        m_singleCamera->close();
    }

    if (m_client) {
        m_client->exitThread();
        m_client = NULL;
    }

    saveWindowState();

    SNCUtils::SNCAppExit();
}

void SNCView::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_directoryTimer) {
        emit requestDir();

        for (int i = 0; i < SNCVIEW_MAXTABS; i++) {
            while (m_delayedDeleteList.count() > 0) {
                qint64 lastUpdate = m_delayedDeleteList.at(0)->lastUpdate();

                if (!SNCUtils::timerExpired(SNCUtils::clock(), lastUpdate, 5 * SNC_CLOCKS_PER_SEC))
                    break;

                AVSource *avSource = m_delayedDeleteList.at(0);
                m_delayedDeleteList.removeAt(0);
                delete avSource;
            }
        }
    }
    else {
        m_controlStatus->setText(m_client->getLinkState());
    }
}

void SNCView::clientConnected()
{
    emit requestDir();
}

void SNCView::clientClosed()
{
    ui.actionVideoStreams->setEnabled(false);
    m_clientDirectory.clear();
}

void SNCView::dirResponse(QStringList directory)
{
    m_clientDirectory = directory;

    if (m_clientDirectory.length() > 0) {
        if (!ui.actionVideoStreams->isEnabled()) {
            ui.actionVideoStreams->setEnabled(true);
            ui.actionSNCStoreSelection->setEnabled(true);
        }
    }
}

void SNCView::singleCameraClosed()
{
    if (m_singleCamera) {
        delete m_singleCamera;
        m_singleCamera = NULL;
        emit closeSession(0);

        if (m_selectedSource >= 0 && m_selectedSource < m_tabs[m_currentTab].m_windowList.count()) {
            m_tabs[m_currentTab].m_windowList[m_selectedSource]->setSelected(false);
            m_tabs[m_currentTab].m_avSources[m_selectedSource]->enableAudio(false);
        }

        m_selectedSource = -1;
    }
}

void SNCView::imageMousePress(QString name)
{
    m_selectedSource = -1;

    for (int i = 0; i < m_tabs[m_currentTab].m_windowList.count(); i++) {
        if (m_tabs[m_currentTab].m_avSources.at(i)->name() == name) {
            if (m_tabs[m_currentTab].m_windowList[i]->selected()) {
                m_tabs[m_currentTab].m_windowList[i]->setSelected(false);
                m_tabs[m_currentTab].m_avSources[i]->enableAudio(false);
            } else {
                m_tabs[m_currentTab].m_windowList[i]->setSelected(true);
                m_tabs[m_currentTab].m_avSources[i]->enableAudio(true);
                m_selectedSource = i;
            }
        } else {
            m_tabs[m_currentTab].m_windowList[i]->setSelected(false);
            m_tabs[m_currentTab].m_avSources[i]->enableAudio(false);
        }
    }

    if (!m_singleCamera)
        return;

    if (m_selectedSource == -1)
        return;

    m_singleCamera->setSource(m_tabs[m_currentTab].m_avSources[m_selectedSource], m_tabs[m_currentTab].m_windowList.at(m_selectedSource));
}

void SNCView::imageDoubleClick(QString name)
{
    // mousePress handles this
    if (m_singleCamera)
        return;

    m_selectedSource = -1;

    for (int i = 0; i < m_tabs[m_currentTab].m_windowList.count(); i++) {
        if (m_tabs[m_currentTab].m_avSources.at(i)->name() == name) {
            m_selectedSource = i;
            break;
        }
    }

    if (m_selectedSource == -1)
        return;

    m_singleCamera = new ViewSingleCamera(NULL);

    if (!m_singleCamera)
        return;

    connect(m_singleCamera, SIGNAL(closed()), this, SLOT(singleCameraClosed()));
    connect(m_singleCamera, SIGNAL(getTimestamp(int, QString,qint64)), m_client, SLOT(getTimestamp(int, QString,qint64)));
    connect(m_client, SIGNAL(newImage(int, QImage, qint64)), m_singleCamera, SLOT(newHistoricImage(int, QImage,qint64)));
    connect(m_client, SIGNAL(newSensorData(int, QJsonObject, qint64)),
            m_singleCamera, SLOT(newHistoricSensorData(int, QJsonObject,qint64)));
    connect(m_client, SIGNAL(newCFSState(int, QString)), m_singleCamera, SLOT(newCFSState(int, QString)));
    connect(m_client, SIGNAL(newDirectory(QStringList)), m_singleCamera, SLOT(newDirectory(QStringList)));
    connect(this, SIGNAL(openSession(int)), m_client, SLOT(openSession(int)));
    connect(this, SIGNAL(closeSession(int)), m_client, SLOT(closeSession(int)));

    emit openSession(0);

    m_singleCamera->show();
    m_singleCamera->setSource(m_tabs[m_currentTab].m_avSources[m_selectedSource], m_tabs[m_currentTab].m_windowList.at(m_selectedSource));

    m_tabs[m_currentTab].m_windowList[m_selectedSource]->setSelected(true);
    m_tabs[m_currentTab].m_avSources[m_selectedSource]->enableAudio(true);
}

void SNCView::onShowName()
{
    m_showName = ui.actionShow_name->isChecked();

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        for (int i = 0; i < m_tabs[tab].m_windowList.count(); i++)
            m_tabs[tab].m_windowList[i]->setShowName(m_showName);
    }
}

void SNCView::onShowDate()
{
    m_showDate = ui.actionShow_date->isChecked();

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        for (int i = 0; i < m_tabs[tab].m_windowList.count(); i++)
            m_tabs[tab].m_windowList[i]->setShowDate(m_showDate);
    }
}

void SNCView::onShowTime()
{
    m_showTime = ui.actionShow_time->isChecked();

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        for (int i = 0; i < m_tabs[tab].m_windowList.count(); i++)
            m_tabs[tab].m_windowList[i]->setShowTime(m_showTime);
    }
}

void SNCView::onTextColor()
{
    m_textColor = QColorDialog::getColor(m_textColor, this);

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {
        for (int i = 0; i < m_tabs[tab].m_windowList.count(); i++)
            m_tabs[tab].m_windowList[i]->setTextColor(m_textColor);
    }
}

void SNCView::onChooseVideoStreams()
{
    QStringList oldStreams;

    for (int i = 0; i < m_tabs[m_currentTab].m_avSources.count(); i++)
        oldStreams.append(m_tabs[m_currentTab].m_avSources.at(i)->name());

    StreamDialog dlg(this, m_clientDirectory, oldStreams);

    if (dlg.exec() != QDialog::Accepted)
        return;

    if (m_singleCamera)
        m_singleCamera->close();

    QStringList newStreams = dlg.newStreams();

    QList<AVSource *> oldSourceList = m_tabs[m_currentTab].m_avSources;
    m_tabs[m_currentTab].m_avSources.clear();

    // don't tear down existing streams if we can rearrange them
    for (int i = 0; i < newStreams.count(); i++) {
        int oldIndex = oldStreams.indexOf(newStreams.at(i));

        if (oldIndex == -1) {
            addAVSource(newStreams.at(i), true);
        } else {
            m_tabs[m_currentTab].m_avSources.append(oldSourceList.at(oldIndex));
        }
    }

    // delete streams we no longer need
    for (int i = 0; i < oldStreams.count(); i++) {
        if (newStreams.contains(oldStreams.at(i)))
            continue;

        AVSource *avSource = oldSourceList.at(i);
        removeAVSource(avSource);
    }

    layoutGrid();
    saveWindowState();
}

void SNCView::onChooseSNCStore()
{
    ui.actionSNCStoreSelection->setEnabled(false);
    CFSDialog dlg(this, m_clientDirectory);

    if (dlg.exec()) {
//        emit newCFSList();
    }
    ui.actionSNCStoreSelection->setEnabled(true);
}

bool SNCView::addAVSource(QString name, bool enableEnable)
{
    AVSource *avSource = NULL;

    // see if source already in use

    for (int i = 0; i < m_avSourceList.count(); i++) {
        if (m_avSourceList.at(i)->name() == name) {
            // found it
            avSource = m_avSourceList.at(i);
        }
    }
    if (avSource == NULL) {
        // need new one

        avSource = new AVSource(name);

        if (!avSource)
            return false;

        m_avSourceList.append(avSource);
        connect(avSource, SIGNAL(newAudio(QByteArray, int, int, int)), this, SLOT(newAudio(QByteArray, int, int, int)));
        m_displayStats->addSource(avSource);
        emit addService(avSource);
        if (avSource->isSensor() || enableEnable)
            emit enableService(avSource);
    }
    avSource->incRefCount();
    m_tabs[m_currentTab].m_avSources.append(avSource);
    if (avSource->isSensor() || enableEnable)
        emit enableService(avSource);

    return true;
}

void SNCView::removeAVSource(AVSource *avSource)
{

    avSource->decRefCount();
    if (avSource->getRefCount() <= 0) {

        // nobody is using this any more
        // we will delete 5 seconds from now

        avSource->setLastUpdate(SNCUtils::clock());
        disconnect(avSource, SIGNAL(newAudio(QByteArray, int, int, int)), this, SLOT(newAudio(QByteArray, int, int, int)));
        m_displayStats->removeSource(avSource->name());
        m_delayedDeleteList.append(avSource);
        if (avSource->servicePort() >= 0) {
            emit removeService(avSource);
        }

        for (int i = 0; i < m_avSourceList.count(); i++) {
            if (m_avSourceList.at(i) == avSource) {
                m_avSourceList.removeAt(i);
                return;
            }
        }
    }
}

void SNCView::currentTabChanged(int index)
{
    if (m_currentTab == index)
        return;

    if (m_currentTab != -1) {
        for (int i = 0; i < m_tabs[m_currentTab].m_avSources.count(); i++) {
            if (!m_tabs[m_currentTab].m_avSources[i]->isSensor()) {
                emit disableService(m_tabs[m_currentTab].m_avSources[i]);
            }
        }
    }
    for (int i = 0; i < m_tabs[index].m_avSources.count(); i++) {
        if ((m_currentTab == -1) || (!m_tabs[index].m_avSources[i]->isSensor())) {
            emit enableService(m_tabs[index].m_avSources[i]);
        }
    }
    m_currentTab = index;
}

void SNCView::layoutGrid()
{
    int rows = 1;
    int count = m_tabs[m_currentTab].m_avSources.count();

    m_centralWidget->removeTab(m_currentTab);
    m_tabs[m_currentTab].m_widget = new QWidget();

    for (int i = 0; i < m_tabs[m_currentTab].m_windowList.count(); i++) {
        delete m_tabs[m_currentTab].m_windowList.at(i);
    }

    m_tabs[m_currentTab].m_windowList.clear();


    if (count > 30)
        count = 30;

    if (count < 3)
        rows = 1;
    else if (count < 7)
        rows = 2;
    else if (count < 13)
        rows = 3;
    else if (count < 23)
        rows = 4;
    else
        rows = 5;

    int cols = count / rows;

    if (count % rows)
        cols++;

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(3);
    grid->setContentsMargins(1, 1, 1, 1);

    for (int i = 0, k = 0; i < rows && k < count; i++) {
        for (int j = 0; j < cols && k < count; j++) {
            ImageWindow *win = new ImageWindow(m_tabs[m_currentTab].m_avSources.at(k),
                                  m_showName, m_showDate, m_showTime, m_textColor, m_tabs[m_currentTab].m_widget);
            connect(win, SIGNAL(imageMousePress(QString)), this, SLOT(imageMousePress(QString)));
            connect(win, SIGNAL(imageDoubleClick(QString)), this, SLOT(imageDoubleClick(QString)));
            m_tabs[m_currentTab].m_windowList.append(win);

            grid->addWidget(m_tabs[m_currentTab].m_windowList.at(k), i, j);
            k++;
        }
    }

    for (int i = 0; i < rows; i++)
        grid->setRowStretch(i, 1);

    for (int i = 0; i < cols; i++)
        grid->setColumnStretch(i, 1);

    m_tabs[m_currentTab].m_widget->setLayout(grid);
    m_centralWidget->insertTab(m_currentTab, m_tabs[m_currentTab].m_widget, m_tabs[m_currentTab].m_name);
    m_centralWidget->setCurrentIndex(m_currentTab);
}

void SNCView::initStatusBar()
{
    m_controlStatus = new QLabel(this);
    m_controlStatus->setAlignment(Qt::AlignLeft);
    ui.statusBar->addWidget(m_controlStatus, 1);
}

void SNCView::initMenus()
{
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
    connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));

    connect(ui.actionVideoStreams, SIGNAL(triggered()), this, SLOT(onChooseVideoStreams()));
    ui.actionVideoStreams->setEnabled(false);

    connect(ui.actionSNCStoreSelection, SIGNAL(triggered()), this, SLOT(onChooseSNCStore()));
    ui.actionSNCStoreSelection->setEnabled(false);

    connect(ui.actionAudioSetup, SIGNAL(triggered()), this, SLOT(onAudioSetup()));

    connect(ui.onStats, SIGNAL(triggered()), this, SLOT(onStats()));
    connect(ui.actionShow_name, SIGNAL(triggered()), this, SLOT(onShowName()));
    connect(ui.actionShow_date, SIGNAL(triggered()), this, SLOT(onShowDate()));
    connect(ui.actionShow_time, SIGNAL(triggered()), this, SLOT(onShowTime()));
    connect(ui.actionText_color, SIGNAL(triggered()), this, SLOT(onTextColor()));

    ui.actionShow_name->setChecked(m_showName);
    ui.actionShow_date->setChecked(m_showDate);
    ui.actionShow_time->setChecked(m_showTime);
    ui.actionSNCStoreSelection->setEnabled(false);
}

void SNCView::saveWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("Window");
    settings->setValue("Geometry", saveGeometry());
    settings->setValue("State", saveState());
    settings->setValue("showName", m_showName);
    settings->setValue("showDate", m_showDate);
    settings->setValue("showTime", m_showTime);
    settings->setValue("textColor", m_textColor);
    settings->endGroup();

    settings->beginWriteArray("tabs");

    for (int tab = 0; tab < SNCVIEW_MAXTABS; tab++) {

        settings->setArrayIndex(tab);

        settings->setValue("tabName", m_tabs[tab].m_name);

        settings->beginWriteArray("streamSources");

        for (int i = 0; i < m_tabs[tab].m_avSources.count(); i++) {
            settings->setArrayIndex(i);
            settings->setValue("source", m_tabs[tab].m_avSources[i]->name());
        }

        settings->endArray();
    }
    settings->endArray();

    delete settings;
}

void SNCView::restoreWindowState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("Window");
    restoreGeometry(settings->value("Geometry").toByteArray());
    restoreState(settings->value("State").toByteArray());

    if (settings->contains("showName"))
        m_showName = settings->value("showName").toBool();
    else
        m_showName = true;

    if (settings->contains("showDate"))
        m_showDate = settings->value("showDate").toBool();
    else
        m_showDate = true;

    if (settings->contains("showTime"))
        m_showTime = settings->value("showTime").toBool();
    else
        m_showTime = true;

    if (settings->contains("textColor"))
        m_textColor = settings->value("textColor").value<QColor>();
    else
        m_textColor = Qt::white;

    settings->endGroup();

    int tabCount = settings->beginReadArray("tabs");

    for (int tab = 0; tab < tabCount; tab++) {
        m_currentTab = tab;
        settings->setArrayIndex(tab);

        QString tabName = settings->value("tabName").toString();
        if (tabName != "") {
            m_tabs[tab].m_name = tabName;
            m_centralWidget->setTabText(tab, tabName);
        }
        int count = settings->beginReadArray("streamSources");

        for (int i = 0; i < count; i++) {
            settings->setArrayIndex(i);
            QString name = settings->value("source", "").toString();

            if (name.length() > 0)
                addAVSource(name);
        }

        settings->endArray();
    }

    settings->endArray();

    m_currentTab = 0;

    delete settings;
}

void SNCView::onAudioSetup()
{
#if !defined(Q_OS_WIN)
    AudioOutputDlg *aod = new AudioOutputDlg(this);
    if (aod->exec()) {
        audioOutClose();
        QSettings *settings = SNCUtils::getSettings();
        settings->beginGroup(AUDIO_GROUP);
        m_audioEnabled = settings->value(AUDIO_ENABLE).toBool();
        settings->endGroup();
        delete settings;
    }
#endif
}

void SNCView::onAbout()
{
    AboutDlg dlg(this);
    dlg.exec();
}

void SNCView::onBasicSetup()
{
    BasicSetupDlg dlg(this);
    dlg.exec();
}

void SNCView::newAudio(QByteArray data, int rate, int channels, int size)
{
#if !defined(Q_OS_WIN)
    if (!m_audioEnabled)
        return;

    if ((m_audioRate != rate) || (m_audioSize != size) || (m_audioChannels != channels)) {
        if (!audioOutOpen(rate, channels, size)) {
            qDebug() << "Failed to open audio out device";
            return;
        }

        m_audioRate = rate;
        m_audioSize = size;
        m_audioChannels = channels;
    }

    if (!audioOutWrite(data))
        qDebug() << "Audio write failed";
#endif
}

#if !defined(Q_OS_WIN)
#if defined(Q_OS_OSX)
bool SNCView::audioOutOpen(int rate, int channels, int size)
{
    QString outputDeviceName;
    QAudioDeviceInfo outputDeviceInfo;
    bool found = false;
    int bufferSize;

    if (m_audioOut != NULL)
        audioOutClose();

    if ((rate == 0) || (channels == 0) || (size == 0))
        return false;

    QSettings *settings = SNCUtils::getSettings();
    settings->beginGroup(AUDIO_GROUP);
    outputDeviceName = settings->value(AUDIO_OUTPUT_DEVICE).toString();
    settings->endGroup();
    delete settings;

    if (outputDeviceName != AUDIO_DEFAULT_DEVICE) {
        foreach (const QAudioDeviceInfo& deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
/*            qDebug() << "Device name: " << deviceInfo.deviceName();
            qDebug() << "    codec: " << deviceInfo.supportedCodecs();
            qDebug() << "    channels:" << deviceInfo.supportedChannelCounts();
            qDebug() << "    rates:" << deviceInfo.supportedSampleRates();
            qDebug() << "    sizes:" << deviceInfo.supportedSampleSizes();
            qDebug() << "    types:" << deviceInfo.supportedSampleTypes();
            qDebug() << "    order:" << deviceInfo.supportedByteOrders();*/

            if (deviceInfo.deviceName() == outputDeviceName) {
                outputDeviceInfo = deviceInfo;
                found = true;
            }
        }
        if (!found) {
            qWarning() << "Could not find audio device " << outputDeviceName;
            return false;
        }
    } else {
        outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    }

    //  check if need to map audio data

    QList<int> rateList = outputDeviceInfo.supportedSampleRates();
    QList<int> channelList = outputDeviceInfo.supportedChannelCounts();

    m_outputRate = rate;
    m_outputChannels = channels;

    for (int i = 0; i < rateList.count(); i++) {
        if (rateList.at(i) < rate)
            continue;                                       // don't want this mapping
        if (rateList.at(i) == rate)
            break;                                          // phew - have correct rate
        if (((rateList.at(i) / rate) * rate) == rateList.at(i)) {
            m_outputRate = rateList.at(i);                  // this is the first integer multiple that's supported
            break;
        }
    }

    for (int i = 0; i < channelList.count(); i++) {
        if (channelList.at(i) == channels)
            break;                                          // got the exact match
        if (((channelList.at(i) / channels) * channels) == channelList.at(i)) {
            m_outputChannels = channelList.at(i);           // this is the first integer multiple that's supported
        }
    }
    SNCUtils::logDebug(m_logTag, QString("Audio using %1 sps and %2 channels").arg(m_outputRate).arg(m_outputChannels));

    QAudioFormat format;

    bufferSize = m_outputRate * m_outputChannels * (size / 8);
    format.setSampleRate(m_outputRate);
    format.setChannelCount(m_outputChannels);

    format.setSampleSize(size);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

     if (!outputDeviceInfo.isFormatSupported(format)) {
        qWarning() << "Cannot play audio.";
        return false;
    }

    if (m_audioOut != NULL) {
        delete m_audioOut;
        m_audioOut = NULL;
        m_audioOutDevice = NULL;
    }

    m_audioOut = new QAudioOutput(outputDeviceInfo, format, this);
    m_audioOut->setBufferSize(bufferSize);

//    qDebug() << "Buffer size: " << m_audioOut->bufferSize();

//    connect(m_audioOut, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleAudioOutStateChanged(QAudio::State)));

    m_audioOutDevice = m_audioOut->start();

    return true;
}

void SNCView::audioOutClose()
{
    if (m_audioOut != NULL) {
        delete m_audioOut;
        m_audioOut = NULL;
    }

    m_audioOutDevice = NULL;
    m_audioRate = -1;
    m_audioSize = -1;
    m_audioChannels = -1;
}

bool SNCView::audioOutWrite(const QByteArray& audioData)
{
    if (m_audioOutDevice == NULL)
        return false;

    QByteArray newData = convertAudioFormat(audioData);
    return m_audioOutDevice->write(newData) == newData.length();
}

void SNCView::handleAudioOutStateChanged(QAudio::State /* state */)
{
//	qDebug() << "Audio state " << state;
}

#else

bool SNCView::audioOutOpen(int rate, int channels, int size)
{
    int err;
    snd_pcm_hw_params_t *params;
    QString deviceString;

    if (m_audioOutIsOpen)
        audioOutClose();
    if ((rate == 0) || (channels == 0) || (size == 0))
        return false;

    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup(AUDIO_GROUP);

    int device = settings->value(AUDIO_OUTPUT_DEVICE).toInt();
    int card = settings->value(AUDIO_OUTPUT_CARD).toInt();

    settings->endGroup();
    delete settings;

    deviceString = QString("plughw:%1,%2").arg(card).arg(device);

    if ((err = snd_pcm_open(&m_audioOutHandle, qPrintable(deviceString), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        return false;
    }
    snd_pcm_format_t sampleSize;

    switch (size) {
    case 8:
        sampleSize = SND_PCM_FORMAT_S8;
        break;

    case 32:
        sampleSize = SND_PCM_FORMAT_S32_LE;
        break;

    default:
        sampleSize = SND_PCM_FORMAT_S16_LE;
        break;

   }

    params = NULL;
    if (snd_pcm_hw_params_malloc(&params) < 0)
        goto openError;
    if (snd_pcm_hw_params_any(m_audioOutHandle, params) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_access(m_audioOutHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_format(m_audioOutHandle, params, sampleSize) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_rate_near(m_audioOutHandle, params, (unsigned int *)&rate, 0) < 0)
        goto openError;
    if (snd_pcm_hw_params_set_channels(m_audioOutHandle, params, channels) < 0)
        goto openError;
    if (snd_pcm_hw_params(m_audioOutHandle, params) < 0)
        goto openError;
    if (snd_pcm_nonblock(m_audioOutHandle, 1) < 0)
        goto openError;
    snd_pcm_hw_params_free(params);
    params = NULL;

    if ((err = snd_pcm_prepare(m_audioOutHandle)) < 0)
        goto openError;

    m_audioOutSampleSize = channels * (size / 8);                       // bytes per sample

    m_audioOutIsOpen = true;
    return true;

openError:
    snd_pcm_close(m_audioOutHandle);
    if (params != NULL)
        snd_pcm_hw_params_free(params);
    m_audioOutIsOpen = false;
    return false;
}

void SNCView::audioOutClose()
{
    if (m_audioOutIsOpen)
        snd_pcm_close(m_audioOutHandle);
    m_audioOutIsOpen = false;
    m_audioRate = -1;
    m_audioSize = -1;
    m_audioChannels = -1;
}

bool SNCView::audioOutWrite(const QByteArray& audioData)
{
    int writtenLength;
    int samples = audioData.length() / m_audioOutSampleSize;

    writtenLength = snd_pcm_writei(m_audioOutHandle, audioData.constData(), samples);
    if (writtenLength != samples) {
        qDebug() << "Audio write error " << writtenLength << " is not equal to " << samples;
        snd_pcm_prepare(m_audioOutHandle);
        for (int i = 0; i < 2; i++)
            snd_pcm_writei(m_audioOutHandle, audioData.constData(), samples);
        writtenLength = snd_pcm_writei(m_audioOutHandle, audioData.constData(), samples);
        if (writtenLength != samples) {
            qDebug() << "Audio retry error " << writtenLength << " is not equal to " << samples;
        }
    }
    return writtenLength == samples;
}
#endif
#endif


QByteArray SNCView::convertAudioFormat(const QByteArray& audioData)
{
    QByteArray newData;
    int sampleLength;
    int sampleBytes;
    int sampleCount;
    int sampleOffset;
    int sampleCopyCount;

    if ((m_outputRate == m_audioRate) && (m_outputChannels == m_audioChannels))
        return audioData;

    sampleCopyCount = (m_outputRate / m_audioRate) * (m_outputChannels / m_audioChannels);

    sampleBytes = m_audioSize / 8;
    sampleLength = sampleBytes * m_audioChannels;
    sampleCount = audioData.count() / sampleLength;

    sampleOffset = 0;

    for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++, sampleOffset += sampleLength) {
        for (int sampleCopy = 0; sampleCopy < sampleCopyCount; sampleCopy++) {
            for (int i = 0; i < sampleLength; i++)
                newData.append(audioData.at(sampleOffset + i));
        }
    }
    return newData;
}

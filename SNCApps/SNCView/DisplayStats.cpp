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

#include "DisplayStats.h"
#include "SNCUtils.h"

#include <qsettings.h>

#define STAT_UPDATE_INTERVAL_SECS 2

DisplayStats::DisplayStats(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_logTag = "DisplayStats";

    ui.setupUi(this);

    ui.statusCounts->setColumnCount(5);
    ui.statusCounts->setColumnWidth(0, 120);
    ui.statusCounts->setColumnWidth(1, 120);
    ui.statusCounts->setColumnWidth(2, 120);
    ui.statusCounts->setColumnWidth(3, 120);
    ui.statusCounts->setColumnWidth(4, 120);

    ui.statusCounts->setHorizontalHeaderLabels(
                QStringList() << tr("Stream") << tr("RX records")
                << tr("RX bytes") << tr("RX record rate") << tr("RX byte rate"));


    ui.statusCounts->setSelectionMode(QAbstractItemView::NoSelection);

    ui.statusCounts->setMinimumSize(200, 60);

    m_timer = startTimer(STAT_UPDATE_INTERVAL_SECS * SNC_CLOCKS_PER_SEC);
    hide();
}

DisplayStats::~DisplayStats()
{
    killTimer(m_timer);
}

void DisplayStats::showEvent(QShowEvent *)
{
    restoreDialogState();
}

void DisplayStats::closeEvent(QCloseEvent *event)
{
    saveDialogState();
    hide();
    event->ignore();
}

void DisplayStats::addSource(AVSource *avSource)
{
    int row = ui.statusCounts->rowCount();

    DisplayStatsData *stats = avSource->stats();

    if (!stats)
        return;

    ui.statusCounts->insertRow(row);
    ui.statusCounts->setRowHeight(row, DEFAULT_ROW_HEIGHT);

    QTableWidgetItem *item = new QTableWidgetItem(avSource->name());
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
    ui.statusCounts->setItem(row, 0, item);

    item = new QTableWidgetItem(QString::number(stats->totalRecords()));
    ui.statusCounts->setItem(row, 1, item);

    item = new QTableWidgetItem(formatByteTotalForDisplay(stats->totalBytes()));
    ui.statusCounts->setItem(row, 2, item);

    item = new QTableWidgetItem(QString::number(stats->recordRate(), 'f', 1));
    ui.statusCounts->setItem(row, 3, item);

    item = new QTableWidgetItem(QString::number(stats->byteRate(), 'f', 0));
    ui.statusCounts->setItem(row, 4, item);

    m_avSources.append(avSource);
}

void DisplayStats::removeSource(QString name)
{
    int row = -1;

    for (int i = 0; i < m_avSources.count(); i++) {
        if (m_avSources.at(i)->name() == name) {
            row = i;
            break;
        }
    }

    if (row == -1)
        return;

    ui.statusCounts->removeRow(row);
    m_avSources.removeAt(row);
}

void DisplayStats::timerEvent(QTimerEvent *)
{
    for (int i = 0; i < m_avSources.count(); i++) {
        DisplayStatsData *stats = m_avSources.at(i)->stats();

        if (stats) {
            ui.statusCounts->item(i, 1)->setText(QString::number(stats->totalRecords()));
            ui.statusCounts->item(i, 2)->setText(formatByteTotalForDisplay(stats->totalBytes()));
            ui.statusCounts->item(i, 3)->setText(QString::number(stats->recordRate(), 'f', 1));
            ui.statusCounts->item(i, 4)->setText(QString::number(stats->byteRate(), 'f', 0));
        }
    }
}

QString DisplayStats::formatByteTotalForDisplay(qint64 bytes)
{
    qreal val;

    if (bytes < 1000) {
        return QString::number(bytes);
    }
    else if (bytes < 1000000) {
        val = bytes;
        val /= 1000.0;
        return QString::number(val, 'f', 2) + " k";
    }
    else if (bytes < 1000000000) {
        val = bytes;
        val /= 1000000.0;
        return QString::number(val, 'f', 2) + " M";
    }
    else {
        val = bytes;
        val /= 1000000000.0;
        return QString::number(val, 'f', 2) + " G";
    }
}

void DisplayStats::saveDialogState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("StatsDialog");
    settings->setValue("Geometry", saveGeometry());
    settings->endGroup();

    delete settings;
}

void DisplayStats::restoreDialogState()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->beginGroup("StatsDialog");
    restoreGeometry(settings->value("Geometry").toByteArray());
    settings->endGroup();

    delete settings;
}

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

#include <qboxlayout.h>
#include <qformlayout.h>
#include <qlabel.h>

#include "UpdateDialog.h"

UpdateDialog::UpdateDialog(const QString& app, const QString& dialogName, QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_app = app;
    m_dialogName = dialogName;
    layoutWindow();
    m_timer = startTimer(100);
    m_tickCount = 0;
}

UpdateDialog::~UpdateDialog()
{
    if (m_timer != -1)
        killTimer(m_timer);
    m_timer = -1;
}

void UpdateDialog::timerEvent(QTimerEvent * /* event */)
{
    m_bar->setValue(m_tickCount++);
    if (m_tickCount == UPDATEDIALOG_WAIT_TICKS) {
        killTimer(m_timer);
        m_timer = -1;
        QMessageBox::warning(this, "Update dialog",
                         QString("The attempt to update the dialog data for ") + m_app + " timed out.",
                                 QMessageBox::Ok);
        reject();
    }
}

void UpdateDialog::receiveDialogData(QJsonObject json)
{
    if (json[SNCJSON_RECORD_TYPE].toString() != SNCJSON_RECORD_TYPE_DIALOGUPDATE)
        return;

    QJsonObject param = json[SNCJSON_RECORD_PARAM].toObject();
    if (param[SNCJSON_PARAM_DIALOG_NAME].toString() != m_dialogName)
        return;

    accept();
}


void UpdateDialog::layoutWindow()
{
    setModal(true);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(20);
    vLayout->setContentsMargins(11, 11, 11, 11);
    QLabel *label = new QLabel(QString("Sending dialog data update to ") + m_app);
    vLayout->addWidget(label, Qt::AlignCenter);

    vLayout->addSpacing(20);

    m_bar = new QProgressBar();
    m_bar->setRange(0, UPDATEDIALOG_WAIT_TICKS);
    vLayout->addWidget(m_bar, Qt::AlignCenter);
}

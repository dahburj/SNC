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

#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qlistwidget.h>


class StreamDialog : public QDialog
{
    Q_OBJECT

public:
    StreamDialog(QWidget *parent, QStringList directory, QStringList currentStreams);

    QStringList newStreams();

public slots:
    void onAddStreams();
    void onRemoveStreams();
    void onMoveUp();
    void onMoveDown();
    void onCurrentStreamsSelectionChanged();

private:
    void layoutWindow();
    void parseAvailableServices(QStringList directory);

    QStringList m_currentStreams;
    QStringList m_availableStreams;

    QListWidget *m_currentList;
    QListWidget *m_availableList;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // STREAMDIALOG_H

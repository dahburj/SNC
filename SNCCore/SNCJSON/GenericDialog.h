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

#ifndef GENERICDIALOG_H
#define GENERICDIALOG_H

#include <qdialog.h>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qlist.h>
#include <qprogressbar.h>
#include <qformlayout.h>
#include <qpushbutton.h>

#include "SNCJSON.h"
#include "Dialog.h"

#define GENERICDIALOG_WAIT_TICKS       50

class HexValidator : public QValidator
{
    Q_OBJECT

public:
    HexValidator(unsigned int min, unsigned int max) { m_min = min; m_max = max;}
    QValidator::State validate(QString &input, int &pos) const;

private:
    unsigned int m_min;
    unsigned int m_max;
};

class UIDValidator : public QValidator
{
    Q_OBJECT

public:
    QValidator::State validate(QString &input, int &pos) const;
};

class ServiceNameValidator : public QValidator
{
    Q_OBJECT

    QValidator::State validate(QString &input, int &pos) const;
};

class ServicePathValidator : public QValidator
{
    Q_OBJECT

    QValidator::State validate(QString &input, int &pos) const;
};

class DialogButton : public QPushButton
{
    Q_OBJECT

public:
    DialogButton(const QString& text, const QString& dialogName, int row, QWidget *parent);

public slots:
    void originalClicked(bool);

signals:
    void buttonClicked(const QString&, int);

private:
    int m_row;
    QString m_dialogName;
};

class GenericDialog : public QDialog
{
    Q_OBJECT

public:
    GenericDialog(const QString& app, const QString& dialogName, const QString& dialogDesc,
                  const QJsonObject& param, QWidget *parent);      // if fetching remotely
    GenericDialog(const QString& app, Dialog *dialog, const QJsonObject& param, QWidget *parent);  // if we have the dialog already
    ~GenericDialog();

public slots:
    void onOk();
    void clicked(QAbstractButton *);
    void updateRateChanged(int);
    void receiveDialogData(QJsonObject json);
    void receiveSubDialogData(QJsonObject json);
    QJsonObject getUpdatedDialog();
    void buttonClicked(const QString&, int);

signals:
    void sendDialogData(QJsonObject json);
    void sendSubDialogData(QJsonObject json);

protected:
    void timerEvent(QTimerEvent *event);

private:
    void layoutWindow();
    void layoutDialogWindow();
    void updateDialogWindow();
    void processDialog();
    void singleUpdate();

    int m_getTimer;
    int m_updateTimer;

    int m_tickCount;
    QProgressBar *m_bar;
    QLabel *m_label;
    QFormLayout *m_formLayout;

    QString m_app;
    QString m_dialogName;
    QString m_dialogDesc;
    QString m_dialogCookie;
    QJsonObject m_param;
    bool m_dialogUpdateFlag;

    QDialogButtonBox *m_buttons;

    ServiceNameValidator m_serviceValidator;
    ServicePathValidator m_pathValidator;

    Dialog *m_dialog;
    QJsonObject m_jsonDialog;
    QJsonObject m_updatedDialog;

    QJsonArray m_varArray;
    QList<QWidget *> m_varWidgets;

    bool m_local;
    bool m_update;
    int m_updateTickCount;
    int m_autoUpdateTicks;

    QLabel *m_status;

    bool m_subDialogMode;
};

#endif // GENERICDIALOG_H

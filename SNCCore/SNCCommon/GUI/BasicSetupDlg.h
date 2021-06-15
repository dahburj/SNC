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

#ifndef BASICSETUPDLG_H
#define BASICSETUPDLG_H

#include <QDialog>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>

#include "SNCDefs.h"
#include "SNCEndpoint.h"
#include "GenericDialog.h"

class BasicSetupDlg : public QDialog
{
    Q_OBJECT

public:
    BasicSetupDlg(QWidget *parent = 0);
    ~BasicSetupDlg();

public slots:
    void onOk();

private:
    void layoutWindow();
    void populateAdaptors();

    QLineEdit *m_controlName[SNCENDPOINT_MAX_SNCCONTROLS];
    QLineEdit *m_appName;
    QDialogButtonBox *m_buttons;
    ServiceNameValidator *m_validator;

    QComboBox *m_adaptor;
    QCheckBox *m_revert;
    QCheckBox *m_encrypt;
};

#endif // BASICSETUPDLG_H

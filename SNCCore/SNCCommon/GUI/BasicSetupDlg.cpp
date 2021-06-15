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

#include "BasicSetupDlg.h"
#include "SNCUtils.h"
#include <qboxlayout.h>
#include <qformlayout.h>

BasicSetupDlg::BasicSetupDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    layoutWindow();
    setWindowTitle("Basic setup");
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

BasicSetupDlg::~BasicSetupDlg()
{
    delete m_validator;
}

void BasicSetupDlg::onOk()
{
    QMessageBox msgBox;

    QSettings *settings = SNCUtils::getSettings();

    // check to see if any setting has changed

    if (m_appName->text() != settings->value(SNC_PARAMS_APPNAME).toString())
        goto goChanged;

    settings->beginReadArray(SNC_PARAMS_CONTROL_NAMES);
    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
        settings->setArrayIndex(control);
        if (m_controlName[control]->text() != settings->value(SNC_PARAMS_CONTROL_NAME).toString()) {
            settings->endArray();
            goto goChanged;
        }
    }
    settings->endArray();

    if ((m_revert->checkState() == Qt::Checked) != settings->value(SNC_PARAMS_CONTROLREVERT).toBool())
        goto goChanged;

    if (m_adaptor->currentText() == "<any>") {
        if (settings->value(SNC_RUNTIME_ADAPTER).toString() != "")
            goto goChanged;
    } else {
        if (m_adaptor->currentText() != settings->value(SNC_RUNTIME_ADAPTER).toString())
            goto goChanged;
    }

    if ((m_encrypt->checkState() == Qt::Checked) != settings->value(SNC_PARAMS_ENCRYPT_LINK).toBool())
        goto goChanged;


    delete settings;

    reject();
    return;

goChanged:
    // save changes to settings

    settings->setValue(SNC_PARAMS_APPNAME, m_appName->text());

    settings->beginWriteArray(SNC_PARAMS_CONTROL_NAMES);
    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
        settings->setArrayIndex(control);
        settings->setValue(SNC_PARAMS_CONTROL_NAME, m_controlName[control]->text());
    }
    settings->endArray();

    settings->setValue(SNC_PARAMS_CONTROLREVERT, m_revert->checkState() == Qt::Checked);
    settings->setValue(SNC_PARAMS_ENCRYPT_LINK, m_encrypt->checkState() == Qt::Checked);

    if (m_adaptor->currentText() == "<any>")
        settings->setValue(SNC_RUNTIME_ADAPTER, "");
    else
        settings->setValue(SNC_RUNTIME_ADAPTER, m_adaptor->currentText());

    settings->sync();

    msgBox.setText("The component must be restarted for these changes to take effect");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    delete settings;

    accept();
}

void BasicSetupDlg::layoutWindow()
{
    QSettings *settings = SNCUtils::getSettings();

    setModal(true);

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->setSpacing(20);
    centralLayout->setContentsMargins(11, 11, 11, 11);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(16);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_appName = new QLineEdit(this);
    m_appName->setToolTip("The name of this instance of the app.");
    m_appName->setMinimumWidth(200);
    formLayout->addRow(tr("App name:"), m_appName);
    m_validator = new ServiceNameValidator();
    m_appName->setValidator(m_validator);
    m_appName->setText(settings->value(SNC_PARAMS_APPNAME).toString());

    settings->beginReadArray(SNC_PARAMS_CONTROL_NAMES);
    for (int control = 0; control < SNCENDPOINT_MAX_SNCCONTROLS; control++) {
        settings->setArrayIndex(control);
        m_controlName[control] = new QLineEdit(this);
        m_controlName[control]->setToolTip("The component name of the SNCControl to which the app should connect.\n If empty, use any available SNCControl.");
        m_controlName[control]->setMinimumWidth(200);
        formLayout->addRow(tr("SNCControl (priority %1):").arg(SNCENDPOINT_MAX_SNCCONTROLS - control), m_controlName[control]);
        m_controlName[control]->setText(settings->value(SNC_PARAMS_CONTROL_NAME).toString());
        m_controlName[control]->setValidator(m_validator);
    }
    settings->endArray();

    m_revert = new QCheckBox();
    formLayout->addRow(tr("Revert to best SNCControl:"), m_revert);
    if (settings->value(SNC_PARAMS_CONTROLREVERT).toBool())
        m_revert->setCheckState(Qt::Checked);

    m_adaptor = new QComboBox;
    m_adaptor->setEditable(false);
    populateAdaptors();
    QHBoxLayout *a = new QHBoxLayout;
    a->addWidget(m_adaptor);
    a->addStretch();
    formLayout->addRow(new QLabel("Ethernet adaptor:"), a);
    int findIndex = m_adaptor->findText(settings->value(SNC_RUNTIME_ADAPTER).toString());
    if (findIndex != -1)
        m_adaptor->setCurrentIndex(findIndex);
    else
        m_adaptor->setCurrentIndex(0);

    m_encrypt = new QCheckBox();
    formLayout->addRow(tr("Encrypt link:"), m_encrypt);
    if (settings->value(SNC_PARAMS_ENCRYPT_LINK).toBool())
        m_encrypt->setCheckState(Qt::Checked);

    centralLayout->addLayout(formLayout);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    m_buttons->setCenterButtons(true);

    centralLayout->addWidget(m_buttons);
}

void BasicSetupDlg::populateAdaptors()
{
    QNetworkInterface		cInterface;
    int index;

    m_adaptor->insertItem(0, "<any>");
    index = 1;
    QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
    foreach (cInterface, ani)
        m_adaptor->insertItem(index++, cInterface.humanReadableName());
}

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

#include "GenericDialog.h"

#include <qboxlayout.h>
#include <qlabel.h>
#include <qtablewidget.h>
#include <qheaderview.h>
#include "UpdateDialog.h"

#define TAG "GenericDialog"

//----------------------------------------------------------
//
//  Validators

#include "SNCUtils.h"

QValidator::State HexValidator::validate(QString &input, int &pos) const
{
    if (pos == 0)
        return QValidator::Acceptable;						// empty string ok

    char c = input.at(pos-1).toLatin1();
    if ((c >= '0') && (c <= '9'))
        return QValidator::Acceptable;
    if ((c >= 'A') && (c <= 'F'))
        return QValidator::Acceptable;
    if ((c >= 'a') && (c <= 'f'))
        return QValidator::Acceptable;

    return QValidator::Invalid;
}

QValidator::State UIDValidator::validate(QString &input, int &pos) const
{
    if (pos == 0)
        return QValidator::Acceptable;						// empty string ok

    if (pos >= SNC_MACADDRSTR_LEN)
        return QValidator::Invalid;

    char c = input.at(pos-1).toLatin1();
    if ((c >= '0') && (c <= '9'))
        return QValidator::Acceptable;
    if ((c >= 'A') && (c <= 'F'))
        return QValidator::Acceptable;
    if ((c >= 'a') && (c <= 'f'))
        return QValidator::Acceptable;

    return QValidator::Invalid;
}

QValidator::State ServiceNameValidator::validate(QString &input, int &pos) const
{
    if (pos == 0)
        return QValidator::Acceptable;						// empty string ok

    if (SNCUtils::isReservedNameCharacter(input.at(pos-1).toLatin1()))
        return QValidator::Invalid;
    if (pos >= SNC_MAX_NAME)
        return QValidator::Invalid;

    return QValidator::Acceptable;
}

QValidator::State ServicePathValidator::validate(QString &input, int &pos) const
{
    if (pos == 0)
        return QValidator::Acceptable;						// empty string ok

    if (SNCUtils::isReservedPathCharacter(input.at(pos-1).toLatin1()))
        return QValidator::Invalid;
    if (pos >= SNC_MAX_SERVPATH)
        return QValidator::Invalid;

    return QValidator::Acceptable;
}


GenericDialog::GenericDialog(const QString& app, const QString& dialogName, const QString& dialogDesc, const QJsonObject& param, QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_local = false;
    m_update = false;
    m_app = app;
    m_dialogName = dialogName;
    m_dialogDesc = dialogDesc;
    layoutWindow();
    m_getTimer = startTimer(100);
    m_updateTimer = -1;
    m_tickCount = 0;
    m_status = NULL;
    setWindowTitle(m_app + " - " + m_dialogDesc);
    m_subDialogMode = false;
    m_param = param;
}

GenericDialog::GenericDialog(const QString& app, Dialog *dialog, const QJsonObject& param, QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    m_local = true;
    m_update = false;
    m_app = app;
    m_bar = NULL;
    m_label = NULL;
    m_dialog = dialog;
    m_dialogName = dialog->getName();
    m_dialogDesc = dialog->getDesc();
    m_status = NULL;
    m_param = param;
    setWindowTitle(m_app + " - " + m_dialogDesc);

    dialog->loadLocalData(param);
    dialog->getDialog(m_jsonDialog);

    m_getTimer = -1;
    m_updateTimer = -1;
    m_subDialogMode = false;
    processDialog();
}

GenericDialog::~GenericDialog()
{
    if (m_getTimer != -1)
        killTimer(m_getTimer);
    m_getTimer = -1;
    if (m_updateTimer != -1)
        killTimer(m_updateTimer);
    m_updateTimer = -1;
}

QJsonObject GenericDialog::getUpdatedDialog()
{
    m_updatedDialog = QJsonObject();
    m_updatedDialog[SNCJSON_DIALOG_NAME] = m_jsonDialog[SNCJSON_DIALOG_NAME];
    m_updatedDialog[SNCJSON_DIALOG_DESC] = m_jsonDialog[SNCJSON_DIALOG_DESC];
    m_updatedDialog[SNCJSON_DIALOG_DATA] = m_varArray;
    return m_updatedDialog;
}


void GenericDialog::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_getTimer) {
        if (m_tickCount == 0) {

            SNCJSON sj;

            m_param[SNCJSON_PARAM_DIALOG_NAME] = m_dialogName;
            emit sendDialogData(sj.completeRequest(SNCJSON_RECORD_TYPE_DIALOG, SNCJSON_COMMAND_REQUEST, m_param));
            if (m_status != NULL)
                m_status->setText("requesting data...");
        }
        if (m_bar != NULL)
            m_bar->setValue(m_tickCount++);
        if (m_tickCount == GENERICDIALOG_WAIT_TICKS) {
            killTimer(m_getTimer);
            m_getTimer = -1;
            QMessageBox::warning(this, "Get management dialog app",
                         QString("The attempt to get the management dialog from ") + m_app + " timed out.",
                                 QMessageBox::Ok);
            reject();
            if (m_status != NULL)
                m_status->setText("data request timed out");
        }
    } else {
        if (m_subDialogMode) {
            m_updateTickCount = 0;
        } else {
            if (++m_updateTickCount >= m_autoUpdateTicks) {
                m_updateTickCount = 0;
                if (m_local) {
                    m_dialog->loadLocalData(m_param);
                    m_dialog->getDialog(m_jsonDialog);
                    processDialog();

                } else {
                    m_tickCount = 0;
                    if (m_getTimer == -1)
                        m_getTimer = startTimer(100);
                }
            }
        }
    }
}

void GenericDialog::layoutWindow()
{
    setModal(true);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(16);
    vLayout->setContentsMargins(11, 11, 11, 11);
    m_label = new QLabel(QString("Requesting configuration data from ") + m_app);
    vLayout->addWidget(m_label, Qt::AlignCenter);

    vLayout->addSpacing(20);

    m_bar = new QProgressBar();
    m_bar->setRange(0, GENERICDIALOG_WAIT_TICKS);
    vLayout->addWidget(m_bar, Qt::AlignCenter);
}

void GenericDialog::receiveSubDialogData(QJsonObject json)
{
    emit sendDialogData(json);
}

void GenericDialog::receiveDialogData(QJsonObject json)
{
    if (m_subDialogMode) {
        emit sendSubDialogData(json);
        return;
    }

    killTimer(m_getTimer);
    m_getTimer = -1;
    if (m_status != NULL) {
        if (m_updateTimer != -1)
            m_status->setText("auto updating");
        else
            m_status->setText("idle");
    }

    if (!json.contains(SNCJSON_RECORD_TYPE))
        return;
    if (json[SNCJSON_RECORD_TYPE].toString() != SNCJSON_RECORD_TYPE_DIALOG)
        return;
    if (!json.contains(SNCJSON_RECORD_COMMAND))
        return;
    if (json[SNCJSON_RECORD_COMMAND].toString() != SNCJSON_COMMAND_RESPONSE)
        return;

    if (!json.contains(SNCJSON_RECORD_DATA))
        return;

    m_jsonDialog = json[SNCJSON_RECORD_DATA].toObject();

    processDialog();
}


void GenericDialog::processDialog()
{
    if (!m_jsonDialog.contains(SNCJSON_DIALOG_NAME))
        return;
    if (!m_jsonDialog.contains(SNCJSON_DIALOG_DESC))
        return;

    if (!m_jsonDialog.contains(SNCJSON_DIALOG_DATA))
        return;

    if (m_dialogName != m_jsonDialog[SNCJSON_DIALOG_NAME].toString())
        return;

    m_dialogDesc = m_jsonDialog[SNCJSON_DIALOG_DESC].toString();
    m_dialogUpdateFlag = m_jsonDialog[SNCJSON_DIALOG_UPDATE].toBool();
    m_dialogCookie = m_jsonDialog[SNCJSON_DIALOG_COOKIE].toString();
    m_varArray = m_jsonDialog[SNCJSON_DIALOG_DATA].toArray();

    m_param[SNCJSON_PARAM_COOKIE] = m_dialogCookie;

    if (!m_update)
        layoutDialogWindow();
    else
        updateDialogWindow();
 }


void GenericDialog::onOk()
{
    bool changed = false;
    bool ok;

    for (int row = 0; row < m_varArray.count(); row++) {
        QJsonObject var = m_varArray[row].toObject();
        if (!var.contains(SNCJSON_DIALOG_VAR_DESC)) {
            continue;
        }

        if (!var.contains(SNCJSON_DIALOG_VAR_TYPE)) {
            continue;
        }

        if (row >= m_varWidgets.count()) {
            SNCUtils::logError(TAG, QString("JSON/varWidget count mismatch at ") + var.value(SNCJSON_DIALOG_VAR_DESC).toString());
            continue;
        }

        QString type = var.value(SNCJSON_DIALOG_VAR_TYPE).toString();

        if (type == SNCJSON_CONFIG_VAR_TYPE_STRING) {
            if (((QLineEdit *)m_varWidgets.at(row))->text() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text();
            }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_PASSWORD) {
                if (((QLineEdit *)m_varWidgets.at(row))->text() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                    changed = true;
                    var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text();
                }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_APPNAME) {
                if (((QLineEdit *)m_varWidgets.at(row))->text() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                    changed = true;
                    var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text();
                }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_UID) {
                if (((QLineEdit *)m_varWidgets.at(row))->text() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                    changed = true;
                    var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text();
                }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_SERVICEPATH) {
            if (((QLineEdit *)m_varWidgets.at(row))->text() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text();
            }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_BOOL) {
            bool state;
            state = ((QCheckBox *)m_varWidgets.at(row))->checkState() == Qt::Checked;
            if (var.value(SNCJSON_CONFIG_VAR_VALUE).toBool() != state) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = state;
            }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_INTEGER) {
            if ((((QLineEdit *)m_varWidgets.at(row))->text().toInt()) != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text().toInt();
            }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_HEXINTEGER) {
            if ((((QLineEdit *)m_varWidgets.at(row))->text().toInt(&ok, 16)) != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text().toInt(&ok, 16);
            }
         } else if (type == SNCJSON_CONFIG_VAR_TYPE_SELECTION) {
            if (((QComboBox *)m_varWidgets.at(row))->currentText() != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QComboBox *)m_varWidgets.at(row))->currentText();
            }
        } else if (type == SNCJSON_CONFIG_VAR_TYPE_DOW) {
            if ((((QLineEdit *)m_varWidgets.at(row))->text().toInt()) != var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()) {
                changed = true;
                var[SNCJSON_CONFIG_VAR_VALUE] = ((QLineEdit *)m_varWidgets.at(row))->text().toInt();
            }
        } else {
            continue;                                       // something that doesn't need to be sent back
        }
        m_varArray[row] = var;
    }

    if (changed)
        accept();
    else
        reject();
}

void GenericDialog::layoutDialogWindow()
{
    int layoutRow = 0;
    bool configEntries = false;
    int minWidth = 200;

    QLayout *oldLayout = layout();

    if (oldLayout != NULL)
        delete oldLayout;

    if (m_bar != NULL)
        delete m_bar;
    m_bar = NULL;

    if (m_label != NULL)
        delete m_label;
    m_label = NULL;

    if (m_status != NULL)
        delete m_status;
    m_status = NULL;

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->setSpacing(16);
    centralLayout->setContentsMargins(11, 11, 11, 11);

#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    QLabel *header = new QLabel(m_dialogDesc);
    header->setAlignment(Qt::AlignHCenter);
    centralLayout->addWidget(header);
#endif

    m_formLayout = new QFormLayout();
    m_formLayout->setSpacing(12);
    m_formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    for (int row = 0; row < m_varArray.count(); row++) {
        QJsonObject var = m_varArray.at(row).toObject();
        if (!var.contains(SNCJSON_DIALOG_VAR_DESC)) {
            SNCUtils::logError(TAG, QString("var %1 has no description").arg(row));
            continue;
        }

        if (!var.contains(SNCJSON_DIALOG_VAR_TYPE)) {
            SNCUtils::logError(TAG, QString("var %1 has no type").arg(var.value(SNCJSON_DIALOG_VAR_DESC).toString()));
            continue;
        }

        QString type = var.value(SNCJSON_DIALOG_VAR_TYPE).toString();

        if (type == SNCJSON_CONFIG_VAR_TYPE_APPNAME) {
            QLineEdit *name = new QLineEdit(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            m_varWidgets.append(name);
            name->setMinimumWidth(200);
            name->setValidator(&m_serviceValidator);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), name);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_SERVICEPATH) {
            QLineEdit *name = new QLineEdit(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            m_varWidgets.append(name);
            name->setMinimumWidth(200);
            name->setValidator(&m_pathValidator);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), name);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_BOOL) {
            QCheckBox *box = new QCheckBox();
            box->setStyleSheet("QCheckBox::indicator{width:40px; height:40px;}");
            m_varWidgets.append(box);
            box->setCheckState(var.value(SNCJSON_CONFIG_VAR_VALUE).toBool() ? Qt::Checked : Qt::Unchecked);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), box);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_INTEGER) {
            QLineEdit *value = new QLineEdit(QString::number(var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()));
            m_varWidgets.append(value);
            if ((var.contains(SNCJSON_CONFIG_VAR_MINIMUM)) && (var.contains(SNCJSON_CONFIG_VAR_MAXIMUM))) {
                value->setValidator(new QIntValidator(var.value(SNCJSON_CONFIG_VAR_MINIMUM).toInt(),
                                                        var.value(SNCJSON_CONFIG_VAR_MAXIMUM).toInt()));
            }
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_HEXINTEGER) {
            QLineEdit *value = new QLineEdit(QString::number(var.value(SNCJSON_CONFIG_VAR_VALUE).toInt(), 16));
            m_varWidgets.append(value);
            if ((var.contains(SNCJSON_CONFIG_VAR_MINIMUM)) && (var.contains(SNCJSON_CONFIG_VAR_MAXIMUM))) {
                value->setValidator(new HexValidator(var.value(SNCJSON_CONFIG_VAR_MINIMUM).toInt(),
                                                        var.value(SNCJSON_CONFIG_VAR_MAXIMUM).toInt()));
            }
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;
            configEntries = true;

         } else if (type == SNCJSON_CONFIG_VAR_TYPE_STRING) {
            QLineEdit *value = new QLineEdit(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            m_varWidgets.append(value);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;
            configEntries = true;

         } else if (type == SNCJSON_CONFIG_VAR_TYPE_UID) {
            QLineEdit *value = new QLineEdit(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            m_varWidgets.append(value);
            value->setValidator(new UIDValidator());
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_PASSWORD) {
            QLineEdit *value = new QLineEdit(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            value->setEchoMode(QLineEdit::Password);
            m_varWidgets.append(value);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_SELECTION) {
            QComboBox *cb = new QComboBox();
            m_varWidgets.append(cb);
            cb->setEditable(false);

            QJsonArray ja = var.value(SNCJSON_CONFIG_STRING_ARRAY).toArray();

            for (int i = 0; i < ja.count(); i++) {
                QJsonObject jo = ja.at(i).toObject();
                cb->addItem(jo.value(SNCJSON_CONFIG_VAR_ENTRY).toString());
            }
            cb->setCurrentText(var.value(SNCJSON_CONFIG_VAR_VALUE).toString());
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), cb);
            layoutRow++;
            configEntries = true;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_BUTTON) {
            DialogButton *button = new DialogButton(var.value(SNCJSON_DIALOG_VAR_DESC).toString(),
                                                   var.value(SNCJSON_CONFIG_VAR_VALUE).toString(), 0, this);
            m_varWidgets.append(button);
            connect(button, SIGNAL(buttonClicked(QString,int)), this, SLOT(buttonClicked(QString,int)));
            QSizePolicy policy;
            policy.setHorizontalPolicy(QSizePolicy::Fixed);
            button->setSizePolicy(policy);
            QHBoxLayout *hl = new QHBoxLayout;
            hl->setAlignment(Qt::AlignHCenter);
            hl->addWidget(button);
            m_formLayout->addRow(hl);
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_DOW) {
           QLineEdit *value = new QLineEdit(QString::number(var.value(SNCJSON_CONFIG_VAR_VALUE).toInt()));
           m_varWidgets.append(value);
           m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
           layoutRow++;
           configEntries = true;

//      Info entries

        } else if (type == SNCJSON_INFO_VAR_TYPE_STRING) {
            QLabel *value = new QLabel(var.value(SNCJSON_INFO_VAR_VALUE).toString());
            m_varWidgets.append(value);
            m_formLayout->addRow(var.value(SNCJSON_DIALOG_VAR_DESC).toString(), value);
            layoutRow++;

        } else if (type == SNCJSON_INFO_VAR_TYPE_BUTTON) {
            DialogButton *button = new DialogButton(var.value(SNCJSON_DIALOG_VAR_DESC).toString(),
                                                   var.value(SNCJSON_INFO_VAR_VALUE).toString(), 0, this);
            m_varWidgets.append(button);
            connect(button, SIGNAL(buttonClicked(QString,int)), this, SLOT(buttonClicked(QString,int)));
            QSizePolicy policy;
            policy.setHorizontalPolicy(QSizePolicy::Fixed);
            button->setSizePolicy(policy);
            QHBoxLayout *hl = new QHBoxLayout;
            hl->setAlignment(Qt::AlignHCenter);
            hl->addWidget(button);
            m_formLayout->addItem(hl);
            layoutRow++;

        } else if (type == SNCJSON_INFO_VAR_TYPE_TABLE) {
            int row = 0;
            int col = 0;

            QTableWidget *table = new QTableWidget();
            int configColumn = -1;
            QString configDialogName;

            m_varWidgets.append(table);

            QJsonArray ch = var.value(SNCJSON_INFO_TABLEHEADER_ARRAY).toArray();
            QJsonArray cw = var.value(SNCJSON_INFO_TABLECOLUMNWIDTH_ARRAY).toArray();

            if (var.contains(SNCJSON_INFO_TABLE_CONFIG_COLUMN)) {
                configColumn = var.value(SNCJSON_INFO_TABLE_CONFIG_COLUMN).toInt();
                configDialogName = var.value(SNCJSON_INFO_TABLE_CONFIG_DIALOGNAME).toString();
            }

            if (ch.count() != cw.count()) {
                SNCUtils::logError(TAG, QString("Table header (%1)/width(%2) mismatch")
                               .arg(ch.count()).arg(cw.count()));
            }

            table->setColumnCount(ch.count());

            QStringList columnHeaders;
            int totalWidth = 0;

            for (int i = 0; i < ch.count(); i++) {
                QJsonObject jo = ch.at(i).toObject();
                columnHeaders.append(jo.value(SNCJSON_INFO_VAR_ENTRY).toString());
                jo = cw.at(i).toObject();
                int width = jo.value(SNCJSON_INFO_VAR_ENTRY).toInt();
                table->setColumnWidth(i, width);
                totalWidth += width + 4;
            }

            QJsonArray data = var.value(SNCJSON_INFO_VAR_VALUE_ARRAY).toArray();
            for (int i = 0; i < data.count(); i++) {
                if (col == 0) {
                    table->insertRow(row);
                    table->setRowHeight(row, 20);
                }
                QJsonObject jo = data.at(i).toObject();
                if (col == configColumn) {
                    DialogButton *button = new DialogButton(jo.value(SNCJSON_INFO_VAR_ENTRY).toString(), configDialogName, row, this);
                    table->setCellWidget(row, col, button);
                    connect(button, SIGNAL(buttonClicked(QString,int)), this, SLOT(buttonClicked(QString,int)));
                } else {
                    QTableWidgetItem *item = new QTableWidgetItem();
                    item->setFlags(Qt::ItemIsEnabled);
                    item->setText(jo.value(SNCJSON_INFO_VAR_ENTRY).toString());
                    table->setItem(row, col, item);
                }
                if (++col == table->columnCount()) {
                    col = 0;
                    row++;
                }
            }

            table->setHorizontalHeaderLabels(columnHeaders);
            table->verticalHeader()->setVisible(false);

            int tableHeight = row * 20 + 40;

            if (tableHeight < 200)
                tableHeight = 200;

            if (tableHeight > 400)
                tableHeight = 400;
            table->setMinimumHeight(tableHeight);
            if (totalWidth > minWidth)
                minWidth = totalWidth;
            m_formLayout->addRow(table);
            layoutRow++;

        } else if (type == SNCJSON_GRAPHICS_VAR_TYPE_STRING) {
            QLabel *label = new QLabel(var.value(SNCJSON_GRAPHICS_VAR_VALUE).toString());
            m_varWidgets.append(label);
            label->setStyleSheet(var.value(SNCJSON_GRAPHICS_VAR_STYLE).toString());
            m_formLayout->addRow(label);
            layoutRow++;

        } else if (type == SNCJSON_GRAPHICS_VAR_TYPE_LINE) {
            QFrame *line = new QFrame(this);
            m_varWidgets.append(line);
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setMinimumWidth(20);
            m_formLayout->addRow(line);
            layoutRow++;
        }
    }

    setMinimumWidth(minWidth);

    m_buttons = new QDialogButtonBox(Qt::Horizontal, this);

    if (configEntries)
        m_buttons->addButton(QDialogButtonBox::Cancel);

    m_buttons->addButton(QDialogButtonBox::Ok);

    if (m_dialogUpdateFlag) {
        m_buttons->addButton("Update", QDialogButtonBox::ActionRole);
        connect(m_buttons, SIGNAL(clicked(QAbstractButton *)), this, SLOT(clicked(QAbstractButton *)));
        //  add the auto update dropdown

        QHBoxLayout *updateLayout = new QHBoxLayout();
        QComboBox *updateBox = new QComboBox;

        updateLayout->addWidget(new QLabel("Auto update rate: "));
        updateBox->addItem("Inactive");
        updateBox->addItem("every 1 second");
        updateBox->addItem("every 5 seconds");
        updateBox->addItem("every 10 seconds");
        connect(updateBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRateChanged(int)));
        updateLayout->addWidget(updateBox);
        updateLayout->addStretch(1);

        centralLayout->addLayout(updateLayout);
    }
    m_buttons->setCenterButtons(true);

    centralLayout->addLayout(m_formLayout);
    centralLayout->addWidget(m_buttons);

    if (m_dialogUpdateFlag) {
        QHBoxLayout *statusLayout = new QHBoxLayout();
        if (m_status == NULL)
            m_status = new QLabel();
        m_status->setText("idle");
        m_status->setAlignment(Qt::AlignLeft);
        statusLayout->addWidget(new QLabel("Status: "));
        statusLayout->addWidget(m_status);
        statusLayout->addStretch(1);
        centralLayout->addLayout(statusLayout);
    }
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

void GenericDialog::updateDialogWindow()
{
    int layoutRow = 0;

    for (int row = 0; row < m_varArray.count(); row++) {
        QJsonObject var = m_varArray.at(row).toObject();
        if (!var.contains(SNCJSON_DIALOG_VAR_DESC)) {
            SNCUtils::logError(TAG, QString("var %1 has no description").arg(row));
            continue;
        }

        if (!var.contains(SNCJSON_DIALOG_VAR_TYPE)) {
            SNCUtils::logError(TAG, QString("var %1 has no type").arg(var.value(SNCJSON_DIALOG_VAR_DESC).toString()));
            continue;
        }

        QString type = var.value(SNCJSON_DIALOG_VAR_TYPE).toString();

        if (type == SNCJSON_CONFIG_VAR_TYPE_APPNAME) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_SERVICEPATH) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_BOOL) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_INTEGER) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_HEXINTEGER) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_STRING) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_PASSWORD) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_SELECTION) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_BUTTON) {
            layoutRow++;

        } else if (type == SNCJSON_CONFIG_VAR_TYPE_DOW) {
            layoutRow++;

        } else if (type == SNCJSON_INFO_VAR_TYPE_STRING) {
            QLabel *value = (QLabel *)(m_formLayout->itemAt(layoutRow, QFormLayout::FieldRole)->widget());
            value->setText(var.value(SNCJSON_INFO_VAR_VALUE).toString());
            layoutRow++;

        } else if (type == SNCJSON_INFO_VAR_TYPE_BUTTON) {
            layoutRow++;

        } else if (type == SNCJSON_INFO_VAR_TYPE_TABLE) {
            int row = 0;
            int col = 0;

            int configColumn = -1;
            QString configDialogName;

            if (var.contains(SNCJSON_INFO_TABLE_CONFIG_COLUMN)) {
                configColumn = var.value(SNCJSON_INFO_TABLE_CONFIG_COLUMN).toInt();
                configDialogName = var.value(SNCJSON_INFO_TABLE_CONFIG_DIALOGNAME).toString();
            }

            QJsonArray data = var.value(SNCJSON_INFO_VAR_VALUE_ARRAY).toArray();
            QTableWidget *table = (QTableWidget *)(m_formLayout->itemAt(layoutRow, QFormLayout::SpanningRole)->widget());
            for (int i = 0; i < data.count(); i++) {
                QJsonObject jo = data.at(i).toObject();
                if (col == 0) {
                    if (row == table->rowCount()) {
                        table->insertRow(row);
                        table->setRowHeight(row, 20);
                        for (int newRowCol = 0; newRowCol < table->columnCount(); newRowCol++) {
                            if (newRowCol == configColumn) {
                                DialogButton *button = new DialogButton(jo.value(SNCJSON_INFO_VAR_ENTRY).toString(), configDialogName, row, this);
                                table->setCellWidget(row, newRowCol, button);
                                connect(button, SIGNAL(buttonClicked(QString,int)), this, SLOT(buttonClicked(QString,int)));
                            } else {
                                QTableWidgetItem *item = new QTableWidgetItem();
                                item->setFlags(Qt::ItemIsEnabled);
                                table->setItem(row, newRowCol, item);
                            }
                        }
                    }
                }
                if (col != configColumn)
                    table->item(row, col)->setText(jo.value(SNCJSON_INFO_VAR_ENTRY).toString());
                if (++col == table->columnCount()) {
                    col = 0;
                    row++;
                }
            }
            while (row < table->rowCount())
                table->removeRow(row);
            layoutRow++;

        } else if (type == SNCJSON_GRAPHICS_VAR_TYPE_STRING) {
            QLabel *value = (QLabel *)(m_formLayout->itemAt(layoutRow, QFormLayout::SpanningRole)->widget());
            value->setText(var.value(SNCJSON_GRAPHICS_VAR_VALUE).toString());
            layoutRow++;

        } else if (type == SNCJSON_GRAPHICS_VAR_TYPE_LINE) {
            layoutRow++;
        }
    }
}

void GenericDialog::clicked(QAbstractButton *button)
{
    if (button->text() == "Update") {
        singleUpdate();
    }
}

void GenericDialog::updateRateChanged(int index)
{
    switch (index) {
    case 0:
        if (m_updateTimer != -1)
            killTimer(m_updateTimer);
        m_updateTimer = -1;
        if (m_status != NULL)
            m_status->setText("idle");
        return;

    case 1:
        m_autoUpdateTicks = 1;
        break;

    case 2:
        m_autoUpdateTicks = 5;
        break;

    case 3:
        m_autoUpdateTicks = 10;
        break;
    }

    m_update = true;
    m_updateTickCount = 0;
    if (m_updateTimer == -1)
        m_updateTimer = startTimer(1000);
    if (m_status != NULL)
        m_status->setText("auto updating");
}

void GenericDialog::buttonClicked(const QString& dialogName, int row)
{
    int ret;

    QJsonObject param;
    param[SNCJSON_PARAM_DIALOG_NAME] = dialogName;
    param[SNCJSON_PARAM_INDEX] = row;

    if (m_local) {
        Dialog *jsonDialog = SNCJSON::mapNameToDialog(dialogName);
        if (jsonDialog == NULL)
            return;

        GenericDialog dlg(SNCUtils::getAppName(), jsonDialog, param, this);

        ret = dlg.exec();
        if (jsonDialog->isConfigDialog() && (ret == QDialog::Accepted)) {
            jsonDialog->setDialog(dlg.getUpdatedDialog());
            jsonDialog->saveLocalData();
        }
        singleUpdate();
        return;
    }
    GenericDialog dlg(m_app, dialogName, "", param, this);

    m_subDialogMode = true;
    connect(this, SIGNAL(sendSubDialogData(QJsonObject)), &dlg, SLOT(receiveDialogData(QJsonObject)));
    connect(&dlg, SIGNAL(sendDialogData(QJsonObject)), this, SLOT(receiveSubDialogData(QJsonObject)));

    ret = dlg.exec();

    disconnect(this, SIGNAL(sendSubDialogData(QJsonObject)), &dlg, SLOT(receiveDialogData(QJsonObject)));
    disconnect(&dlg, SIGNAL(sendDialogData(QJsonObject)), this, SLOT(receiveSubDialogData(QJsonObject)));

    if (ret == QDialog::Accepted) {
        SNCJSON sj;

        sj.addVar(dlg.getUpdatedDialog(), SNCJSON_RECORD_DATA);
        UpdateDialog updateDlg(m_app, dialogName, this);
        connect(this, SIGNAL(sendSubDialogData(QJsonObject)), &updateDlg, SLOT(receiveDialogData(QJsonObject)));
        emit sendDialogData(sj.completeRequest(SNCJSON_RECORD_TYPE_DIALOGUPDATE, SNCJSON_COMMAND_REQUEST, param));
        updateDlg.exec();
        disconnect(this, SIGNAL(sendSubDialogData(QJsonObject)), &updateDlg, SLOT(receiveDialogData(QJsonObject)));
    }

    m_subDialogMode = false;
    singleUpdate();
}

void GenericDialog::singleUpdate()
{
    thread()->msleep(10);                                             // in case need to local processing
    m_update = true;
    if (m_updateTimer != -1)
        return;
    if (m_local) {
        m_dialog->loadLocalData(m_param);
        m_dialog->getDialog(m_jsonDialog);
        processDialog();
    } else {
        if (m_getTimer != -1)
            return;
        m_tickCount = 0;
        m_getTimer = startTimer(100);
    }
}

//----------------------------------------------------------
//
// DialogButton functions

DialogButton::DialogButton(const QString& text, const QString& dialogName, int row, QWidget *parent)
    : QPushButton(text, parent)
{
    m_row = row;
    m_dialogName = dialogName;
    connect(this, SIGNAL(clicked(bool)), this, SLOT(originalClicked(bool)));
};

void DialogButton::originalClicked(bool )
{
    emit buttonClicked(m_dialogName, m_row);
}

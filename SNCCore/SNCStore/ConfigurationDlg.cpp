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

#include "ConfigurationDlg.h"
#include "SNCStore.h"
#include "SNCUtils.h"

ConfigurationDlg::ConfigurationDlg() : Dialog(SNCSTORE_CONFIGURATION_NAME, SNCSTORE_CONFIGURATION_DESC)
{
    setConfigDialog(true);
}

void ConfigurationDlg::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createConfigStringVar("storePath", "Path to store root", m_storePath));
    return dialog(newDialog);
}

bool ConfigurationDlg::setVar(const QString& name, const QJsonObject& var)
{
    bool changed = false;

    if (name == "storePath") {
        if (m_storePath != var.value(SNCJSON_CONFIG_VAR_VALUE).toString()) {
            changed = true;
            m_storePath = var.value(SNCJSON_CONFIG_VAR_VALUE).toString();
        }
    }

    return changed;
}

void ConfigurationDlg::loadLocalData(const QJsonObject& /* param */)
{
    QSettings *settings = SNCUtils::getSettings();
    m_storePath = settings->value(SNCSTORE_PARAMS_ROOT_DIRECTORY).toString();
    delete settings;
}

void ConfigurationDlg::saveLocalData()
{
    QSettings *settings = SNCUtils::getSettings();

    settings->setValue(SNCSTORE_PARAMS_ROOT_DIRECTORY, m_storePath);
    delete settings;
}

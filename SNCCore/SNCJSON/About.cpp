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

#include "About.h"
#include "SNCUtils.h"

About::About() : Dialog(SNCJSON_DIALOG_NAME_ABOUT, SNCJSON_DIALOG_DESC_ABOUT)
{
}

void About::getDialog(QJsonObject& newDialog)
{
    clearDialog();
    addVar(createGraphicsStringVar(SNCUtils::getAppType(), "font-size: 20px; qproperty-alignment: AlignCenter"));
    addVar(createGraphicsStringVar("A SNC Application", "qproperty-alignment: AlignCenter"));
    addVar(createGraphicsStringVar("Copyright (c) 2014-2021, Richard Barnett", "qproperty-alignment: AlignCenter"));
    addVar(createGraphicsLineVar());
    addVar(createInfoStringVar("Name:", SNCUtils::getAppName()));
    addVar(createInfoStringVar("Build date:", QString("%1 %2").arg(__DATE__).arg(__TIME__)));
    addVar(createInfoStringVar("SNCLib version:", SNCUtils::SNCLibVersion()));
    addVar(createInfoStringVar("Qt version:", qVersion()));

    return dialog(newDialog);
}


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

#include "AboutDlg.h"
#include "SNCUtils.h"

AboutDlg::AboutDlg(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    layoutWindow();

    setWindowTitle("About " + SNCUtils::getAppType());
    m_appType->setText(SNCUtils::getAppType());
    m_appName->setText(SNCUtils::getAppName());
    m_buildDate->setText(QString("%1 %2").arg(__DATE__).arg(__TIME__));
    m_qtRuntime->setText(qVersion());

    connect(m_actionOk, SIGNAL(clicked()), this, SLOT(close()));
}

void AboutDlg::layoutWindow()
{
    resize(300, 240);
    setModal(true);

    QVBoxLayout *centralLayout = new QVBoxLayout(this);
    centralLayout->setSpacing(6);
    centralLayout->setContentsMargins(11, 11, 11, 11);

    QVBoxLayout *verticalLayout = new QVBoxLayout();
    verticalLayout->setSpacing(6);

    m_appType = new QLabel(this);
    m_appType->setAlignment(Qt::AlignCenter);

    verticalLayout->addWidget(m_appType);

    QLabel *label_1 = new QLabel("An SNC Application", this);
    label_1->setAlignment(Qt::AlignCenter);

    verticalLayout->addWidget(label_1);

    QLabel *label_2 = new QLabel("Copyright (c) 2014-2021 Richard Barnett", this);
    label_2->setAlignment(Qt::AlignCenter);

    verticalLayout->addWidget(label_2);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    verticalLayout->addWidget(line);

    centralLayout->addLayout(verticalLayout);

    // vertical spacer
    QSpacerItem *verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    centralLayout->addItem(verticalSpacer_2);


    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(6);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    QLabel *label_3 = new QLabel("Name:", this);
    formLayout->setWidget(0, QFormLayout::LabelRole, label_3);
    m_appName = new QLabel(this);
    formLayout->setWidget(0, QFormLayout::FieldRole, m_appName);

    QLabel *label_5 = new QLabel("Build Date:", this);
    formLayout->setWidget(2, QFormLayout::LabelRole, label_5);
    m_buildDate = new QLabel(this);
    formLayout->setWidget(2, QFormLayout::FieldRole, m_buildDate);

    QLabel *label_7 = new QLabel("Qt Version:", this);
    formLayout->setWidget(4, QFormLayout::LabelRole, label_7);
    m_qtRuntime = new QLabel(this);
    formLayout->setWidget(4, QFormLayout::FieldRole, m_qtRuntime);

    centralLayout->addLayout(formLayout);


    // vertical spacer
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 8, QSizePolicy::Minimum, QSizePolicy::Expanding);
    centralLayout->addItem(verticalSpacer);


    // OK button, centered at bottom by two spacers
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(6);

    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);

    m_actionOk = new QPushButton("Ok", this);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_actionOk->sizePolicy().hasHeightForWidth());
    m_actionOk->setSizePolicy(sizePolicy);
    horizontalLayout->addWidget(m_actionOk);

    QSpacerItem *horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer_2);

    centralLayout->addLayout(horizontalLayout);
}

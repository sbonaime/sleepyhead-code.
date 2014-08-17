/* SleepLib Progress Dialog Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "progressdialog.h"

ProgressDialog::ProgressDialog(QWidget * parent):
    QDialog(parent, Qt::SplashScreen)
{
    waitmsg = new QLabel(QObject::tr("PLease Wait..."));
    hlayout = new QHBoxLayout;

    imglabel = new QLabel(this);

    vlayout = new QVBoxLayout;
    progress = new QProgressBar(this);
    this->setLayout(vlayout);
    vlayout->addLayout(hlayout);
    hlayout->addWidget(imglabel);
    hlayout->addWidget(waitmsg,1,Qt::AlignCenter);
    vlayout->addWidget(progress,1);
    progress->setMaximum(100);
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::doUpdateProgress(int cnt, int total)
{
    progress->setMaximum(total);
    progress->setValue(cnt);
}


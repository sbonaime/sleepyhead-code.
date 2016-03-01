/* SleepLib Progress Dialog Header
 *
 * Copyright (c) 2011-2016 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>

class ProgressDialog:public QDialog {
public:
    explicit ProgressDialog(QWidget * parent);
    virtual ~ProgressDialog();

    void setMessage(QString msg) { waitmsg->setText(msg); }
    void setPixmap(QPixmap &pixmap) { imglabel->setPixmap(pixmap); }
    QProgressBar * progress;
public slots:
    void doUpdateProgress(int cnt, int total);

protected:
    QLabel * waitmsg;
    QHBoxLayout *hlayout;
    QLabel * imglabel;
    QVBoxLayout * vlayout;

};

#endif // PROGRESSDIALOG_H

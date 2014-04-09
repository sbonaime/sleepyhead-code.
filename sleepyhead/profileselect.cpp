/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Profile Select Implementation (Login Screen)
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include "profileselect.h"
#include <QDebug>
#include <QStringListModel>
#include <QStandardItem>
#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QTimer>
#include <QDir>
#include "ui_profileselect.h"
#include "SleepLib/profiles.h"
#include "newprofile.h"
#include "mainwindow.h"

ProfileSelect::ProfileSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileSelect)
{
    ui->setupUi(this);
    QStringList str;
    model=new QStandardItemModel (0,0);

    int i=0;
    int sel=-1;
    QString name;

    QIcon icon(":/icons/moon.png");
    for (QMap<QString,Profile *>::iterator p=Profiles::profiles.begin();p!=Profiles::profiles.end();p++) {
        name=p.key();

        QStandardItem *item=new QStandardItem(icon,name);
        if (PREF.contains(STR_GEN_Profile) && (name==PREF[STR_GEN_Profile].toString())) {
            sel=i;
        }
        item->setData(p.key());
        item->setEditable(false);

        // Profile fonts arern't loaded yet.. Using generic font.
        item->setFont(QFont("Sans Serif",18,QFont::Bold,false));
        model->appendRow(item);
        i++;
    }
    ui->listView->setModel(model);
    ui->listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
    if (sel>=0) ui->listView->setCurrentIndex(model->item(sel)->index());
    m_tries=0;

    /*PREF["SkipLogin"]=false;
    if ((i==1) && PREF["SkipLogin"].toBool()) {
        if (!Profiles::profiles.contains(name))
            PREF[STR_GEN_Profile]=name;
        QTimer::singleShot(0,this,SLOT(earlyExit()));
        hide();
    } */
    popupMenu=new QMenu(this);
    popupMenu->addAction(tr("Open Profile"),this,SLOT(openProfile()));
    popupMenu->addAction(tr("Edit Profile"),this,SLOT(editProfile()));
    popupMenu->addSeparator();
    popupMenu->addAction(tr("Delete Profile"),this,SLOT(deleteProfile()));

    ui->labelAppName->setText(STR_TR_SleepyHead);
    ui->labelVersion->setText("v"+VersionString+" "+ReleaseStatus);
//    if (GIT_BRANCH!="master")
//        ui->labelBuild->setText(GIT_BRANCH);
//    else ui->labelBuild->setText(QString());
    ui->labelFolder->setText(GetAppRoot());
    ui->labelFolder->setToolTip("Current SleepyHead data folder\n"+GetAppRoot());
}

ProfileSelect::~ProfileSelect()
{
    delete model; // why is this not being cleaned up by Qt?
    delete popupMenu;
    delete ui;
}
void ProfileSelect::earlyExit()
{
    accept();
}
void ProfileSelect::editProfile()
{
    QString name=ui->listView->currentIndex().data().toString();
    Profile *profile=Profiles::Get(name);
    if (!profile) return;
    bool reallyEdit=false;
    if (profile->user->hasPassword()) {
        QDialog dialog(this,Qt::Dialog);
        QLineEdit *e=new QLineEdit(&dialog);
        e->setEchoMode(QLineEdit::Password);
        dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
        dialog.setWindowTitle(tr("Enter Password for %1").arg(name));
        dialog.setMinimumWidth(300);
        QVBoxLayout *lay=new QVBoxLayout();
        dialog.setLayout(lay);
        lay->addWidget(e);
        int tries=0;
        do {
            e->setText("");
            if (dialog.exec()!=QDialog::Accepted) break;
            tries++;
            if (profile->user->checkPassword(e->text())) {
                reallyEdit=true;
                break;
            } else {
                if (tries<3) {
                    QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("Incorrect Password"),QMessageBox::Ok);
                } else {
                    QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("You entered the password wrong too many times."),QMessageBox::Ok);
                    reject();
                }
            }
        } while (tries<3);
    } else reallyEdit=true;

    if (reallyEdit) {
        NewProfile newprof(this);
        newprof.edit(name);
        newprof.exec();
    }
    //qDebug() << "edit" << name;
}
void ProfileSelect::deleteProfile()
{
    QString name=ui->listView->currentIndex().data().toString();
    if (QMessageBox::question(this,tr("Question"),tr("Are you sure you want to trash the profile \"%1\"?").arg(name),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
        if (QMessageBox::question(this,tr("Question"),tr("Double Checking:\n\nDo you really want \"%1\" profile to be obliterated?").arg(name),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
            if (QMessageBox::question(this,tr("Question"),tr("Okay, I am about to totally OBLITERATE the profile \"%1\" and all it's contained data..\n\nDon't say you weren't warned. :-p").arg(name),QMessageBox::Cancel,QMessageBox::Ok)==QMessageBox::Ok){
                bool reallydelete=false;
                Profile *profile=Profiles::profiles[name];
                if (!profile) {
                    QMessageBox::warning(this,tr("WTH???"),tr("If you can read this you need to delete this profile directory manually (It's under %1)").arg(GetAppRoot()+"/Profiles/"+PROFILE.user->userName()),QMessageBox::Ok);
                    return;
                }
                if (profile->user->hasPassword()) {
                    QDialog dialog(this,Qt::Dialog);
                    QLineEdit *e=new QLineEdit(&dialog);
                    e->setEchoMode(QLineEdit::Password);
                    dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
                    dialog.setWindowTitle(tr("Enter Password for %1").arg(name));
                    dialog.setMinimumWidth(300);
                    QVBoxLayout *lay=new QVBoxLayout();
                    dialog.setLayout(lay);
                    lay->addWidget(e);
                    int tries=0;
                    do {
                        e->setText("");
                        if (dialog.exec()!=QDialog::Accepted) break;
                        tries++;
                        if (profile->user->checkPassword(e->text())) {
                            reallydelete=true;
                            break;
                        } else {
                            if (tries<3) {
                                QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("Incorrect Password"),QMessageBox::Ok);
                            } else {
                                QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("Meheh... If your trying to delete because you forgot the password, your going the wrong way about it. Read the docs.\n\nSigned: Nasty Programmer"),QMessageBox::Ok);
                            }
                        }
                    } while (tries<3);
                } else reallydelete=true;

                if (reallydelete) {
                    QString path=profile->Get(PrefMacro(STR_GEN_DataFolder));
                    if (!path.isEmpty()) {
                        if (!removeDir(path)) {
                            QMessageBox::information(this,tr("Whoops."),tr("There was an error deleting the profile directory.. You need to manually remove %1").arg(path),QMessageBox::Ok);
                        }
                    }
                    model->removeRow(ui->listView->currentIndex().row());

                    qDebug() << "Delete" << path;
                }
            }
        }
    }
}

//! \fn ProfileSelect::QuickLogin()
//! \brief For programmatically bypassing the login window
void ProfileSelect::QuickLogin()
{
    on_listView_activated(ui->listView->currentIndex());
}

void ProfileSelect::on_selectButton_clicked()
{
    on_listView_activated(ui->listView->currentIndex());
}
void ProfileSelect::openProfile()
{
    on_listView_activated(ui->listView->currentIndex());
}

void ProfileSelect::on_newProfileButton_clicked()
{
    NewProfile newprof(this);
    newprof.skipWelcomeScreen();
    if (newprof.exec()==NewProfile::Rejected) {
//        reject();
    } else accept();
}


//! \fn ProfileSelect::on_listView_activated(const QModelIndex &index)
//! \brief Process the selected login, requesting passwords if required.
void ProfileSelect::on_listView_activated(const QModelIndex &index)
{
    QString name=index.data().toString();
    Profile *profile=Profiles::profiles[name];
    if (!profile) return;
    if (!profile->user->hasPassword()) {
        m_selectedProfile=name;
        PREF[STR_GEN_Profile]=name;
        accept();
        return;
    } else {
        int tries=0;
        do {
            QDialog dialog(this,Qt::Dialog);
            QLineEdit *e=new QLineEdit(&dialog);
            e->setEchoMode(QLineEdit::Password);
            dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
            dialog.setWindowTitle(tr("Enter Password"));
            QVBoxLayout *lay=new QVBoxLayout();
            dialog.setLayout(lay);
            lay->addWidget(e);
            dialog.exec();
            if (profile->user->checkPassword(e->text())) {
                m_selectedProfile=name;
                PREF[STR_GEN_Profile]=name;
                accept();
                return;
            }
            tries++;
            if (tries<3) {
                QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("Incorrect Password"),QMessageBox::Ok);
            } else {
                QMessageBox::warning(this,STR_MESSAGE_ERROR,tr("You entered an Incorrect Password too many times. Exiting!"),QMessageBox::Ok);
            }
        } while (tries<3);
    }
    reject();
    return;
}

void ProfileSelect::on_listView_customContextMenuRequested(const QPoint &pos)
{
    popupMenu->popup(QWidget::mapToGlobal(pos));
}

void ProfileSelect::on_pushButton_clicked()
{
    MainWindow::RestartApplication(false,true);
}

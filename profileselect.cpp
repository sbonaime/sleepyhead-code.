#include "profileselect.h"
#include <QDebug>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QTimer>
#include "ui_profileselect.h"
#include "SleepLib/profiles.h"
#include "newprofile.h"

ProfileSelect::ProfileSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProfileSelect)
{
    ui->setupUi(this);
    //ui->listView->setViewMode(QListView::IconMode);
    QStringList str;
    QStandardItemModel *model=new QStandardItemModel (0,0);
    //QList<QStandardItem *> items;


    int i=0;
    int sel=-1;
    QString name;
    for (QHash<QString,Profile *>::iterator p=Profiles::profiles.begin();p!=Profiles::profiles.end();p++) {
        //str.append(p.key());
        //Profile &profile=**p;
        name=p.key();
       // if (!PROFILE["FirstName"].toString().isEmpty())
       //     name+=" ("+PROFILE["FirstName"].toString()+" "+PROFILE["LastName"].toString()+")";
        QStandardItem *item=new QStandardItem(*new QIcon(":/icons/moon.png"),name);
        if (PREF.Exists("Profile") && (name==PREF["Profile"].toString())) {
            sel=i;
        }
        item->setData(p.key());
        item->setEditable(false);
        item->setFont(QFont("Sans Serif",18,QFont::Bold,false));
        model->appendRow(item);
        i++;
    }
    ui->listView->setModel(model);
    ui->listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
    if (sel>=0) ui->listView->setCurrentIndex(model->item(sel)->index());
    m_tries=0;

    PREF["SkipLogin"]=false;
    if ((i==1) && PREF["SkipLogin"].toBool()) {
        if (!Profiles::profiles.contains(name))
            PREF["Profile"]=name;
        QTimer::singleShot(0,this,SLOT(earlyExit()));
        hide();
    }
    popupMenu=new QMenu(this);
    popupMenu->addAction("Open Profile",this,SLOT(openProfile()));
    popupMenu->addAction("Edit Profile",this,SLOT(editProfile()));
    popupMenu->addSeparator();
    popupMenu->addAction("Delete Profile",this,SLOT(deleteProfile()));
}

ProfileSelect::~ProfileSelect()
{
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
    if (profile->Exists("Password")) {
        QDialog dialog(this,Qt::Dialog);
        QLineEdit *e=new QLineEdit(&dialog);
        e->setEchoMode(QLineEdit::Password);
        dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
        dialog.setWindowTitle("Enter Password for "+name);
        dialog.setMinimumWidth(300);
        QVBoxLayout *lay=new QVBoxLayout();
        dialog.setLayout(lay);
        lay->addWidget(e);
        int tries=0;
        do {
            e->setText("");
            if (dialog.exec()!=QDialog::Accepted) break;
            QByteArray ba=e->text().toUtf8();
            tries++;
            if (QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex()==(*profile)["Password"]) {
                reallyEdit=true;
                break;
            } else {
                if (tries<3) {
                    QMessageBox::warning(this,"Error","Incorrect Password",QMessageBox::Ok);
                } else {
                    QMessageBox::warning(this,"Error","You entered the password wrong too many times.",QMessageBox::Ok);
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
    if (QMessageBox::question(this,"Question","Are you sure you want to trash the profile \""+name+"\"?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
        if (QMessageBox::question(this,"Question","Double Checking: Do you really want \""+name+"\" profile to be obliterated?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
            if (QMessageBox::question(this,"Question","Last chance to save the \""+name+"\" profile. Are you totally sure?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes){
                bool reallydelete=false;
                Profile *profile=Profiles::profiles[name];
                if (!profile) {
                    QMessageBox::warning(this,"WTH???","If you can read this you need to delete this profile directory manually (It's under Your Documents folder -> SleepApp -> Profiles -> [profile_name])",QMessageBox::Ok);
                    return;
                }
                if (profile->Exists("Password")) {
                    QDialog dialog(this,Qt::Dialog);
                    QLineEdit *e=new QLineEdit(&dialog);
                    e->setEchoMode(QLineEdit::Password);
                    dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
                    dialog.setWindowTitle("Enter Password for "+name);
                    dialog.setMinimumWidth(300);
                    QVBoxLayout *lay=new QVBoxLayout();
                    dialog.setLayout(lay);
                    lay->addWidget(e);
                    int tries=0;
                    do {
                        e->setText("");
                        if (dialog.exec()!=QDialog::Accepted) break;
                        QByteArray ba=e->text().toUtf8();
                        tries++;
                        if (QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex()==(*profile)["Password"]) {
                            reallydelete=true;
                            break;
                        } else {
                            if (tries<3) {
                                QMessageBox::warning(this,"Error","Incorrect Password",QMessageBox::Ok);
                            } else {
                                QMessageBox::warning(this,"Error","Meheh... If your trying to delete because you forgot the password, your going the wrong way about it. Read the docs.\n\nSigned: Nasty Programmer",QMessageBox::Ok);
                            }
                        }
                    } while (tries<3);
                } else reallydelete=true;

                if (reallydelete) {
                    QMessageBox::information(this,"Whoops.","After all that nagging, I haven't got around to writing this code yet.. For now you can delete the directory in SleepApp -> Profiles -> [profile_name]",QMessageBox::Ok);
                    qDebug() << "delete" << name;
                }
            }
        }
    }
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
    newprof.exec();
    accept();
}

void ProfileSelect::on_listView_activated(const QModelIndex &index)
{
    QString name=index.data().toString();
    Profile *profile=Profiles::profiles[name];
    if (!profile) return;
    if (!profile->Exists("Password")) {
        m_selectedProfile=name;
        PREF["Profile"]=name;
        accept();
        return;
    } else {
        int tries=0;
        do {
            QDialog dialog(this,Qt::Dialog);
            QLineEdit *e=new QLineEdit(&dialog);
            e->setEchoMode(QLineEdit::Password);
            dialog.connect(e,SIGNAL(returnPressed()),&dialog,SLOT(accept()));
            dialog.setWindowTitle("Enter Password");
            QVBoxLayout *lay=new QVBoxLayout();
            dialog.setLayout(lay);
            lay->addWidget(e);
            dialog.exec();
            QByteArray ba=e->text().toUtf8();
            if (QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex()==(*profile)["Password"]) {
                m_selectedProfile=name;
                PREF["Profile"]=name;
                accept();
                return;
            }
            tries++;
            if (tries<3) {
                QMessageBox::warning(this,"Error","Incorrect Password",QMessageBox::Ok);
            } else {
                QMessageBox::warning(this,"Error","You entered an Incorrect Password too many times. Exiting!",QMessageBox::Ok);
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

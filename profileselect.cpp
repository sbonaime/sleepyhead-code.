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
    for (QHash<QString,Profile *>::iterator p=Profiles::profiles.begin();p!=Profiles::profiles.end();p++) {
        //str.append(p.key());
        Profile &profile=**p;
        QString name=p.key();
       // if (!profile["FirstName"].toString().isEmpty())
       //     name+=" ("+profile["FirstName"].toString()+" "+profile["LastName"].toString()+")";
        QStandardItem *item=new QStandardItem(*new QIcon(":/icons/moon.png"),name);
        item->setData(p.key());
        item->setEditable(false);
        item->setFont(QFont("Sans Serif",18,QFont::Bold,false));
        model->appendRow(item);
        i++;
    }
    ui->listView->setModel(model);
    ui->listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tries=0;
}

ProfileSelect::~ProfileSelect()
{
    delete ui;
}

void ProfileSelect::on_selectButton_clicked()
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
    if ((*profile)["Password"].toString().isEmpty()) {
        m_selectedProfile=name;
        pref["Profile"]=name;
    } else {
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
            pref["Profile"]=name;
        } else {
            m_tries++;
            if (m_tries>2) {
                QMessageBox::warning(this,"Error","You entered an Incorrect Password too many times. Exiting!",QMessageBox::Ok);
                this->reject();
            } else {
                QMessageBox::warning(this,"Error","Incorrect Password",QMessageBox::Ok);
            }
            return;
        }
    }
    accept();
}

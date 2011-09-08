#include <QLabel>
#include <QColorDialog>
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "SleepLib/machine_common.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);

    ui->firstNameEdit->setText((*profile)["FirstName"].toString());
    ui->lastNameEdit->setText((*profile)["LastName"].toString());
    ui->addressEdit->clear();
    ui->addressEdit->appendPlainText((*profile)["Address"].toString());
    ui->emailEdit->setText((*profile)["EmailAddress"].toString());
    ui->phoneEdit->setText((*profile)["Phone"].toString());
    bool gender=(*profile)["Gender"].toBool();
    if (gender) ui->genderMale->setChecked(true); else ui->genderFemale->setChecked(true);

    bool ok;
    ui->heightEdit->setValue((*profile)["Height"].toDouble(&ok));
    ui->dobEdit->setDate((*profile)["DOB"].toDate());
    int i=ui->unitCombo->findText((*profile)["UnitSystem"].toString());
    ui->unitCombo->setCurrentIndex(i);

    i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    ui->timeZoneCombo->setCurrentIndex(i);

    if (pref.Exists("DaySplitTime")) {
        QTime t=pref["DaySplitTime"].toTime();
        ui->timeEdit->setTime(t);
    }
    int val;

    if (pref.Exists("CombineCloserSessions")) {
        val=pref["CombineCloserSessions"].toInt();
        ui->combineSlider->setValue(val);
    } else {
        ui->combineSlider->setValue(val=0);
        pref["CombineCloserSessions"]=val;
    }
    if (val>0) {
        ui->combineLCD->display(val);
    } else ui->combineLCD->display(tr("OFF"));


    if (pref.Exists("IgnoreShorterSessions")) {
        val=pref["IgnoreShorterSessions"].toInt();
        ui->IgnoreSlider->setValue(val);
    } else {
        ui->IgnoreSlider->setValue(val=0);
        pref["IgnoreShorterSessions"]=val;
    }
    if (val>0) {
        ui->IgnoreLCD->display(val);
    } else ui->IgnoreLCD->display(tr("OFF"));


    bool b;
    if (pref.Exists("MemoryHog")) {
        b=pref["MemoryHog"].toBool();
    } else {
        pref["MemoryHog"]=b=false;
    }
    ui->memoryHogCheckbox->setChecked(b);

    ui->eventTable->setColumnWidth(0,40);
    ui->eventTable->setColumnWidth(1,55);
    int row=0;
    QTableWidgetItem *item;
    QHash<ChannelID, Channel>::iterator ci;
    for (ci=channel.begin();ci!=channel.end();ci++) {
        if ((ci.value().channeltype()==CT_Event) || (ci.value().channeltype()==CT_Graph)) {
            ui->eventTable->insertRow(row);
            item=new QTableWidgetItem(ci.value().details());
            ui->eventTable->setItem(row,2,item);
            QCheckBox *c=new QCheckBox(ui->eventTable);
            c->setChecked(true);
            QLabel *pb=new QLabel(ui->eventTable);
            pb->setText("foo");
            ui->eventTable->setCellWidget(row,0,c);
            ui->eventTable->setCellWidget(row,1,pb);
            QColor a(rand() % 255, rand() % 255, rand() % 255, 255);
            QPalette p(a,a,a,a,a,a,a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }
    ui->profileTab->setTabOrder(ui->firstNameEdit,ui->lastNameEdit);
    ui->profileTab->setTabOrder(ui->lastNameEdit,ui->addressEdit);
    ui->profileTab->setTabOrder(ui->addressEdit,ui->genderMale);
    ui->profileTab->setTabOrder(ui->genderMale,ui->genderFemale);
    ui->profileTab->setTabOrder(ui->genderFemale,ui->dobEdit);
    ui->profileTab->setTabOrder(ui->dobEdit,ui->heightEdit);
    ui->profileTab->setTabOrder(ui->heightEdit,ui->phoneEdit);
    ui->profileTab->setTabOrder(ui->phoneEdit,ui->timeZoneCombo);
    ui->profileTab->setTabOrder(ui->timeZoneCombo,ui->emailEdit);
    ui->profileTab->setTabOrder(ui->emailEdit,ui->unitCombo);
}


PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_eventTable_doubleClicked(const QModelIndex &index)
{
    int row=index.row();
    int col=index.column();
    if (col==1) {
        QWidget *w=ui->eventTable->cellWidget(row,col);
        QColorDialog a;
        QColor color=w->palette().background().color();
        a.setCurrentColor(color);
        if (a.exec()==QColorDialog::Accepted) {
            QColor c=a.currentColor();
            QPalette p(c,c,c,c,c,c,c);
            w->setPalette(p);
            qDebug() << "Color accepted" << col;
        }
    }
}

void PreferencesDialog::Save()
{
    (*profile)["FirstName"]=ui->firstNameEdit->text();
    (*profile)["LastName"]=ui->lastNameEdit->text();
    (*profile)["Gender"]=ui->genderMale->isChecked();
    (*profile)["Height"]=ui->heightEdit->value();
    (*profile)["DOB"]=ui->dobEdit->date();
    (*profile)["EmailAddress"]=ui->emailEdit->text();
    (*profile)["Phone"]=ui->phoneEdit->text();
    (*profile)["Address"]=ui->addressEdit->toPlainText();
    (*profile)["UnitSystem"]=ui->unitCombo->currentText();
    (*profile)["TimeZone"]=ui->timeZoneCombo->currentText();

    pref["CombineCloserSessions"]=ui->combineSlider->value();
    pref["IgnoreShorterSessions"]=ui->IgnoreSlider->value();

    pref["MemoryHog"]=ui->memoryHogCheckbox->isChecked();
    pref["DaySplitTime"]=ui->timeEdit->time();

    profile->Save();
    pref.Save();
}


void PreferencesDialog::on_combineSlider_sliderMoved(int position)
{
    if (position>0) {
        ui->combineLCD->display(position);
    } else ui->combineLCD->display(tr("OFF"));
}

void PreferencesDialog::on_IgnoreSlider_sliderMoved(int position)
{
    if (position>0) {
        ui->IgnoreLCD->display(position);
    } else ui->IgnoreLCD->display(tr("OFF"));
}

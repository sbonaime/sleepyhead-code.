#include <QLabel>
#include <QColorDialog>
#include "SleepLib/profiles.h"
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "SleepLib/machine_common.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

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
            QColor a(random() % 255, random() % 255, random() % 255, 255);
            QPalette p(a,a,a,a,a,a,a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }
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

void PreferencesDialog::on_timeEdit_editingFinished()
{
    pref["DaySplitTime"]=ui->timeEdit->time();
}

void PreferencesDialog::on_memoryHogCheckbox_toggled(bool checked)
{
    pref["MemoryHog"]=checked;
}

void PreferencesDialog::on_combineSlider_valueChanged(int position)
{
    if (position>0) {
        ui->combineLCD->display(position);
    } else ui->combineLCD->display(tr("OFF"));
    pref["CombineCloserSessions"]=position;
}

void PreferencesDialog::on_IgnoreSlider_valueChanged(int position)
{
    if (position>0) {
        ui->IgnoreLCD->display(position);
    } else ui->IgnoreLCD->display(tr("OFF"));
    pref["IgnoreShorterSessions"]=position;
}

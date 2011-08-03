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
    ui->eventTable->setColumnWidth(0,40);
    ui->eventTable->setColumnWidth(1,50);
    int row=0;
    QTableWidgetItem *item;
    QHash<ChannelID, Channel>::iterator ci;
    for (ci=channel.begin();ci!=channel.end();ci++) {
        if ((ci.value().channeltype()==CT_Event) || (ci.value().channeltype()==CT_Graph)) {
            ui->eventTable->insertRow(row);
            item=new QTableWidgetItem(ci.value().details());
            ui->eventTable->setItem(row,2,item);
            QCheckBox *c=new QCheckBox(ui->eventTable);
            QLabel *pb=new QLabel(ui->eventTable);
            pb->setText("foo");
            ui->eventTable->setCellWidget(row,0,c);
            ui->eventTable->setCellWidget(row,1,pb);
            QColor a(random() % 255, random() %255, random()%255, 255);
            QPalette p(a,a,a,a,a,a,a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }
}
void PreferencesDialog::on_eventColor_selected(QString col)
{
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_combineSlider_sliderMoved(int position)
{
    ui->combineLCD->display(position);
}

void PreferencesDialog::on_IgnoreSlider_sliderMoved(int position)
{
    if (position>0) {
        ui->IgnoreLCD->display(position);
    } else ui->IgnoreLCD->display(tr("OFF"));
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

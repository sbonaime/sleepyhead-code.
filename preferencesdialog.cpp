#include <QLabel>
#include <QColorDialog>
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "SleepLib/machine_common.h"

PreferencesDialog::PreferencesDialog(QWidget *parent,Profile * _profile) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    profile(_profile)
{
    ui->setupUi(this);
    Q_ASSERT(profile!=NULL);
    ui->tabWidget->setCurrentIndex(0);
    int i=ui->unitCombo->findText((*profile)["UnitSystem"].toString());
    if (i<0) i=0;
    ui->unitCombo->setCurrentIndex(i);

    //i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    //ui->timeZoneCombo->setCurrentIndex(i);

    QTime t=(*profile)["DaySplitTime"].toTime();
    ui->timeEdit->setTime(t);
    int val;

    val=(*profile)["CombineCloserSessions"].toInt();
    ui->combineSlider->setValue(val);
    if (val>0) {
        ui->combineLCD->display(val);
    } else ui->combineLCD->display(tr("OFF"));


    val=(*profile)["IgnoreShorterSessions"].toInt();
    ui->IgnoreSlider->setValue(val);

    if (val>0) {
        ui->IgnoreLCD->display(val);
    } else ui->IgnoreLCD->display(tr("OFF"));

    ui->overlayFlagsCombo->setCurrentIndex((*profile)["AlwaysShowOverlayBars"].toInt());
    ui->useAntiAliasing->setChecked((*profile)["UseAntiAliasing"].toBool());
    ui->memoryHogCheckbox->setChecked((*profile)["MemoryHog"].toBool());
    ui->useGraphSnapshots->setChecked((*profile)["EnableGraphSnapshots"].toBool());
    ui->intentionalLeakEdit->setValue((*profile)["IntentionalLeak"].toDouble());
    ui->useMultithreading->setChecked((*profile)["EnableMultithreading"].toBool());

    ui->eventTable->setColumnWidth(0,40);
    ui->eventTable->setColumnWidth(1,55);
    ui->eventTable->setColumnHidden(3,true);
    int row=0;
    QTableWidgetItem *item;
    QHash<QString, schema::Channel *>::iterator ci;
    for (ci=schema::channel.names.begin();ci!=schema::channel.names.end();ci++) {
        if (ci.value()->type()==schema::DATA) {
            ui->eventTable->insertRow(row);
            int id=ci.value()->id();
            ui->eventTable->setItem(row,3,new QTableWidgetItem(QString::number(id)));
            item=new QTableWidgetItem(ci.value()->description());
            ui->eventTable->setItem(row,2,item);
            QCheckBox *c=new QCheckBox(ui->eventTable);
            c->setChecked(true);
            QLabel *pb=new QLabel(ui->eventTable);
            pb->setText("foo");
            ui->eventTable->setCellWidget(row,0,c);
            ui->eventTable->setCellWidget(row,1,pb);


            QColor a=ci.value()->defaultColor();//(rand() % 255, rand() % 255, rand() % 255, 255);
            QPalette p(a,a,a,a,a,a,a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }
/*    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }*/

    QTreeWidget *tree=ui->graphTree;
    tree->clear();
    tree->setColumnCount(1); // 1 visible common.. (1 hidden)

    QTreeWidgetItem *daily=new QTreeWidgetItem((QTreeWidget *)0,QStringList("Daily Graphs"));
    QTreeWidgetItem *overview=new QTreeWidgetItem((QTreeWidget *)0,QStringList("Overview Graphs"));
    tree->insertTopLevelItem(0,daily);
    tree->insertTopLevelItem(0,overview);
    QTreeWidgetItem *it=new QTreeWidgetItem(daily,QStringList("Event Flags"));//,QTreeWidgetItem::UserType);
    it->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    it->setCheckState(0,Qt::Checked);
    daily->addChild(it);
    //QTreeWidgetItem *root=NULL;//new QTreeWidgetItem((QTreeWidget *)0,QStringList("Stuff"));
    //=new QTreeWidgetItem(root,l);
    //ui->graphTree->setModel(
    tree->sortByColumn(0,Qt::AscendingOrder);

}


PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_eventTable_doubleClicked(const QModelIndex &index)
{
    int row=index.row();
    int col=index.column();
    bool ok;
    int id=ui->eventTable->item(row,3)->text().toInt(&ok);
    if (col==1) {
        QWidget *w=ui->eventTable->cellWidget(row,col);
        QColorDialog a;
        QColor color=w->palette().background().color();
        a.setCurrentColor(color);
        if (a.exec()==QColorDialog::Accepted) {
            QColor c=a.currentColor();
            QPalette p(c,c,c,c,c,c,c);
            w->setPalette(p);
            m_new_colors[id]=c;
            //qDebug() << "Color accepted" << col << id;
        }
    }
}

void PreferencesDialog::Save()
{
    (*profile)["UnitSystem"]=ui->unitCombo->currentText();
    //(*profile)["TimeZone"]=ui->timeZoneCombo->currentText();

    (*profile)["CombineCloserSessions"]=ui->combineSlider->value();
    (*profile)["IgnoreShorterSessions"]=ui->IgnoreSlider->value();

    (*profile)["MemoryHog"]=ui->memoryHogCheckbox->isChecked();
    (*profile)["DaySplitTime"]=ui->timeEdit->time();

    (*profile)["AlwaysShowOverlayBars"]=ui->overlayFlagsCombo->currentIndex();
    (*profile)["UseAntiAliasing"]=ui->useAntiAliasing->isChecked();
    (*profile)["MemoryHog"]=ui->memoryHogCheckbox->isChecked();
    (*profile)["EnableGraphSnapshots"]=ui->useGraphSnapshots->isChecked();
    (*profile)["IntentionalLeak"]=ui->intentionalLeakEdit->value();
    (*profile)["EnableMultithreading"]=ui->useMultithreading->isChecked();

    for (QHash<int,QColor>::iterator i=m_new_colors.begin();i!=m_new_colors.end();i++) {
        schema::Channel &chan=schema::channel[i.key()];
        if (!chan.isNull()) {
            qDebug() << "TODO: Change" << chan.name() << "color to" << i.value();
            chan.setDefaultColor(i.value());
        }
    }
    qDebug() << "TODO: Save channels.xml to update channel data";

    profile->Save();
    PREF.Save();
}

void PreferencesDialog::on_combineSlider_valueChanged(int position)
{
    if (position>0) {
        ui->combineLCD->display(position);
    } else ui->combineLCD->display(tr("OFF"));
}

void PreferencesDialog::on_IgnoreSlider_valueChanged(int position)
{
    if (position>0) {
        ui->IgnoreLCD->display(position);
    } else ui->IgnoreLCD->display(tr("OFF"));
}

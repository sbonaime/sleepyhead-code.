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
    ui->firstNameEdit->setText((*profile)["FirstName"].toString());
    ui->lastNameEdit->setText((*profile)["LastName"].toString());
    ui->addressEdit->clear();
    ui->addressEdit->appendPlainText((*profile)["Address"].toString());
    ui->emailEdit->setText((*profile)["EmailAddress"].toString());
    ui->phoneEdit->setText((*profile)["Phone"].toString());
    QString gender;

    if ((*profile).Exists("Gender")) {
        gender=(*profile)["Gender"].toString().toLower();
    } else gender="male";

    // I know this looks sexist.. This was originally a boolean.. :)
    if ((gender=="male") || (gender=="true")) ui->genderMale->setChecked(true);
    else if ((gender=="female") || (gender=="false")) ui->genderFemale->setChecked(true);


    bool ok;
    ui->heightEdit->setValue((*profile)["Height"].toDouble(&ok));
    if (!(*profile).Exists("DOB")) {
        ui->dobEdit->setDate(QDate(1970,1,1));
    } else {
        ui->dobEdit->setDate((*profile)["DOB"].toDate());
    }
    int i=ui->unitCombo->findText((*profile)["UnitSystem"].toString());
    if (i<0) i=0;
    ui->unitCombo->setCurrentIndex(i);

    i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    ui->timeZoneCombo->setCurrentIndex(i);

    QTime t=pref["DaySplitTime"].toTime();
    ui->timeEdit->setTime(t);
    int val;

    val=pref["CombineCloserSessions"].toInt();
    ui->combineSlider->setValue(val);
    if (val>0) {
        ui->combineLCD->display(val);
    } else ui->combineLCD->display(tr("OFF"));


    val=pref["IgnoreShorterSessions"].toInt();
    ui->IgnoreSlider->setValue(val);

    if (val>0) {
        ui->IgnoreLCD->display(val);
    } else ui->IgnoreLCD->display(tr("OFF"));

    ui->overlayFlagsCombo->setCurrentIndex(pref["AlwaysShowOverlayBars"].toInt());
    ui->useAntiAliasing->setChecked(pref["UseAntiAliasing"].toBool());
    ui->memoryHogCheckbox->setChecked(pref["MemoryHog"].toBool());
    ui->useGraphSnapshots->setChecked(pref["EnableGraphSnapshots"].toBool());
    ui->intentionalLeakEdit->setValue(pref["IntentionalLeak"].toDouble());
    ui->useMultithreading->setChecked(pref["EnableMultithreading"].toBool());

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
    ui->eventTable->sortByColumn(2,Qt::AscendingOrder);
    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }
    ui->dobEdit->setDisplayFormat(shortformat);

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
    (*profile)["FirstName"]=ui->firstNameEdit->text();
    (*profile)["LastName"]=ui->lastNameEdit->text();
    if (ui->genderMale->isChecked())
        (*profile)["Gender"]="male";
    else (*profile)["Gender"]="female";
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

    pref["AlwaysShowOverlayBars"]=ui->overlayFlagsCombo->currentIndex();
    pref["UseAntiAliasing"]=ui->useAntiAliasing->isChecked();
    pref["MemoryHog"]=ui->memoryHogCheckbox->isChecked();
    pref["EnableGraphSnapshots"]=ui->useGraphSnapshots->isChecked();
    pref["IntentionalLeak"]=ui->intentionalLeakEdit->value();
    pref["EnableMultithreading"]=ui->useMultithreading->isChecked();

    for (QHash<int,QColor>::iterator i=m_new_colors.begin();i!=m_new_colors.end();i++) {
        schema::Channel &chan=schema::channel[i.key()];
        if (!chan.isNull()) {
            qDebug() << "TODO: Change" << chan.name() << "color to" << i.value();
            chan.setDefaultColor(i.value());
        }
    }
    qDebug() << "TODO: Save channels.xml to update channel data";

    profile->Save();
    pref.Save();
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

/*
 Create New Profile Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include "SleepLib/profiles.h"

#include "newprofile.h"
#include "ui_newprofile.h"
#include "mainwindow.h"

extern MainWindow *mainwin;

NewProfile::NewProfile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProfile)
{
    ui->setupUi(this);
    ui->userNameEdit->setText(getUserName());
    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }
    ui->dobEdit->setDisplayFormat(shortformat);
    ui->dateDiagnosedEdit->setDisplayFormat(shortformat);
    m_firstPage=0;
    ui->backButton->setEnabled(false);
    ui->nextButton->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(0);
    on_cpapModeCombo_activated(0);
    m_passwordHashed=false;
    ui->heightEdit2->setVisible(false);
    ui->heightEdit->setDecimals(2);
    ui->heightEdit->setSuffix(tr("cm"));

    { // process countries list
    QFile f(":/docs/countries.txt");
    f.open(QFile::ReadOnly);
    QTextStream cnt(&f);
    QString a;
    ui->countryCombo->clear();
    ui->countryCombo->addItem(tr("Select Country"));
    do {
        a=cnt.readLine();
        if (a.isEmpty()) break;
        ui->countryCombo->addItem(a);
    } while(1);
    f.close();
    }
    { // timezone list
    QFile f(":/docs/tz.txt");
    f.open(QFile::ReadOnly);
    QTextStream cnt(&f);
    QString a;
    ui->timezoneCombo->clear();
    //ui->countryCombo->addItem("Select TimeZone");
    do {
        a=cnt.readLine();
        if (a.isEmpty()) break;
        QStringList l;
        l=a.split("=");
        ui->timezoneCombo->addItem(l[1],l[0]);
    } while(1);
    f.close();
    }


}

NewProfile::~NewProfile()
{
    delete ui;
}

void NewProfile::on_nextButton_clicked()
{
    if (!ui->agreeCheckbox->isChecked())
        return;

    int index=ui->stackedWidget->currentIndex();
    switch(index) {
    case 1:
        if (ui->userNameEdit->text().isEmpty()) {
            QMessageBox::information(this,tr("Error"),tr("Empty Username"),QMessageBox::Ok);
            return;
        }
        if (ui->genderCombo->currentIndex()==0) {
            //QMessageBox::information(this,tr("Notice"),tr("You did not specify Gender."),QMessageBox::Ok);
        }
        if (ui->passwordGroupBox->isChecked()) {
            if (ui->passwordEdit1->text()!=ui->passwordEdit2->text()) {
                QMessageBox::information(this,tr("Error"),tr("Passwords don't match"),QMessageBox::Ok);
                return;
            }
            if (ui->passwordEdit1->text().isEmpty())
                ui->passwordGroupBox->setChecked(false);
        }

        break;
    case 2:
        break;
    case 3:
        break;
    default:
        break;
    }

    int max_pages=ui->stackedWidget->count()-1;
    if (index<max_pages) {
        index++;
        ui->stackedWidget->setCurrentIndex(index);
    } else {
        // Finish button clicked.
        if (QMessageBox::question(this,tr("Profile Changes"),tr("Accept and save this information?"),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            Profile *profile=Profiles::Get(ui->userNameEdit->text());
            if (!profile) { // No profile, create one.
                profile=Profiles::Create(ui->userNameEdit->text());
            }
            Profile &prof=*profile;
            prof["FirstName"]=ui->firstNameEdit->text();
            prof["LastName"]=ui->lastNameEdit->text();
            prof["DOB"]=ui->dobEdit->date();
            prof["EmailAddress"]=ui->emailEdit->text();
            prof["Phone"]=ui->phoneEdit->text();
            prof["Address"]=ui->addressEdit->toPlainText();
            if (ui->passwordGroupBox->isChecked()) {
                if (!m_passwordHashed) {
                    QByteArray ba=ui->passwordEdit1->text().toUtf8();
                    prof["Password"]=QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex());
                }
            } else {
                prof.Erase("Password");
            }
            //prof["Password"]="";
            if (ui->genderCombo->currentIndex()==1) {
                prof["Gender"]=tr("Male");
            } else if (ui->genderCombo->currentIndex()==2) {
                prof["Gender"]=tr("Female");
            }
            prof["DateDiagnosed"]=ui->dateDiagnosedEdit->date();
            prof["UntreatedAHI"]=ui->untreatedAHIEdit->value();
            prof["CPAPPrescribedMode"]=ui->cpapModeCombo->currentIndex();
            prof["CPAPPrescribedMinPressure"]=ui->minPressureEdit->value();
            prof["CPAPPrescribedMaxPressure"]=ui->minPressureEdit->value();
            prof["CPAPNotes"]=ui->cpapNotes->toPlainText();
            prof["DoctorName"]=ui->doctorNameEdit->text();
            prof["DoctorPractice"]=ui->doctorPracticeEdit->text();
            prof["DoctorAddress"]=ui->doctorAddressEdit->toPlainText();
            prof["DoctorPhone"]=ui->doctorPhoneEdit->text();
            prof["DoctorEmail"]=ui->doctorEmailEdit->text();
            prof["DoctorPatientID"]=ui->doctorPatientIDEdit->text();
            prof["Language"]=ui->languageCombo->currentText();
            prof["TimeZone"]=ui->timezoneCombo->itemData(ui->timezoneCombo->currentIndex()).toString();
            prof["Country"]=ui->countryCombo->currentText();
            prof["DST"]=ui->DSTcheckbox->isChecked();
            if (prof["Units"].toString()!=ui->heightCombo->currentText()) {

                prof["Units"]=ui->heightCombo->currentText();
                if (mainwin && mainwin->getDaily()) mainwin->getDaily()->UnitsChanged();
            }
            double v=0;
            if (ui->heightCombo->currentIndex()==1) {
                // convert to metric
                v=(ui->heightEdit->value()*30.48);
                v+=ui->heightEdit2->value()*2.54;
            } else {
                v=ui->heightEdit->value();
            }
            prof["Height"]=v;

            PREF["Profile"]=ui->userNameEdit->text();


            this->accept();
        }
    }

    if (index>=max_pages) {
        ui->nextButton->setText(tr("&Finish"));
    } else {
        ui->nextButton->setText(tr("&Next"));
    }
    ui->backButton->setEnabled(true);

}

void NewProfile::on_backButton_clicked()
{
    ui->nextButton->setText(tr("&Next"));
    if (ui->stackedWidget->currentIndex()>m_firstPage) {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->currentIndex()-1);
    }
    if (ui->stackedWidget->currentIndex()==m_firstPage) {
        ui->backButton->setEnabled(false);
    } else {
        ui->backButton->setEnabled(true);
    }


}


void NewProfile::on_cpapModeCombo_activated(int index)
{
    if (index==0) {
        ui->maxPressureEdit->setVisible(false);
    } else {
        ui->maxPressureEdit->setVisible(true);
    }
}

void NewProfile::on_agreeCheckbox_clicked(bool checked)
{
    ui->nextButton->setEnabled(checked);
}

void NewProfile::skipWelcomeScreen()
{
    ui->agreeCheckbox->setChecked(true);
    ui->stackedWidget->setCurrentIndex(m_firstPage=1);
    ui->backButton->setEnabled(false);
    ui->nextButton->setEnabled(true);
}
void NewProfile::edit(const QString name)
{
    skipWelcomeScreen();
    Profile *profile=Profiles::Get(name);
    if (!profile) {
        profile=Profiles::Create(name);
        (*profile)["FirstName"]="";
        (*profile)["LastName"]="";
    }
    ui->userNameEdit->setText(name);
    ui->userNameEdit->setReadOnly(true);
    ui->firstNameEdit->setText((*profile)["FirstName"].toString());
    ui->lastNameEdit->setText((*profile)["LastName"].toString());
    if (profile->Exists("Password")) {
        // leave the password box blank..
        ui->passwordEdit1->setText("");
        ui->passwordEdit2->setText("");
        ui->passwordGroupBox->setChecked(true);
        m_passwordHashed=true;
    }
    ui->dobEdit->setDate((*profile)["DOB"].toDate());
    if (profile->Get("Gender").toLower()=="male") {
        ui->genderCombo->setCurrentIndex(1);
    } else if (profile->Get("Gender").toLower()=="female") {
        ui->genderCombo->setCurrentIndex(2);
    } else ui->genderCombo->setCurrentIndex(0);
    ui->heightEdit->setValue((*profile)["Height"].toDouble());
    ui->addressEdit->setText(profile->Get("Address"));
    ui->emailEdit->setText(profile->Get("EmailAddress"));
    ui->phoneEdit->setText(profile->Get("Phone"));
    ui->dateDiagnosedEdit->setDate((*profile)["DateDiagnosed"].toDate());
    ui->cpapNotes->clear();
    ui->cpapNotes->appendPlainText(profile->Get("CPAPNotes"));
    ui->minPressureEdit->setValue((*profile)["CPAPPrescribedMinPressure"].toDouble());
    ui->maxPressureEdit->setValue((*profile)["CPAPPrescribedMaxPressure"].toDouble());
    ui->untreatedAHIEdit->setValue((*profile)["UntreatedAHI"].toDouble());
    ui->cpapModeCombo->setCurrentIndex((*profile)["CPAPPrescribedMode"].toInt());

    ui->doctorNameEdit->setText(profile->Get("DoctorName"));
    ui->doctorPracticeEdit->setText(profile->Get("DoctorPractice"));
    ui->doctorPhoneEdit->setText(profile->Get("DoctorPhone"));
    ui->doctorEmailEdit->setText(profile->Get("DoctorEmail"));
    ui->doctorAddressEdit->setText(profile->Get("DoctorAddress"));
    ui->doctorPatientIDEdit->setText(profile->Get("DoctorPatientID"));

    ui->DSTcheckbox->setChecked((*profile)["DST"].toBool());
    int i=ui->timezoneCombo->findData(profile->Get("TimeZone"));
    ui->timezoneCombo->setCurrentIndex(i);
    i=ui->countryCombo->findText(profile->Get("Country"));
    ui->countryCombo->setCurrentIndex(i);

    i=ui->heightCombo->findText(profile->Get("Units"));
    if (i<0) i=0;
    ui->heightCombo->setCurrentIndex(i);

    bool ok;
    double v=(*profile)["Height"].toDouble(&ok);
    if (!ok) v=0;

    if (i==1)  { // evil non-metric
        int ti=v/2.54;
        int feet=ti / 12;
        int inches=ti % 12;
        ui->heightEdit->setValue(feet);
        ui->heightEdit2->setValue(inches);
        ui->heightEdit2->setVisible(true);
        ui->heightEdit->setDecimals(0);
        ui->heightEdit2->setDecimals(0);
        ui->heightEdit->setSuffix(tr("ft")); // foot
        ui->heightEdit2->setSuffix(tr("\"")); // inches
    } else { // good wholesome metric
        ui->heightEdit->setValue(v);
        ui->heightEdit2->setVisible(false);
        ui->heightEdit->setDecimals(2);
        ui->heightEdit->setSuffix(tr("cm"));
    }
}

void NewProfile::on_passwordEdit1_editingFinished()
{
    m_passwordHashed=false;
}

void NewProfile::on_passwordEdit2_editingFinished()
{
    m_passwordHashed=false;
}


void NewProfile::on_heightCombo_currentIndexChanged(int index)
{
    if (index==0) {
        //metric
        ui->heightEdit2->setVisible(false);
        ui->heightEdit->setDecimals(2);
        ui->heightEdit->setSuffix(tr("cm"));
        double v=ui->heightEdit->value()*30.48;
        v+=ui->heightEdit2->value()*2.54;
        ui->heightEdit->setValue(v);
    } else {        //evil
        ui->heightEdit->setDecimals(0);
        ui->heightEdit2->setDecimals(0);
        ui->heightEdit->setSuffix(tr("ft"));
        ui->heightEdit2->setVisible(true);
        ui->heightEdit2->setSuffix(tr("\""));
        int v=ui->heightEdit->value()/2.54;
        int feet=v / 12;
        int inches=v % 12;
        ui->heightEdit->setValue(feet);
        ui->heightEdit2->setValue(inches);
    }
}

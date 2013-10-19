/*
 Create New Profile Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QSettings>
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
    ui->heightEdit->setSuffix(STR_UNIT_CM);

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
    ui->AppTitle->setText("SleepyHead v"+VersionString);
    ui->releaseStatus->setText(ReleaseStatus);

    ui->textBrowser->setSource(QUrl("qrc:/docs/intro.html"));
}

NewProfile::~NewProfile()
{
    delete ui;
}

void NewProfile::on_nextButton_clicked()
{
    const QString xmlext=".xml";

    QSettings settings(getDeveloperName(), getAppName());

    int index=ui->stackedWidget->currentIndex();
    switch(index) {
    case 0:
        if (!ui->agreeCheckbox->isChecked())
            return;
        // Reload Preferences object
        break;
    case 1:
        if (ui->userNameEdit->text().isEmpty()) {
            QMessageBox::information(this,STR_MESSAGE_ERROR,tr("Empty Username"),QMessageBox::Ok);
            return;
        }
        if (ui->genderCombo->currentIndex()==0) {
            //QMessageBox::information(this,tr("Notice"),tr("You did not specify Gender."),QMessageBox::Ok);
        }
        if (ui->passwordGroupBox->isChecked()) {
            if (ui->passwordEdit1->text()!=ui->passwordEdit2->text()) {
                QMessageBox::information(this,STR_MESSAGE_ERROR,tr("Passwords don't match"),QMessageBox::Ok);
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
        QString username=ui->userNameEdit->text();
        if (QMessageBox::question(this,tr("Profile Changes"),tr("Accept and save this information?"),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            Profile *profile=Profiles::Get(username);
            if (!profile) { // No profile, create one.
                profile=Profiles::Create(username);
            }
            Profile &prof=*profile;
            profile->user->setFirstName(ui->firstNameEdit->text());
            profile->user->setLastName(ui->lastNameEdit->text());
            profile->user->setDOB(ui->dobEdit->date());
            profile->user->setEmail(ui->emailEdit->text());
            profile->user->setPhone(ui->phoneEdit->text());
            profile->user->setAddress(ui->addressEdit->toPlainText());
            if (ui->passwordGroupBox->isChecked()) {
                if (!m_passwordHashed) {
                    profile->user->setPassword(ui->passwordEdit1->text().toUtf8());
                }
            } else {

                prof.Erase(STR_UI_Password);
            }

            profile->user->setGender((Gender)ui->genderCombo->currentIndex());

            profile->cpap->setDateDiagnosed(ui->dateDiagnosedEdit->date());
            profile->cpap->setUntreatedAHI(ui->untreatedAHIEdit->value());
            profile->cpap->setMode((CPAPMode)ui->cpapModeCombo->currentIndex());
            profile->cpap->setMinPressure(ui->minPressureEdit->value());
            profile->cpap->setMaxPressure(ui->maxPressureEdit->value());
            profile->cpap->setNotes(ui->cpapNotes->toPlainText());
            profile->doctor->setName(ui->doctorNameEdit->text());
            profile->doctor->setPracticeName(ui->doctorPracticeEdit->text());
            profile->doctor->setAddress(ui->doctorAddressEdit->toPlainText());
            profile->doctor->setPhone(ui->doctorPhoneEdit->text());
            profile->doctor->setEmail(ui->doctorEmailEdit->text());
            profile->doctor->setPatientID(ui->doctorPatientIDEdit->text());
            profile->user->setTimeZone(ui->timezoneCombo->itemData(ui->timezoneCombo->currentIndex()).toString());
            profile->user->setCountry(ui->countryCombo->currentText());
            profile->user->setDaylightSaving(ui->DSTcheckbox->isChecked());
            UnitSystem us;
            if (ui->heightCombo->currentIndex()==0) us=US_Metric;
            else if (ui->heightCombo->currentIndex()==1) us=US_Archiac;
            else us=US_Metric;

            if (profile->general->unitSystem() != us) {
                profile->general->setUnitSystem(us);
                if (mainwin && mainwin->getDaily()) mainwin->getDaily()->UnitsChanged();
            }

            double v=0;
            if (us==US_Archiac) {
                // convert to metric
                v=(ui->heightEdit->value()*30.48);
                v+=ui->heightEdit2->value()*2.54;
            } else {
                v=ui->heightEdit->value();
            }
            profile->user->setHeight(v);

            //profile->user->setUserName(username);
            PREF[STR_GEN_Profile]=username;


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
    }
    ui->userNameEdit->setText(name);
    ui->userNameEdit->setReadOnly(true);
    ui->firstNameEdit->setText(profile->user->firstName());
    ui->lastNameEdit->setText(profile->user->lastName());
    if (profile->contains(STR_UI_Password) && !profile->p_preferences[STR_UI_Password].toString().isEmpty()) {
        // leave the password box blank..
        QString a="******";
        ui->passwordEdit1->setText(a);
        ui->passwordEdit2->setText(a);
        ui->passwordGroupBox->setChecked(true);
        m_passwordHashed=true;
    }
    ui->dobEdit->setDate(profile->user->DOB());
    if (profile->user->gender()==Male) {
        ui->genderCombo->setCurrentIndex(1);
    } else if (profile->user->gender()==Female) {
        ui->genderCombo->setCurrentIndex(2);
    } else ui->genderCombo->setCurrentIndex(0);
    ui->heightEdit->setValue(profile->user->height());
    ui->addressEdit->setText(profile->user->address());
    ui->emailEdit->setText(profile->user->email());
    ui->phoneEdit->setText(profile->user->phone());
    ui->dateDiagnosedEdit->setDate(profile->cpap->dateDiagnosed());
    ui->cpapNotes->clear();
    ui->cpapNotes->appendPlainText(profile->cpap->notes());
    ui->minPressureEdit->setValue(profile->cpap->minPressure());
    ui->maxPressureEdit->setValue(profile->cpap->maxPressure());
    ui->untreatedAHIEdit->setValue(profile->cpap->untreatedAHI());
    ui->cpapModeCombo->setCurrentIndex((int)profile->cpap->mode());

    ui->doctorNameEdit->setText(profile->doctor->name());
    ui->doctorPracticeEdit->setText(profile->doctor->practiceName());
    ui->doctorPhoneEdit->setText(profile->doctor->phone());
    ui->doctorEmailEdit->setText(profile->doctor->email());
    ui->doctorAddressEdit->setText(profile->doctor->address());
    ui->doctorPatientIDEdit->setText(profile->doctor->patientID());

    ui->DSTcheckbox->setChecked(profile->user->daylightSaving());
    int i=ui->timezoneCombo->findData(profile->user->timeZone());
    ui->timezoneCombo->setCurrentIndex(i);
    i=ui->countryCombo->findText(profile->user->country());
    ui->countryCombo->setCurrentIndex(i);

    UnitSystem us=profile->general->unitSystem();
    i=(int)us - 1;
    if (i<0) i=0;
    ui->heightCombo->setCurrentIndex(i);

    double v=profile->user->height();

    if (us==US_Archiac)  { // evil non-metric
        int ti=v/2.54;
        int feet=ti / 12;
        int inches=ti % 12;
        ui->heightEdit->setValue(feet);
        ui->heightEdit2->setValue(inches);
        ui->heightEdit2->setVisible(true);
        ui->heightEdit->setDecimals(0);
        ui->heightEdit2->setDecimals(0);
        ui->heightEdit->setSuffix(STR_UNIT_FOOT); // foot
        ui->heightEdit2->setSuffix(STR_UNIT_INCH); // inches
    } else { // good wholesome metric
        ui->heightEdit->setValue(v);
        ui->heightEdit2->setVisible(false);
        ui->heightEdit->setDecimals(2);
        ui->heightEdit->setSuffix(STR_UNIT_CM);
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
        ui->heightEdit->setSuffix(STR_UNIT_CM);
        double v=ui->heightEdit->value()*30.48;
        v+=ui->heightEdit2->value()*2.54;
        ui->heightEdit->setValue(v);
    } else {        //evil
        ui->heightEdit->setDecimals(0);
        ui->heightEdit2->setDecimals(0);
        ui->heightEdit->setSuffix(STR_UNIT_FOOT);
        ui->heightEdit2->setVisible(true);
        ui->heightEdit2->setSuffix(STR_UNIT_INCH);
        int v=ui->heightEdit->value()/2.54;
        int feet=v / 12;
        int inches=v % 12;
        ui->heightEdit->setValue(feet);
        ui->heightEdit2->setValue(inches);
    }
}

#include <QMessageBox>
#include <QCryptographicHash>
#include "SleepLib/profiles.h"

#include "newprofile.h"
#include "ui_newprofile.h"

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
            QMessageBox::information(this,"Error","Empty Username",QMessageBox::Ok);
            return;
        }
        if (ui->genderCombo->currentIndex()==0) {
            //QMessageBox::information(this,"Notice","You did not specify Gender.",QMessageBox::Ok);
        }
        if (ui->passwordGroupBox->isChecked()) {
            if (ui->passwordEdit1->text()!=ui->passwordEdit2->text()) {
                QMessageBox::information(this,"Error","Passwords don't match",QMessageBox::Ok);
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
        if (QMessageBox::question(this,"Profile Changes","Accept and save this information?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes) {
            Profile *profile=Profiles::Get(ui->userNameEdit->text());
            if (!profile) { // No profile, create one.
                profile=Profiles::Create(ui->userNameEdit->text());
            }
            Profile &prof=*profile;
            prof["FirstName"]=ui->firstNameEdit->text();
            prof["LastName"]=ui->lastNameEdit->text();
            prof["DOB"]=ui->dobEdit->date();
            prof["Height"]=ui->heightEdit->value();
            prof["EmailAddress"]=ui->emailEdit->text();
            prof["Phone"]=ui->phoneEdit->text();
            prof["Address"]=ui->addressEdit->toPlainText();
            if (ui->passwordGroupBox->isChecked()) {
                QByteArray ba=ui->passwordEdit1->text().toUtf8();
                prof["Password"]=QString(QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex());
            }
            //prof["Password"]="";
            if (ui->genderCombo->currentIndex()==1) {
                prof["Gender"]="Male";
            } else if (ui->genderCombo->currentIndex()==2) {
                prof["Gender"]="Female";
            }
            prof["DateDiagnosed"]=ui->dateDiagnosedEdit->date();
            prof["UntreatedAHI"]=ui->untreatedAHIEdit->value();
            prof["CPAPPrescribedMode"]=ui->cpapModeCombo->currentIndex();
            prof["CPAPPrescribedMinPressure"]=ui->minPressureEdit->value();
            prof["CPAPPrescribedMaxPressure"]=ui->minPressureEdit->value();
            prof["CPAPNotes"]=ui->cpapNotes->toPlainText();
            prof["DoctorName"]=ui->doctorNameEdit->text();
            prof["DcotorPractice"]=ui->doctorPracticeEdit->text();
            prof["DoctorAddress"]=ui->doctorAddressEdit->toPlainText();
            prof["DoctorPhone"]=ui->doctorPhoneEdit->text();
            prof["DoctorEmail"]=ui->doctorEmailEdit->text();
            prof["DoctorPatientID"]=ui->doctorPatientIDEdit->text();


            pref["Profile"]=ui->userNameEdit->text();


            this->accept();
        }
    }

    if (index>=max_pages) {
        ui->nextButton->setText("&Finish");
    } else {
        ui->nextButton->setText("&Next");
    }
    ui->backButton->setEnabled(true);

}

void NewProfile::on_backButton_clicked()
{
    ui->nextButton->setText("&Next");
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

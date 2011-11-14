#include <QFileDialog>
#include <QLocale>
#include <QMessageBox>
#include "SleepLib/profiles.h"
#include "exportcsv.h"
#include "ui_exportcsv.h"

ExportCSV::ExportCSV(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportCSV)
{
    ui->setupUi(this);
    ui->rb1_Summary->setChecked(true);
    ui->quickRangeCombo->setCurrentIndex(0);

    // Set Date controls locale to 4 digit years
    QLocale locale=QLocale::system();
    QString shortformat=locale.dateFormat(QLocale::ShortFormat);
    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy","yyyy");
    }
    ui->startDate->setDisplayFormat(shortformat);
    ui->endDate->setDisplayFormat(shortformat);

    on_quickRangeCombo_activated("Most Recent Day");
    ui->rb1_details->clearFocus();
    ui->quickRangeCombo->setFocus();
    ui->exportButton->setEnabled(false);
}

ExportCSV::~ExportCSV()
{
    delete ui;
}

void ExportCSV::on_filenameBrowseButton_clicked()
{
    QString timestamp="SleepyHead_";

    if (ui->rb1_details->isChecked()) timestamp+="Details_";
    if (ui->rb1_Sessions->isChecked()) timestamp+="Sessions_";
    if (ui->rb1_Summary->isChecked()) timestamp+="Summary_";

    timestamp+=ui->startDate->date().toString(Qt::ISODate);
    if (ui->startDate->date()!=ui->endDate->date()) timestamp+="_"+ui->endDate->date().toString(Qt::ISODate);
    timestamp+=".csv";
    QString name=QFileDialog::getSaveFileName(this,"Select file to export to",PREF.Get("{home}/")+timestamp,"CSV Files (*.csv)");
    if (name.isEmpty()) {
        ui->exportButton->setEnabled(false);
        return;
    }
    if (!name.toLower().endsWith(".csv")) {
        name+=".csv";
    }

    ui->filenameEdit->setText(name);
    ui->exportButton->setEnabled(true);
}

void ExportCSV::on_quickRangeCombo_activated(const QString &arg1)
{
    if (arg1=="Custom") {
        ui->startDate->setEnabled(true);
        ui->endDate->setEnabled(true);
        ui->startLabel->setEnabled(true);
        ui->endLabel->setEnabled(true);
    } else {
        ui->startDate->setEnabled(false);
        ui->endDate->setEnabled(false);
        ui->startLabel->setEnabled(false);
        ui->endLabel->setEnabled(false);
        if (arg1=="Everything") {
            ui->startDate->setDate(PROFILE.FirstDay());
            ui->endDate->setDate(PROFILE.LastDay());
        } else if (arg1=="Most Recent Day") {
            ui->startDate->setDate(PROFILE.LastDay());
            ui->endDate->setDate(PROFILE.LastDay());
        } else if (arg1=="Last Week") {
            ui->startDate->setDate(QDate::currentDate().addDays(-7));
            ui->endDate->setDate(QDate::currentDate());
        } else if (arg1=="Last Fortnight") {
            ui->startDate->setDate(QDate::currentDate().addDays(-14));
            ui->endDate->setDate(QDate::currentDate());
        } else if (arg1=="Last Month") {
            ui->startDate->setDate(QDate::currentDate().addMonths(-1));
            ui->endDate->setDate(QDate::currentDate());
        } else if (arg1=="Last 6 Months") {
            ui->startDate->setDate(QDate::currentDate().addMonths(-6));
            ui->endDate->setDate(QDate::currentDate());
        } else if (arg1=="Last Year") {
            ui->startDate->setDate(QDate::currentDate().addYears(-1));
            ui->endDate->setDate(QDate::currentDate());
        }
    }
}

void ExportCSV::on_exportButton_clicked()
{
    QMessageBox::information(this,"Stub Feature","Not implemented yet:\n"+ui->filenameEdit->text(),QMessageBox::Ok);

    ExportCSV::accept();
}

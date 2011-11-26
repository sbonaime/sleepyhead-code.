/*
 ExportCSV module header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef EXPORTCSV_H
#define EXPORTCSV_H

#include <QDateEdit>
#include <QDialog>

namespace Ui {
    class ExportCSV;
}

class ExportCSV : public QDialog
{
    Q_OBJECT

public:
    explicit ExportCSV(QWidget *parent = 0);
    ~ExportCSV();

private slots:
    void on_filenameBrowseButton_clicked();

    void on_quickRangeCombo_activated(const QString &arg1);

    void on_exportButton_clicked();

    void startDate_currentPageChanged(int year, int month);
    void endDate_currentPageChanged(int year, int month);


private:
    void UpdateCalendarDay(QDateEdit * dateedit,QDate date);

    Ui::ExportCSV *ui;
};

#endif // EXPORTCSV_H

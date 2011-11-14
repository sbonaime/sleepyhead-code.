#ifndef EXPORTCSV_H
#define EXPORTCSV_H

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

private:
    Ui::ExportCSV *ui;
};

#endif // EXPORTCSV_H

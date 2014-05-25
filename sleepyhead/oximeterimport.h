#ifndef OXIMETERIMPORT_H
#define OXIMETERIMPORT_H

#include <QDialog>

#include "Graphs/gGraphView.h"
#include "Graphs/gLineChart.h"
#include "SleepLib/machine_loader.h"
#include "sessionbar.h"

namespace Ui {
class OximeterImport;
}

class OximeterImport : public QDialog
{
    Q_OBJECT

public:
    explicit OximeterImport(QWidget *parent = 0);
    ~OximeterImport();

private slots:
    void on_nextButton_clicked();

    void on_directImportButton_clicked();
    void doUpdateProgress(int, int);
    void on_fileImportButton_clicked();

    void on_liveImportButton_clicked();

    void on_retryButton_clicked();

    void on_stopButton_clicked();

    void on_calendarWidget_clicked(const QDate &date);

    void on_calendarWidget_selectionChanged();

    void onSessionSelected(Session * session);

    void on_sessionBackButton_clicked();

    void on_sessionForwardButton_clicked();

    void on_radioSyncCPAP_clicked();

    void on_radioSyncOximeter_clicked();

    void on_radioSyncManually_clicked();

    void on_syncSaveButton_clicked();

protected:
    MachineLoader * detectOximeter();
    void updateStatus(QString msg);

private:
    Ui::OximeterImport *ui;
    MachineLoader * oximodule;
    gGraphView * liveView;
    gGraph * PLETHY;
    gLineChart * plethyChart;
    SessionBar * sessbar;

};

#endif // OXIMETERIMPORT_H

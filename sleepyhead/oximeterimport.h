#ifndef OXIMETERIMPORT_H
#define OXIMETERIMPORT_H

#include <QDialog>

#include "Graphs/gGraphView.h"
#include "Graphs/gLineChart.h"
#include "SleepLib/serialoximeter.h"
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

    void on_cancelButton_clicked();

    void on_showLiveGraphs_clicked(bool checked);

    void on_skipWelcomeCheckbox_clicked(bool checked);

    void on_informationButton_clicked();

protected slots:
    void on_updatePlethy(QByteArray plethy);
    void finishedRecording();
    void finishedImport(SerialOximeter*);
    void updateLiveDisplay();

protected:
    SerialOximeter * detectOximeter();
    void updateStatus(QString msg);

private:
    Ui::OximeterImport *ui;
    SerialOximeter * oximodule;
    gGraphView * liveView;
    gGraph * plethyGraph;
    Session * session;
    Day * dummyday;
    gLineChart * plethyChart;
    SessionBar * sessbar;
    EventList * ELplethy;
    qint64 start_ti, ti;
    QTimer updateTimer;

    int pulse;
    int spo2;

};

#endif // OXIMETERIMPORT_H

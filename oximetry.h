#ifndef OXIMETRY_H
#define OXIMETRY_H

#include <QWidget>
#include <QSplitter>
#include <qextserialport/qextserialport.h>

#include "SleepLib/profiles.h"
#include "Graphs/graphwindow.h"
#include "Graphs/graphdata_custom.h"

namespace Ui {
    class Oximetry;
}

enum PORTMODE { PM_LIVE, PM_RECORDING };
const int max_data_points=1000000;

class Oximetry : public QWidget
{
    Q_OBJECT

public:
    explicit Oximetry(QWidget *parent = 0);
    ~Oximetry();

    void AddData(gPointData *d) { Data.push_back(d);  }
    void AddGraph(gGraphWindow *w) { Graphs.push_back(w); }
    void RedrawGraphs();

private slots:
    void on_RefreshPortsButton_clicked();

    void on_RunButton_toggled(bool checked);

    void on_SerialPortsCombo_activated(const QString &arg1);
    void onReadyRead();
    void onDsrChanged(bool status);

    void on_ImportButton_clicked();

private:
    bool UpdatePulseSPO2(qint8 pulse,qint8 spo2);
    void UpdatePlethy(qint8 plethy);


    Ui::Oximetry *ui;
    Profile *profile;
    QSplitter *gSplitter;
    gPointData *pulse,*spo2,*plethy;
    gGraphWindow *PULSE,*SPO2,*PLETHY;
    list<gGraphWindow *> Graphs;
    list<gPointData *> Data;
    QextSerialPort *port;
    QString portname;
    PORTMODE portmode;
    qint64 lasttime,starttime;
    int lastpulse, lastspo2;

};

#endif // OXIMETRY_H

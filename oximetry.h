#ifndef OXIMETRY_H
#define OXIMETRY_H

#include <QWidget>
#include <QGLContext>

#include <QSplitter>
#include <qextserialport/qextserialport.h>

#include "SleepLib/profiles.h"
#include "SleepLib/day.h"
#include "SleepLib/session.h"

#include "Graphs/graphwindow.h"
#include "Graphs/gLineChart.h"

namespace Ui {
    class Oximetry;
}

enum PORTMODE { PM_LIVE, PM_RECORDING };
const int max_data_points=1000000;

class Oximetry : public QWidget
{
    Q_OBJECT

public:
    explicit Oximetry(QWidget *parent,QGLWidget * shared=NULL);
    ~Oximetry();

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
    bool UpdatePulse(qint8 pulse);
    bool UpdateSPO2(qint8 spo2);
    void UpdatePlethy(qint8 plethy);

    Ui::Oximetry *ui;
    Profile *profile;
    QSplitter *gSplitter;
    gLineChart *pulse,*spo2,*plethy;
    gGraphWindow *PULSE,*SPO2,*PLETHY;

    QVector<gGraphWindow *> Graphs;
    QVector<gLineChart *> Data;

    QextSerialPort *port;
    QString portname;
    PORTMODE portmode;
    qint64 lasttime,starttime;
    int lastpulse, lastspo2;

    Machine * mach;
    Day * day;
    Session * session;
    EventList * ev_pulse;
    EventList * ev_spo2;
    EventList * ev_plethy;
};

#endif // OXIMETRY_H

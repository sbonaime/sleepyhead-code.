/*
 Oximetry GUI Headers
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef OXIMETRY_H
#define OXIMETRY_H

#include <QWidget>
#include <QGLContext>
#include <QHBoxLayout>
#include <QSplitter>
#include <qextserialport/qextserialport.h>

#include "SleepLib/profiles.h"
#include "SleepLib/day.h"
#include "SleepLib/session.h"

#include "Graphs/gLineChart.h"
#include "Graphs/gFooBar.h"

namespace Ui {
    class Oximetry;
}

enum PORTMODE { PM_LIVE, PM_RECORDING };
const int max_data_points=1000000;

class Oximetry : public QWidget
{
    Q_OBJECT

public:
    explicit Oximetry(QWidget *parent, gGraphView * shared=NULL);
    ~Oximetry();

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

    gGraphView *GraphView;
    MyScrollBar *scrollbar;
    QHBoxLayout *layout;

    gLineChart *pulse,*spo2,*plethy;
    gGraph *PULSE,*SPO2,*PLETHY,*CONTROL;

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
    Layer * foobar;
    gGraphView * m_shared;
};

#endif // OXIMETRY_H

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib CMS50X Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef CMS50LOADER_H
#define CMS50LOADER_H

#include <QtSerialPort/QSerialPort>
#include <QTimer>

#include "SleepLib/machine_loader.h"

const QString cms50_class_name = "CMS50";
const int cms50_data_version = 4;

struct OxiRecord
{
    quint8 pulse;
    quint8 spo2;
    OxiRecord():pulse(0), spo2(0) {}
    OxiRecord(quint8 p, quint8 s): pulse(p), spo2(s) {}
    OxiRecord(const OxiRecord & copy) { pulse = copy.pulse; spo2= copy.spo2; }
};

class SerialLoader : public MachineLoader
{
Q_OBJECT
public:
    SerialLoader() : MachineLoader() {}
    virtual ~SerialLoader() {};

    virtual bool Detect(const QString &path)=0;
    virtual int Open(QString path, Profile *profile)=0;

    static void Register() {}

    virtual int Version()=0;
    virtual const QString &ClassName()=0;


    // Serial Stuff
    virtual bool scanDevice(QString keyword="",quint16 vendor_id=0, quint16 product_id=0);
    virtual bool openDevice();
    virtual void closeDevice();

    virtual void process() {}

signals:
    void noDeviceFound();
    void deviceDetected();

protected slots:
    virtual void dataAvailable();
    virtual void resetImportTimeout() {}
    virtual void startImportTimeout() {}

protected:
    virtual void processBytes(QByteArray buffer) { Q_UNUSED(buffer) }

    virtual void killTimers() {}
    virtual void resetDevice() {}
    virtual void requestData() {}

    QString port;
    QSerialPort serial;

    QTimer startTimer;
    QTimer resetTimer;

    quint16 m_productID;
    quint16 m_vendorID;
};

/*! \class CMS50Loader
    \brief Bulk Importer for CMS50 SPO2Review format.. Deprecated, as the Oximetry module does a better job
    */
class CMS50Loader : public SerialLoader
{
Q_OBJECT
  public:


    CMS50Loader();
    virtual ~CMS50Loader();

    virtual bool Detect(const QString &path);
    virtual int Open(QString path, Profile *profile);

    static void Register();

    virtual int Version() { return cms50_data_version; }
    virtual const QString &ClassName() { return cms50_class_name; }

    Machine *CreateMachine(Profile *profile);

    virtual void process();

protected slots:
//    virtual void dataAvailable();
    virtual void resetImportTimeout();
    virtual void startImportTimeout();

protected:

    bool readSpoRFile(QString path);
    virtual void processBytes(QByteArray bytes);

    int doImportMode();
    int doLiveMode();

    QVector<OxiRecord> oxirec;

    virtual void killTimers();

    // Switch CMS50D+ device to live streaming mode
    virtual void resetDevice();

    // Switch CMS50D+ device to record transmission mode
    void requestData();

  private:
    EventList *PULSE;
    EventList *SPO2;

    QTime m_time;

    QByteArray buffer;

    bool started_import;
    bool finished_import;
    bool started_reading;
    bool cms50dplus;
    QDateTime oxitime;

    int cb_reset,imp_callbacks;

    int received_bytes;

    int m_itemCnt;
    int m_itemTotal;


};

#include <QTimeEdit>
#include <QCalendarWidget>
#include <QHBoxLayout>
#include <QTextCharFormat>
#include <QFont>
#include <QShortcut>
#include <QLineEdit>

class MyTimeEdit: public QDateTimeEdit
{
public:
    MyTimeEdit(QWidget *parent):QDateTimeEdit(parent) {
        setKeyboardTracking(true);
        this->setDisplayFormat("hh:mm:ssap");

        editor()->setAlignment(Qt::AlignCenter);
    }
    QLineEdit * editor() const {
        return this->lineEdit();
    }
};

#include <QLabel>
#include <QDialog>

#include "sessionbar.h"

class DateTimeDialog : public QDialog
{
Q_OBJECT
public:
    DateTimeDialog(QString message, QWidget * parent = nullptr, Qt::WindowFlags flags = Qt::Dialog);
    ~DateTimeDialog();

    QDateTime execute(QDateTime datetime);
    void setMessage(QString msg) { m_msglabel->setText(msg); }
    void setBottomMessage(QString msg) { m_bottomlabel->setText(msg); m_bottomlabel->setVisible(true);}

    QCalendarWidget * cal() { return m_cal; }
signals:
public slots:
    void reject();
    void onDateSelected(QDate date);
    void onSelectionChanged();
    void onCurrentPageChanged(int year, int month);
    void onSessionSelected(Session *);

protected:
    QVBoxLayout * layout;
    QHBoxLayout * layout2;

    QPushButton *acceptbutton;
    QPushButton *resetbutton;

    QLabel * m_msglabel;
    QLabel * m_bottomlabel;

    QCalendarWidget * m_cal;
    MyTimeEdit * m_timeedit;
    SessionBar * sessbar;
    QFont font;
    QShortcut * shortcutQuit;
};


#endif // CMS50LOADER_H

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Oximetry GUI Headers
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef OXIMETRY_H
#define OXIMETRY_H

#include <QWidget>
#include <QGLContext>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QMutex>

#include <qextserialport.h>

#include "SleepLib/profiles.h"
#include "SleepLib/day.h"
#include "SleepLib/session.h"

#include "Graphs/gLineChart.h"
#include "Graphs/gFooBar.h"

//! \brief Oximeters current mode
enum SerialOxMode { SO_OFF, SO_IMPORT, SO_LIVE, SO_WAIT };

/*! \class SerialOximeter
    \author Mark Watkins <jedimark_at_users.sourceforge.net>
    \brief Base class for Serial Oximeters
    */
class SerialOximeter: public QObject
{
    Q_OBJECT
  public:
    explicit SerialOximeter(QObject *parent, QString oxiname, QString portname = "",
                            BaudRateType baud = BAUD19200, FlowType flow = FLOW_OFF, ParityType parity = PAR_ODD,
                            DataBitsType databits = DATA_8, StopBitsType stopbits = STOP_1);
    virtual ~SerialOximeter();

    virtual void setSession(Session *sess) { session = sess; }

    //! \brief Open the serial port in either EventDriven or Polling mode
    virtual bool Open(QextSerialPort::QueryMode mode = QextSerialPort::EventDriven);

    //! \brief Close the serial port
    virtual void Close();

    //! \brief Virtual method for Importing the Oximeters internal recording.
    virtual bool startImport() = 0;
    //! \brief Virtual method to Abort importing the Oximeters internal recording.
    virtual void stopImport() {} // abort, default do nothing.

    //! \brief Start Serial "Live" Recording
    virtual bool startLive();
    //! \brief Stop Serial "Live" Recording
    virtual void stopLive();

    //! \brief Put the device in standard transmit mode
    virtual void resetDevice() = 0;

    //! \brief Put the device in record request mode
    virtual void requestData() = 0;

    //! \brief Return the current SerialOxMode, either SO_OFF, SO_IMPORT, SO_LIVE, SO_WAIT
    SerialOxMode mode() { return m_mode; }

    //! \brief Trash the session object
    void destroySession() { delete session; session = NULL; }

    //! \brief Returns true if the serial port is currently open
    bool isOpen() { return m_opened; }

    //! \brief Returns a count of callbacks, so a Timer can see the ports alive or dead.
    int callbacks() { return m_callbacks; }

    //! \brief Returns the time of the last callback in milliseconds since epoch
    qint64 lastTime() { return lasttime; }
    //! \brief Sets the time of the last callback in milliseconds since epoch
    void setLastTime(qint64 t) { lasttime = t; }

    //! \brief Return the current machine object
    Machine *getMachine() { return machine; }

    //! \brief Create a new Session object for the specified date
    Session *createSession(QDateTime date = QDateTime::currentDateTime());

    //! \brief Returns the current session
    Session *getSession() { return session; }

    //! \brief Removes the TimeCodes, converting the EventList to Waveform type
    void compactToWaveform(EventList *el);

    //! \brief Packs EventList to time delta format, also pruning zeros.
    static void compactToEvent(EventList *el);

    //! \brief Packs SPO2 & Pulse to Events, and Plethy to Waveform EventList types.
    void compactAll();

    //! \brief Sets the serial port device name
    void setPortName(QString portname);

    //! \brief Sets the serial ports Baud Rate (eg. BAUD19200, BAUD115200)
    void setBaudRate(BaudRateType baud);

    //! \brief Sets the serial ports Flow control to one of FLOW_OFF, FLOW_HARDWARE, or FLOW_XONXOFF
    void setFlowControl(FlowType flow);

    //! \brief Sets the serial ports Parity to one of PAR_NONE, PAR_ODD, PAR_EVEN, PAR_MARK (WINDOWS ONLY), PAR_SPACE
    void setParity(ParityType parity);

    //! \brief Sets the serial ports Data Bits to either DATA_5, DATA_6, DATA_7, or DATA_8
    void setDataBits(DataBitsType databits);

    //! \brief Sets the serial ports Stop Bits to either STOP_1, STOP_1_5 (WINDOWS ONLY) or STOP_2
    void setStopBits(StopBitsType stopbits);

    //! \brief Returns the serial port device name
    QString portName() { return m_portname; }

    //! \brief Returns the serial ports baud rate
    BaudRateType baudRate() { return m_baud; }
    //! \brief Returns the serial ports flow control setting
    FlowType flowControl() { return m_flow; }

    //! \brief Returns the serial ports parity setting
    ParityType parity() { return m_parity; }

    //! \brief Returns the serial ports data bits setting
    DataBitsType dataBits() { return m_databits; }

    //! \brief Returns the serial ports stop bits setting
    StopBitsType stopBits() { return m_stopbits; }

    bool isImporting() { return import_mode; }

    EventList *Pulse() { return pulse; }
    EventList *Spo2() { return spo2; }
    EventList *Plethy() { return plethy; }
    virtual void addPulse(qint64 time, EventDataType pr);
    virtual void addSpO2(qint64 time, EventDataType o2);
    virtual void addPlethy(qint64 time, EventDataType pleth);
    virtual void killTimers() = 0;

  signals:
    void sessionCreated(Session *);
    void dataChanged();

    //! \brief This signal is called after import completion, to parse the event data.
    void importProcess();

    //! \brief importProcess emits this signal after completion.
    void importComplete(Session *);

    //! \brief emitted when something goes wrong during import
    void importAborted();

    //! \brief emitted to allow for UI updates to the progress bar
    void updateProgress(float f); // between 0 and 1.

    //! \brief emitted when live mode stops recording, passing the current Session
    void liveStopped(Session *);


    void updatePulse(float p);
    void updateSpO2(float p);

  protected slots:
    //! \brief Override this to process the serial import as it's received
    virtual void ReadyRead() = 0;

    //! \brief Override this to parse the read import data
    virtual void import_process() = 0;

    //! \brief This slot gets called when the serial port Times out
    virtual void Timeout();

    //! \brief Override this to start the Import Timeout
    virtual void startImportTimeout() = 0;
    virtual void resetImportTimeout() = 0;

  protected:

    //virtual void addEvents(EventDataType pr, EventDataType o2, EventDataType pleth=-1000000);

    //! \brief Pointer to current session object
    Session *session;

    EventList *pulse;
    EventList *spo2;
    EventList *plethy;

    //! \brief Holds the serial port object
    QextSerialPort *m_port;

    SerialOxMode m_mode;
    bool m_opened;
    QString m_oxiname;
    QString m_portname;
    BaudRateType m_baud;
    FlowType m_flow;
    ParityType m_parity;
    DataBitsType m_databits;
    StopBitsType m_stopbits;
    QextSerialPort::QueryMode m_portmode;
    Machine *machine;

    qint64 lasttime;
    bool import_mode;

    int m_callbacks, cb_start, cb_reset;
    bool done_import;
    bool started_import, started_reading, finished_import;
    QTimer *timer;
    EventDataType lasto2, lastpr;

    QByteArray buffer;
};

/*! \class CMS50Serial
    \author Mark Watkins <jedimark_at_users.sourceforge.net>
    \brief Serial Import & Live module
    */
class CMS50Serial: public SerialOximeter
{
    Q_OBJECT
  public:
    explicit CMS50Serial(QObject *parent, QString portname);
    virtual ~CMS50Serial();

    //! \brief Start the serial parts of Import mode.
    virtual bool startImport();

    //! \brief Stop the serial parts of Import mode.
    virtual void stopImport();

    //! \brief Sends the 0xf6,0xf6,0xf6 data string to the serial port to start live mode again
    virtual void resetDevice();

    //! \brief Sends the 0xf5, 0xf5 data string to request devices serial recording
    virtual void requestData();

    //! \brief Kill any CMS50 specific timers (used internally)
    virtual void killTimers();

  protected:
    //! \brief CMS50 Time-out detection
    virtual void startImportTimeout();
    virtual void resetImportTimeout();


    //! \brief Called on completion of data import, to convert bytearray into event data
    virtual void import_process();

    //! \brief Serial callback to process live view & store import data
    virtual void ReadyRead();
    bool waitf6;
    short cntf6;
    short failcnt;

    QByteArray data;
    QByteArray buffer;

    QDateTime oxitime, cpaptime;

    bool cms50dplus;
    int datasize;

    int received_bytes;
    int import_fails;

    int imp_callbacks;

    QTime imptime;

    EventDataType plmin, plmax;
    EventDataType o2min, o2max;
    int plcnt, o2cnt;
    qint64 lastpltime, lasto2time;
    short lastpl, lasto2;
    bool first;
    QTimer *cms50timer, *cms50timer2;
};

namespace Ui {
class Oximetry;
}

enum PORTMODE { PM_LIVE, PM_RECORDING };
const int max_data_points = 1000000;

/*! \class Oximetry
    \author Mark Watkins <jedimark_at_users.sourceforge.net>
    \brief Oximetry view for working with Pulse Oximetry data and devices
    */
class Oximetry : public QWidget
{
    Q_OBJECT

  public:
    explicit Oximetry(QWidget *parent, gGraphView *shared = NULL);
    ~Oximetry();

    //! \brief Calls updateGL to redraw the graphs
    void RedrawGraphs();

    //! \brief Returns the gGraphView object containing Oximetry graphs
    gGraphView *graphView() { return GraphView; }

    //! \brief Loads and displays a session containing oximetry data into into the Oximetry module
    void openSession(Session *session);

    //! \brief Initiate an automated serial import
    void serialImport();
    QMessageBox *connectDeviceMsgBox;

  private slots:
    //! \brief Scans the list of serial ports and detects any oximetry devices
    void on_RefreshPortsButton_clicked();

    //! \brief Start or Stop live view mode
    void on_RunButton_toggled(bool checked); // Live mode button

    //! \brief This slot gets called when a new serial port is selected from the drop down
    void on_SerialPortsCombo_activated(const QString &arg1);

    //! \brief Start the Serial import process from the devices internal recordings
    void on_ImportButton_clicked();

    //! \brief Asks to save oximetry session into SleepLib database
    void on_saveButton_clicked();

    //! \brief Data has been changed, so it sets all the bits for live graph display
    void data_changed();

    //! \brief Updates the Pulse Rate LCD widget when the live pulse changes
    void pulse_changed(float p);

    //! \brief Updates the SpO2 LCD widget when the live spO2 changes
    void spo2_changed(float o2);

    //! \brief Updates the progress bar during import
    void update_progress(float f);

    //! \brief Import failed, so cleanup.
    void import_aborted();

    //! \brief Import completed, so get ready to display graphs
    void import_complete(Session *session);

    //! \brief Callback to make sure the oximeter is running
    void oximeter_running_check();

    //! \brief Callback after liveView mode is stopped
    void live_stopped(Session *session);

    //! \brief Open button was clicked, so select and load .spo/.spoR data files
    void on_openButton_clicked();

    //! \brief The datetime editor changed, so move the session data accordingly.
    void on_dateEdit_dateTimeChanged(const QDateTime &date);

    //! \brief Reset the datetime to what was set when first loaded
    void on_resetTimeButton_clicked();

    void timeout_CheckPorts();
    void cancel_CheckPorts(QAbstractButton *);

  private:
    //! \brief Imports a .spo file
    bool openSPOFile(QString filename);
    //! \brief Imports a .spoR file (from SPO2Review software in windows)
    bool openSPORFile(QString filename);

    //! \brief Clean up after import process, whether successful or not
    void import_finished();

    //! \brief update the graphs to show the session information
    void updateGraphs();
    Ui::Oximetry *ui;

    bool cancel_Import;
    gGraphView *GraphView;
    MyScrollBar *scrollbar;
    QHBoxLayout *layout;

    gLineChart *pulse, *spo2, *plethy;
    Layer *lo1, *lo2;
    gGraph *PULSE, *SPO2, *PLETHY, *CONTROL;

    //! \brief Contains a list of gLineCharts that display Pulse, Plethy & SPO2 data
    QVector<gLineChart *> Data;

    QextSerialPort *port;
    QString portname;
    PORTMODE portmode;
    double lasttime, starttime;
    int lastpulse, lastspo2;

    Day *day;
    //Session * session;
    //EventList * ev_pulse;
    //EventList * ev_spo2;
    //EventList * ev_plethy;
    Layer *foobar;
    gGraphView *m_shared;

    SerialOximeter *oximeter;
    qint64 saved_starttime;
    bool firstSPO2Update;
    bool firstPulseUpdate;
    bool secondPulseUpdate;
    bool secondSPO2Update;
    bool dont_update_date;
};

#endif // OXIMETRY_H

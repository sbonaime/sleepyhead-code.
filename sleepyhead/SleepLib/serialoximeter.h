/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib MachineLoader Base Class Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef SERIALOXIMETER_H
#define SERIALOXIMETER_H

#include <QTimer>
#include <QtSerialPort/QSerialPort>

#include "SleepLib/machine_loader.h"

const int START_TIMEOUT = 30000;


struct OxiRecord
{
    quint8 pulse;
    quint8 spo2;
    OxiRecord():pulse(0), spo2(0) {}
    OxiRecord(quint8 p, quint8 s): pulse(p), spo2(s) {}
    OxiRecord(const OxiRecord & copy) { pulse = copy.pulse; spo2= copy.spo2; }
};


class SerialOximeter : public MachineLoader
{
Q_OBJECT
public:
    SerialOximeter() : MachineLoader() {
        m_importing = m_streaming = false;
        m_productID = m_vendorID = 0;
    }
    virtual ~SerialOximeter() {}

    virtual bool Detect(const QString &path)=0;
    virtual int Open(QString path)=0;

    static void Register() {}

    virtual int Version()=0;
    virtual const QString &ClassName()=0;


    // Serial Stuff
    virtual bool scanDevice(QString keyword="",quint16 vendor_id=0, quint16 product_id=0);
    virtual bool openDevice();
    virtual void closeDevice();

    inline bool isStreaming() { return m_streaming; }
    inline bool isImporting() { return m_importing; }


    virtual void process() {}

    virtual Machine *CreateMachine()=0;

    // available sessions
    QMap<QDateTime, QVector<OxiRecord> *> oxisessions;

    // current session
    QVector<OxiRecord> * oxirec;

    QDateTime startTime() { return m_startTime; }
    void setStartTime(QDateTime datetime) { m_startTime = datetime; }
    virtual bool isStartTimeValid() { return true; }

    virtual qint64 importResolution() { return 1000; }
    virtual qint64 liveResolution() { return 20; }

    void trashRecords();

signals:
    void noDeviceFound();
    void deviceDetected();
    void updatePlethy(QByteArray plethy);
    void importComplete(SerialOximeter *);

protected slots:
    virtual void dataAvailable();
    virtual void resetImportTimeout() {}
    virtual void startImportTimeout() {}

    virtual void stopRecording();
    virtual void shutdownPorts() {}
//    virtual void abortTask();

protected:
    virtual void processBytes(QByteArray buffer) { Q_UNUSED(buffer) }

    virtual void killTimers() {}
    virtual void resetDevice() {}
    virtual void requestData() {}

    QString port;
    QSerialPort serial;

    QTimer startTimer;
    QTimer resetTimer;

    QDateTime m_startTime;

    quint16 m_productID;
    quint16 m_vendorID;

    bool m_streaming;
    bool m_importing;

};

#endif // SERIALOXIMETER_H

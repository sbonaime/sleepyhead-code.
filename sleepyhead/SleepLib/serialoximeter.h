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

class SerialOximeter : public MachineLoader
{
Q_OBJECT
public:
    SerialOximeter() : MachineLoader() {}
    virtual ~SerialOximeter() {}

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

    virtual Machine *CreateMachine(Profile *profile)=0;


signals:
    void noDeviceFound();
    void deviceDetected();
    void updatePlethy(QByteArray plethy);

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

#endif // SERIALOXIMETER_H

/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib ChoiceMMed MD300W1 Oximeter Loader Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef MD300W1LOADER_H
#define MD300W1LOADER_H

#include "SleepLib/serialoximeter.h"

const QString md300w1_class_name = "MD300W1";
const int md300w1_data_version = 1;


/*! \class MD300W1Loader
    \brief Importer for ChoiceMMed MD300W1 data format..
    */
class MD300W1Loader : public SerialOximeter
{
Q_OBJECT
  public:


    MD300W1Loader();
    virtual ~MD300W1Loader();

    virtual bool Detect(const QString &path);
    virtual int Open(QString path);

    static void Register();

    virtual int Version() { return md300w1_data_version; }
    virtual const QString &ClassName() { return md300w1_class_name; }

    Machine *CreateMachine();

    virtual void process();

    virtual bool isStartTimeValid() { return true; }

protected slots:
    virtual void resetImportTimeout();
    virtual void startImportTimeout();

protected:

    bool readDATFile(QString path);
    virtual void processBytes(QByteArray bytes);

    int doImportMode();
    int doLiveMode();

    virtual void killTimers();

    // Switch MD300W1 device to live streaming mode
    virtual void resetDevice();

    // Switch MD300W1 device to record transmission mode
    void requestData();

  private:
    EventList *PULSE;
    EventList *SPO2;

    QTime m_time;

    QByteArray buffer;

    bool started_import;
    bool finished_import;
    bool started_reading;
    QDateTime oxitime;

    int cb_reset,imp_callbacks;

    int received_bytes;

    int m_itemCnt;
    int m_itemTotal;


};


#endif // MD300W1LOADER_H

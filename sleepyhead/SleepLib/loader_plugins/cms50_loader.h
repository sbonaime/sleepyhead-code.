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

#include "SleepLib/serialoximeter.h"

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


/*! \class CMS50Loader
    \brief Bulk Importer for CMS50 SPO2Review format.. Deprecated, as the Oximetry module does a better job
    */
class CMS50Loader : public SerialOximeter
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


#endif // CMS50LOADER_H

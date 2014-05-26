/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * SleepLib CMS50X Loader Implementation
 *
 * Copyright (c) 2011 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

//********************************************************************************************
/// IMPORTANT!!!
//********************************************************************************************
// Please INCREMENT the cms50_data_version in cms50_loader.h when making changes to this loader
// that change loader behaviour or modify channels.
//********************************************************************************************

#include <QProgressBar>
#include <QApplication>
#include <QDir>
#include <QString>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QList>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

using namespace std;

#include "cms50_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

CMS50Loader::CMS50Loader()
{
    m_type = MT_OXIMETER;
    m_abort = false;
    m_streaming = false;
    m_importing = false;
    imp_callbacks = 0;

    m_vendorID = 0x10c4;
    m_productID = 0xea60;

    startTimer.setParent(this);
    resetTimer.setParent(this);

}

CMS50Loader::~CMS50Loader()
{
}

bool CMS50Loader::Detect(const QString &path)
{
    Q_UNUSED(path);
    return false;
}

int CMS50Loader::Open(QString path, Profile *profile)
{

    // Only one active Oximeter module at a time, set in preferences
    Q_UNUSED(profile)

    m_itemCnt = 0;
    m_itemTotal = 0;

    m_abort = false;
    m_importing = false;

    started_import = false;
    started_reading = false;
    finished_import = false;

    imp_callbacks = 0;
    cb_reset = 0;

    m_time.start();

    // Cheating using path for two serial oximetry modes

    if (path.compare("import") == 0) {
        setStatus(IMPORTING);

        startTimer.stop();
        startImportTimeout();
        return 1;
    } else if (path.compare("live") == 0) {
        oxitime = QDateTime::currentDateTime();
        setStatus(LIVE);
        return 1;
    }
    QString ext = path.section(".",1);
    if ((ext.compare("spo", Qt::CaseInsensitive)==0) || (ext.compare("spor", Qt::CaseInsensitive)==0)) {
        // try to read and process SpoR file..
        return readSpoRFile(path) ? 1 : 0;
    }

    return 0;
}


void CMS50Loader::processBytes(QByteArray bytes)
{
    // Sync to start of message type we are interested in
    quint8 c;
    quint8 msgcode = (m_status == IMPORTING) ? 0xf0 : 0x80;

    int idx=0;
    int bytesread = bytes.size();
    while ((idx < bytesread) && (((c=(quint8)bytes.at(idx)) & msgcode)!=msgcode)) {
        if (buffer.length()>0) {
            // If buffer is the start of a valid but short frame, add to it..
            buffer.append(c);
        }// otherwise dump these bytes, as they are out of sequence.
        ++idx;
    }

    // Copy the rest to the buffer.
    buffer.append(bytes.mid(idx));

    int available = buffer.length();

    switch (status()) {
    case IMPORTING:
        idx = doImportMode();
        break;
    case LIVE:
        idx = doLiveMode();
        break;
    default:
        qDebug() << "Device mode not supported by" << ClassName();
    }

    if (idx >= available) {
        buffer.clear();
    } else if (idx > 0) {
        // Trim any processed bytes from the buffer.
        buffer = buffer.mid(idx);
    }

    if (buffer.length() > 0) {
        // If what's left doesn't start with a marker bit, dump it
        if (((unsigned char)buffer.at(0) & 0x80) != 0x80) {
            buffer.clear();
        }
    }

}

int CMS50Loader::doImportMode()
{
    if (finished_import) {
        // CMS50E/F continue streaming after import, CMS50D+ stops dead
        // there is a timer running at this stage that will kill the 50D
        killTimers();
        closeDevice();
        m_importing = false;
        imp_callbacks = 0;
        return 0;
    }
    int hour,minute;
    int available = buffer.size();
    int idx = 0;
    while (idx < available) {
        unsigned char c=(unsigned char)buffer.at(idx);
        if (!started_import) {
            if (c != 0xf2) { // If not started, continue scanning for a valie header.
                idx++;
                continue;
            } else {
                received_bytes=0;

                hour=(unsigned char)buffer.at(idx + 1) & 0x7f;
                minute=(unsigned char)buffer.at(idx + 2) & 0x7f;

                qDebug() << QString("Receiving Oximeter transmission %1:%2").arg(hour).arg(minute);
                // set importing to true or whatever..

                finished_import = false;
                started_import = true;
                started_reading = false;

                m_importing = true;

                m_itemCnt=0;
                m_itemTotal=5000;

                killTimers();
                qDebug() << "Getting ready for import";

                oxirec.clear();
                oxirec.reserve(10000);

                QDate oda=QDate::currentDate();
                QTime oti=QTime(hour,minute); // Only CMS50E/F's have a realtime clock. CMS50D+ will set this to midnight

                cms50dplus = (hour == 0) && (minute == 0); // Either a CMS50D+ or it's really midnight, set a flag anyway for later to help choose the right sync time

                // If the oximeter record time is more than the current time, then assume it was from the day before
                // Or should I use split time preference instead??? Foggy Confusements..
                if (oti > QTime::currentTime()) {
                    oda = oda.addDays(-1);
                }

                oxitime = QDateTime(oda,oti);

                // Convert it to UTC
                oxitime=oxitime.toTimeSpec(Qt::UTC);

                qDebug() << "Session start (according to CMS50)" << oxitime<< hex << buffer.at(idx + 1) << buffer.at(idx + 2) << ":" << dec << hour << minute ;

                cb_reset = 1;

                // CMS50D+ needs an end timer because it just stops dead after uploading
                resetTimer.singleShot(2000,this,SLOT(resetImportTimeout()));
            }
            idx += 3;
        } else { // have started import
            if (c == 0xf2) { // Header is repeated 3 times, ignore the extras

                hour=(unsigned char)buffer.at(idx + 1) & 0x7f;
                minute=(unsigned char)buffer.at(idx + 2) & 0x7f;
                // check..

                idx += 3;
                continue;
            } else if ((c & 0xf0) == 0xf0) { // Data trio
                started_reading=true; // Sometimes errornous crap is sent after data rec header

                // Recording import
                if ((idx + 2) >= available) {
                    return idx;
                }

                quint8 pulse=(unsigned char)buffer.at(idx + 1) & 0xff;
                quint8 spo2=(unsigned char)buffer.at(idx + 2) & 0xff;

                oxirec.append(OxiRecord(pulse,spo2));
                received_bytes+=3;

                // TODO: Store the data to the session

                m_itemCnt++;
                m_itemCnt=m_itemCnt % m_itemTotal;
                emit updateProgress(m_itemCnt, m_itemTotal);

                idx += 3;
            } else if (!started_reading) { // have not got a valid trio yet, skip...
                idx += 1;
            } else {
                // trio's are over.. finish up.
                killTimers();
                closeDevice();

                started_import = false;
                started_reading = false;
                finished_import = true;
                m_importing = false;
                break;
                //Completed
            }
        }
    }

    if (!started_import) {
        imp_callbacks = 0;
    } else {
        imp_callbacks++;
    }

    return idx;
}

int CMS50Loader::doLiveMode()
{
    int available = buffer.size();
    int idx = 0;

    QByteArray plethy;
    while (idx < available-5) {
        if (((unsigned char)buffer.at(idx) & 0x80) != 0x80) {
            idx++;
            continue;
        }
        int pwave=(unsigned char)buffer.at(idx + 1);
        int pbeat=(unsigned char)buffer.at(idx + 2);
        int pulse=((unsigned char)buffer.at(idx + 3) & 0x7f) | ((pbeat & 0x40) << 1);
        int spo2=(unsigned char)buffer.at(idx + 4) & 0x7f;

        oxirec.append(OxiRecord(pulse, spo2));
        plethy.append(pwave);

        int elapsed = m_time.elapsed();

        // update the graph plots
        if (elapsed > 1000) {
            m_time.start();
            emit updateDisplay(this);
        }
        idx += 5;
    }
    emit updatePlethy(plethy);

    return idx;
}

void CMS50Loader::resetDevice() // Switch CMS50D+ device to live streaming mode
{
    //qDebug() << "Sending reset code to CMS50 device";
    //m_port->flush();
    static unsigned char b1[3]={0xf6,0xf6,0xf6};
    if (serial.write((char *)b1,3)==-1) {
        qDebug() << "Couldn't write data reset bytes to CMS50";
    }
}

void CMS50Loader::requestData() // Switch CMS50D+ device to record transmission mode
{
    static unsigned char b1[2]={0xf5,0xf5};

    //qDebug() << "Sending request code to CMS50 device";
    if (serial.write((char *)b1,2)==-1) {
        qDebug() << "Couldn't write data request bytes to CMS50";
    }
}

void CMS50Loader::killTimers()
{
    startTimer.stop();
    resetTimer.stop();
}

void CMS50Loader::startImportTimeout()
{
    if (!m_streaming)
        return;

    if (started_import) {
        return;
    }
    Q_ASSERT(finished_import == false);

    //qDebug() << "Starting oximeter import timeout";

    // Wait until events really are jammed on the CMS50D+ before re-requesting data.

    const int delay = 500;

    if (m_abort) {
        m_streaming = false;
        closeDevice();
        return;
    }

    if (imp_callbacks == 0) { // Frozen, but still hasn't started?
        m_itemCnt = m_time.elapsed();
        if (m_itemCnt > START_TIMEOUT) { // Give up after START_TIMEOUT
            closeDevice();
            abort();
            QMessageBox::warning(nullptr, STR_MessageBox_Error, "<h2>"+tr("Could not get data transmission from oximeter.")+"<br/><br/>"+tr("Please ensure you select 'upload' from the oximeter devices menu.")+"</h2>");
            return;
        } else {
            // Note: Newer CMS50 devices transmit from user input, but there is no way of differentiating between models
            requestData();
        }
        emit updateProgress(m_itemCnt, START_TIMEOUT);

        // Schedule another callback to make sure it's started
        startTimer.singleShot(delay, this, SLOT(startImportTimeout()));
    }
}

void CMS50Loader::resetImportTimeout()
{
    if (finished_import) {
        return;
    }

    if (imp_callbacks != cb_reset) {
        // Still receiving data.. reset timer
        qDebug() << "Still receiving data in resetImportTimeout()";
        if (resetTimer.isActive())
            resetTimer.stop();

        if (!finished_import) resetTimer.singleShot(2000, this, SLOT(resetImportTimeout()));
    } else {
        qDebug() << "Oximeter device stopped transmitting.. Transfer complete";
        // We were importing, but now are done
        if (!finished_import && (started_import && started_reading)) {
            qDebug() << "Switching CMS50 back to live mode and finalizing import";
            // Turn back on live streaming so the end of capture can be dealt with
            resetTimer.stop();

            serial.flush();
            resetDevice(); // Send Reset to CMS50D+
            //started_import = false;
            finished_import = true;
            m_streaming=false;

            closeDevice();
            //emit transferComplete();
            //doImportComplete();
            return;
        }
        qDebug() << "Should CMS50 resetImportTimeout reach here?";
        // else what???
    }
    cb_reset=imp_callbacks;
}




bool CMS50Loader::readSpoRFile(QString path)
{
    QFile file(path);
    if (!file.exists() || !file.isReadable()) {
        return false;
    }

    file.open(QFile::ReadOnly);

    QByteArray data;

    data = file.readAll();
    long size = data.size();

    // position data stream starts at
    int pos = ((unsigned char)data.at(1) << 8) | (unsigned char)data.at(0);

    // Read date and time (it's a 16bit charset)
    char dchr[20];
    int j = 0;
    for (int i = 0; i < 18 * 2; i += 2) {
        dchr[j++] = data.at(8 + i);
    }

    dchr[j] = 0;
    QString dstr(dchr);
    oxitime = QDateTime::fromString(dstr, "MM/dd/yy HH:mm:ss");

    if (oxitime.date().year() < 2000) { oxitime = oxitime.addYears(100); }

    unsigned char o2, pr;

    // Read all Pulse and SPO2 data
    for (int i = pos; i < size - 2;) {
        o2 = (unsigned char)(data.at(i + 1));
        pr = (unsigned char)(data.at(i + 0));
        oxirec.append(OxiRecord(pr, o2));
        i += 2;
    }

    // processing gets done later
    return true;
}

Machine *CMS50Loader::CreateMachine(Profile *profile)
{
    if (!profile) {
        return nullptr;
    }

    // NOTE: This only allows for one CMS50 machine per profile..
    // Upgrading their oximeter will use this same record..

    QList<Machine *> ml = profile->GetMachines(MT_OXIMETER);

    for (QList<Machine *>::iterator i = ml.begin(); i != ml.end(); i++) {
        if ((*i)->GetClass() == cms50_class_name)  {
            return (*i);
            break;
        }
    }

    qDebug() << "Create CMS50 Machine Record";

    Machine *m = new Oximeter(profile, 0);
    m->SetClass(cms50_class_name);
    m->properties[STR_PROP_Brand] = "Contec";
    m->properties[STR_PROP_Model] = "CMS50X";
    m->properties[STR_PROP_DataVersion] = QString::number(cms50_data_version);

    profile->AddMachine(m);
    QString path = "{" + STR_GEN_DataFolder + "}/" + m->GetClass() + "_" + m->hexid() + "/";
    m->properties[STR_PROP_Path] = path;

    return m;
}

void CMS50Loader::process()
{
    int size=oxirec.size();
    if (size<10)
        return;


//    EventList *PULSE=new EventList(EVL_Event);
//    EventList *SPO2=new EventList(EVL_Event);


//    quint64 ti = oxitime.toMSecsSinceEpoch();

//    for (int i=0; i < size; ++i) {
//        //PULSE->AddWaveform
//    }
//    qDebug() << "Processing" << oxirec.size() << "oximetry records";
}



static bool cms50_initialized = false;

void CMS50Loader::Register()
{
    if (cms50_initialized) { return; }

    qDebug() << "Registering CMS50Loader";
    RegisterLoader(new CMS50Loader());
    cms50_initialized = true;
}


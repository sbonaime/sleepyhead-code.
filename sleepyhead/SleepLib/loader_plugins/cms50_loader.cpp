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

#include <QtSerialPort/QSerialPortInfo>

using namespace std;

#include "cms50_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

const int START_TIMEOUT = 30000;

// Possibly need to replan this to include oximetry

bool SerialLoader::scanDevice(QString keyword,quint16 vendor_id, quint16 product_id)
{
    QStringList ports;

    //qDebug() << "Scanning for USB Serial devices";
    QList<QSerialPortInfo> list=QSerialPortInfo::availablePorts();

    // How does the mac detect this as a SPO2 device?
    for (int i=0;i<list.size();i++) {
        const QSerialPortInfo * info=&list.at(i);
        QString name=info->portName();
        QString desc=info->description();

        if ((!keyword.isEmpty() && desc.contains(keyword)) ||
            ((info->hasVendorIdentifier() && (info->vendorIdentifier()==vendor_id))
                && (info->hasProductIdentifier() && (info->productIdentifier()==product_id))))
        {
            ports.push_back(name);
            QString dbg=QString("Found Serial Port: %1 %2 %3 %4").arg(name).arg(desc).arg(info->manufacturer()).arg(info->systemLocation());

            if (info->hasProductIdentifier()) //60000
                dbg+=QString(" PID: %1").arg(info->productIdentifier());
            if (info->hasVendorIdentifier()) // 4292
                dbg+=QString(" VID: %1").arg(info->vendorIdentifier());

            qDebug() << dbg.toLocal8Bit().data();
            break;
        }
    }
    if (ports.isEmpty()) {
        return false;
    }
    if (ports.size()>1) {
        qDebug() << "More than one serial device matching these parameters was found, choosing the first by default";
    }
    port=ports.at(0);
    return true;
}

void SerialLoader::closeDevice()
{
    killTimers();
    disconnect(&serial,SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    serial.close();
    m_streaming = false;
    qDebug() << "Port" << port << "closed";
}

bool SerialLoader::openDevice()
{
    if (port.isEmpty()) {
        if (!scanDevice("",m_vendorID, m_productID))
            return false;
    }
    serial.setPortName(port);
    if (!serial.open(QSerialPort::ReadWrite))
        return false;

    // forward this stuff

    // Set up serial port attributes
    serial.setBaudRate(QSerialPort::Baud19200);
    serial.setParity(QSerialPort::OddParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setDataBits(QSerialPort::Data8);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    m_streaming = true;
    m_abort = false;
    m_importing = false;

    // connect relevant signals
    connect(&serial,SIGNAL(readyRead()), this, SLOT(dataAvailable()));

    return true;
}

void SerialLoader::dataAvailable()
{
    QByteArray bytes;

    int available = serial.bytesAvailable();
    bytes.resize(available);

    int bytesread = serial.read(bytes.data(), available);
    if (bytesread == 0)
        return;

    if (m_abort) {
        closeDevice();
        return;
    }

    processBytes(bytes);
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
    while (idx < available) {
        if (((unsigned char)buffer.at(idx) & 0x80) != 0x80) {
            idx++;
            continue;
        }
        int pwave=(unsigned char)buffer.at(idx + 1);
        int pbeat=(unsigned char)buffer.at(idx + 2);
        int pulse=((unsigned char)buffer.at(idx + 3) & 0x7f) | ((pbeat & 0x40) << 1);
        int spo2=(unsigned char)buffer.at(idx + 4) & 0x7f;
        idx += 5;
    }

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
    Q_ASSERT(m_streaming == true);

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

    // Cheating using path for two serial oximetry modes

    if (path.compare("import") == 0) {
        setStatus(IMPORTING);

        m_time.start();

        startTimer.stop();
        startImportTimeout();
        return 1;
    } else if (path.compare("live") == 0) {
        setStatus(LIVE);
        return 1;
    }
    // try to read and process SpoR file..
    return readSpoRFile(path);
}

bool CMS50Loader::readSpoRFile(QString path)
{
    QFile file(path);
    if (!file.exists() || !file.isReadable()) {
        return false;
    }

    QString ext = path.section(".",1);
    if ((ext.compare("spo",Qt::CaseInsensitive)==0)
       && (ext.compare("spor",Qt::CaseInsensitive)==0)) {
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

Qt::DayOfWeek firstDayOfWeekFromLocale();
#include <QAction>

DateTimeDialog::DateTimeDialog(QString message, QWidget * parent, Qt::WindowFlags flags)
:QDialog(parent, flags)
{
    layout = new QVBoxLayout(this);
    setModal(true);
    font.setPointSize(25);
    m_msglabel = new QLabel(message);

    m_msglabel->setAlignment(Qt::AlignHCenter);
    m_msglabel->setFont(font);
    layout->addWidget(m_msglabel);

    m_cal = new QCalendarWidget(this);
    m_cal->setFirstDayOfWeek(Qt::Sunday);
    QTextCharFormat format = m_cal->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    m_cal->setWeekdayTextFormat(Qt::Saturday, format);
    m_cal->setWeekdayTextFormat(Qt::Sunday, format);
    m_cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();
    m_cal->setFirstDayOfWeek(dow);

    m_timeedit = new MyTimeEdit(this);
    m_timeedit->setDisplayFormat("hh:mm:ss ap");
    m_timeedit->setMinimumHeight(m_timeedit->height() + 10);
    m_timeedit->setFont(font);
    m_timeedit->setCurrentSectionIndex(0);
    m_timeedit->editor()->setStyleSheet(
    "selection-color: white;"
    "selection-background-color: lightgray;"
    );

    layout->addWidget(m_cal);
    m_bottomlabel = new QLabel(this);
    m_bottomlabel->setVisible(false);
    m_bottomlabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_bottomlabel);

    sessbar = new SessionBar(this);
    sessbar->setMinimumHeight(40);
    sessbar->setSelectMode(true);
    sessbar->setMouseTracking(true);
    connect(sessbar, SIGNAL(sessionClicked(Session*)), this, SLOT(onSessionSelected(Session*)));
    layout->addWidget(sessbar,1);

    layout2 = new QHBoxLayout();
    layout->addLayout(layout2);

    acceptbutton = new QPushButton (QObject::tr("&Accept"));
    resetbutton = new QPushButton (QObject::tr("&Abort"));

    resetbutton->setShortcut(QKeySequence(Qt::Key_Escape));
//    shortcutQuit = new QShortcut(, this, SLOT(reject()), SLOT(reject()));

    layout2->addWidget(m_timeedit);
    layout2->addWidget(acceptbutton);
    layout2->addWidget(resetbutton);

    connect(resetbutton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbutton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_cal, SIGNAL(clicked(QDate)), this, SLOT(onDateSelected(QDate)));
    connect(m_cal, SIGNAL(activated(QDate)), this, SLOT(onDateSelected(QDate)));
    connect(m_cal, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
    connect(m_cal, SIGNAL(currentPageChanged(int,int)), this, SLOT(onCurrentPageChanged(int,int)));

}
DateTimeDialog::~DateTimeDialog()
{
    disconnect(m_cal, SIGNAL(currentPageChanged(int,int)), this, SLOT(onCurrentPageChanged(int,int)));
    disconnect(m_cal, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
    disconnect(m_cal, SIGNAL(activated(QDate)), this, SLOT(onDateSelected(QDate)));
    disconnect(m_cal, SIGNAL(clicked(QDate)), this, SLOT(onDateSelected(QDate)));
    disconnect(acceptbutton, SIGNAL(clicked()), this, SLOT(accept()));
    disconnect(resetbutton, SIGNAL(clicked()), this, SLOT(reject()));
    disconnect(sessbar, SIGNAL(sessionClicked(Session*)), this, SLOT(onSessionSelected(Session*)));
}

void DateTimeDialog::onCurrentPageChanged(int year, int month)
{
    Q_UNUSED(year);
    Q_UNUSED(month);
    onDateSelected(m_cal->selectedDate());
}
void DateTimeDialog::onSelectionChanged()
{
    onDateSelected(m_cal->selectedDate());
}

void DateTimeDialog::onDateSelected(QDate date)
{
    Day * day = PROFILE.GetGoodDay(date, MT_CPAP);

    sessbar->clear();
    if (day) {
        QDateTime time=QDateTime::fromMSecsSinceEpoch(day->first());
        sessbar->clear();
        QList<QColor> colors;
        colors.push_back("#ffffe0");
        colors.push_back("#ffe0ff");
        colors.push_back("#e0ffff");
        QList<Session *>::iterator i;
        int j=0;
        for (i=day->begin(); i != day->end(); ++i) {
            sessbar->add((*i),colors.at(j++ % colors.size()));
        }
        //sessbar->setVisible(true);
        setBottomMessage(QString("%1 session(s), starting at %2").arg(day->size()).arg(time.time().toString("hh:mm:ssap")));
    } else {
        setBottomMessage("No CPAP Data available for this date");
       // sessbar->setVisible(false);
    }

    sessbar->update();
}

void DateTimeDialog::onSessionSelected(Session * session)
{
    QDateTime time=QDateTime::fromMSecsSinceEpoch(session->first());
    m_timeedit->setTime(time.time());
}

QDateTime DateTimeDialog::execute(QDateTime datetime)
{
    m_timeedit->setTime(datetime.time());
    m_cal->setCurrentPage(datetime.date().year(), datetime.date().month());
    m_cal->setSelectedDate(datetime.date());
    m_cal->setMinimumDate(PROFILE.FirstDay());
    m_cal->setMaximumDate(QDate::currentDate());
    onDateSelected(datetime.date());
    if (QDialog::exec() == QDialog::Accepted) {
        return QDateTime(m_cal->selectedDate(), m_timeedit->time());
    } else {
        return datetime;
    }
}
void DateTimeDialog::reject()
{
    if (QMessageBox::question(this,STR_MessageBox_Warning,
        QObject::tr("Closing this dialog will leave this time unedited.")+"<br/><br/>"+
        QObject::tr("Are you sure you want to do this?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        QDialog::reject();
    }
}


void CMS50Loader::process()
{
    int size=oxirec.size();
    if (size<10)
        return;

        return;

    QDateTime cpaptime;
    Day *day = PROFILE.GetDay(PROFILE.LastDay(), MT_CPAP);

    if (day) {
        int ti = day->first() / 1000L;

        cpaptime = QDateTime::fromTime_t(ti);

        if (cms50dplus) {
            if (QMessageBox::question(nullptr, STR_MessageBox_Question,
            "<h2>"+tr("Oximeter import completed")+"</h2><br/><br/>"+
                tr("Oximeter import completed successfully, but your device did not record a starting time.")+"<br/><br/>"+
                tr("If you remembered to start your oximeter at exactly the same time as your CPAP session, you can sync to the first CPAP session of the day last recorded.")+
                tr("Otherwise you can adjust the time yourself."), tr("Use CPAP"), tr("Set Manually"), "", 0, 1)==0) {
                oxitime=cpaptime;
            } else {
                DateTimeDialog dlg(tr("Oximeter starting time"));
                oxitime = dlg.execute(oxitime);
            }
        } else {
            if (qAbs(oxitime.secsTo(cpaptime)) > 60) {
                if (QMessageBox::question(nullptr, STR_MessageBox_Question,
                    "<h2>"+tr("Oximeter import completed")+"</h2><br/>"+
                    tr("Which devices starting time do you wish to use for this oximetry session?")+"<br/><br/>"+
                    tr("If you started both devices at exactly the same time, or don't know if the clock is set correctly on either devices, choose CPAP."),STR_TR_CPAP, STR_TR_Oximeter, "", 0, -1)==0) {
                        oxitime=cpaptime;
                        qDebug() << "Chose CPAP starting time";
                } else {
                    DateTimeDialog dlg(tr("Oximeter starting time"));
                    oxitime = dlg.execute(oxitime);
                    qDebug() << "Chose Oximeter starting time";
                }
            } else {
                // don't bother asking.. use CPAP
                oxitime=cpaptime;
            }
        }
    } else {
        if (cms50dplus) {
            QMessageBox::information(nullptr, STR_MessageBox_Information,
                    "<h2>"+tr("Oximeter import completed")+"</h2><br/>"+
                    tr("Your oximeter does not record a starting time, and no CPAP session is available to match it to")+"<br/><br/>"+
                    tr("You will have to adjust the starting time of this oximetery session manually."),
                    QMessageBox::Ok, QMessageBox::Ok);
            DateTimeDialog dlg(tr("Oximeter starting time"));
            oxitime = dlg.execute(oxitime);
        }
    }

    EventList *PULSE=new EventList(EVL_Event);
    EventList *SPO2=new EventList(EVL_Event);


    quint64 ti = oxitime.toMSecsSinceEpoch();

    for (int i=0; i < size; ++i) {
        //PULSE->AddWaveform
    }
    qDebug() << "Processing" << oxirec.size() << "oximetry records";
}



static bool cms50_initialized = false;

void CMS50Loader::Register()
{
    if (cms50_initialized) { return; }

    qDebug() << "Registering CMS50Loader";
    RegisterLoader(new CMS50Loader());
    cms50_initialized = true;
}


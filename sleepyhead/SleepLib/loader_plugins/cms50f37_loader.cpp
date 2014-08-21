/* SleepLib CMS50X Loader Implementation
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
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QList>
#include <QMessageBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

using namespace std;

#include "cms50f37_loader.h"
#include "SleepLib/machine.h"
#include "SleepLib/session.h"

extern QProgressBar *qprogress;

CMS50F37Loader::CMS50F37Loader()
{
    m_type = MT_OXIMETER;
    m_abort = false;
    m_streaming = false;
    m_importing = false;
    started_reading = false;

    imp_callbacks = 0;

    m_vendorID = 0x10c4;
    m_productID = 0xea60;

    oxirec = nullptr;

    startTimer.setParent(this);
    resetTimer.setParent(this);

    duration_divisor = 2;

    model = QString();
    vendor = QString();

}

CMS50F37Loader::~CMS50F37Loader()
{
}

bool CMS50F37Loader::openDevice()
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
    serial.setBaudRate(QSerialPort::Baud115200);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setDataBits(QSerialPort::Data8);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    m_streaming = true;
    m_abort = false;
    m_importing = false;

    // connect relevant signals
    connect(&serial,SIGNAL(readyRead()), this, SLOT(dataAvailable()));

    started_import = false;
    started_reading = false;
    finished_import = false;

    resetDevice();
    return true;
}

bool CMS50F37Loader::Detect(const QString &path)
{
    if (p_profile->oxi->oximeterType() == QString("Contec CMS50F v3.7+")) {
        return true;
    }
    Q_UNUSED(path);
    return false;
}

int CMS50F37Loader::Open(QString path)
{
    // Only one active Oximeter module at a time, set in preferences

    m_itemCnt = 0;
    m_itemTotal = 0;

    m_abort = false;
    m_importing = false;

    started_import = false;
    started_reading = false;
    finished_import = false;
    setStatus(NEUTRAL);

    imp_callbacks = 0;
    cb_reset = 0;

    m_time.start();

    if (oxirec) {
        trashRecords();
    }

    // Cheating using path for two serial oximetry modes

    if (path.compare("import") == 0) {
        serial.clear();

        sequence = 0;
        buffer.clear();

//        nextCommand();
        setStatus(IMPORTING);

        return 1;
    } else if (path.compare("live") == 0) {
        return 0;
    }
    QString ext = path.section(".",1);
    if ((ext.compare("spo2", Qt::CaseInsensitive)==0) || (ext.compare("spo", Qt::CaseInsensitive)==0) || (ext.compare("spor", Qt::CaseInsensitive)==0)) {
        // try to read and process SpoR file..
        return readSpoRFile(path) ? 1 : 0;
    }

    return 0;
}

unsigned char cms50_sequence[] = { 0xa7, 0xa2, 0xa0, 0xb0, 0xac, 0xb3, 0xad, 0xa3, 0xab, 0xa4, 0xa5, 0xaf, 0xa7, 0xa2, 0xa6 };

const int TIMEOUT = 2000;


const quint8 COMMAND_CMS50_HELLO1 = 0x27;          // 0xa7
const quint8 COMMAND_CMS50_HELLO2 = 0x22;          // 0xa2
const quint8 COMMAND_GET_SESSION_COUNT = 0x23;     // 0xA3
const quint8 COMMAND_GET_SESSION_TIME = 0x25;      // 0xA5
const quint8 COMMAND_GET_SESSION_DURATION = 0x24;  // 0xA4
const quint8 COMMAND_GET_USER_INFO = 0x2B;         // 0xAB
const quint8 COMMAND_GET_SESSION_DATA = 0x26;      // 0xA6
const quint8 COMMAND_GET_OXIMETER_INFO = 0x30;     // 0xb0
const quint8 COMMAND_GET_OXIMETER_MODEL = 0x28;    // 0xA8
const quint8 COMMAND_GET_OXIMETER_VENDOR = 0x29;    // 0xA9


int cms50_seqlength = sizeof(cms50_sequence);


QString CMS50F37Loader::getUser()
{
    if (!user.isEmpty()) return user;

    sendCommand(COMMAND_GET_USER_INFO);

    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while (user.isEmpty() && (time.elapsed() < TIMEOUT));

    return user;
}

QString CMS50F37Loader::getVendor()
{
    if (!vendor.isEmpty()) return vendor;

    sendCommand(COMMAND_GET_OXIMETER_VENDOR);

    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while (vendor.isEmpty() && (time.elapsed() < TIMEOUT));
    return vendor;
}

QString CMS50F37Loader::getModel()
{
    if (!model.isEmpty()) return model;

    sendCommand(COMMAND_GET_OXIMETER_MODEL);

    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while (model.isEmpty() && (time.elapsed() < TIMEOUT));

    if (model.startsWith("CMS50I")) {
        duration_divisor = 4;
    } else {
        duration_divisor = 2;
    }
    return model;
}

QString CMS50F37Loader::getDeviceString()
{
    return QString("%1 %2").arg(getVendor()).arg(getModel());
}

int CMS50F37Loader::getSessionCount()
{
    session_count = -1;
    sendCommand(COMMAND_GET_SESSION_COUNT);

    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while ((session_count < 0) && (time.elapsed() < TIMEOUT));

    return session_count;
}

int CMS50F37Loader::getOximeterInfo()
{
    device_info = -1;
    sendCommand(COMMAND_GET_OXIMETER_INFO);
    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while ((device_info < 0) && (time.elapsed() < TIMEOUT));

    return device_info;
}

int CMS50F37Loader::getDuration(int session)
{
    getOximeterInfo();

    duration = -1;
    sendCommand(COMMAND_GET_SESSION_DURATION, session);

    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while ((duration < 0) && (time.elapsed() < TIMEOUT));

    return duration / duration_divisor;
}


QDateTime CMS50F37Loader::getDateTime(int session)
{
    imp_date = QDate();
    imp_time = QTime();
    sendCommand(COMMAND_GET_SESSION_TIME, session);
    QTime time;
    time.start();
    do {
        QApplication::processEvents();
    } while ((imp_date.isNull() || imp_time.isNull()) && (time.elapsed() < TIMEOUT));

    if (imp_date.isNull() || imp_time.isNull())
        return QDateTime();

    return QDateTime(imp_date, imp_time);
}



void CMS50F37Loader::processBytes(QByteArray bytes)
{
    static quint8 resimport = 0;
    int data;

    QString tmpstr;

    int lengths[32] = { 0, 0, 9, 9, 0, 9, 4, 8, 8, 6, 4, 0, 2, 0, 3, 8, 3, 9, 8, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    buffer.append(bytes);
    int size = buffer.size();

    int idx = 0;
    int len;

    int year, month, day;

    quint8 pulse;

    do {
        quint8 res = buffer.at(idx);

        len = lengths[res & 0x1f];

        if ((idx+len) > size)
            break;

        if (started_reading && (res != resimport)) {
            len = 0;
        }

        if (len == 0) {
            // lost sync
            if (started_reading) {
                do {
                    res = buffer.at(idx++);
                } while (res != resimport || idx < size);
                // add a dummy to make up for it.
                qDebug() << "lost sync, padding...";
                oxirec->append(OxiRecord(0,0,0));
            }
            continue;
        }
        for (int i = 1; i < len; i++) {
            buffer[idx+i] = buffer[idx+i] ^ 0x80;
        }

        if (!started_reading) switch(res) {
        case 0x02:
            data = buffer.at(idx+1);
            if (data == 0) {
                model = QString(buffer.mid(idx+3, 6));
                qDebug() << "Model:" << model;
            }
            break;
        case 0x03:
            data = buffer.at(idx+1);
            if (data == 0) {
                vendor = QString(buffer.mid(idx+2, 6));
                qDebug() << "Vendor:" << vendor;
            }
            break;

            // COMMAND_GET_USER_INFO
        case 0x05: // 5,80,80,f5,f3,e5,f2,80,80
            // User
            user = QString(buffer.mid(idx+3).trimmed());
            qDebug() << "0x05:" << user;

            break;
        case 0x6: // 6,80,80,87
            data = buffer.at(idx+3) ^ 0x80;
            break;

            // COMMAND_GET_SESSION_TIME
        case 0x07: // 7,80,80,80,94,8e,88,92

            for (int i = 2, msb = buffer.at(idx+1); i < len; i++, msb>>= 1) {
                buffer[idx+i] = (buffer[idx+i] & 0x7f) | (msb & 0x01 ? 0x80 : 0);
            }

            year = QString().sprintf("%02i%02i",buffer.at(idx+4), buffer.at(idx+5)).toInt();
            month = QString().sprintf("%02i", buffer.at(idx+6)).toInt();
            day = QString().sprintf("%02i", buffer.at(idx+7)).toInt();

            imp_date = QDate(year,month,day);
            qDebug() << imp_date;
            break;

            // COMMAND_GET_SESSION_DURATION
        case 0x08: // 8,80,80,80,a4,81,80,80  // 00, 00, 24, 01, 00, 00

            duration = ((buffer.at(idx+1) & 0x4) << 5);
            duration |= buffer.at(idx+4);
            duration |= (buffer.at(idx+5) | ((buffer.at(idx+1) & 0x8) << 4)) << 8;
            duration |= (buffer.at(idx+6) | ((buffer.at(idx+1) & 0x10) << 3)) << 16;
            break;

            // COMMAND_GET_SESSION_COUNT
        case 0x0a: // a,80,80,81
            session_count = buffer.at(idx+3);
            break;

            // COMMAND_CMS50_HELLO1 && COMMAND_CMS50_HELLO2
        case 0xc: // a7 & a2  // responds with: c,80
            data = buffer.at(idx+1);
            break;
        case 0x0e: // e,80,81
            break;
        case 0x10: // 10,80,81
            data = buffer.at(idx+2);
            break;

            // COMMAND_GET_OXIMETER_INFO
        case 0x11: // 11,80,81,81,80,80,80,80,80
            device_info = buffer.at(idx+3);
            break;

            // COMMAND_GET_SESSION_TIME
        case 0x12: // 12,80,80,80,82,a6,92,80
            tmpstr = QString().sprintf("%02i:%02i:%02i",buffer.at(idx+4), buffer.at(idx+5), buffer.at(idx+6));
            imp_time = QTime::fromString(tmpstr, "HH:mm:ss");
            qDebug() << imp_time;

            break;
        case 0x13: // 13,80,a0,a0,a0,a0,a0,a0,a0
            break;
        case 0x14: //14,80,80,80,80,80,80,80,80
            break;

        case 0x09: // cms50i data sequence
        case 0x0f: // f,80,de,c2,de,c2,de,c2  cms50F data...
            if (!started_import) {
                started_import = true;
                started_reading = true;
                finished_import = false;

                m_importing = true;

                m_itemCnt=0;
                m_itemTotal=duration;

                have_perfindex = (res == 0x9);

                oxirec = new QVector<OxiRecord>;
                oxirec->reserve(30000);

                oxisessions[m_startTime] = oxirec;

                cb_reset = 1;

                resetTimer.singleShot(2000,this,SLOT(resetImportTimeout()));
                resimport = res;
            }

            break;
        default:
            qDebug() << "unknown cms50 result?" << hex << (int)res;
            break;
        }

        if (res == 0x09) {
            // 9,80,e1,c4,ce,82  // cms50i data

            quint8 * buf = &((quint8 *)buffer.data())[idx];

            quint8 msb = buf[1];
            for (int i = 2; i < len-1; i++, msb >>= 1) {
                buf[i] = (buf[i] & 0x7f) | (msb & 0x01) ? 0x80 : 0;
            }

            quint16 pi = buf[4] | buf[5] << 8;
            pulse = buf[3];
            quint8 spo2 = buf[2] & 0x7f;
            qDebug() << "Pulse=" << pulse << "SPO2=" << spo2 << "Pi=" << pi;

            oxirec->append(((spo2 == 0) || (pulse == 0)) ? OxiRecord(0,0,0) : OxiRecord(pulse, spo2, pi));
        } else if (res == 0x0f) {
            // f,80,de,c2,de,c2,de,c2  cms50F data...

            for (int i = 2, msb = buffer.at(idx+1); i < len; i++, msb>>= 1) {
                buffer[idx+i] = (buffer[idx+i] & 0x7f) | (msb & 0x01 ? 0x80 : 0);
            }

            pulse = buffer.at(idx+3);
            oxirec->append((pulse == 0xff) ? OxiRecord(0,0) : OxiRecord(pulse, buffer.at(idx+2)));

            pulse = buffer.at(idx+5);
            oxirec->append((pulse == 0xff) ? OxiRecord(0,0) : OxiRecord(pulse, buffer.at(idx+4)));

            pulse = buffer.at(idx+7);
            oxirec->append((pulse == 0xff) ? OxiRecord(0,0) : OxiRecord(pulse, buffer.at(idx+6)));
        }

        QStringList str;
        for (int i=0; i < len; ++i) {
            str.append(QString::number((unsigned char)buffer.at(idx + i),16));
        }

        if (!started_import) {
          //  startTimer.singleShot(2000, this, SLOT(requestData()));
            qDebug() << "Read:" << len << size << str.join(",");
        } else {
            qDebug() << "Import:" << len << size << str.join(",");
        }

        idx += len;
    } while (idx < size);


    if (!started_import) {
        imp_callbacks = 0;
    } else {
        emit updateProgress(oxirec->size(), duration);

        imp_callbacks++;
    }

    buffer = buffer.mid(idx);
}


//int CMS50F37Loader::doLiveMode()
//{
//    Q_ASSERT(oxirec != nullptr);

//    int available = buffer.size();
//    int idx = 0;

//    QByteArray plethy;
//    while (idx < available-5) {
//        if (((unsigned char)buffer.at(idx) & 0x80) != 0x80) {
//            idx++;
//            continue;
//        }
//        int pwave=(unsigned char)buffer.at(idx + 1);
//        int pbeat=(unsigned char)buffer.at(idx + 2);
//        int pulse=((unsigned char)buffer.at(idx + 3) & 0x7f) | ((pbeat & 0x40) << 1);
//        int spo2=(unsigned char)buffer.at(idx + 4) & 0x7f;

//        oxirec->append(OxiRecord(pulse, spo2));
//        plethy.append(pwave);

//        idx += 5;
//    }
//    emit updatePlethy(plethy);

//    return idx;
//}


void CMS50F37Loader::sendCommand(quint8 c)
{
    quint8 cmd[] = { 0x7d, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    cmd[2] |= (c & 0x7f);

    QString out;
    for (int i=0;i < 9;i++) out += QString().sprintf("%02X ",cmd[i]);
    qDebug() << "Write:" << out;

    if (serial.write((char *)cmd, 9) == -1) {
        qDebug() << "Couldn't write data reset bytes to CMS50";
    }
}

void CMS50F37Loader::sendCommand(quint8 c, quint8 c2)
{
    quint8 cmd[] = { 0x7d, 0x81, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    cmd[2] |= (c & 0x7f);
    cmd[4] |= (c2 & 0x7f);

    QString out;
    for (int i=0; i < 9; ++i) out += QString().sprintf("%02X ",cmd[i]);
    qDebug() << "Write:" << out;

    if (serial.write((char *)cmd, 9) == -1) {
        qDebug() << "Couldn't write data reset bytes to CMS50";
    }
}

void CMS50F37Loader::nextCommand()
{
    if (++sequence < cms50_seqlength) {
        // Send the next command packet in sequence
        sendCommand(cms50_sequence[sequence]);
    } else {
        qDebug() << "Run out of startup tasks to do and import failed!";
    }
}

void CMS50F37Loader::getSessionData(int session)
{
    resetDevice();
    selected_session = session;
    requestData();
}

void CMS50F37Loader::resetDevice()
{
    sendCommand(COMMAND_CMS50_HELLO1);
    QThread::msleep(100);
    QApplication::processEvents();
    sendCommand(COMMAND_CMS50_HELLO2);
    QThread::msleep(100);
    QApplication::processEvents();
}

void CMS50F37Loader::requestData()
{
    sendCommand(COMMAND_GET_SESSION_DATA, selected_session);
}

void CMS50F37Loader::killTimers()
{
    if (resetTimer.isActive()) resetTimer.stop();
    if (startTimer.isActive()) startTimer.stop();
}

void CMS50F37Loader::startImportTimeout()
{
}

void CMS50F37Loader::resetImportTimeout()
{
    if (finished_import) {
        return;
    }

    if (imp_callbacks != cb_reset) {
        // Still receiving data.. reset timer
        qDebug() << "Still receiving data in resetImportTimeout()" << imp_callbacks << cb_reset;
        if (resetTimer.isActive())
            resetTimer.stop();

        if (!finished_import) resetTimer.singleShot(2000, this, SLOT(resetImportTimeout()));
    } else {
        qDebug() << "Oximeter device stopped transmitting.. Transfer complete";
        // We were importing, but now are done
        if (!finished_import && (started_import && started_reading)) {
            qDebug() << "Switching CMS50 back to live mode and finalizing import";
            // Turn back on live streaming so the end of capture can be dealt with


            killTimers();

            finished_import = true;
            m_streaming = false;
            m_importing = false;
            started_reading = false;
            started_import = false;

            emit importComplete(this);

            m_status = NEUTRAL;

            shutdownPorts();

            return;
        }
        qDebug() << "Should CMS50 resetImportTimeout reach here?";
        // else what???
    }
    cb_reset = imp_callbacks;
}

void CMS50F37Loader::shutdownPorts()
{
    closeDevice();
}




bool CMS50F37Loader::readSpoRFile(QString path)
{
    QFile file(path);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QFile::ReadOnly)) {
        return false;
    }

    bool spo2header = false;
    QString ext = path.section('.', -1);
    if (ext.compare("spo2",Qt::CaseInsensitive) == 0) {
        spo2header = true;
    }

    QByteArray data;

    qint64 filesize = file.size();
    data = file.readAll();
    QDataStream in(data);
    in.setByteOrder(QDataStream::LittleEndian);
    quint16 pos;
    in >> pos;

    in.skipRawData(pos - 2);

    //long size = data.size();
    int bytes_per_record = 2;

    if (!spo2header) {
        // next is 0x0002
        // followed by 16bit duration in seconds

        // Read date and time (it's a 16bit charset)

        char dchr[20];
        int j = 0;
        for (int i = 0; i < 18 * 2; i += 2) {
            dchr[j++] = data.at(8 + i);
        }

        dchr[j] = 0;
        if (dchr[0]) {
            QString dstr(dchr);
            m_startTime = QDateTime::fromString(dstr, "MM/dd/yy HH:mm:ss");
            if (m_startTime.date().year() < 2000) { m_startTime = m_startTime.addYears(100); }
        } else {
            m_startTime = QDateTime(QDate::currentDate(), QTime(0,0,0));
        }
    } else { // !spo2header

        quint32 samples = 0; // number of samples

        quint32 year, month, day;
        quint32 hour, minute, second;

        if (data.at(pos) != 1) {
            qWarning() << ".spo2 file" << path << "might be a different";
        }

        // Unknown cruft header...
        in.skipRawData(200);

        in >> year >> month >> day;
        in >> hour >> minute >> second;

        m_startTime = QDateTime(QDate(year, month, day), QTime(hour, minute, second));

        // ignoring it for now
        pos += 0x1c + 200;

        in >> samples;

        int remainder = filesize - pos;

        bytes_per_record = remainder / samples;
        qDebug() << samples << "samples of" << bytes_per_record << "bytes each";

        // CMS50I .spo2 data have 4 digits, a 16bit, followed by spo2 then pulse

    }

    oxirec = new QVector<OxiRecord>;
    oxisessions[m_startTime] = oxirec;

    unsigned char o2, pr;
    quint16 un;

    // Read all Pulse and SPO2 data
    do {
        if (bytes_per_record > 2) {
            in >> un;
        }
        in >> o2;
        in >> pr;

        if ((o2 == 0x7f) && (pr == 0xff)) {
            o2 = pr = 0;
            un = 0;
        }

        if (spo2header) {
            oxirec->append(OxiRecord(pr, o2));
        } else {
            oxirec->append(OxiRecord(o2, pr));
        }
    } while (!in.atEnd());


//    for (int i = pos; i < size - 2;) {
//        o2 = (unsigned char)(data.at(i + 1));
//        pr = (unsigned char)(data.at(i + 0));
//        oxirec->append(OxiRecord(pr, o2));
//        i += 2;
//    }

    // processing gets done later
    return true;
}

void CMS50F37Loader::process()
{
    // Just clean up any extra crap before oximeterimport parses the oxirecords..
    return;
//    if (!oxirec)
//        return;
//    int size=oxirec->size();
//    if (size<10)
//        return;

}



static bool cms50f37_initialized = false;

void CMS50F37Loader::Register()
{
    if (cms50f37_initialized) { return; }

    qDebug() << "Registering CMS50F37Loader";
    RegisterLoader(new CMS50F37Loader());
    cms50f37_initialized = true;
}


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
    imp_callbacks = 0;

    m_vendorID = 0x10c4;
    m_productID = 0xea60;
    cms50dplus = false;

    oxirec = nullptr;

    startTimer.setParent(this);
    resetTimer.setParent(this);

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

        nextCommand();
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


int cms50_seqlength = sizeof(cms50_sequence);


void CMS50F37Loader::processBytes(QByteArray bytes)
{
    int data;

    QString tmpstr;

    int lengths[32] = { 0, 0, 0, 0, 0, 9, 4, 8, 8, 6, 4, 0, 2, 0, 3, 8, 3, 9, 8, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    buffer.append(bytes);
    int size = buffer.size();

    int idx = 0;
    int len;

    do {
        unsigned char res = bytes.at(idx);

        len = lengths[res & 0x1f];

        if (size < len)
            break;

        if (len == 0) {
            // lost sync
            idx++;
            continue;
        }
        for (int i = 1; i < len; i++) {
            buffer[idx+i] = buffer[idx+i] ^ 0x80;
        }

        switch(res) {
        case 0x05: // 5,80,80,f5,f3,e5,f2,80,80
            break;
        case 0x6: // 6,80,80,87
            data = buffer.at(idx+3) ^ 0x80;
            break;
        case 0x07: // 7,80,80,80,94,8e,88,92
            tmpstr = QString().sprintf("%02i%02i%02i%02i",buffer.at(idx+5), buffer.at(idx+4), buffer.at(idx+6), buffer.at(idx+7));
            imp_date = QDate::fromString(tmpstr, "yyyyMMdd");
            break;
        case 0x08: // 8,80,80,80,a4,81,80,80
            break;
        case 0x0a: // a,80,80,81
            data = buffer.at(idx+3);
            break;
        case 0xc: // a7 & a2  // responds with: c,80
            data = buffer.at(idx+1);
            break;
        case 0x0e: // e,80,81
            break;
        case 0x10: // 10,80,81
            data = buffer.at(idx+2);
            break;
        case 0x11: // 11,80,81,81,80,80,80,80,80
            break;
        case 0x12: // 12,80,80,80,82,a6,92,80
            tmpstr = QString().sprintf("%02i:%02i:%02i",buffer.at(idx+4), buffer.at(idx+5), buffer.at(idx+6));
            imp_time = QTime::fromString(tmpstr, "HHmmss");
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
                m_itemTotal=5000;

                m_startTime = QDateTime(imp_date, imp_time);
                oxirec = new QVector<OxiRecord>;
                oxirec->reserve(30000);
                cb_reset = 1;

                resetTimer.singleShot(2000,this,SLOT(resetImportTimeout()));
            }

            break;
        default:
            qDebug() << "unknown cms50 result?" << hex << (int)res;
            break;
        }

        if (res == 0x09) {
            int pi = buffer.at(idx + 4) | buffer.at(idx + 5) << 7;
            oxirec->append(OxiRecord(buffer.at(idx+3), buffer.at(idx+2), pi));
            // 9,80,e1,c4,ce,82  // cms50i data
        } else if (res == 0x0f) {
            oxirec->append(OxiRecord(buffer.at(idx+3), buffer.at(idx+2)));
            oxirec->append(OxiRecord(buffer.at(idx+5), buffer.at(idx+4)));
            oxirec->append(OxiRecord(buffer.at(idx+7), buffer.at(idx+6)));
            // f,80,de,c2,de,c2,de,c2  cms50F data...
        }

        QStringList str;
        for (int i=0; i < len; ++i) {
            str.append(QString::number((unsigned char)bytes.at(idx + i),16));
        }

        if (!started_import) {
            startTimer.singleShot(300, this, SLOT(nextCommand()));
            qDebug() << "Read:" << str.join(",");
        } else {
            qDebug() << "Import:" << str.join(",");
        }

        idx += len;
    } while (idx < size);

    if (!started_import) {
        imp_callbacks = 0;
    } else {
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


void CMS50F37Loader::sendCommand(unsigned char c)
{
    static unsigned char cmd[] = { 0x7d, 0x81, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };
    cmd[2] = c;

    QString out;
    for (int i=0;i < 9;i++) out += QString().sprintf("%02X ",cmd[i]);
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


void CMS50F37Loader::resetDevice() // Switch CMS50D+ device to live streaming mode
{
}

void CMS50F37Loader::requestData() // Switch CMS50D+ device to record transmission mode
{
}

void CMS50F37Loader::killTimers()
{
    if (resetTimer.isActive()) resetTimer.stop();
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

            resetTimer.stop();

            finished_import = true;
            m_streaming = false;

            emit importComplete(this);

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


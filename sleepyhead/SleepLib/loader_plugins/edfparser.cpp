/* EDF Parser Implementation
 *
 * Copyright (c) 2011-2018 Mark Watkins <mark@jedimark.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include "zlib.h"


#include "edfparser.h"

EDFParser::EDFParser(QString name)
{
    buffer = nullptr;
    Open(name);
}
EDFParser::~EDFParser()
{
    for (QVector<EDFSignal>::iterator s = edfsignals.begin(); s != edfsignals.end(); s++) {
        if ((*s).data) { delete [](*s).data; }
    }

    if (buffer) { delete [] buffer; }
}
// Read a 16 bits integer
qint16 EDFParser::Read16()
{
    if ((pos + 2) > filesize) {
        return 0;
    }

#ifdef Q_LITTLE_ENDIAN
    // Intel, etc...
    qint16 res = *(qint16 *)&buffer[pos];
#else
    // ARM, PPC, etc..
    qint16 res = quint8(buffer[pos]) | (qint8(buffer[pos+1]) << 8);
#endif

    pos += 2;
    return res;
}

QString EDFParser::Read(unsigned n)
{
    if ((pos + long(n)) > filesize) {
        return "";
    }

    QByteArray buf(&buffer[pos], n);
    pos+=n;

    return buf.trimmed();
}
bool EDFParser::Parse()
{
    bool ok;
    QString temp, temp2;

    version = QString::fromLatin1(header.version, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    //patientident=QString::fromLatin1(header.patientident,80);
    recordingident = QString::fromLatin1(header.recordingident, 80); // Serial number is in here..
    int snp = recordingident.indexOf("SRN=");
    serialnumber.clear();
    /*char * idx=index(header.recordingident,'=');
    idx++;
    for (int i=0;i<16;++i) {
        if (*idx==0x20) break;
        serialnumber+=*idx;
        ++idx;
    } */

    for (int i = snp + 4; i < recordingident.length(); i++) {
        if (recordingident[i] == ' ') {
            break;
        }

        serialnumber += recordingident[i];
    }

    QDateTime startDate = QDateTime::fromString(QString::fromLatin1(header.datetime, 16), "dd.MM.yyHH.mm.ss");
    //startDate.toTimeSpec(Qt::UTC);

    QDate d2 = startDate.date();

    if (d2.year() < 2000) {
        d2.setDate(d2.year() + 100, d2.month(), d2.day());
        startDate.setDate(d2);
    }

    if (!startDate.isValid()) {
        qDebug() << "Invalid date time retreieved parsing EDF File " << filename;
        return false;
    }

    startdate = qint64(startDate.toTime_t()) * 1000L;
    //startdate-=timezoneOffset();

    //qDebug() << startDate.toString("yyyy-MM-dd HH:mm:ss");

    num_header_bytes = QString::fromLatin1(header.num_header_bytes, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    //reserved44=QString::fromLatin1(header.reserved,44);
    num_data_records = QString::fromLatin1(header.num_data_records, 8).toLong(&ok);

    if (!ok) {
        return false;
    }

    dur_data_record = (QString::fromLatin1(header.dur_data_records, 8).toDouble(&ok) * 1000.0L);

    if (!ok) {
        return false;
    }

    num_signals = QString::fromLatin1(header.num_signals, 4).toLong(&ok);

    if (!ok) {
        return false;
    }

    enddate = startdate + dur_data_record * qint64(num_data_records);
    // if (dur_data_record==0)
    //   return false;

    // this could be loaded quicker by transducer_type[signal] etc..

    // Initialize fixed-size signal list.
    edfsignals.resize(num_signals);

    for (int i = 0; i < num_signals; i++) {
        EDFSignal &sig = edfsignals[i];
        sig.data = nullptr;
        sig.label = Read(16);

        signal_labels.push_back(sig.label);
        signalList[sig.label].push_back(&sig);
        signal.push_back(&sig);
    }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].transducer_type = Read(80); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_dimension = Read(8); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_minimum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].physical_maximum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].digital_minimum = Read(8).toDouble(&ok); }

    for (int i = 0; i < num_signals; i++) {
        EDFSignal &e = edfsignals[i];
        e.digital_maximum = Read(8).toDouble(&ok);
        e.gain = (e.physical_maximum - e.physical_minimum) / (e.digital_maximum - e.digital_minimum);
        e.offset = 0;
    }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].prefiltering = Read(80); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].nr = Read(8).toLong(&ok); }

    for (int i = 0; i < num_signals; i++) { edfsignals[i].reserved = Read(32); }

    // allocate the buffers
    for (int i = 0; i < num_signals; i++) {
        //qDebug//cout << "Reading signal " << signals[i]->label << endl;
        EDFSignal &sig = edfsignals[i];

        long recs = sig.nr * num_data_records;

        if (num_data_records < 0) {
            sig.data = nullptr;
            continue;
        }

        sig.data = new qint16 [recs];
        sig.pos = 0;
    }

    for (int x = 0; x < num_data_records; x++) {
        for (int i = 0; i < num_signals; i++) {
            EDFSignal &sig = edfsignals[i];
#ifdef Q_LITTLE_ENDIAN
            // Intel x86, etc..
            memcpy((char *)&sig.data[sig.pos], (char *)&buffer[pos], sig.nr * 2);
            sig.pos += sig.nr;
            pos += sig.nr * 2;
#else
            // Big endian safe
            for (int j=0;j<sig.nr;j++) {
                qint16 t=Read16();
                sig.data[sig.pos++]=t;
            }
#endif
        }
    }

    return true;
}
bool EDFParser::Open(const QString & name)
{
    if (buffer != nullptr) {
        qWarning() << "EDFParser::Open() called with buffer already initialized";
        return false;
    }

    if (name.endsWith(STR_ext_gz)) {
        // Open and decempress file to buffer

        filename = name.mid(0, -3);

        // Get file length from inside gzip file
        QFile fi(name);

        if (!fi.open(QFile::ReadOnly) || !fi.seek(fi.size() - 4)) {
            goto badfile;
        }

        unsigned char ch[4];
        fi.read((char *)ch, 4);
        filesize = ch[0] | (ch [1] << 8) | (ch[2] << 16) | (ch[3] << 24);

        datasize = filesize - EDFHeaderSize;
        if (datasize < 0) {
            goto badfile;
        }

        // Open gzip file for reading
        gzFile f = gzopen(name.toLatin1(), "rb");
        if (!f) {
            goto badfile;
        }

        // Decompressed header and data block
        gzread(f, (char *)&header, EDFHeaderSize);
        buffer = new char [datasize];
        gzread(f, buffer, datasize);
        gzclose(f);
    } else {

        // Open and read uncompressed file
        QFile f(name);

        if (!f.open(QIODevice::ReadOnly)) {
            goto badfile;
        }

        filename = name;
        filesize = f.size();
        datasize = filesize - EDFHeaderSize;

        if (datasize < 0) {
            goto badfile;
        }

        f.read((char *)&header, EDFHeaderSize);

        buffer = new char [datasize];
        f.read(buffer, datasize);
        f.close();
    }

    pos = 0;
    return true;

badfile:
    qDebug() << "EDFParser::Open() Couldn't open file" << name;
    return false;
}

EDFSignal *EDFParser::lookupLabel(const QString & name, int index)
{
    QHash<QString, QList<EDFSignal *> >::iterator it = signalList.find(name);

    if (it == signalList.end()) return nullptr;

    if (index >= it.value().size()) return nullptr;

    return it.value()[index];
}

/*

SleepLib BinaryFile Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef BINARY_FILE_H
#define BINARY_FILE_H

//#include <wx/ffile.h>
//#include <wx/utils.h>

#include <QString>
#include <QFile>
#include <QDateTime>

enum BFOpenMode { BF_READ, BF_WRITE };
const long max_buffer_size=1048576*2;
bool IsPlatformLittleEndian();

class UnpackError
{

};

class PackBuffer
{
public:
    PackBuffer();
    ~PackBuffer();
    bool Open(QString filename,BFOpenMode mode);
    void Close();

};

class BinaryFile
{
public:
    BinaryFile();
    ~BinaryFile();
    bool Open(QString filename,BFOpenMode mode);
    void Close();

    bool Unpack(bool & data);
    bool Unpack(qint8 & data);
    bool Unpack(qint16 & data);
    bool Unpack(qint32 & data);
    bool Unpack(qint64 & data);
    bool Unpack(quint8 & data);
    bool Unpack(quint16 & data);
    bool Unpack(quint32 & data);
    bool Unpack(quint64 & data);
    bool Unpack(float & data);
    bool Unpack(double & data);
    bool Unpack(QString & data);
    bool Unpack(QDateTime & data);

    bool Pack(const bool &data);
    bool Pack(const qint8 &data);
    bool Pack(const qint16 & data);
    bool Pack(const qint32 & data);
    bool Pack(const qint64 & data);
    bool Pack(const quint8 & data);
    bool Pack(const quint16 & data);
    bool Pack(const quint32 & data);
    bool Pack(const quint64 & data);
    bool Pack(const float & data);
    bool Pack(const double & data);
    bool Pack(const QString & data);
    bool Pack(const QDateTime & data);

    size_t Write(const char * buffer, size_t count);

protected:
    BFOpenMode bf_mode;
    QFile f;
    char * buffer;
    int buff_read;
    long pos;
    QString bf_filename;
    long size;
};

#endif //BINARY_FILE_H

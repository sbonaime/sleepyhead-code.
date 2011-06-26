/*

SleepLib BinaryFile Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

//#include <wx/filename.h>
#include "binary_file.h"

bool IsPlatformLittleEndian()
{
    quint32 j=1;
    *((char*)&j) = 0;
    return j!=1;
}

BinaryFile::BinaryFile()
{
    size=pos=0;
    buffer=NULL;
}
void BinaryFile::Close()
{
    if (bf_mode==BF_WRITE) {
        //f.Write(buffer,pos);
    }
    f.close();
    if (buffer) delete [] buffer;
    size=pos=0;
}
BinaryFile::~BinaryFile()
{
    Close();
}
size_t BinaryFile::Write(const char * buffer, size_t count)
{
    return f.write(buffer,count);
}

bool BinaryFile::Open(QString filename,BFOpenMode mode)
{
    pos=size=0;
    bf_mode=mode;
    f.setFileName(filename);
    if (f.exists()) {
        if (mode==BF_WRITE) {
            // hmm.. file exists. unsure of best course of action here..
          //  return false;
        }
        size=f.size();
    }
    QIODevice::OpenMode om;
    if (mode==BF_READ) om=QIODevice::ReadOnly;
    else if (mode==BF_WRITE) om=QIODevice::WriteOnly;
    if (!f.open(om)) return false;

    if (mode==BF_READ) {
        buffer=new char [size];
        size=f.read(buffer,size);
    }
    return true;
}
bool BinaryFile::Unpack(qint8 & data)
{
    if (pos>=size) return false;
    data=((qint8 *)buffer)[pos++];
    return true;
}
bool BinaryFile::Unpack(bool & data)
{
    if (pos>=size) return false;
    data=((qint8 *)buffer)[pos++];
    return true;
}

bool BinaryFile::Unpack(qint16 & data)
{
    if (pos+1>=size) return false;
    data=((quint8 *)buffer)[pos++];
    data|=((qint8 *)buffer)[pos++] << 8;
    return true;
}
bool BinaryFile::Unpack(qint32 & data)
{
    if (pos+3>=size) return false;
    if (IsPlatformLittleEndian()) {
        data=*((qint32 *)(&buffer[pos]));
        pos+=4;
    } else {
        data=((quint8 *)buffer)[pos++];
        data|=((quint8 *)buffer)[pos++] << 8;
        data|=((quint8 *)buffer)[pos++] << 16;
        data|=((qint8 *)buffer)[pos++] << 24;

    }
    return true;
}
bool BinaryFile::Unpack(qint64 & data)
{
    if (pos+7>=size) return false;

    if (IsPlatformLittleEndian()) {
        data=*((qint64 *)(&buffer[pos]));
        pos+=8;
    } else {
        //for (int i=7;i>=0;i--) data=(data << 8) | ((quint8 *)buffer))[pos+i];
        // pos+=8;
        data=qint64(((quint8 *)buffer)[pos++]);
        data|=qint64(((quint8 *)buffer)[pos++]) << 8;
        data|=qint64(((quint8 *)buffer)[pos++]) << 16;
        data|=qint64(((quint8 *)buffer)[pos++]) << 24;
        data|=qint64(((quint8 *)buffer)[pos++]) << 32;
        data|=qint64(((quint8 *)buffer)[pos++]) << 40;
        data|=qint64(((quint8 *)buffer)[pos++]) << 48;
        data|=qint64(((qint8 *)buffer)[pos++]) << 56;

    }

    return true;
}
bool BinaryFile::Unpack(quint8 & data)
{
    if (pos>=size) return false;
    data=((qint8 *)buffer)[pos++];
    return true;
}
bool BinaryFile::Unpack(quint16 & data)
{
    if (pos+1>=size) return false;
    data=quint16(((quint8 *)buffer)[pos++]);
    data|=quint16(((quint8 *)buffer)[pos++]) << 8;

    return true;
}
bool BinaryFile::Unpack(quint32 & data)
{
    if (pos>=size) return false;
    if (IsPlatformLittleEndian()) {
        data=*((quint32 *)(&buffer[pos]));
        pos+=4;
    } else {
        data=quint32(((quint8 *)buffer)[pos++]);
        data|=quint32(((quint8 *)buffer)[pos++]) << 8;
        data|=quint32(((quint8 *)buffer)[pos++]) << 16;
        data|=quint32(((quint8 *)buffer)[pos++]) << 24;
    }

    return true;
}
bool BinaryFile::Unpack(quint64 & data)
{
    if (pos>=size) return false;
    if (IsPlatformLittleEndian()) {
        data=*((qint64 *)(&buffer[pos]));
        pos+=8;
    } else {
        //for (int i=7;i>=0;i--) data=(data << 8) | ((quint8 *)buffer))[pos+i];
        // pos+=8;
        data=quint64(((quint8 *)buffer)[pos++]);
        data|=quint64(((quint8 *)buffer)[pos++]) << 8;
        data|=quint64(((quint8 *)buffer)[pos++]) << 16;
        data|=quint64(((quint8 *)buffer)[pos++]) << 24;
        data|=quint64(((quint8 *)buffer)[pos++]) << 32;
        data|=quint64(((quint8 *)buffer)[pos++]) << 40;
        data|=quint64(((quint8 *)buffer)[pos++]) << 48;
        data|=quint64(((quint8 *)buffer)[pos++]) << 56;
    }

    return true;
}
bool BinaryFile::Unpack(float & data)
{
    if ((pos+4)>=size) return false;
    if (IsPlatformLittleEndian()) {
        data=*((float *)(&buffer[pos]));
        pos+=4;
    } else {
        unsigned char b[4];
        for (int i=0; i<4; i++) {
            b[3-i]=buffer[pos+i];
        }
        data=*((float *)b);
        pos+=4;
    }

    return true;
}
bool BinaryFile::Unpack(double & data)
{
    if ((pos+7)>=size) return false;
    if (IsPlatformLittleEndian()) {
        data=*((double *)(&buffer[pos]));
        pos+=8;
    } else {
        unsigned char b[8];
        for (int i=0; i<8; i++) {
            b[7-i]=buffer[pos+i];
        }
        data=*((double *)b);
        pos+=8;
    }

    return true;
}
bool BinaryFile::Unpack(QString & data)
{
    qint16 i16,t16;
    if (!Unpack(i16)) return false;
    if ((pos+(i16*2))>size) return false;
    data="";
    QChar c;
    for (int i=0; i<i16; i++) {
        Unpack(t16);
        c=t16;
        data+=c;
    }
    return true;
}
bool BinaryFile::Unpack(QDateTime & data)
{
    qint32 i32;
    if (!Unpack(i32)) return false;
    time_t t=i32;
    data.setTime_t(t);
    return true;
}


bool BinaryFile::Pack(const qint8 &data)
{
    f.write((char *)&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const bool &data)
{
    f.write((char*)&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const qint16 & data)
{
    qint8 *d=(qint8*)&data;
    f.write((char *)&d[0],1);
    f.write((char *)&d[1],1);
    pos+=2;
    return true;
}
bool BinaryFile::Pack(const qint32 & data)
{
    qint8 *d=(qint8*)&data;
    for (int i=0; i<4; i++) f.write((char *)&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const qint64 & data)
{
    qint8 *d=(qint8 *)&data;
    for (int i=0; i<8; i++) f.write((char *)&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const quint8 & data)
{
    f.write((char *)&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const quint16 & data)
{
    quint8 *d=(quint8 *)&data;
    f.write((char *)&d[0],1);
    f.write((char *)&d[1],1);
    pos+=2;

    return true;
}
bool BinaryFile::Pack(const quint32 & data)
{
    quint8 *d=(quint8 *)&data;
    for (int i=0; i<4; i++) f.write((char *)&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const quint64 & data)
{
    quint8 *d=(quint8 *)&data;
    for (int i=0; i<8; i++) f.write((char *)&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const float & data)
{
    quint8 *d=(quint8 *)&data;
    for (int i=0; i<4; i++) f.write((char *)&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const double & data)
{
    quint8 *d=(quint8 *)&data;
    for (int i=0; i<8; i++) f.write((char *)&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const QString & data)
{
    const QChar *s=data.data();

    qint16 i16=data.length();
    Pack(i16);
    qint16 t;

    for (int i=0; i<i16; i++) {
        t=s[i].unicode();
        Pack(t);
    }

    return true;
}
bool BinaryFile::Pack(const QDateTime & data)
{
    time_t t=data.toTime_t();
    qint32 i32=t;
    Pack(i32);
    return true;
}



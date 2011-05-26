/*

SleepLib BinaryFile Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#include <wx/filename.h>
#include "binary_file.h"

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
    f.Close();
    if (buffer) delete [] buffer;
    size=pos=0;
}
BinaryFile::~BinaryFile()
{
    Close();
}
bool BinaryFile::Open(wxString filename,BFOpenMode mode)
{
    pos=size=0;
    bf_mode=mode;
    if (wxFileExists(filename)) {
        if (mode==BF_WRITE) {
            // hmm.. file exists. unsure of best course of action here..
            return false;
        }
        size=wxFileName::GetSize(filename).GetLo();
    }
    wxString om;
    if (mode==BF_READ) om=wxT("rb");
    else if (mode==BF_WRITE) om=wxT("wb");

    if (!f.Open(filename,om)) return false;

    if (mode==BF_READ) {
        buffer=new char [size];
        size=f.Read(buffer,size);
    }
    return true;
}
bool BinaryFile::Unpack(wxInt8 & data)
{
    if (pos>=size) return false;
    data=((wxInt8 *)buffer)[pos++];
    return true;
}
bool BinaryFile::Unpack(bool & data)
{
    if (pos>=size) return false;
    data=((wxInt8 *)buffer)[pos++];
    return true;
}

bool BinaryFile::Unpack(wxInt16 & data)
{
    if (pos+1>=size) return false;
    data=((wxUint8 *)buffer)[pos++];
    data|=((wxInt8 *)buffer)[pos++] << 8;
    return true;
}
bool BinaryFile::Unpack(wxInt32 & data)
{
    if (pos+3>=size) return false;
    if (wxIsPlatformLittleEndian()) {
        data=*((wxInt32 *)(&buffer[pos]));
        pos+=4;
    } else {
        data=((wxUint8 *)buffer)[pos++];
        data|=((wxUint8 *)buffer)[pos++] << 8;
        data|=((wxUint8 *)buffer)[pos++] << 16;
        data|=((wxInt8 *)buffer)[pos++] << 24;

    }
    return true;
}
bool BinaryFile::Unpack(wxInt64 & data)
{
    if (pos+7>=size) return false;

    if (wxIsPlatformLittleEndian()) {
        data=*((wxInt64 *)(&buffer[pos]));
        pos+=8;
    } else {
        //for (int i=7;i>=0;i--) data=(data << 8) | ((wxUint8 *)buffer))[pos+i];
        // pos+=8;
        data=wxInt64(((wxUint8 *)buffer)[pos++]);
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 8;
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 16;
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 24;
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 32;
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 40;
        data|=wxInt64(((wxUint8 *)buffer)[pos++]) << 48;
        data|=wxInt64(((wxInt8 *)buffer)[pos++]) << 56;

    }

    return true;
}
bool BinaryFile::Unpack(wxUint8 & data)
{
    if (pos>=size) return false;
    data=((wxInt8 *)buffer)[pos++];
    return true;
}
bool BinaryFile::Unpack(wxUint16 & data)
{
    if (pos+1>=size) return false;
    data=wxUint16(((wxUint8 *)buffer)[pos++]);
    data|=wxUint16(((wxUint8 *)buffer)[pos++]) << 8;

    return true;
}
bool BinaryFile::Unpack(wxUint32 & data)
{
    if (pos>=size) return false;
    if (wxIsPlatformLittleEndian()) {
        data=*((wxUint32 *)(&buffer[pos]));
        pos+=4;
    } else {
        data=wxUint32(((wxUint8 *)buffer)[pos++]);
        data|=wxUint32(((wxUint8 *)buffer)[pos++]) << 8;
        data|=wxUint32(((wxUint8 *)buffer)[pos++]) << 16;
        data|=wxUint32(((wxUint8 *)buffer)[pos++]) << 24;
    }

    return true;
}
bool BinaryFile::Unpack(wxUint64 & data)
{
    if (pos>=size) return false;
    if (wxIsPlatformLittleEndian()) {
        data=*((wxInt64 *)(&buffer[pos]));
        pos+=8;
    } else {
        //for (int i=7;i>=0;i--) data=(data << 8) | ((wxUint8 *)buffer))[pos+i];
        // pos+=8;
        data=wxUint64(((wxUint8 *)buffer)[pos++]);
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 8;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 16;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 24;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 32;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 40;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 48;
        data|=wxUint64(((wxUint8 *)buffer)[pos++]) << 56;
    }

    return true;
}
bool BinaryFile::Unpack(float & data)
{
    if ((pos+4)>=size) return false;
    if (wxIsPlatformLittleEndian()) {
        data=*((float *)(&buffer[pos]));
        pos+=4;
    } else {
        unsigned char b[4];
        for (int i=0; i<4; i++) {
            b[3-i]=buffer[pos+i];
        }
        data=*((float *)(b));
        pos+=4;
    }

    return true;
}
bool BinaryFile::Unpack(double & data)
{
    if ((pos+7)>=size) return false;
    if (wxIsPlatformLittleEndian()) {
        data=*((double *)(&buffer[pos]));
        pos+=8;
    } else {
        unsigned char b[8];
        for (int i=0; i<8; i++) {
            b[7-i]=buffer[pos+i];
        }
        data=*((double *)(b));
        pos+=8;
    }

    return true;
}
bool BinaryFile::Unpack(wxString & data)
{
    wxInt16 i16;
    if (!Unpack(i16)) return false;
    if ((pos+i16)>=size) return false;
    data=wxT("");
    for (int i=0; i<i16; i++) {
        data+=wxChar(buffer[pos++]);
    }
    return true;
}
bool BinaryFile::Unpack(wxDateTime & data)
{
    wxInt32 i32;
    if (!Unpack(i32)) return false;
    time_t t=i32;
    data.Set(t);
    return true;
}


bool BinaryFile::Pack(const wxInt8 &data)
{
    f.Write(&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const bool &data)
{
    f.Write((wxInt8*)&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const wxInt16 & data)
{
    wxInt8 *d=(wxInt8 *)&data;
    f.Write(&d[0],1);
    f.Write(&d[1],1);
    pos+=2;
    return true;
}
bool BinaryFile::Pack(const wxInt32 & data)
{
    wxInt8 *d=(wxInt8 *)&data;
    for (int i=0; i<4; i++) f.Write(&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const wxInt64 & data)
{
    wxInt8 *d=(wxInt8 *)&data;
    for (int i=0; i<8; i++) f.Write(&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const wxUint8 & data)
{
    f.Write(&data,1);
    pos++;
    return true;
}
bool BinaryFile::Pack(const wxUint16 & data)
{
    wxUint8 *d=(wxUint8 *)&data;
    f.Write(&d[0],1);
    f.Write(&d[1],1);
    pos+=2;

    return true;
}
bool BinaryFile::Pack(const wxUint32 & data)
{
    wxUint8 *d=(wxUint8 *)&data;
    for (int i=0; i<4; i++) f.Write(&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const wxUint64 & data)
{
    wxUint8 *d=(wxUint8 *)&data;
    for (int i=0; i<8; i++) f.Write(&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const float & data)
{
    wxUint8 *d=(wxUint8 *)&data;
    for (int i=0; i<4; i++) f.Write(&d[i],1);
    pos+=4;
    return true;
}
bool BinaryFile::Pack(const double & data)
{
    wxUint8 *d=(wxUint8 *)&data;
    for (int i=0; i<8; i++) f.Write(&d[i],1);
    pos+=8;
    return true;
}
bool BinaryFile::Pack(const wxString & data)
{
    char *s=strdup(data.mb_str());

    wxInt16 i16=strlen(s);
    Pack(i16);

    for (int i=0; i<i16; i++) {
        Pack(s[i]);
    }
    free(s);

    return true;
}
bool BinaryFile::Pack(const wxDateTime & data)
{
    time_t t=data.GetTicks();
    wxInt32 i32=t;
    Pack(i32);
    return true;
}



/*

SleepLib BinaryFile Header

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL

*/

#ifndef BINARY_FILE_H
#define BINARY_FILE_H

#include <wx/ffile.h>
#include <wx/utils.h>

enum BFOpenMode { BF_READ, BF_WRITE };
const long max_buffer_size=1048576*2;

class UnpackError
{

};

class PackBuffer
{
public:
    PackBuffer();
    ~PackBuffer();
    bool Open(wxString filename,BFOpenMode mode);
    void Close();

};

class BinaryFile
{
public:
    BinaryFile();
    ~BinaryFile();
    bool Open(wxString filename,BFOpenMode mode);
    void Close();

    bool Unpack(bool & data);
    bool Unpack(wxInt8 & data);
    bool Unpack(wxInt16 & data);
    bool Unpack(wxInt32 & data);
    bool Unpack(wxInt64 & data);
    bool Unpack(wxUint8 & data);
    bool Unpack(wxUint16 & data);
    bool Unpack(wxUint32 & data);
    bool Unpack(wxUint64 & data);
    bool Unpack(float & data);
    bool Unpack(double & data);
    bool Unpack(wxString & data);
    bool Unpack(wxDateTime & data);

    bool Pack(const bool &data);
    bool Pack(const wxInt8 &data);
    bool Pack(const wxInt16 & data);
    bool Pack(const wxInt32 & data);
    bool Pack(const wxInt64 & data);
    bool Pack(const wxUint8 & data);
    bool Pack(const wxUint16 & data);
    bool Pack(const wxUint32 & data);
    bool Pack(const wxUint64 & data);
    bool Pack(const float & data);
    bool Pack(const double & data);
    bool Pack(const wxString & data);
    bool Pack(const wxDateTime & data);

    size_t Write(const void* buffer, size_t count);

protected:
    BFOpenMode bf_mode;
    wxFFile f;
    char * buffer;
    int buff_read;
    long pos;
    wxString bf_filename;
    long size;
};

#endif //BINARY_FILE_H

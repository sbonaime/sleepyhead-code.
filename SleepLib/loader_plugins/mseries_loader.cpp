/*

SleepLib RemStar M-Series Loader Implementation

Author: Mark Watkins <jedimark64@users.sourceforge.net>
License: GPL
Copyright: (c)2012 Mark Watkins

*/

#include <QDir>
#include <QProgressBar>

#include "mseries_loader.h"
extern QProgressBar *qprogress;



MSeries::MSeries(Profile *p,MachineID id)
    :CPAP(p,id)
{
    m_class=mseries_class_name;
    properties[STR_PROP_Brand]="Respironics";
    properties[STR_PROP_Model]=STR_MACH_MSeries;
}

MSeries::~MSeries()
{
}

MSeriesLoader::MSeriesLoader()
{
}

MSeriesLoader::~MSeriesLoader()
{
    for (QHash<QString,Machine *>::iterator i=MachList.begin(); i!=MachList.end(); i++) {
        delete i.value();
    }
}

//struct MSeriesHeader {
//    quint8   b1;            //0x52
//    quint32  a32;           //0x00000049
//    quint16  u16[8];
//    quint8   b2;            //0x02
//    char     setname[16];
//    char     firstname[25];
//    char     lastname[25];
//    char     serial[50];
//    quint16  b3;            //0x00
//    quint16  b4;            //0x66
//    quint16  b5;            //0xff

//} __attribute__((packed));
/*

blockLayoutOffsets   {
            cardInformationBlock = 0,
                brandID = 0,
                cardType = 2,
                cardVersion = 3
                startUIDB = 4
                endUIDB = 6,
                startCPB = 8,
                endCPB = 10,
                startCDCB = 12,
                endCDCB = 14,
                startCDB = 0x10,
                endCDB = 0x12,
                checksum = 20,

            userIDBlock = 0x15
                personalID = 1,
                patientFName = 0x11,
                patientLName = 0x2a,
                serialNumber = 0x43,
                modelNumber = 0x4d,
                textData = 0x57
                checksum = 0x77,

            cardPrescriptionBlock = 0x8d,


            cardDataControlBlock = 0xa3,
                validFlagOne = 3,
                headPtrOne = 4,
                tailPtrOne = 6,
                cdbChecksumOne = 8,
                validFlagTwo = 9
                headPtrTwo = 10,
                tailPtrTwo = 12,
                cdbChecksumTwo = 14,

            cardDataBlock = 0xb2,
                basicCompliance = 1,
                fosq = 2,
                Invalid = 0xff,
                sleepProfile = 8,
                sleepProfile2 = 10,
                sleepProfile3 = 14,
                sleepTrend = 9,
                sleepTrend2 = 11,
                sleepTrend3 = 15,
                smartAutoCPAPProfile = 3,
                smartAutoCPAPTrend = 4,
                ventCompliance2 = 13,
                ventilatorCompliance = 7,
                ventilatorProfile = 6,
                ventProfile2 = 12
                startChar = 0xfe,
                stopChar = 0x7f
                    */
int MSeriesLoader::Open(QString & path,Profile *profile)
{
    // Until a smartcard reader is written, this is not an auto-scanner.. it just opens a block file..

    QFile file(path);
    if (!file.exists()) return 0;
    if (file.size()!=32768)    // Check filesize matches smartcard?
        return 0;

    if (!file.open(QFile::ReadOnly)) {
        qDebug() << "Couldn't open M-Series file:" << path;
        return 0;
    }
    QByteArray block=file.readAll();


    // Thanks to Phil Gillam for the pointers on this one..

    const unsigned char * cardinfo=(unsigned char *)block.data();
    quint16 magic=cardinfo[0] << 8 | cardinfo[1];
    if (magic!=0x5249) { // "RI" Respironics Magic number
        return 0;
    }
    quint8 cardtype=cardinfo[2];
    quint8 cardver=cardinfo[3];

    quint16 user_offset=(cardinfo[4] << 8) | cardinfo[5];
    quint16 rx_offset=(cardinfo[8] << 8) | cardinfo[9];
    quint16 control_offset=(cardinfo[12] << 8) | cardinfo[13];
    quint16 data_offset=(cardinfo[16] << 8) | cardinfo[17];


    const char * userinfo=block.data()+user_offset;
    QString setname=QString(userinfo+0x1);
    QString firstname=QString(userinfo+0x11);
    QString lastname=QString(userinfo+0x2a);
    QString serial=QString(userinfo+0x43);
    serial.truncate(10);
    QString model=QString(userinfo+0x4d);
    QString textdata=QString(userinfo+0x57);
    quint8 userinfochk=*(userinfo+0x77);
    quint8 tmp=0;
    for (int i=0;i<0x77;i++) {
        tmp+=userinfo[i];
    }
    if (tmp!=userinfochk) {
        qDebug() << "MSeries UserInfo block checksum failure" << path;
    }

    const unsigned char * rxblock=(unsigned char *)block.data()+rx_offset;

    const unsigned char * controlblock=(unsigned char *)block.data()+control_offset;

    quint16 headptr1=controlblock[4] << 8 | controlblock[5];
    quint16 tailptr1=controlblock[6] << 8 | controlblock[7];
    quint16 headptr2=controlblock[10] << 8 | controlblock[11];
    quint16 tailptr2=controlblock[12] << 8 | controlblock[13];
//    validFlagOne = 3,
//    headPtrOne = 4,
//    tailPtrOne = 6,
//    cdbChecksumOne = 8,
//    validFlagTwo = 9
//    headPtrTwo = 10,
//    tailPtrTwo = 12,
//    cdbChecksumTwo = 14,

    const char * datablock=block.data()+data_offset;
    quint8 basicCompliance=datablock[1];
    quint8 fosq=datablock[2];
    quint8 smartAutoCPAPProfile=datablock[3];
    quint8 smartAutoCPAPTrend=datablock[4];
    quint8 ventProfile=datablock[6];
    quint8 ventCompliance1=datablock[7];
    quint8 sleepProfile1=datablock[8];
    quint8 sleepTrend1=datablock[9];
    quint8 sleepProfile2=datablock[10];
    quint8 sleepTrend2=datablock[11];
    quint8 ventProfile2=datablock[12];
    quint8 ventCompliance2=datablock[13];
    quint8 sleepProfile3=datablock[14];
    quint8 sleepTrend3=datablock[15];

//    basicCompliance = 1,
//    fosq = 2,
//    sleepProfile = 8,
//    sleepProfile2 = 10,
//    sleepProfile3 = 14,
//    sleepTrend = 9,
//    sleepTrend2 = 11,
//    sleepTrend3 = 15,
//    smartAutoCPAPProfile = 3,
//    smartAutoCPAPTrend = 4,
//    ventCompliance2 = 13,
//    ventilatorCompliance = 7,
//    ventilatorProfile = 6,
//    ventProfile2 = 12

//    Invalid = 0xff,
//    startChar = 0xfe,
//    stopChar = 0x7f


    //Machine *mach=CreateMachine(serial,profile);


    // 0xcount till next block (between f3 02... blocks)
    // 0xc0 00 // varies
    // 0xf3 02 f0 97 f2 ff f2 81

    return 1;
}

Machine *MSeriesLoader::CreateMachine(QString serial,Profile *profile)
{
    if (!profile)
        return NULL;
    qDebug() << "Create Machine " << serial;

    QList<Machine *> ml=profile->GetMachines(MT_CPAP);
    bool found=false;
    QList<Machine *>::iterator i;
    for (i=ml.begin(); i!=ml.end(); i++) {
        if (((*i)->GetClass()==mseries_class_name) && ((*i)->properties[STR_PROP_Serial]==serial)) {
            MachList[serial]=*i;
            found=true;
            break;
        }
    }
    if (found) return *i;

    Machine *m=new MSeries(profile,0);

    MachList[serial]=m;
    profile->AddMachine(m);

    m->properties[STR_PROP_Serial]=serial;
    m->properties[STR_PROP_DataVersion]=QString::number(mseries_data_version);

    QString path="{"+STR_GEN_DataFolder+"}/"+m->GetClass()+"_"+serial+"/";
    m->properties[STR_PROP_Path]=path;
    m->properties[STR_PROP_BackupPath]=path+"Backup/";

    return m;
}

bool mseries_initialized=false;
void MSeriesLoader::Register()
{
    if (mseries_initialized) return;
    qDebug() << "Registering RemStar M-Series Loader";
    RegisterLoader(new MSeriesLoader());
    //InitModelMap();
    mseries_initialized=true;
}

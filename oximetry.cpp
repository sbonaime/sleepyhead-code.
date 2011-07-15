#include <QDebug>
#include <QProgressBar>

#include "oximetry.h"
#include "ui_oximetry.h"
#include "qextserialport/qextserialenumerator.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gBarChart.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gFooBar.h"

Oximetry::Oximetry(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Oximetry)
{
    ui->setupUi(this);
    port=NULL;
    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);
    if (!profile) {
        qWarning("Couldn't get profile.. Have to abort!");
        abort();
    }

    gSplitter=new QSplitter(Qt::Vertical,ui->scrollArea);
    gSplitter->setStyleSheet("QSplitter::handle { background-color: 'dark grey'; }");
    gSplitter->setHandleWidth(2);
    ui->graphLayout->addWidget(gSplitter);

    AddData(pulse=new WaveData(OXI_Pulse));
    PULSE=new gGraphWindow(gSplitter,tr("Pulse Rate"),(QGLWidget *)NULL);
    PULSE->AddLayer(new gXAxis());
    PULSE->AddLayer(new gYAxis());
    PULSE->AddLayer(new gFooBar());
    PULSE->AddLayer(new gLineChart(pulse,Qt::red,65536,false,false,false));
    PULSE->setMinimumHeight(150);

    AddData(spo2=new WaveData(OXI_SPO2));
    SPO2=new gGraphWindow(gSplitter,tr("SPO2"),PULSE);
    SPO2->AddLayer(new gXAxis());
    SPO2->AddLayer(new gYAxis());
    SPO2->AddLayer(new gFooBar());
    SPO2->AddLayer(new gLineChart(spo2,Qt::red,65536,false,false,false));
    SPO2->setMinimumHeight(150);

    AddData(plethy=new WaveData(OXI_Plethy));
    plethy->AddSegment(1000000);
    plethy->np[0]=0;
    plethy->SetMaxY(100);
    plethy->SetMinY(0);
    PLETHY=new gGraphWindow(gSplitter,tr("Plethysomogram"),PULSE);
    //PLETHY->AddLayer(new gXAxis());
    PLETHY->AddLayer(new gYAxis());
    PLETHY->AddLayer(new gFooBar());
    PLETHY->AddLayer(new gLineChart(plethy,Qt::red,65536,true,false,false));
    PLETHY->setMinimumHeight(150);
    PLETHY->SetBlockZoom(true);

    portname="";


    gGraphWindow * graphs[]={PLETHY,PULSE,SPO2};
    int ss=sizeof(graphs)/sizeof(gGraphWindow *);

    for (int i=0;i<ss;i++) {
        AddGraph(graphs[i]);
        for (int j=0;j<ss;j++) {
            if (graphs[i]!=graphs[j])
                graphs[i]->LinkZoom(graphs[j]);
        }
        gSplitter->addWidget(graphs[i]);
        graphs[i]->SetSplitter(gSplitter);
    }

    on_RefreshPortsButton_clicked();
}

Oximetry::~Oximetry()
{
    delete ui;
}

void Oximetry::on_RefreshPortsButton_clicked()
{
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    ui->SerialPortsCombo->clear();
    int z=0;
    QString firstport;
    bool current_found=false;
    for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).friendName.toUpper().contains("USB")) {
            if (firstport.isEmpty()) firstport=ports.at(i).physName;
            if (!portname.isEmpty() && ports.at(i).physName==portname) current_found=true;
            ui->SerialPortsCombo->addItem(ports.at(i).physName);
            z++;
        }
        //qDebug() << "port name:" << ports.at(i).portName;
        qDebug() << "Serial Port:" << ports.at(i).physName << ports.at(i).friendName;
        //qDebug() << "enumerator name:" << ports.at(i).enumName;
        //qDebug() << "vendor ID:" << QString::number(ports.at(i).vendorID, 16);
        //qDebug() << "product ID:" << QString::number(ports.at(i).productID, 16);
        //qDebug() << "===================================";
    }

    if (z>0) {
        ui->RunButton->setEnabled(true);
        ui->ImportButton->setEnabled(true);
        if (!current_found) portname=firstport;
    } else {
        ui->RunButton->setEnabled(false);
        ui->ImportButton->setEnabled(false);
        portname="";
    }
}
void Oximetry::RedrawGraphs()
{
    for (list<gGraphWindow *>::iterator g=Graphs.begin();g!=Graphs.end();g++) {
        (*g)->updateGL();
    }
}

void Oximetry::on_RunButton_toggled(bool checked)
{
    if (checked) {
        ui->RunButton->setText("&Stop");
        ui->SerialPortsCombo->setEnabled(false);
        // Disconnect??
        port=new QextSerialPort(portname,QextSerialPort::EventDriven);
        port->setBaudRate(BAUD19200);
        port->setFlowControl(FLOW_OFF);
        port->setParity(PAR_ODD);
        port->setDataBits(DATA_8);
        port->setStopBits(STOP_1);
        if (port->open(QIODevice::ReadWrite) == true) {
            connect(port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
            connect(port, SIGNAL(dsrChanged(bool)), this, SLOT(onDsrChanged(bool)));
            if (!(port->lineStatus() & LS_DSR))
                qDebug() << "warning: device is not turned on";
            qDebug() << "listening for data on" << port->portName();
        } else {
            qDebug() << "device failed to open:" << port->errorString();
        }
        portmode=PM_LIVE;
    } else {
        ui->RunButton->setText("&Start");
        ui->SerialPortsCombo->setEnabled(true);
        delete port;
        port=NULL;

    }
}

void Oximetry::on_SerialPortsCombo_activated(const QString &arg1)
{
    portname=arg1;
}

void Oximetry::onReadyRead()
{
    static int lastpulse=-1, lastspo2=-1;
    static int lastsize=-1;
    QByteArray bytes;
    int a = port->bytesAvailable();
    bytes.resize(a);
    port->read(bytes.data(), bytes.size());

    static qint64 starttime=0;
    static qint64 lasttime=0;
    static int idx=0;
    if (!lasttime) {
        lasttime=QDateTime::currentMSecsSinceEpoch();
        starttime=lasttime;

        plethy->SetRealMinX(double(lasttime)/86400000.0);
        plethy->SetRealMaxX(double(lasttime+1800000)/86400000.0);
        plethy->SetMinX(double(lasttime)/86400000.0);
        plethy->SetMaxX(double(lasttime+600000)/86400000.0);
        plethy->SetRealMinY(0);
        plethy->SetRealMaxY(120);
        plethy->SetMaxY(120);
        plethy->SetMinY(0);
        PLETHY->MinX();
        PLETHY->MaxX();
        PLETHY->RealMinX();
        PLETHY->RealMaxX();
        PLETHY->MinY();
        PLETHY->MaxY();
        plethy->SetReady(true);
        plethy->SetVC(1);
        plethy->np[0]=600;
    }

    if (bytes.size()==3) {
        EventDataType d=bytes[1] & 0x7f;
        plethy->point[0][idx].setX(double(lasttime)/86400000.0);
        plethy->point[0][idx++].setY(d);
        lasttime+=1000;
        if (idx>600) {
            idx=0;
            lasttime=starttime;
        }
        PLETHY->updateGL();
    }
    /*if (portmode!=PM_RECORDING) {
        return;
    }
    if (bytes.size()==3) { // control & waveform bytes
        if ((bytes[0]==0xf0) && (bytes[1]==0x80) && (bytes[2]==0x00)) portmode==PM_LIVE;
        //qDebug() << "A=" << int(bytes[0]) << "B=" << int(bytes[1]) << "C=" << int(bytes[2]);
        // Pulse data..
    } else if (bytes.size()==2) { // Data bytes in live mode
        // Plethy data
        if (lastpulse!=bytes[0])
            qDebug() << "Pulse=" << int(bytes[0]);
        if (lastspo2!=bytes[1])
            qDebug() << "SpO2=" << int(bytes[1]);

        lastpulse=bytes[0];
        lastspo2=bytes[1];
    } else {
        qDebug() << "bytes read:" << bytes.size(); */
        //QString aa;
        //for (int i=0;i<bytes.size();i++)
          //  aa+=QString::number((unsigned char)bytes[i],16)+" ";

        //qDebug() << hex << aa;
        /*QByteArray b;
        b.resize(3);
        b[0]=0xf6;
        b[1]=0xf6;
        b[2]=0xf6;

        port->write(b);
        portmode=PM_LIVE;
        qDebug() << "Killing Record Retrieval Mode";

    } */
    lastsize=bytes.size();
}
void Oximetry::onDsrChanged(bool status)
{
    if (status)
        qDebug() << "device was turned on";
    else
        qDebug() << "device was turned off";
}
extern QProgressBar *qprogress;
extern QLabel *qstatus;

// Move this code to CMS50 Importer??
void Oximetry::on_ImportButton_clicked()
{
    Machine *mach=profile->GetMachine(MT_OXIMETER);
    if (!mach) {
        CMS50Loader *l=dynamic_cast<CMS50Loader *>(GetLoader("CMS50"));
        if (l) {
            mach=l->CreateMachine(profile);
        }

        qDebug() << "Needed to create Oximeter device";
    }
    unsigned char b1[2];
    unsigned char b2[3];
    unsigned char rb[0x20];
    b1[0]=0xf5;
    b1[1]=0xf5;
    b2[0]=0xf6;
    b2[1]=0xf6;
    b2[2]=0xf6;
    unsigned char * buffer=NULL;
    ui->SerialPortsCombo->setEnabled(false);
    ui->RunButton->setText("&Start");
    ui->RunButton->setChecked(false);

    // port->write((char *)b1,2);
   // return;
    if (port) {
        port->close();
        delete port;
    }
    // Disconnect??
    //qDebug() << "Initiating Polling Mode";
    port=new QextSerialPort(portname,QextSerialPort::Polling);
    port->setBaudRate(BAUD19200);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_ODD);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_1);
    port->setTimeout(500);
    if (port->open(QIODevice::ReadWrite) == true) {
       // if (!(port->lineStatus() & LS_DSR))
         //   qDebug() << "warning: device is not turned on"; // CMS50 doesn't do this..

    } else {
        delete port;
        port=NULL;
        ui->SerialPortsCombo->setEnabled(true);

        return;
    }
    bool done=false;
    int res;
    int blocks=0;
    unsigned int bytes=0;
    QString aa;
    port->flush();
    bool oneoff=false;

    //qprogress->reset();
    qstatus->setText("Importing");
    qprogress->setValue(0);
    qprogress->show();
    int fails=0;
    while (!done) {

        //port->setRts(true);
        if (port->write((char *)b1,2)==-1) {
            qDebug() << "Couldn't write 2 lousy bytes to CMS50";
        }
        //usleep(500);
       // port->setRts(false);
        //qDebug() << "Available " << port->bytesAvailable();
        blocks=0;
        int startpos=0;
        unsigned int length=0;
        do {
            bool fnd=false;
            res=port->read((char *)rb,0x20);

            if (blocks==0) {
                for (int i=0;i<res-1;i++) {
                    if ((rb[i]==0xf2) && (rb[i+1]==0x80)) {
                        fnd=true;
                        startpos=i+10;
                        break;
                    }
                }
                if (!fnd) {
                    qDebug() << "Retrying..";
                    fails++;
                    break; // reissue the F5 and try again
                }

                length=(rb[startpos] ^ 0x80)<< 7 | (rb[startpos+1]&0x7f);
                startpos+=2;
                if (!(rb[startpos]&0x80)) {
                    length|=(rb[startpos]&0x7f) << 14;
                    startpos++;
                } else oneoff=true;
                buffer=new unsigned char [length+32];

                //qDebug() << length << startpos;
                bytes+=res-startpos;
                memcpy((char *)buffer,(char *)&rb[startpos],bytes);
            } else {
                qprogress->setValue((100.0/length)*bytes);
                memcpy((char *)&buffer[bytes],(char *)rb,res);
                bytes+=res;
            }
            //aa="";
            //for (int i=0;i<res;i++) {
            //    aa+=QString::number(unsigned(rb[i]),16)+" ";
            //}
            //qDebug() << aa;

            blocks++;
            if (res<0x20) {
                done=true;
                break;
            }
        } while (bytes<length);
        if (done) break;
        if (fails>4) break;
    }
    if (done) {
        if (oneoff) bytes--; // this is retarded..

        QDateTime date=QDateTime::currentDateTimeUtc();
        SessionID sid=date.toTime_t();
        Session *sess=new Session(mach,sid);
        qDebug() << "Read " << bytes << "Bytes";
        qDebug() << "Creating session " << sid;
        char pulse,spo2,lastpulse=-1,lastspo2=-1;

        qint64 tt=sid-(bytes/3);
        tt*=1000;
        sess->set_first(tt);
        EventDataType data;
        unsigned i=0;
        while (i<bytes) {
            if (buffer[i++]!=0xf0) {
                qDebug() << "Faulty PulseOx data";
                continue;
            }
            pulse=buffer[i++] ^ 0x80;
            spo2=buffer[i++];
            if (pulse!=0 && pulse!=lastpulse) {
                data=pulse;
                sess->AddEvent(new Event(tt,OXI_Pulse,&data,1));
                //qDebug() << "Pulse: " << int(pulse);
            }
            if (spo2 != 0 && spo2!=lastspo2) {
                data=spo2;
                sess->AddEvent(new Event(tt,OXI_SPO2,&data,1));
                //qDebug() << "SpO2: " << int(spo2);
            }

            lastpulse=pulse;
            lastspo2=spo2;
            tt+=1000;
        }
        data=pulse;
        sess->AddEvent(new Event(tt,OXI_Pulse,&data,1));
        data=spo2;
        sess->AddEvent(new Event(tt,OXI_SPO2,&data,1));
        sess->summary[OXI_PulseMin]=sess->min_event_field(OXI_Pulse,0);
        sess->summary[OXI_PulseMax]=sess->max_event_field(OXI_Pulse,0);
        sess->summary[OXI_PulseAverage]=sess->weighted_avg_event_field(OXI_Pulse,0);
        sess->summary[OXI_SPO2Min]=sess->min_event_field(OXI_SPO2,0);
        sess->summary[OXI_SPO2Max]=sess->max_event_field(OXI_SPO2,0);
        sess->summary[OXI_SPO2Average]=sess->weighted_avg_event_field(OXI_SPO2,0);
        sess->SetChanged(true);
        mach->AddSession(sess,profile);
        mach->Save();
        // Output Pulse & SPO2 here..
        delete [] buffer;
        port->write((char *)b2,3);
    }
    delete port;
    port=NULL;
    qprogress->hide();
    ui->SerialPortsCombo->setEnabled(true);
    qstatus->setText("Ready");
}

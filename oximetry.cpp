#include <QApplication>
#include <QDebug>
#include <QProgressBar>
#include <QMessageBox>

#include "oximetry.h"
#include "ui_oximetry.h"
#include "qextserialport/qextserialenumerator.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/event.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gBarChart.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"
#include "Graphs/gFooBar.h"

Oximetry::Oximetry(QWidget *parent,QGLWidget * shared) :
    QWidget(parent),
    ui(new Ui::Oximetry)
{
    ui->setupUi(this);
    port=NULL;
    portname="";
    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);
    if (!profile) {
        qWarning("Couldn't get profile.. Have to abort!");
        exit(-1);
    }
    mach=profile->GetMachine(MT_OXIMETER);
    if (!mach) {
        CMS50Loader *l=dynamic_cast<CMS50Loader *>(GetLoader("CMS50"));
        if (l) {
            mach=l->CreateMachine(profile);
        }
        qDebug() << "Create Oximeter device";
    }

    // Create dummy day & session for holding eventlists.
    day=new Day(mach);
    session=new Session(mach,0);
    day->AddSession(session);

    splitter=ui->graphLayout;
    //splitter=new QSplitter(Qt::Vertical,ui->scrollArea);
    //gSplitter->setStyleSheet("QSplitter::handle { background-color: 'dark grey'; }");
    //gSplitter->setHandleWidth(2);
    //ui->graphLayout->addWidget(splitter);

    // Create the Event Lists to store / import data
    ev_plethy=new EventList(OXI_Plethysomogram,EVL_Event,1,0,0,0,1000.0/50.0);
    session->eventlist[OXI_Plethysomogram].push_back(ev_plethy);

    ev_pulse=new EventList(OXI_Pulse,EVL_Event,1);
    session->eventlist[OXI_Pulse].push_back(ev_pulse);

    ev_spo2=new EventList(OXI_SPO2,EVL_Event,1);
    session->eventlist[OXI_SPO2].push_back(ev_spo2);

    QWidget * parental=ui->scrollArea;
    plethy=new gLineChart(OXI_Plethysomogram,Qt::black,false,true);
    AddGraph(PLETHY=new gGraphWindow(parental,tr("Plethysomogram"),shared));
    plethy->SetDay(day);

    pulse=new gLineChart(OXI_Pulse,Qt::red,true);
    AddGraph(PULSE=new gGraphWindow(parental,tr("Pulse Rate"),shared));
    pulse->SetDay(day);

    spo2=new gLineChart(OXI_SPO2,Qt::blue,true);
    AddGraph(SPO2=new gGraphWindow(parental,tr("SPO2"),shared));
    spo2->SetDay(day);

    for (int i=0;i<Graphs.size();i++) {
        for (int j=0;j<Graphs.size();j++) {
            if (Graphs[i]!=Graphs[j])
                Graphs[i]->LinkZoom(Graphs[j]);
        }
        Graphs[i]->AddLayer(new gYAxis());
        Graphs[i]->AddLayer(new gXAxis());
        //Graphs[i]->AddLayer(new gFooBar());

        splitter->addWidget(Graphs[i]);
    }
    PLETHY->AddLayer(plethy);
    PLETHY->AddLayer(new gFooBar());
    PULSE->AddLayer(pulse);
    SPO2->AddLayer(spo2);

    for (int i=0;i<Graphs.size();i++) {
        Graphs[i]->setMinimumHeight(150);
        Graphs[i]->SetSplitter(splitter);
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

    // Windows build mixes these up
#ifdef Q_WS_WIN32
#define qesPORTNAME portName
#else
#define qesPORTNAME physName
#endif
    for (int i = 0; i < ports.size(); i++) {
        if (ports.at(i).friendName.toUpper().contains("USB")) {
            if (firstport.isEmpty()) firstport=ports.at(i). qesPORTNAME;
            if (!portname.isEmpty() && ports.at(i).qesPORTNAME==portname) current_found=true;
            ui->SerialPortsCombo->addItem(ports.at(i).qesPORTNAME);
            z++;
        }
        //qDebug() << "Serial Port:" << ports.at(i).qesPORTNAME << ports.at(i).friendName;
        qDebug() << "port name:" << ports.at(i).portName;
        qDebug() << "phys name:" << ports.at(i).physName;
        qDebug() << "friendly name:" << ports.at(i).friendName;
        qDebug() << "enumerator name:" << ports.at(i).enumName;
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
    for (QVector<gGraphWindow *>::iterator g=Graphs.begin();g!=Graphs.end();g++) {
        (*g)->updateGL();
    }
}

void Oximetry::on_RunButton_toggled(bool checked)
{
    if (checked) {
        lasttime=0;
        lastpulse=-1;
        lastspo2=-1;

        // Wipe any current data
        ev_plethy->getData().clear();
        ev_plethy->getTime().clear();
        ev_plethy->setCount(0);
        ev_pulse->getData().clear();
        ev_pulse->getTime().clear();
        ev_pulse->setCount(0);
        ev_spo2->getData().clear();
        ev_spo2->getTime().clear();
        ev_spo2->setCount(0);

        lasttime=QDateTime::currentMSecsSinceEpoch();
        starttime=lasttime;

        day->setFirst(lasttime);
        day->setLast(lasttime+30000);
        session->set_first(lasttime);
        session->set_last(lasttime+30000);

        ev_plethy->setFirst(lasttime);
        ev_plethy->setLast(lasttime+3600000);
        PLETHY->SetMinX(lasttime);
        PLETHY->SetMaxX(lasttime+30000);

        ev_pulse->setFirst(lasttime);
        ev_pulse->setLast(lasttime+3600000);
        PULSE->SetMinX(lasttime);
        PULSE->SetMaxX(lasttime+30000);

        ev_spo2->setFirst(lasttime);
        ev_spo2->setLast(lasttime+3600000);
        SPO2->SetMinX(lasttime);
        SPO2->SetMaxX(lasttime+30000);

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

        ev_spo2->setLast(lasttime);
        ev_pulse->setLast(lasttime);
        ev_plethy->setLast(lasttime);
        day->setLast(lasttime);
        session->set_last(lasttime);

        SPO2->SetMinX(ev_spo2->first());
        SPO2->SetMaxX(lasttime);
        PULSE->SetMinX(ev_pulse->first());
        PULSE->SetMaxX(lasttime);
        PLETHY->SetMinX(ev_plethy->first());
        PLETHY->SetMaxX(lasttime);

        PLETHY->updateGL();
        SPO2->updateGL();
        PULSE->updateGL();
    }
}

void Oximetry::on_SerialPortsCombo_activated(const QString &arg1)
{
    portname=arg1;
}
void Oximetry::UpdatePlethy(qint8 d)
{
    ev_plethy->AddEvent(lasttime,d);
    lasttime+=20;  // 50 samples per second
    //ev_plethy->setLast(lasttime);
    //if (plethy->RealMaxX()-plethy->RealMinX()>(1.0/(24.0*120.0))) {
    //    plethy->SetMinX(lasttime/86400000.0-(1.0/(24.0*120.0)));
    //    plethy->SetMaxX(lasttime/86400000.0);
    //}

    //plethy->setMinY(ev_plethy->min());
    //plethy->setMaxY(ev_plethy->max());
    //plethy->setMinY(ev_plethy->min());
    //plethy->setMaxY(ev_plethy->max());
    PLETHY->MinX();
    PLETHY->MaxX();
    //PLETHY->MinY();
    //PLETHY->MaxY();
    PLETHY->SetMinY(ev_plethy->min());
    PLETHY->SetMaxY(ev_plethy->max());
    PULSE->SetMinY(ev_pulse->min());
    PULSE->SetMaxY(ev_pulse->max());
    SPO2->SetMinY(ev_spo2->min());
    SPO2->SetMaxY(ev_spo2->max());
    //PLETHY->MaxY();
    PLETHY->SetMaxX(lasttime);
    PLETHY->SetMinX(lasttime-30000);
    PULSE->SetMaxX(lasttime);
    PULSE->SetMinX(lasttime-30000);
    SPO2->SetMaxX(lasttime);
    SPO2->SetMinX(lasttime-30000);
    session->set_last(lasttime);
    day->setLast(lasttime);
}
bool Oximetry::UpdatePulse(qint8 pul)
{
    bool ret=false;

    // Don't block zeros.. If the data is used, it's needed
    // Can make the graph can skip them.
    if (lastpulse!=pul)
    {
        ev_pulse->AddEvent(lasttime,pul);
        ret=true;
        //qDebug() << "Pulse=" << int(bytes[0]);
    }
    lastpulse=pul;
    return ret;
}
bool Oximetry::UpdateSPO2(qint8 sp)
{
    bool ret=false;

    if (lastspo2!=sp)
    {
        ev_spo2->AddEvent(lasttime,sp);
        ret=true;
        //qDebug() << "SpO2=" << int(bytes[1]);
    }

    lastspo2=sp;
    return ret;
}

void Oximetry::onReadyRead()
{
    QByteArray bytes;
    int a = port->bytesAvailable();
    bytes.resize(a);
    port->read(bytes.data(), bytes.size());

    int i=0;
    bool redraw_pulse,redraw_spo2;
    redraw_pulse=redraw_spo2=false;
    while (i<bytes.size()) {
        if (bytes[i]&0x80) {
            EventDataType d=bytes[i+1] & 0x7f;
            UpdatePlethy(d);
            //qDebug() << d;
            i+=3;
        } else {
            if (UpdatePulse(bytes[i])) redraw_pulse=true;
            if (UpdateSPO2(bytes[i+1])) redraw_spo2=true;
            i+=2;
        }
    }
    PLETHY->updateGL();
    if (redraw_pulse)
        PULSE->updateGL();
    if (redraw_spo2)
        SPO2->updateGL();

}
void Oximetry::onDsrChanged(bool status) // Doesn't work for CMS50's
{
    if (status)
        qDebug() << "device was turned on";
    else
        qDebug() << "device was turned off";
}

extern QProgressBar *qprogress;
extern QLabel *qstatus;


void DumpBytes(int blocks, unsigned char * b,int len)
{
    QString a=QString::number(blocks,16)+": Bytes "+QString::number(len,16)+": ";
    for (int i=0;i<len;i++) {
        a.append(QString::number(b[i],16)+" ");
    }
    qDebug() << a;
}

// Move this code to CMS50 Importer??
void Oximetry::on_ImportButton_clicked()
{
    ui->ImportButton->setDisabled(true);
    QMessageBox msgbox(QMessageBox::Information,"Importing","Please Wait",QMessageBox::NoButton,this);
    msgbox.show();
    QApplication::processEvents();
    const int rb_size=0x200;
    static unsigned char b1[2]={0xf5,0xf5};
    static unsigned char b2[3]={0xf6,0xf6,0xf6};
    static unsigned char rb[rb_size];

    unsigned char * buffer=NULL;
    ui->SerialPortsCombo->setEnabled(false);
    ui->RunButton->setText("&Start");
    ui->RunButton->setChecked(false);

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
        if (port->write((char *)b1,2)==-1) {
            qDebug() << "Couldn't write 2 lousy bytes to CMS50";
        }
        blocks=0;
        int startpos=0;
        unsigned int length=0;
        int dr;
        int ec;
        do {
            bool fnd=false;
            dr=0;
            ec=0;
            do {
                res=port->read((char *)rb,rb_size);
                DumpBytes(blocks,rb,res);
                if (blocks>0) break;
                if (res<=0) ec++;
                dr+=res;
            } while ((res<=5) && (dr<0x200) && (ec<5));

            //if (res>5) DumpBytes(blocks,rb,res);


            done=false;
            if (blocks==0) {
                if (res>5)
                for (int i=0;i<res-2;i++) {
                    if ((rb[i]==0xf2) && (rb[i+1]==0x80) && (rb[i+2]==0x00)) {
                        fnd=true;
                        startpos=i+9;
                        break;
                    }
                }
                if (!fnd) {

                    qDebug() << "Retrying..";
                    fails++;
                    break; // reissue the F5 and try again
                } else {
                    qDebug() << "Found";
                }

                //  84 95 7c
                // 0x10556 bytes..
                // 0x82b blocks
                length=0;
                while (rb[startpos]!=0xf0) {
                    length=(length << 7) | (rb[startpos++] & 0x7f);
                }
                //length=(rb[startpos++] & 0x7f) << 14;
                //length|=(rb[startpos++] & 0x7f) << 7;
                //length|=(rb[startpos++] & 0x7f);
                //if (!(rb[startpos]&0x80)) {
                //    length <<= 8;
                //    length|=rb[startpos++];
                //}
                //length=(rb[startpos] ^ 0x80)<< 7 | (rb[startpos+1]);
                //startpos+=2;
                //if (!(rb[startpos]&0x80)) {
                //    length|=(rb[startpos]&0x7f) << 14;
                //    startpos++;
                //} else oneoff=true;
                buffer=new unsigned char [length+32];

                //qDebug() << length << startpos;
                bytes+=res-startpos;
                memcpy((char *)buffer,(char *)&rb[startpos],bytes);
                qprogress->setValue((75.0/length)*bytes);
                QApplication::processEvents();
            } else {
                qprogress->setValue((75.0/length)*bytes);
                QApplication::processEvents();

                memcpy((char *)&buffer[bytes],(char *)rb,res);
                bytes+=res;
            }

            blocks++;
            if (res<rb_size) {
                qDebug() << "Read "<< bytes << " bytes of " << length;
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
        session->SetSessionID(sid);
        qDebug() << "Read " << bytes << "Bytes";
        qDebug() << "Creating session " << sid;
        unsigned short pulse,spo2,lastpulse=0,lastspo2=0;

        qint64 tt=sid-(bytes/3);
        tt*=1000;
        session->set_first(tt);
        ev_pulse->setMin(999999);
        ev_pulse->setMax(-999999);
        ev_spo2->setMin(999999);
        ev_spo2->setMax(-999999);

        ev_pulse->setFirst(tt);
        ev_spo2->setFirst(tt);

        EventList *oxf1=NULL,*oxf2=NULL;
        EventDataType data;
        unsigned i=0;
        const int rb_size=60; // last rb_size seconds of data
        unsigned rb_pulse[rb_size]={0};
        unsigned rb_spo2[rb_size]={0};
        int rb_pos=0;
        while (i<bytes) {
            if (buffer[i++]!=0xf0) {
                qDebug() << "Faulty PulseOx data";
                continue;
            }
            pulse=buffer[i++] & 0x7f;
            spo2=buffer[i++];
            if (pulse!=lastpulse) {
                data=pulse;
                ev_pulse->AddEvent(tt,data);
                //qDebug() << "Pulse: " << int(pulse);
            }
            if (spo2!=lastspo2) {
                data=spo2;
                ev_spo2->AddEvent(tt,data);
                //qDebug() << "SpO2: " << int(spo2);
            }

            lastpulse=pulse;
            lastspo2=spo2;

            rb_pulse[rb_pos]=(unsigned)pulse;
            rb_spo2[rb_pos]=(unsigned)spo2;

            unsigned int min=255,max=0;
            for (int k=rb_pos;k>rb_pos-4;k--) {
                int j=abs(k % rb_size);
                if (rb_pulse[j]<min) min=rb_pulse[j];
                if (rb_pulse[j]>min) max=rb_pulse[j];
            }
            if (min>0 && max>0) {
                int drop=max-min;
                if (drop>6) {
                    if (!oxf1) {
                        oxf1=new EventList(OXI_PulseChange,EVL_Event);
                        session->eventlist[OXI_PulseChange].push_back(oxf1);
                    }
                    oxf1->AddEvent(tt,drop);
                }
            }

            min=255,max=0;
            for (int k=rb_pos;k>rb_pos-10;k--) {
                int j=abs(k % rb_size);
                if (rb_spo2[j]<min) min=rb_spo2[j];
                if (rb_spo2[j]>min) max=rb_spo2[j];
            }
            if (min>0 && max>0) {
                int drop=max-min;
                if (drop>4) {
                    if (!oxf1) {
                        oxf2=new EventList(OXI_SPO2Drop,EVL_Event);
                        session->eventlist[OXI_SPO2Drop].push_back(oxf2);
                    }
                    oxf2->AddEvent(tt,drop);
                }
            }



            ++rb_pos;
            rb_pos=rb_pos % rb_size;

            tt+=1000;
            qprogress->setValue(75+(25.0/bytes)*i);
            QApplication::processEvents();
        }
        ev_pulse->AddEvent(tt,pulse);
        ev_spo2->AddEvent(tt,spo2);
        session->set_last(tt);

        session->setMin(OXI_Pulse,ev_pulse->min());
        session->setMax(OXI_Pulse,ev_pulse->max());
        session->avg(OXI_Pulse);
        session->p90(OXI_Pulse);
        session->cph(OXI_Pulse);
        session->wavg(OXI_Pulse);

        session->setMin(OXI_SPO2,ev_pulse->min());
        session->setMax(OXI_SPO2,ev_pulse->max());
        session->avg(OXI_SPO2);
        session->p90(OXI_SPO2);
        session->cph(OXI_SPO2);
        session->wavg(OXI_SPO2);

        session->SetChanged(true);
        mach->AddSession(session,profile);
        mach->Save();
        // Output Pulse & SPO2 here..
        delete [] buffer;
        port->write((char *)b2,3);

        // Need to create a new session as this one got pinched.
        session=new Session(mach,0);
        day->getSessions().clear();
        day->AddSession(session);

        // As did these
        ev_plethy=new EventList(OXI_Plethysomogram,EVL_Waveform,1,0,0,0,1.0/50.0);
        session->eventlist[OXI_Plethysomogram].push_back(ev_plethy);

        ev_pulse=new EventList(OXI_Pulse,EVL_Event,1);
        session->eventlist[OXI_Pulse].push_back(ev_pulse);

        ev_spo2=new EventList(OXI_SPO2,EVL_Event,1);
        session->eventlist[OXI_SPO2].push_back(ev_spo2);
    }
    delete port;
    port=NULL;
    msgbox.hide();
    qprogress->hide();
    ui->SerialPortsCombo->setEnabled(true);
    qstatus->setText("Ready");
    ui->ImportButton->setDisabled(false);

}


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

    AddData(pulse=new EventData(OXI_Pulse));
    PULSE=new gGraphWindow(gSplitter,tr("Pulse Rate"),(QGLWidget *)NULL);
    PULSE->AddLayer(new gXAxis());
    PULSE->AddLayer(new gYAxis());
    PULSE->AddLayer(new gFooBar());
    PULSE->AddLayer(new gLineChart(pulse,Qt::red,65536,false,false,false));
    PULSE->setMinimumHeight(150);

    AddData(spo2=new EventData(OXI_SPO2));
    SPO2=new gGraphWindow(gSplitter,tr("SPO2"),PULSE);
    SPO2->AddLayer(new gXAxis());
    SPO2->AddLayer(new gYAxis());
    SPO2->AddLayer(new gFooBar());
    SPO2->AddLayer(new gLineChart(spo2,Qt::blue,65536,false,false,false));
    SPO2->setMinimumHeight(150);

    pulse->AddSegment(1000000);
    pulse->np.push_back(0);
    spo2->AddSegment(1000000);
    spo2->np.push_back(0);


    AddData(plethy=new WaveData(OXI_Plethy));
    plethy->AddSegment(1000000);
    plethy->np.push_back(0);
    plethy->SetMaxY(100);
    plethy->SetMinY(0);
    PLETHY=new gGraphWindow(gSplitter,tr("Plethysomogram"),PULSE);
    //PLETHY->AddLayer(new gXAxis());
    PLETHY->AddLayer(new gYAxis());
    PLETHY->AddLayer(new gFooBar());
    PLETHY->AddLayer(new gLineChart(plethy,Qt::black,65536,true,false,false));
    PLETHY->setMinimumHeight(150);
    //PLETHY->SetBlockZoom(true);

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
        lasttime=0;
        lastpulse=-1;
        lastspo2=-1;
        plethy->np[0]=0;
        lasttime=QDateTime::currentMSecsSinceEpoch();
        starttime=lasttime;

        plethy->SetRealMinX(double(lasttime)/86400000.0);
        plethy->SetRealMaxX(double(lasttime+60000)/86400000.0);
        plethy->SetMinX(double(lasttime)/86400000.0);
        plethy->SetMaxX(double(lasttime+30000)/86400000.0);
        plethy->SetRealMinY(0);
        plethy->SetRealMaxY(130);
        plethy->SetMaxY(130);
        plethy->SetMinY(0);
        PLETHY->MinX();
        PLETHY->MaxX();
        PLETHY->RealMinX();
        PLETHY->RealMaxX();
        PLETHY->MinY();
        PLETHY->MaxY();
        plethy->SetReady(true);
        plethy->SetVC(1);
        plethy->np[0]=0;


        pulse->SetRealMinX(double(lasttime)/86400000.0);
        pulse->SetRealMaxX(double(lasttime)/86400000.0+(1.0/24.0));
        pulse->SetMinX(double(lasttime)/86400000.0);
        pulse->SetMaxX(double(lasttime)/86400000.0+(1.0/24.0));
        pulse->SetRealMinY(40);
        pulse->SetRealMaxY(120);
        pulse->SetMaxY(120);
        pulse->SetMinY(40);
        pulse->np[0]=0;
        pulse->SetReady(true);
        pulse->SetVC(1);
        PULSE->MinX();
        PULSE->MaxX();
        PULSE->RealMinX();
        PULSE->RealMaxX();
        PULSE->MinY();
        PULSE->MaxY();

        spo2->SetRealMinX(double(lasttime)/86400000.0);
        spo2->SetRealMaxX(double(lasttime)/86400000.0+(1.0/24.0));
        spo2->SetMinX(double(lasttime)/86400000.0);
        spo2->SetMaxX(double(lasttime)/86400000.0+(1.0/24.0));
        spo2->SetRealMinY(40);
        spo2->SetRealMaxY(100);
        spo2->SetMaxY(100);
        spo2->SetMinY(40);
        spo2->np[0]=0;
        spo2->SetReady(true);
        spo2->SetVC(1);
        SPO2->MinX();
        SPO2->MaxX();
        SPO2->RealMinX();
        SPO2->RealMaxX();
        SPO2->MinY();
        SPO2->MaxY();

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

        spo2->point[0][spo2->np[0]].setX(lasttime/86400000.0);
        spo2->point[0][spo2->np[0]++].setY(lastspo2);
        pulse->point[0][pulse->np[0]].setX(lasttime/86400000.0);
        pulse->point[0][pulse->np[0]++].setY(lastpulse);

        pulse->SetRealMaxX(plethy->RealMaxX());
        spo2->SetRealMaxX(plethy->RealMaxX());

        spo2->SetMinX(plethy->RealMinX());
        spo2->SetMaxX(plethy->RealMaxX());
        pulse->SetMinX(plethy->RealMinX());
        pulse->SetMaxX(plethy->RealMaxX());
        plethy->SetMinX(plethy->RealMinX());

        PULSE->RealMaxX();
        PULSE->MaxX();
        PULSE->MinX();
        SPO2->RealMaxX();
        SPO2->MaxX();
        SPO2->MinX();
        PLETHY->MinX();
        PLETHY->MaxX();


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
    plethy->point[0][plethy->np[0]].setX(double(lasttime)/86400000.0);
    plethy->point[0][plethy->np[0]++].setY(d);
    lasttime+=20;  // 50 samples per second?
    plethy->SetRealMaxX(lasttime/86400000.0);
    if (plethy->RealMaxX()-plethy->RealMinX()>(1.0/(24.0*120.0))) {
        plethy->SetMinX(lasttime/86400000.0-(1.0/(24.0*120.0)));
        plethy->SetMaxX(lasttime/86400000.0);
    }
    PLETHY->MinX();
    PLETHY->MaxX();
    PLETHY->RealMaxX();
    if (plethy->np[0]>max_data_points) {
        //TODO: Stop Serial recording..

        // for now overwrite..
        plethy->np[0]=0;
        lasttime=0;
    }
    //PLETHY->updateGL(); // Move this to a timer.
}
bool Oximetry::UpdatePulseSPO2(qint8 pul,qint8 sp)
{
    bool ret=false;

    // Don't block zeros.. If the data is used, it's needed
    // Can make the graph can skip them.
    if (lastpulse!=pul) {
        pulse->point[0][pulse->np[0]].setX(double(lasttime)/86400000.0);
        pulse->point[0][pulse->np[0]++].setY(pul);
        PULSE->updateGL();
        if (pulse->np[0]>max_data_points) {
            //TODO: Stop Serial recording..

            // for now overwrite..
            pulse->np[0]=0;
            lasttime=0;
        }
        ret=true;
        //qDebug() << "Pulse=" << int(bytes[0]);
    }
    if (lastspo2!=sp) {
        spo2->point[0][spo2->np[0]].setX(double(lasttime)/86400000.0);
        spo2->point[0][spo2->np[0]++].setY(sp);
        SPO2->updateGL();
        if (spo2->np[0]>max_data_points) {
            //TODO: Stop Serial recording..

            // for now overwrite..
            spo2->np[0]=0;
            lasttime=0;
        }
        ret=true;
        //qDebug() << "SpO2=" << int(bytes[1]);
    }

    lastpulse=pul;
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
    bool redraw_ps=false;
    while (i<bytes.size()) {
        if (bytes[i]&0x80) {
            EventDataType d=bytes[i+1] & 0x7f;
            UpdatePlethy(d);
            i+=3;
        } else {
            if (UpdatePulseSPO2(bytes[i], bytes[i+1])) redraw_ps=true;
            i+=2;
        }
    }
    PLETHY->updateGL();
    if (redraw_ps) {
        PULSE->updateGL();
        SPO2->updateGL();
    }
    /*if (bytes.size()==3) {
    } else if (bytes.size()==2) { // Data bytes in live mode
        // Plethy data
    } else {
        //qDebug() << "Got " << bytes.size() << " bytes";
    }*/
    /*QString aa=QString::number(bytes.size(),16)+"bytes: ";
    for (int i=0;i<bytes.size();i++) {
        aa+=" "+QString::number((unsigned char)bytes[i],16);
    }
    qDebug() << aa; */

    //lastsize=bytes.size();
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


void DumpBytes(unsigned char * b,int len)
{
    QString a="Bytes "+QString::number(len,16)+": ";
    for (int i=0;i<len;i++) {
        a.append(QString::number(b[i],16)+" ");
    }
    qDebug() << a;
}

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
            DumpBytes(rb,0x20);

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

#include <QApplication>
#include <QDebug>
#include <QProgressBar>
#include <QMessageBox>
#include <QLabel>

#include "oximetry.h"
#include "ui_oximetry.h"
#include "qextserialport/qextserialenumerator.h"
#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/event.h"
#include "Graphs/gXAxis.h"
#include "Graphs/gSummaryChart.h"
#include "Graphs/gLineChart.h"
#include "Graphs/gYAxis.h"

extern QLabel * qstatus2;

int lastpulse;
SerialOximeter::SerialOximeter(QObject * parent,QString oxiname, QString portname, BaudRateType baud, FlowType flow, ParityType parity, DataBitsType databits, StopBitsType stopbits) :
    QObject(parent),
    session(NULL),pulse(NULL),spo2(NULL),plethy(NULL),m_port(NULL),
    m_opened(false),
    m_oxiname(oxiname),
    m_portname(portname),
    m_baud(baud),
    m_flow(flow),
    m_parity(parity),
    m_databits(databits),
    m_stopbits(stopbits)
{
    machine=PROFILE.GetMachine(MT_OXIMETER);
    if (!machine) {
        // Create generic Serial Oximeter object..
        CMS50Loader *l=dynamic_cast<CMS50Loader *>(GetLoader("CMS50"));
        if (l) {
            machine=l->CreateMachine(p_profile);
        }
        qDebug() << "Create Oximeter device";
    }
}

SerialOximeter::~SerialOximeter()
{
    if (m_opened) {
        if (m_port) m_port->close();
    }
    // free up??
}

bool SerialOximeter::Open(QextSerialPort::QueryMode mode)
{
    if (m_portname.isEmpty()) {
        qDebug() << "Tried to open with empty portname";
        return false;
    }

    qDebug() << "Opening serial port" << m_portname << "in mode" << mode;

    if (m_opened) { // Open already?
        // Just close it
        if (m_port) m_port->close();
    }

    m_mode=mode;

    m_port=new QextSerialPort(m_portname,m_mode);
    m_port->setBaudRate(m_baud);
    m_port->setFlowControl(m_flow);
    m_port->setParity(m_parity);
    m_port->setDataBits(m_databits);
    m_port->setStopBits(m_stopbits);

    if (m_port->open(QIODevice::ReadWrite) == true) {
       // if (m_mode==QextSerialPort::EventDriven)
            connect(m_port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        //connect(port, SIGNAL(dsrChanged(bool)), this, SLOT(onDsrChanged(bool)));
        if (!(m_port->lineStatus() & LS_DSR))
            qDebug() << "check device is turned on";
        qDebug() << "listening for data on" << m_port->portName();
        return m_opened=true;
    } else {
        qDebug() << "device failed to open:" << m_port->errorString();
        return m_opened=false;
    }
}

void SerialOximeter::Close()
{
    qDebug() << "Closing serial port" << m_portname;
    if (!m_opened) return;

    if (m_port) m_port->close();
    if (m_mode==QextSerialPort::EventDriven)
        disconnect(m_port, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    m_opened=false;
}

void SerialOximeter::setPortName(QString portname)
{
    if (m_opened) {
        qDebug() << "Can't change serial PortName settings while port is open!";
        return;
    }
    m_portname=portname;
}

void SerialOximeter::setBaudRate(BaudRateType baud)
{
    if (m_opened) {
        qDebug() << "Can't change serial BaudRate settings while port is open!";
        return;
    }
    m_baud=baud;
}

void SerialOximeter::setFlowControl(FlowType flow)
{
    if (m_opened) {
        qDebug() << "Can't change serial FlowControl settings while port is open!";
        return;
    }
    m_flow=flow;
}

void SerialOximeter::setParity(ParityType parity)
{
    if (m_opened) {
        qDebug() << "Can't change serial Parity settings while port is open!";
        return;
    }
    m_parity=parity;
}

void SerialOximeter::setDataBits(DataBitsType databits)
{
    if (m_opened) {
        qDebug() << "Can't change serial DataBit settings while port is open!";
        return;
    }
    m_databits=databits;
}

void SerialOximeter::setStopBits(StopBitsType stopbits)
{
    if (m_opened) {
        qDebug() << "Can't change serial StopBit settings while port is open!";
        return;
    }
    m_stopbits=stopbits;
}

void SerialOximeter::onReadyRead()
{
    int i=5;
    qDebug() << "Foo"  << i;
}

void SerialOximeter::addPulse(qint64 time, EventDataType pr)
{
    pulse->AddEvent(time,pr);
    session->setCount(OXI_Pulse,pulse->count()); // update the cache
    session->setMin(OXI_Pulse,pulse->min());
    session->setMax(OXI_Pulse,pulse->max());
    session->setLast(OXI_Pulse,time);
    pulse->setLast(time);
    session->set_last(lasttime);
    emit(updatePulse(pr));
}

void SerialOximeter::addSpO2(qint64 time, EventDataType o2)
{
    spo2->AddEvent(time,o2);
    session->setCount(OXI_SPO2,spo2->count()); // update the cache
    session->setMin(OXI_SPO2,spo2->min());
    session->setMax(OXI_SPO2,spo2->max());
    session->setLast(OXI_SPO2,time);
    session->set_last(lasttime);
    spo2->setLast(time);
    emit(updateSpO2(o2));
}

void SerialOximeter::addPlethy(qint64 time, EventDataType pleth)
{
    plethy->AddEvent(time,pleth);
    session->setCount(OXI_Plethy,plethy->count()); // update the cache
    session->setMin(OXI_Plethy,plethy->min());
    session->setMax(OXI_Plethy,plethy->max());
    session->setLast(OXI_Plethy,time);
    session->set_last(lasttime);
    plethy->setLast(time);
}

/*void SerialOximeter::addEvents(EventDataType pr,EventDataType o2, EventDataType pleth)
{
    lasttime=qint64(QDateTime::currentDateTime().toTime_t())*1000L;
    addPulse(lasttime,pr);
    addSpO2(lasttime,o2);
    addPlethy(lasttime,pleth);
    session->set_last(lasttime);
    //emit(dataChanged());
}*/

Session *SerialOximeter::createSession()
{
    if (session) {
         delete session;
    }
    int sid=QDateTime::currentDateTime().toTime_t();
    lasttime=qint64(sid)*1000L;

    session=new Session(machine,sid);
    session->SetChanged(true);

    session->set_first(lasttime);
    pulse=new EventList(EVL_Event);
    spo2=new EventList(EVL_Event);
    plethy=new EventList(EVL_Event);
    session->eventlist[OXI_Pulse].push_back(pulse);
    session->eventlist[OXI_SPO2].push_back(spo2);
    session->eventlist[OXI_Plethy].push_back(plethy);

    session->setFirst(OXI_Pulse,lasttime);
    session->setFirst(OXI_SPO2,lasttime);
    session->setFirst(OXI_Plethy,lasttime);

    pulse->setFirst(lasttime);
    spo2->setFirst(lasttime);
    plethy->setFirst(lasttime);

    emit(sessionCreated(session));
    return session;
}

bool SerialOximeter::startLive()
{
    import_mode=false;
    if (Open(QextSerialPort::EventDriven)) {
        createSession();
        return true;
    } else {
        return false;
    }
}

void SerialOximeter::stopLive()
{
    Close();
    emit(liveStopped(session));
}

CMS50Serial::CMS50Serial(QObject * parent, QString portname="") :
 SerialOximeter(parent,"CMS50", portname, BAUD19200, FLOW_OFF, PAR_ODD, DATA_8, STOP_1)
{
    import_mode=false;
}

CMS50Serial::~CMS50Serial()
{
}

void CMS50Serial::on_import_process()
{
    qDebug() << "CMS50 import complete. Processing" << data.size() << "bytes";
    unsigned char a,pl,o2,lastpl=0,lasto2=0;
    int i=0;
    int size=data.size();
    EventList * pulse=(session->eventlist[OXI_Pulse][0]);
    EventList * spo2=(session->eventlist[OXI_SPO2][0]);
    lasttime=f2time[0].toTime_t();
    session->SetSessionID(lasttime);
    lasttime*=1000;

    session->set_first(lasttime);
    pulse->setFirst(lasttime);
    spo2->setFirst(lasttime);

    EventDataType plmin=999,plmax=0;
    EventDataType o2min=100,o2max=0;
    int plcnt=0,o2cnt=0;
    while (i<(size-3)) {
        a=data.at(i++);
        pl=data.at(i++) ^ 0x80;
        o2=data.at(i++);
        if (pl==0) {
            if (lastpl!=pl) {
                pulse->setLast(lasttime);
                if (pulse->min()<plmin) plmin=pulse->min();
                if (pulse->max()>plmax) plmax=pulse->max();
                plcnt+=pulse->count();
                pulse=new EventList(EVL_Event);
                session->eventlist[OXI_Pulse].push_back(pulse);
            }
        } else pulse->AddEvent(lasttime,pl);
        if (o2==0) {
            if (lasto2!=o2) {
                spo2->setLast(lasttime);
                if (spo2->min()<o2min) o2min=spo2->min();
                if (spo2->max()>o2max) o2max=spo2->max();
                o2cnt+=spo2->count();
                spo2=new EventList(EVL_Event);
                session->eventlist[OXI_SPO2].push_back(spo2);
            }
        } else spo2->AddEvent(lasttime,o2);


        lasttime+=1000;
        emit(updateProgress(float(i)/float(size)));

        lastpl=pl;
        lasto2=o2;
    }
    pulse->setLast(lasttime);
    spo2->setLast(lasttime);
    session->set_last(lasttime);
    session->setMin(OXI_Pulse,plmin);
    session->setMax(OXI_Pulse,plmax);
    session->setMin(OXI_SPO2,o2min);
    session->setMax(OXI_SPO2,o2max);
    session->setCount(OXI_Pulse,plcnt);
    session->setCount(OXI_SPO2,o2cnt);
    session->UpdateSummaries();
    emit(importComplete(session));
    disconnect(this,SIGNAL(importProcess()),this,SLOT(on_import_process()));
}
void CMS50Serial::onReadyRead()
{
    QByteArray bytes;
    int a = m_port->bytesAvailable();
    bytes.resize(a);
    m_port->read(bytes.data(), bytes.size());

    int i=0;

    // Was going out of sync previously.. To fix this unfortunately requires 4.7

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
    qint64 current=QDateTime::currentMSecsSinceEpoch(); //double(QDateTime::currentDateTime().toTime_t())*1000L;
    //qint64 since=current-lasttime;
    //if (since>25)
    lasttime=current;
#endif
    // else (don't bother - we can work some magic at the end of recording.)

    int size=bytes.size();
    // Process all incoming serial data packets
    unsigned char c;
    while (i<bytes.size()) {
        if (import_mode) {
            if (waitf6) { //ack sequence from f6 command.
                if ((unsigned char)bytes.at(i++)==0xf2) {
                    c=bytes.at(i);
                    if (c & 0x80) {
                        int h=(c & 0x1f);
                        int m=(bytes.at(i+1) % 60);
                        if (!((h==0) && (m==0))) { // CMS50E's have a realtime clock (apparently)
                            QDateTime d(PROFILE.LastDay(),QTime(h,m,0));
                            f2time.push_back(d);
                            qDebug() << "Session start (according to CMS50)" << d << h << m;
                        } else {
                            // otherwise pick the first session of the last days data..
                            Day *day=PROFILE.GetDay(PROFILE.LastDay().addDays(-1),MT_CPAP);
                            QDateTime d;

                            if (day) {
                                int ti=day->first()/1000L;

                                d=QDateTime::fromTime_t(ti);
                                qDebug() << "Guessing session starting from CPAP data" << d;
                            } else {
                                qDebug() << "Can't guess start time, defaulting to 6pm yesterday" << d;
                                d=QDateTime::currentDateTime();
                                d.setTime(QTime(18,0,0));
                                d.addDays(-1);
                            }
                            f2time.push_back(d);
                        }
                        i+=2;
                        cntf6++;
                    } else continue;
                } else {
                    if (cntf6>0) {
                        qDebug() << "Got Acknowledge Sequence" << cntf6;
                        i--;
                        if ((i+3)<size) {
                            c=bytes.at(i);

                            datasize=(((unsigned char)bytes.at(i) & 0x3f) << 14) | (((unsigned char)bytes.at(i+1)&0x7f) << 7) | ((unsigned char)bytes.at(i+2) & 0x7f);
                            received_bytes=0;
                            qDebug() << "Data Size=" << datasize << "??";
                            i+=3;
                        }
                        int z;
                        for (z=i;z<size;z++) {
                            c=bytes.at(z);
                            if (z&0x80) break;
                        }
                        data.clear();
                        for (int z=i;z<size;z++) {
                            data.push_back(bytes.at(z));
                            received_bytes++;
                        }
                        //i=z;
                        //size-=i;
                        waitf6=false;
                        return;
                        //continue;
                    }
                }
            } else {
                qDebug() << "Recieving Block" << size << "(" << received_bytes << "of" << datasize <<")";
                for (int z=i;z<size;z++) {
                    data.push_back(bytes.at(z));
                    received_bytes++;
                }
                emit(updateProgress(float(received_bytes)/float(datasize)));
                if ((received_bytes>=datasize) || (((received_bytes/datasize)>0.7) && (size<250))) {
                    qDebug() << "End";
                    static unsigned char b1[3]={0xf6,0xf6,0xf6};
                    if (m_port->write((char *)b1,2)==-1) {
                        qDebug() << "Couldn't write closing bytes to CMS50";
                    }
                    Close();
                    emit(importProcess());
                }
                break;
                //read data blocks..
            }
            /*if (size<200) && (received_bytes>=datasize) {

            } */
        } else {
            if (bytes[i]&0x80) { // 0x80 == sync bit
                EventDataType d=bytes[i+1] & 0x7f;
                addPlethy(lasttime,d);
                lasttime+=20;
                i+=3;
            } else {
                addPulse(lasttime,bytes[i]);
                addSpO2(lasttime,bytes[i+1]);
                i+=2;
            }
        }
    }
    emit(dataChanged());
}

bool CMS50Serial::startImport()
{
    import_mode=true;
    waitf6=true;
    cntf6=0;
    //QMessageBox::information(0,"!!!Important Notice!!!","This Oximetry import method does NOT allow syncing of Oximetry and CPAP data.\nIf you really wish to record your oximetry data and have sync, you have to use the Live View mode (click Start) with the Oximeter connected to a computer via USB cable all night..\nEven then it will be out a bit because of your CPAP machines realtime clock drifts.",QMessageBox::Ok);
    if (!Open(QextSerialPort::EventDriven)) return false;
    connect(this,SIGNAL(importProcess()),this,SLOT(on_import_process()));

    createSession();

    static unsigned char b1[2]={0xf5,0xf5};

    if (m_port->write((char *)b1,2)==-1) {
        qDebug() << "Couldn't write data request bytes to CMS50";
    }

    return true;
}


Oximetry::Oximetry(QWidget *parent,gGraphView * shared) :
    QWidget(parent),
    ui(new Ui::Oximetry)
{
    m_shared=shared;
    ui->setupUi(this);

    port=NULL;
    portname="";

    oximeter=new CMS50Serial(this);

    day=new Day(oximeter->getMachine());

    layout=new QHBoxLayout(ui->graphArea);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0,0,0,0);
    ui->graphArea->setLayout(layout);
    ui->graphArea->setAutoFillBackground(false);

    GraphView=new gGraphView(ui->graphArea,m_shared);
    GraphView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    scrollbar=new MyScrollBar(ui->graphArea);
    scrollbar->setOrientation(Qt::Vertical);
    scrollbar->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Expanding);
    scrollbar->setMaximumWidth(20);

    GraphView->setScrollBar(scrollbar);
    layout->addWidget(GraphView,1);
    layout->addWidget(scrollbar,0);

    layout->layout();

    PLETHY=new gGraph(GraphView,tr("Plethy"),120);
    CONTROL=new gGraph(GraphView,tr("Control"),75);
    PULSE=new gGraph(GraphView,tr("Pulse Rate"),120);
    SPO2=new gGraph(GraphView,tr("SPO2"),120);
    foobar=new gShadowArea();
    CONTROL->AddLayer(foobar);
    Layer *cl=new gLineChart(OXI_Plethy);
    CONTROL->AddLayer(cl);
    cl->SetDay(day);
    CONTROL->setBlockZoom(true);

    for (int i=0;i<GraphView->size();i++) {
        gGraph *g=(*GraphView)[i];
        g->AddLayer(new gXAxis(),LayerBottom,0,gXAxis::Margin);
        g->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
        g->AddLayer(new gXGrid());
    }

    plethy=new gLineChart(OXI_Plethy,Qt::black,false,true);
    plethy->SetDay(day);

    CONTROL->AddLayer(plethy); //new gLineChart(OXI_Plethysomogram));


    pulse=new gLineChart(OXI_Pulse,Qt::red,true);
    pulse->SetDay(day);

    spo2=new gLineChart(OXI_SPO2,Qt::blue,true);
    spo2->SetDay(day);

    PLETHY->AddLayer(plethy);

    PULSE->AddLayer(pulse);
    SPO2->AddLayer(spo2);

    GraphView->setEmptyText("No Oximetry Data");
    GraphView->updateGL();

    on_RefreshPortsButton_clicked();
    ui->RunButton->setChecked(false);

    ui->saveButton->setEnabled(false);
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
        if (!ports.at(i).friendName.isEmpty()) {
            if (ports.at(i).friendName.toUpper().contains("USB")) {
                if (firstport.isEmpty()) firstport=ports.at(i). qesPORTNAME;
                if (!portname.isEmpty() && ports.at(i).qesPORTNAME==portname) current_found=true;
                ui->SerialPortsCombo->addItem(ports.at(i).qesPORTNAME);
                z++;
            }
        } else { // Mac stuff.
            if (ports.at(i).portName.toUpper().contains("USB") || ports.at(i).portName.toUpper().contains("SPO2")) {
                if (firstport.isEmpty()) firstport=ports.at(i).portName;
                if (!portname.isEmpty() && ports.at(i).portName==portname) current_found=true;
                ui->SerialPortsCombo->addItem(ports.at(i).portName);
                z++;
            }
        }
        //qDebug() << "Serial Port:" << ports.at(i).qesPORTNAME << ports.at(i).friendName;
        //qDebug() << "port name:" << ports.at(i).portName;
        //qDebug() << "phys name:" << ports.at(i).physName;
        //qDebug() << "friendly name:" << ports.at(i).friendName;
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
    oximeter->setPortName(portname);
}
void Oximetry::RedrawGraphs()
{
    GraphView->updateGL();
}
void Oximetry::on_SerialPortsCombo_activated(const QString &arg1)
{
    portname=arg1;
    oximeter->setPortName(arg1);
}

void Oximetry::on_RunButton_toggled(bool checked)
{
    if (!checked) {
            oximeter->stopLive();
            ui->RunButton->setText("&Start");
            ui->SerialPortsCombo->setEnabled(true);
            disconnect(oximeter,SIGNAL(dataChanged()),this,SLOT(onDataChanged()));
            disconnect(oximeter,SIGNAL(updatePulse(float)),this,SLOT(onPulseChanged(float)));
            disconnect(oximeter,SIGNAL(updateSpO2(float)),this,SLOT(onSpO2Changed(float)));
            ui->saveButton->setEnabled(true);

            //CONTROL->setVisible(true);
    } else {
        if (oximeter->getSession() && oximeter->getSession()->IsChanged()) {
            int res=QMessageBox::question(this,"Save Session?","Creating a new oximetry session will destroy the old one.\nWould you like to save it first?","Save","Destroy It","Cancel",0,2);
            if (res==0) {
                ui->RunButton->setChecked(false);
                on_saveButton_clicked();
                return;
            } else if (res==2) {
                ui->RunButton->setChecked(false);
                return;
            }
        } // else it's already saved.

        if (!oximeter->startLive()) {
            QMessageBox::warning(this,"Error","Something is wrong with the device connection.",QMessageBox::Ok);
            return;
        }
        ui->saveButton->setEnabled(false);
        day->getSessions().clear();
        day->AddSession(oximeter->getSession());

        firstPulseUpdate=true;
        firstSPO2Update=true;
        secondPulseUpdate=true;
        secondSPO2Update=true;

        qint64 f=oximeter->getSession()->first();
        day->setFirst(f);
        plethy->setMinX(f);
        pulse->setMinX(f);
        spo2->setMinX(f);
        PLETHY->SetMinX(f);
        CONTROL->SetMinX(f);
        PULSE->SetMinX(f);
        SPO2->SetMinX(f);

        PLETHY->forceMinY(0);
        PLETHY->forceMaxY(128);
        PULSE->forceMinY(30);
        PULSE->forceMaxY(180);
        SPO2->forceMinY(50);
        SPO2->forceMaxY(100);

        connect(oximeter,SIGNAL(dataChanged()),this,SLOT(onDataChanged()));
        connect(oximeter,SIGNAL(updatePulse(float)),this,SLOT(onPulseChanged(float)));
        connect(oximeter,SIGNAL(updateSpO2(float)),this,SLOT(onSpO2Changed(float)));
        CONTROL->setVisible(false);
        // connect.
        ui->RunButton->setText("&Stop");
        ui->SerialPortsCombo->setEnabled(false);
    }

}
void Oximetry::onDataChanged()
{

    qint64 last=oximeter->lastTime();
    qint64 first=last-30000L;
    day->setLast(last);

    //plethy->setMinX(first);
    //plethy->setMaxX(last);
    pulse->setMinX(first);
    pulse->setMaxX(last);
    spo2->setMinX(first);
    spo2->setMaxX(last);

    plethy->setMinY(0);
    plethy->setMaxY(128);
    pulse->setMinY(0);
    pulse->setMaxY(120);
    spo2->setMinY(0);
    spo2->setMaxY(100);

    PLETHY->MinY();
    PLETHY->MaxY();
    PULSE->MinY();
    PULSE->MaxY();
    SPO2->MinY();
    SPO2->MaxY();

    PLETHY->SetMaxX(last);
    PULSE->SetMaxX(last);
    CONTROL->SetMaxX(last);
    SPO2->SetMaxX(last);

    for (int i=0;i<GraphView->size();i++) {
        (*GraphView)[i]->SetXBounds(first,last);
    }

    {
        int len=(last-first)/1000L;
        int h=len/3600;
        int m=(len /60) % 60;
        int s=(len % 60);
        if (qstatus2) qstatus2->setText(QString().sprintf("Rec %02i:%02i:%02i",h,m,s));
    }

    GraphView->updateGL();
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
    connect(oximeter,SIGNAL(importComplete(Session*)),this,SLOT(on_import_complete(Session*)));
    connect(oximeter,SIGNAL(importAborted()),this,SLOT(on_import_aborted()));
    connect(oximeter,SIGNAL(updateProgress(float)),this,SLOT(on_updateProgress(float)));
    //connect(oximeter,SIGNAL(dataChanged()),this,SLOT(onDataChanged()));

    if (!oximeter->startImport()) {
        qDebug() << "Error starting oximetry serial import process";
        return;
    }
    day->getSessions().clear();
    day->AddSession(oximeter->getSession());

    if (qprogress) {
        qprogress->setValue(0);
        qprogress->setMaximum(100);
        qprogress->show();
    }
    ui->ImportButton->setDisabled(true);
    ui->SerialPortsCombo->setEnabled(false);
    ui->RunButton->setText("&Start");
    ui->RunButton->setChecked(false);
}

void Oximetry::import_finished()
{
    disconnect(oximeter,SIGNAL(importComplete(Session*)),this,SLOT(on_import_complete(Session*)));
    disconnect(oximeter,SIGNAL(importAborted()),this,SLOT(on_import_aborted()));
    disconnect(oximeter,SIGNAL(updateProgress(float)),this,SLOT(on_updateProgress(float)));
    //disconnect(oximeter,SIGNAL(dataChanged()),this,SLOT(onDataChanged()));

    ui->SerialPortsCombo->setEnabled(true);
    qstatus->setText("Ready");
    ui->ImportButton->setDisabled(false);
    ui->saveButton->setEnabled(true);

    if (qprogress) {
        qprogress->setValue(100);
        QApplication::processEvents();
        qprogress->hide();
    }
}

void Oximetry::on_import_aborted()
{
    import_finished();
}

void Oximetry::on_import_complete(Session * session)
{
    qDebug() << "Oximetry import complete";
    import_finished();

    PLETHY->setVisible(false);
    CONTROL->setVisible(false);

    qint64 f=session->first();
    qint64 l=session->last();
    day->setFirst(f);
    day->setLast(l);

    plethy->setMinX(f);
    pulse->setMinX(f);
    spo2->setMinX(f);
    PLETHY->SetMinX(f);
    CONTROL->SetMinX(f);
    PULSE->SetMinX(f);
    SPO2->SetMinX(f);

    plethy->setMaxX(l);
    pulse->setMaxX(l);
    spo2->setMaxX(l);
    PLETHY->SetMaxX(l);
    CONTROL->SetMaxX(l);
    PULSE->SetMaxX(l);
    SPO2->SetMaxX(l);


    PLETHY->forceMinY(0);
    PLETHY->forceMaxY(128);
    PULSE->forceMinY(30);
    PULSE->forceMaxY(180);
    SPO2->forceMinY(50);
    SPO2->forceMaxY(100);

    PULSE->setDay(day);
    SPO2->setDay(day);
    for (int i=0;i<GraphView->size();i++) {
        (*GraphView)[i]->SetXBounds(f,l);
    }

    {
        int len=(l-f)/1000L;
        int h=len/3600;
        int m=(len /60) % 60;
        int s=(len % 60);
        if (qstatus2) qstatus2->setText(QString().sprintf("%02i:%02i:%02i",h,m,s));
    }
    GraphView->updateScale();

    GraphView->updateGL();

}

void Oximetry::onPulseChanged(float p)
{
    ui->pulseLCD->display(p);
    if (firstPulseUpdate) {
        if (secondPulseUpdate) {
            secondPulseUpdate=false;
        } else {
            firstPulseUpdate=false;
            GraphView->updateScale();
        }
    }
}

// Only really need to do this once..
void Oximetry::onSpO2Changed(float o2)
{
    ui->spo2LCD->display(o2);
    if (firstSPO2Update) {
        if (secondSPO2Update) {
            secondSPO2Update=false;
        } else {
            firstSPO2Update=false;
            GraphView->updateScale();
        }
    }
}

void Oximetry::on_saveButton_clicked()
{
    if (QMessageBox::question(this,"Keep This Recording?","A save dialog will go here allowing you to edit parameters for this oximetry session..\nFor now, would you like to save this as is?",QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes) {
        Session *session=oximeter->getSession();
        // Process???

        session->UpdateSummaries();
        oximeter->getMachine()->AddSession(session,p_profile);
        oximeter->getMachine()->Save();
    }
}
void Oximetry::on_updateProgress(float f)
{
    if (qprogress) {
        qprogress->setValue(f*100.0);
        QApplication::processEvents();
    }
}

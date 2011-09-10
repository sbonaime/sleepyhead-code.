#include "report.h"
#include "ui_report.h"
#include <QMessageBox>
#include <QBuffer>
#include <Graphs/gYAxis.h>
#include <Graphs/gXAxis.h>
#include <QTimer>
#include <QPrinter>
#include <QPrintDialog>

Report::Report(QWidget *parent, gGraphView * shared, Daily * daily, Overview * overview) :
    QWidget(parent),
    ui(new Ui::Report),
    m_daily(daily),
    m_overview(overview)
{
    ui->setupUi(this);
    QString prof=pref["Profile"].toString();
    profile=Profiles::Get(prof);
    if (!profile) {
        QMessageBox::critical(this,"Profile Error",QString("Couldn't get profile '%1'.. Have to abort!").arg(pref["Profile"].toString()));
        exit(-1);
    }
    GraphView=new gGraphView(this);

    //GraphView->AddGraph(overview->AHI);
    GraphView->hide();
    ui->startDate->setDate(profile->FirstDay());
    ui->endDate->setDate(profile->LastDay());

    // Create a new graph, but reuse the layers..
    int default_height=200;
    UC=new gGraph(GraphView,"Usage",default_height,0);
    /*uc=new SummaryChart(profile,"Hours",GT_BAR);
    uc->addSlice(EmptyChannel,QColor("green"),ST_HOURS); */
    UC->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gXAxis *gx=new gXAxis();
    gx->setUtcFix(true);
    UC->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    UC->AddLayer(m_overview->uc);
    UC->AddLayer(new gXGrid());

    AHI=new gGraph(GraphView,"AHI",default_height,0);

  /*  bc=new SummaryChart(profile,"AHI",GT_BAR);
    bc->addSlice(CPAP_Hypopnea,QColor("blue"),ST_CPH);
    bc->addSlice(CPAP_Apnea,QColor("dark green"),ST_CPH);
    bc->addSlice(CPAP_Obstructive,QColor("#40c0ff"),ST_CPH);
    bc->addSlice(CPAP_ClearAirway,QColor("purple"),ST_CPH);*/
    AHI->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    AHI->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    AHI->AddLayer(m_overview->bc);
    AHI->AddLayer(new gXGrid());

    PR=new gGraph(GraphView,"Pressure",default_height,0);
    /*pr=new SummaryChart(profile,"cmH2O",GT_LINE);
    pr->addSlice(CPAP_Pressure,QColor("orange"),ST_MIN);
    pr->addSlice(CPAP_Pressure,QColor("red"),ST_MAX);
    pr->addSlice(CPAP_EPAP,QColor("light green"),ST_MIN);
    pr->addSlice(CPAP_IPAP,QColor("light blue"),ST_MAX); */
    PR->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    PR->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    PR->AddLayer(m_overview->pr);
    PR->AddLayer(new gXGrid());

    LK=new gGraph(GraphView,"Leaks",default_height,0);
    /*lk=new SummaryChart(profile,"Avg Leak",GT_LINE);
    lk->addSlice(CPAP_Leak,QColor("dark blue"),ST_WAVG);
    lk->addSlice(CPAP_Leak,QColor("dark grey"),ST_90P); */
    //lk->addSlice(CPAP_Leak,QColor("dark yellow"));
    //pr->addSlice(CPAP_IPAP,QColor("red"));
    LK->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
    gx=new gXAxis();
    gx->setUtcFix(true);
    LK->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
    LK->AddLayer(m_overview->lk);
    LK->AddLayer(new gXGrid());

    GraphView->hideSplitter();
    //ui->webView->hide();
    m_ready=false;
//    Reload();
}

Report::~Report()
{
    delete ui;
}
void Report::showEvent (QShowEvent * event)
{
    QTimer::singleShot(0,this,SLOT(on_refreshButton_clicked()));
}

void Report::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    GraphView->setMinimumSize(event->size().width()-20,240);
    GraphView->setMaximumSize(event->size().width()-20,240);
    Reload();
}

QPixmap Report::Snapshot(gGraph * graph)
{
    QDateTime d1(ui->startDate->date(),QTime(0,0,0),Qt::UTC);
    qint64 first=qint64(d1.toTime_t())*1000L;
    QDateTime d2(ui->endDate->date(),QTime(0,0,0),Qt::UTC);
    qint64 last=qint64(d2.toTime_t())*1000L;

    GraphView->TrashGraphs();
    GraphView->AddGraph(graph);
    GraphView->ResetBounds();
    GraphView->SetXBounds(first,last);

    int w=this->width()-20;
    QPixmap pixmap=GraphView->renderPixmap(w,240,false); //gwwidth,gwheight,false);


    return pixmap;
}

void Report::Reload()
{
    if (!m_ready) return;

    //UC->ResetBounds();
    QString html="<html><head><style type='text/css'>p,a,td,body { font-family: 'FreeSans', 'Sans Serif'; } p,a,td,body { font-size: 12px; } </style>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>"
    "<table width=100% cellpadding=0 cellspacing=0>"
    "<tr><td valign=top>";
    html+="<h2>CPAP Overview</h2>";
    html+="<table border='1px'><tr><td valign=top><table border=0>";

    //html+="<i>This is a temporary scratch pad tab so I can see what's going on while designing printing code. These graphs are images, and not controllable.</i>";
    if (!((*profile).Exists("FirstName") && (*profile).Exists("LastName"))) html+="<h1>Please edit your profile (in Preferences)</h1>"; else {
        html+="<tr><td>Name:</td><td>"+(*profile)["FirstName"].toString()+" "+(*profile)["LastName"].toString()+"</td></tr>";
    }
    if ((*profile).Exists("Address")&& !(*profile)["Address"].toString().isEmpty()) {
        QString address=(*profile)["Address"].toString().replace("\n","<br/>");
        html+="<tr><td valign=top>Address:</td><td valign=top>"+address+"</td></tr>";
    }
    if ((*profile).Exists("Phone") && !(*profile)["Phone"].toString().isEmpty()) {
        html+="<tr><td>Phone:</td><td>"+(*profile)["Phone"].toString()+"</td></tr>";
    }
    if ((*profile).Exists("EmailAddress") && !(*profile)["EmailAddress"].toString().isEmpty()) {
        html+="<tr><td>Email:</td><td>"+(*profile)["EmailAddress"].toString()+"</td></tr>";
    }
    html+="</table></td><td valign=top><table>";
    if ((*profile).Exists("Gender")) {
        QString gender=(*profile)["Gender"].toBool() ? "Male" : "Female";
        html+="<tr><td>Gender:</td><td>"+gender+"</td></tr>";
    }
    if ((*profile).Exists("DOB") && !(*profile)["DOB"].toString().isEmpty()) {
        QDate dob=(*profile)["DOB"].toDate();
        //html+="<tr><td>D.O.B.:</td><td>"+dob.toString()+"</td></tr>";
        QDateTime d1(dob,QTime(0,0,0));
        QDateTime d2(QDate::currentDate(),QTime(0,0,0));
        int years=d1.daysTo(d2)/365.25;
        html+="<tr><td>Age:</td><td>"+QString::number(years)+" years</td></tr>";

    }
    if ((*profile).Exists("Height") && !(*profile)["Height"].toString().isEmpty()) {
        html+="<tr><td>Height:</td><td>"+(*profile)["Height"].toString();
        if (!(*profile).Exists("UnitSystem")) {
            (*profile)["UnitSystem"]="Metric";
        }
        if ((*profile)["UnitSystem"].toString()=="Metric") html+="cm"; else html+="inches";
        html+="</td></tr>";
    }

    html+="</table></td></tr></table>";
    html+="<td ><div align=center><img src='qrc:/docs/sheep.png' width=100 height=100'><br/>SleepyHead v"+pref["VersionString"].toString()+"</div></td></tr></table>&nbsp;<br/>"
    "Reporting from <b>"+ui->startDate->date().toString()+"</b> to <b>"+ui->endDate->date().toString()+"</b><br/>"
    "<hr>";


    QVector<gGraph *> graphs;
    graphs.push_back(AHI);
    graphs.push_back(UC);
    graphs.push_back(PR);
    graphs.push_back(LK);

    for (int i=0;i<graphs.size();i++) {
        QPixmap pixmap=Snapshot(graphs[i]);
        QByteArray byteArray;
        QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");
        html += "<div align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\" width=\"100%\"></div>\n"; //
    }

    html+="</body></html>";
    ui->webView->setHtml(html);
}

void Report::on_refreshButton_clicked()
{
    m_ready=true;
    Reload();
}

void Report::on_startDate_dateChanged(const QDate &date)
{
    Reload();
}

void Report::on_endDate_dateChanged(const QDate &date)
{
    Reload();
}

void Report::on_printButton_clicked()
{
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer);
    //printer.setPrinterName("Print to File (PDF)");
    //printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPrintRange(QPrinter::AllPages);
    printer.setOrientation(QPrinter::Portrait);
    printer.setPaperSize(QPrinter::A4);
    printer.setResolution(QPrinter::HighResolution);
    printer.setFullPage(false);
    printer.setNumCopies(1);
    //printer.setOutputFileName("printYou.pdf");
    if ( dialog->exec() == QDialog::Accepted) {
        ui->webView->print(&printer);
    }
}

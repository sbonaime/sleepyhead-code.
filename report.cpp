#include "report.h"
#include "ui_report.h"
#include <QMessageBox>
#include <QBuffer>
#include <Graphs/gYAxis.h>
#include <Graphs/gXAxis.h>
#include <QTimer>
#include <QPrinter>
#include <QPrintDialog>

Report::Report(QWidget *parent, Profile * _profile, gGraphView * shared, Overview * overview) :
    QWidget(parent),
    ui(new Ui::Report),
    profile(_profile),
    m_overview(overview)
{
    ui->setupUi(this);

    Q_ASSERT(profile!=NULL);

    GraphView=new gGraphView(this,shared);
    GraphView->hide();

    // Create a new graph, but reuse the layers..
    int default_height=150;

    // Reusing the layer data from overview screen,
    // (Can't reuse the graphs objects without breaking things)

    UC=new gGraph(GraphView,"Usage",default_height,0);
    UC->AddLayer(m_overview->uc);

    AHI=new gGraph(GraphView,"AHI",default_height,0);
    AHI->AddLayer(m_overview->bc);

    PR=new gGraph(GraphView,"Pressure",default_height,0);
    PR->AddLayer(m_overview->pr);

    LK=new gGraph(GraphView,"Leaks",default_height,0);
    LK->AddLayer(m_overview->lk);

    NPB=new gGraph(GraphView,"% in PB",default_height,0);
    NPB->AddLayer(m_overview->npb);

    graphs.push_back(AHI);
    graphs.push_back(UC);
    graphs.push_back(PR);
    graphs.push_back(LK);
    graphs.push_back(NPB);

    gXAxis *gx;
    for (int i=0;i<graphs.size();i++) {
        graphs[i]->AddLayer(new gYAxis(),LayerLeft,gYAxis::Margin);
        gx=new gXAxis();
        gx->setUtcFix(true);
        graphs[i]->AddLayer(gx,LayerBottom,0,gXAxis::Margin);
        graphs[i]->AddLayer(new gXGrid());
    }


    GraphView->hideSplitter();
    //ui->webView->hide();
    m_ready=false;
    ReloadGraphs();
//    Reload();
}

Report::~Report()
{
    delete ui;
}
void Report::ReloadGraphs()
{
    for (int i=0;i<graphs.size();i++) {
        graphs[i]->setDay(NULL);
    }
    startDate=profile->FirstDay();
    endDate=profile->LastDay();
    for (int i=0;i<graphs.size();i++) {
        graphs[i]->ResetBounds();
    }
    m_ready=true;

}
void Report::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    GraphView->setMinimumSize(event->size().width()-20,240);
    GraphView->setMaximumSize(event->size().width()-20,240);
    //GenerateReport(startDate,endDate);
}

QPixmap Report::Snapshot(gGraph * graph)
{
    QDateTime d1(startDate,QTime(0,0,0),Qt::UTC);
    qint64 first=qint64(d1.toTime_t())*1000L;
    QDateTime d2(endDate,QTime(23,59,59),Qt::UTC);
    qint64 last=qint64(d2.toTime_t())*1000L;

    GraphView->TrashGraphs();
    GraphView->AddGraph(graph);
    GraphView->ResetBounds();
    GraphView->SetXBounds(first,last);

    int w=this->width()-20;
    QPixmap pixmap=GraphView->renderPixmap(w,240,false); //gwwidth,gwheight,false);

    return pixmap;
}

void Report::GenerateReport(QDate start, QDate end)
{
    if (!m_ready) return;
    startDate=start;
    endDate=end;

    //UC->ResetBounds();
    QString html="<html><head><style type='text/css'>p,a,td,body { font-family: 'FreeSans', 'Sans Serif'; } p,a,td,body { font-size: 12px; } </style>"
    "</head>"
    "<body leftmargin=0 rightmargin=0 topmargin=0 marginwidth=0 marginheight=0>"
    "<table width=100% cellpadding=0 cellspacing=0>"
    "<tr><td valign=top>";
    html+="<h2>CPAP Overview</h2>";
    html+="<table border='1px'><tr><td valign=top><table border=0>";

    //html+="<i>This is a temporary scratch pad tab so I can see what's going on while designing printing code. These graphs are images, and not controllable.</i>";
    if (!((*profile).Exists("FirstName") && (*profile).Exists("LastName"))) html+="<h1>Please edit your profile</h1>"; else {
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
    "Reporting from <b>"+startDate.toString()+"</b> to <b>"+endDate.toString()+"</b><br/>"
    "<hr>";



    for (int i=0;i<graphs.size();i++) {
        if (graphs[i]->isEmpty()) continue;
        QPixmap pixmap=Snapshot(graphs[i]);
        QByteArray byteArray;
        QBuffer buffer(&byteArray); // use buffer to store pixmap into byteArray
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");
        html += "<div align=center><img src=\"data:image/png;base64," + byteArray.toBase64() + "\" width=\"100%\" height=\"240px\"></div>\n"; //
    }

    html+="</body></html>";
    ui->webView->setHtml(html);
}

void Report::on_printButton_clicked()
{
    QPrinter printer;
    //printer.setPrinterName("Print to File (PDF)");
    //printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPrintRange(QPrinter::AllPages);
    printer.setOrientation(QPrinter::Portrait);
    //printer.setPaperSize(QPrinter::A4);
    printer.setResolution(QPrinter::HighResolution);
    printer.setFullPage(false);
    printer.setNumCopies(1);
    printer.setPageMargins(10,10,10,10,QPrinter::Millimeter);
    QPrintDialog *dialog = new QPrintDialog(&printer);
    //printer.setOutputFileName("printYou.pdf");
    if ( dialog->exec() == QDialog::Accepted) {
        ui->webView->print(&printer);
    }
}

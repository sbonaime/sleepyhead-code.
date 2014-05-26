#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "Graphs/gYAxis.h"
#include "Graphs/gXAxis.h"

#include "oximeterimport.h"
#include "ui_oximeterimport.h"

#include "SleepLib/loader_plugins/cms50_loader.h"

Qt::DayOfWeek firstDayOfWeekFromLocale();
QList<SerialOximeter *> GetOxiLoaders();

OximeterImport::OximeterImport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OximeterImport)
{
    ui->setupUi(this);
    setWindowTitle(tr("Oximeter Import Wizard"));
    ui->stackedWidget->setCurrentIndex(0);
    oximodule = nullptr;
    ui->retryButton->setVisible(false);
    liveView = new gGraphView(this);
    liveView->setVisible(false);
    ui->stopButton->setVisible(false);
    ui->syncSaveButton->setVisible(false);

    QVBoxLayout * lvlayout = new QVBoxLayout;
    lvlayout->setMargin(0);
    ui->liveViewFrame->setLayout(lvlayout);
    lvlayout->addWidget(liveView);
    plethyGraph = new gGraph(liveView, STR_TR_Plethy, STR_UNIT_Hz);

    plethyGraph->AddLayer(new gYAxis(), LayerLeft, gYAxis::Margin);
    plethyGraph->AddLayer(new gXAxis(), LayerBottom, 0, 20);
    plethyGraph->AddLayer(plethyChart = new gLineChart(OXI_Plethy));
    plethyGraph->setVisible(true);
    plethyGraph->setRecMinY(0);
    plethyGraph->setRecMaxY(128);

    ui->calendarWidget->setFirstDayOfWeek(Qt::Sunday);
    QTextCharFormat format = ui->calendarWidget->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->calendarWidget->setWeekdayTextFormat(Qt::Saturday, format);
    ui->calendarWidget->setWeekdayTextFormat(Qt::Sunday, format);
    ui->calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    Qt::DayOfWeek dow=firstDayOfWeekFromLocale();
    ui->calendarWidget->setFirstDayOfWeek(dow);

    ui->dateTimeEdit->setMinimumHeight(ui->dateTimeEdit->height() + 10);
    ui->syncCPAPGroup->setVisible(false);

    QVBoxLayout * layout = new QVBoxLayout;
    layout->setMargin(0);
    ui->sessBarFrame->setLayout(layout);
    sessbar = new SessionBar(this);
    sessbar->setSelectMode(true);
    sessbar->setMouseTracking(true);
    sessbar->setMinimumHeight(40);
    connect(sessbar, SIGNAL(sessionClicked(Session*)), this, SLOT(onSessionSelected(Session*)));
    layout->addWidget(sessbar, 1);

    dummyday = nullptr;
    session = nullptr;
    ELplethy = nullptr;

    pulse = spo2 = -1;
}

OximeterImport::~OximeterImport()
{
    if (!dummyday) {
        delete dummyday;
    }
    if (!session) {
        delete session;
    }

    disconnect(sessbar, SIGNAL(sessionClicked(Session*)), this, SLOT(onSessionSelected(Session*)));
    delete ui;
}

void OximeterImport::on_nextButton_clicked()
{
    int i = ui->stackedWidget->currentIndex();
    i++;
    if (i >= ui->stackedWidget->count()) i = 0;

    switch (i) {
    case 0:
        ui->nextButton->setVisible(true);
        ui->nextButton->setText("&Start");
        break;
    case 1:
        ui->nextButton->setVisible(false);
        break;
    default:
        ui->nextButton->setVisible(true);


    }
    ui->stackedWidget->setCurrentIndex(i);
}

void OximeterImport::updateStatus(QString msg)
{
    ui->logBox->appendPlainText(msg);
    ui->directImportStatus->setText(msg);
    ui->liveStatusLabel->setText(msg);
}


SerialOximeter * OximeterImport::detectOximeter()
{
    const int PORTSCAN_TIMEOUT=30000;
    const int delay=100;


    ui->retryButton->setVisible(false);

    QList<SerialOximeter *> loaders = GetOxiLoaders();


    updateStatus(tr("Scanning for compatible oximeters"));

    ui->progressBar->setMaximum(PORTSCAN_TIMEOUT);

    QTime time;
    time.start();

    oximodule = nullptr;
    int elapsed=0;
    do {
        for (int i=0; i < loaders.size(); ++i) {
            SerialOximeter * oxi = loaders[i];
            if (oxi->openDevice()) {
                oximodule = oxi;
                break;
            }
         }

         if (oximodule)
            break;

        QThread::msleep(delay);
        elapsed = time.elapsed();
        ui->progressBar->setValue(elapsed);
        QApplication::processEvents();
        if (!isVisible()) {
            return oximodule = nullptr;
        }

    } while (elapsed < PORTSCAN_TIMEOUT);

    if (!oximodule) {
        updateStatus(tr("Could not detect any connected oximeter devices."));
        return nullptr;
    }

    updateStatus(tr("Connecting to %1 Oximeter").arg(oximodule->ClassName()));

    return oximodule;
}

void OximeterImport::on_directImportButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->directImportPage);

    oximodule = detectOximeter();
    if (!oximodule)
        return;

    ui->connectLabel->setText("<h2>"+tr("Select upload option on %1").arg(oximodule->ClassName())+"</h2>");
    updateStatus(tr("Waiting for you to start the upload process..."));


    connect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));

    oximodule->Open("import", p_profile);

    // Wait to start import streaming..
    while (!oximodule->isImporting() && !oximodule->isAborted()) {
        QThread::msleep(100);
        QApplication::processEvents();
        if (!isVisible()) {
            disconnect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));
            oximodule->abort();
            break;
        }

    }

    if (!oximodule->isStreaming()) {
        disconnect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));
        ui->retryButton->setVisible(true);
        ui->progressBar->setValue(0);
        oximodule->abort();
        return;
    }
    ui->connectLabel->setText("<h2>"+tr("%1 device is uploading data...").arg(oximodule->ClassName())+"</h2>");
    updateStatus(tr("Please wait until oximeter upload process completes. Do not unplug your oximeter."));

    // Wait for import streaming to finish

    // Can't abort this bit or the oximeter will get confused...
    ui->cancelButton->setVisible(false);

    connect(oximodule, SIGNAL(importComplete(SerialOximeter*)), this, SLOT(finishedImport(SerialOximeter*)));
}

void OximeterImport::finishedImport(SerialOximeter * oxi)
{
    disconnect(ui->stopButton, SIGNAL(clicked()), this, SLOT(finishedImport()));
    ui->cancelButton->setVisible(true);
    updateStatus(tr("Oximeter import completed.. Processing data"));
    oximodule->process();
    disconnect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));

    ui->stackedWidget->setCurrentWidget(ui->syncPage);
    ui->syncSaveButton->setVisible(true);

    ui->calendarWidget->setMinimumDate(PROFILE.FirstDay());
    ui->calendarWidget->setMaximumDate(PROFILE.LastDay());

    on_calendarWidget_clicked(PROFILE.LastDay());
}

void OximeterImport::doUpdateProgress(int v, int t)
{
    ui->progressBar->setMaximum(t);
    ui->progressBar->setValue(v);
    QApplication::processEvents();
}


void OximeterImport::on_fileImportButton_clicked()
{
#if QT_VERSION  < QT_VERSION_CHECK(5,0,0)
        const QString documentsFolder = QDesktopServices::storageLocation(
                                      QDesktopServices::DocumentsLocation);
#else
        const QString documentsFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif


    QString filename = QFileDialog::getOpenFileName(this, tr("Select a valid oximetry data file"), documentsFolder, "Oximetry Files (*.spo *.spor *.spo2)");

    if (filename.isEmpty())
        return;
    QList<SerialOximeter *> loaders = GetOxiLoaders();

    bool success = false;

    oximodule = nullptr;
    Q_FOREACH(SerialOximeter * loader, loaders) {
        if (loader->Open(filename,p_profile)) {
            success = true;
            oximodule = loader;
            break;
        }
    }
    if (!success) {
        QMessageBox::warning(this, STR_MessageBox_Warning, tr("No Oximetery module could parse the given file:")+QString("\n\n%1").arg(filename), QMessageBox::Ok);
        return;
    }

    ui->stackedWidget->setCurrentWidget(ui->syncPage);
    ui->syncSaveButton->setVisible(true);

    ui->calendarWidget->setMinimumDate(PROFILE.FirstDay());
    ui->calendarWidget->setMaximumDate(PROFILE.LastDay());

    on_calendarWidget_clicked(PROFILE.LastDay());
}

void OximeterImport::on_liveImportButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->liveImportPage);
    ui->liveImportPage->layout()->addWidget(ui->progressBar);
    QApplication::processEvents();

    liveView->setEmptyText(tr("Oximeter not detected"));
    liveView->setVisible(true);
    QApplication::processEvents();

    SerialOximeter * oximodule = detectOximeter();

    if (!oximodule) {
        updateStatus("Couldn't access oximeter");
        ui->retryButton->setVisible(true);
        ui->progressBar->setValue(0);

        return;
    }

    Machine *mach = oximodule->CreateMachine(p_profile);

    connect(oximodule, SIGNAL(updatePlethy(QByteArray)), this, SLOT(on_updatePlethy(QByteArray)));
    ui->liveConnectLabel->setText("Live Oximetery Mode");


    liveView->setEmptyText(tr("Starting up..."));
    updateStatus(tr("If you can still read this after a few seconds, cancel and try again"));
    ui->progressBar->hide();
    liveView->update();
    oximodule->Open("live",p_profile);
    ui->stopButton->setVisible(true);

    dummyday = new Day(mach);

    quint32 starttime = oximodule->startTime().toTime_t();
    ti = qint64(starttime) * 1000L;
    start_ti = ti;

    session = new Session(mach, starttime);

    ELplethy = session->AddEventList(OXI_Plethy, EVL_Waveform, 1.0, 0.0, 0.0, 0.0, 20);

    ELplethy->setFirst(start_ti);
    session->really_set_first(start_ti);
    dummyday->AddSession(session);

    plethyChart->setMinX(start_ti);
    plethyGraph->SetMinX(start_ti);

    liveView->setDay(dummyday);

    updateTimer.setParent(this);
    updateTimer.setInterval(50);
    updateTimer.start();
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateLiveDisplay()));
    connect(ui->stopButton, SIGNAL(clicked()), this, SLOT(finishedRecording()));
}

void OximeterImport::finishedRecording()
{
    updateTimer.stop();
    oximodule->closeDevice();
    disconnect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateLiveDisplay()));
    disconnect(ui->stopButton, SIGNAL(clicked()), this, SLOT(finishedRecording()));

    ui->stopButton->setVisible(false);
    ui->liveConnectLabel->setText("Live Import Stopped");
    liveView->setEmptyText(tr("Live Oximetery Stopped"));
    updateStatus(tr("Live Oximetery import has been stopped"));

    disconnect(oximodule, SIGNAL(updatePlethy(QByteArray)), this, SLOT(on_updatePlethy(QByteArray)));

// Remember to clear sessionlist before deleting dummyday. or it will destroy session.
//    delete dummyday;

    //ui->stackedWidget->setCurrentWidget(ui->syncPage);
    ui->syncSaveButton->setVisible(true);

    ui->calendarWidget->setMinimumDate(PROFILE.FirstDay());
    ui->calendarWidget->setMaximumDate(PROFILE.LastDay());

    plethyGraph->SetMinX(start_ti);
    liveView->SetXBounds(start_ti, ti, 0, true);

    plethyGraph->setBlockZoom(false);


    // detect oximeter
}

void OximeterImport::on_retryButton_clicked()
{
    if (ui->stackedWidget->currentWidget() == ui->directImportPage) {
        on_directImportButton_clicked();
    } else if (ui->stackedWidget->currentWidget() == ui->liveImportPage) {
        on_liveImportButton_clicked();
    }
}

void OximeterImport::on_stopButton_clicked()
{
    if (oximodule) {
        oximodule->abort();
    }
}

void OximeterImport::on_calendarWidget_clicked(const QDate &date)
{
    Day * day = PROFILE.GetGoodDay(date, MT_CPAP);

    sessbar->clear();
    if (day) {
        QDateTime time=QDateTime::fromMSecsSinceEpoch(day->first());
        sessbar->clear();
        QList<QColor> colors;
        colors.push_back("#ffffe0");
        colors.push_back("#ffe0ff");
        colors.push_back("#e0ffff");
        QList<Session *>::iterator i;
        int j=0;
        for (i=day->begin(); i != day->end(); ++i) {
            sessbar->add((*i),colors.at(j++ % colors.size()));
        }
        sessbar->setVisible(true);
        ui->sessbarLabel->setText(QString("%1 session(s), starting at %2").arg(day->size()).arg(time.time().toString("hh:mm:ssap")));
//        sessbar->setSelected(0);
//        ui->dateTimeEdit->setDateTime(time);
    } else {
        ui->sessbarLabel->setText("No CPAP Data available for this date"); // sessbar->setVisible(false);
    }

    sessbar->update();
}

void OximeterImport::on_calendarWidget_selectionChanged()
{
    on_calendarWidget_clicked(ui->calendarWidget->selectedDate());
}
void OximeterImport::onSessionSelected(Session * session)
{
    QDateTime time=QDateTime::fromMSecsSinceEpoch(session->first());
    ui->dateTimeEdit->setDateTime(time);
}

void OximeterImport::on_sessionBackButton_clicked()
{
    int idx = (sessbar->selected()-1);
    if (idx >= 0) {
        sessbar->setSelected(idx);
        QDateTime datetime = QDateTime::fromMSecsSinceEpoch(sessbar->session(idx)->first());
        ui->dateTimeEdit->setDateTime(datetime);
        sessbar->update();
    }
}

void OximeterImport::on_sessionForwardButton_clicked()
{
    int idx = (sessbar->selected()+1);
    if (idx < sessbar->count()) {
        sessbar->setSelected(idx);
        QDateTime datetime = QDateTime::fromMSecsSinceEpoch(sessbar->session(idx)->first());
        ui->dateTimeEdit->setDateTime(datetime);
        sessbar->update();
    }
}

void OximeterImport::on_radioSyncCPAP_clicked()
{
    if (!ui->syncCPAPGroup->isVisible()) {
        if (sessbar->selected() < 0) {
            int idx = 0;
            if (idx < sessbar->count()) {
                sessbar->setSelected(idx);
                QDateTime datetime = QDateTime::fromMSecsSinceEpoch(sessbar->session(idx)->first());
                ui->dateTimeEdit->setDateTime(datetime);
                sessbar->update();
            }
        }
    }
    ui->syncCPAPGroup->setVisible(true);

}

void OximeterImport::on_radioSyncOximeter_clicked()
{
    ui->syncCPAPGroup->setVisible(false);
}

void OximeterImport::on_radioSyncManually_clicked()
{
    ui->syncCPAPGroup->setVisible(false);
}

void OximeterImport::on_syncSaveButton_clicked()
{

}

void OximeterImport::on_updatePlethy(QByteArray plethy)
{
    if (!session) {
        return;
    }
    int size = plethy.size();
    quint64 dur = qint64(size) * 20L;
    ELplethy->AddWaveform(ti, plethy.data(), size, dur);
    ti += dur;
}

void OximeterImport::updateLiveDisplay()
{
    if (!session) {
        return;
    }

    if (ui->showLiveGraphs->isChecked()) {
        qint64 sti = ti - 20000;
        plethyChart->setMinY(ELplethy->Min());
        plethyChart->setMaxY(ELplethy->Max());
        plethyGraph->SetMinY(ELplethy->Min());
        plethyGraph->SetMaxY(ELplethy->Max());
        plethyGraph->SetMinX(sti);
        plethyGraph->SetMaxX(ti);
        plethyGraph->setBlockZoom(true);
        ELplethy->setLast(ti);
        session->really_set_last(ti);


        //liveView->SetXBounds(sti, ti, 0, true);
        session->setMin(OXI_Plethy, ELplethy->Min());
        session->setMax(OXI_Plethy, ELplethy->Max());
        session->setLast(OXI_Plethy, ti);
        session->setCount(OXI_Plethy, ELplethy->count());

        for (int i = 0; i < liveView->size(); i++) {
            (*liveView)[i]->SetXBounds(sti, ti);
        }

        liveView->updateScale();
        liveView->redraw();
    }

    int size = oximodule->oxirec.size();

    if (size > 0) {
        int i = oximodule->startTime().secsTo(QDateTime::currentDateTime());

        int seconds = i % 60;
        int minutes = (i / 60) % 60;
        int hours = i / 3600;

        size--;

        bool datagood = oximodule->oxirec[size].pulse > 0;


        if (datagood & (pulse <= 0)) {
            QString STR_recording = tr("Recording...");
            updateStatus(STR_recording);
            liveView->setEmptyText(STR_recording);
            if (!ui->showLiveGraphs->isChecked()) {
                liveView->redraw();
            }
        } else if (!datagood & (pulse != 0)) {
            QString STR_nofinger = tr("Finger not detected");
            updateStatus(STR_nofinger);
            liveView->setEmptyText(STR_nofinger);
            if (!ui->showLiveGraphs->isChecked()) {
                liveView->redraw();
            }
        }
        pulse = oximodule->oxirec[size].pulse;
        spo2 = oximodule->oxirec[size].spo2;
        if (pulse > 0) {
            ui->pulseDisplay->display(QString().sprintf("%3i", pulse));
        } else {
            ui->pulseDisplay->display("---");
        }
        if (spo2 > 0) {
            ui->spo2Display->display(QString().sprintf("%2i", spo2));
        } else {
            ui->spo2Display->display("--");
        }

        ui->lcdDuration->display(QString().sprintf("%02i:%02i:%02i",hours, minutes, seconds));

    }
}


void OximeterImport::on_cancelButton_clicked()
{
    if (oximodule && oximodule->isStreaming()) {
        oximodule->closeDevice();
    }
    reject();
}

void OximeterImport::on_showLiveGraphs_clicked(bool checked)
{
    if (checked) {
        updateTimer.setInterval(50);
    } else {
        // Don't need to call the timer so often.. Save a little CPU..
        updateTimer.setInterval(500);
    }
    plethyGraph->setVisible(checked);
    liveView->redraw();
}

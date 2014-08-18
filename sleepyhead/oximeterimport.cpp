/* Oximeter Import Wizard Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include "Graphs/gYAxis.h"
#include "Graphs/gXAxis.h"

#include "oximeterimport.h"
#include "ui_oximeterimport.h"
#include "SleepLib/calcs.h"
#include "mainwindow.h"

extern MainWindow * mainwin;

#include "SleepLib/loader_plugins/cms50_loader.h"
#include "SleepLib/loader_plugins/cms50f37_loader.h"

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
    liveView = new gGraphView(this);
    liveView->setVisible(false);
    ui->retryButton->setVisible(false);
    ui->stopButton->setVisible(false);
    ui->saveButton->setVisible(false);
    ui->syncButton->setVisible(false);
    ui->chooseSessionButton->setVisible(false);

    importMode = IM_UNDEFINED;

    QVBoxLayout * lvlayout = new QVBoxLayout;
    lvlayout->setMargin(0);
    ui->liveViewFrame->setLayout(lvlayout);
    lvlayout->addWidget(liveView);
    plethyGraph = new gGraph("Plethy", liveView, STR_TR_Plethy, STR_UNIT_Hz);

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


    ui->skipWelcomeCheckbox->setChecked(p_profile->oxi->skipOxiIntroScreen());
    if (p_profile->oxi->skipOxiIntroScreen()) {
        ui->stackedWidget->setCurrentWidget(ui->importSelectionPage);
        ui->nextButton->setVisible(false);
        ui->informationButton->setVisible(true);
    } else {
        ui->stackedWidget->setCurrentWidget(ui->welcomePage);
        ui->nextButton->setVisible(true);
        ui->informationButton->setVisible(false);
    }

    ui->dateTimeEdit->setMinimumHeight(ui->dateTimeEdit->height()+10);
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
        ui->informationButton->setVisible(true);

        break;
    default:
        ui->informationButton->setVisible(true);

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

    QList<SerialOximeter *> loaders; //= GetOxiLoaders();

    if (p_profile->oxi->oximeterType() == "Contec CMS50D+/E/F") {
        SerialOximeter * oxi = qobject_cast<SerialOximeter *>(lookupLoader(cms50_class_name));
        loaders.push_back(oxi);
    } else if (p_profile->oxi->oximeterType() == "Contec CMS50F v3.7+") {
        SerialOximeter * oxi = qobject_cast<SerialOximeter *>(lookupLoader(cms50f37_class_name));
        loaders.push_back(oxi);
    } else return nullptr;


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

    updateStatus(tr("Connecting to %1 Oximeter").arg(oximodule->loaderName()));

    return oximodule;
}

void OximeterImport::on_directImportButton_clicked()
{
    ui->informationButton->setVisible(false);
    ui->stackedWidget->setCurrentWidget(ui->directImportPage);

    oximodule = detectOximeter();
    if (!oximodule)
        return;

    ui->connectLabel->setText("<h2>"+tr("Select upload option on %1").arg(oximodule->loaderName())+"</h2>");
    updateStatus(tr("Waiting for you to start the upload process..."));

    connect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));

    oximodule->Open("import");

    // Wait to start import streaming..
    while (!oximodule->isImporting() && !oximodule->isAborted()) {
//        QThread::msleep(10);
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
    ui->connectLabel->setText("<h2>"+tr("%1 device is uploading data...").arg(oximodule->loaderName())+"</h2>");
    updateStatus(tr("Please wait until oximeter upload process completes. Do not unplug your oximeter."));

    importMode = IM_RECORDING;

    // Can't abort this bit or the oximeter will get confused...
    ui->cancelButton->setVisible(false);

    connect(oximodule, SIGNAL(importComplete(SerialOximeter*)), this, SLOT(finishedImport(SerialOximeter*)));
}

void OximeterImport::finishedImport(SerialOximeter * oxi)
{
    Q_UNUSED(oxi);

    connect(oximodule, SIGNAL(importComplete(SerialOximeter*)), this, SLOT(finishedImport(SerialOximeter*)));
    ui->cancelButton->setVisible(true);
    disconnect(oximodule, SIGNAL(updateProgress(int,int)), this, SLOT(doUpdateProgress(int,int)));
    updateStatus(tr("Oximeter import completed.."));

    if (oximodule->oxisessions.size() > 1) {
        chooseSession();
    } else {
        on_syncButton_clicked();
    }
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
        const QString documentsFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
        const QString documentsFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif


    QString filename = QFileDialog::getOpenFileName(nullptr , tr("Select a valid oximetry data file"), documentsFolder, tr("Oximetry Files (*.spo *.spor *.spo2 *.dat)"));

    if (filename.isEmpty())
        return;



    // Make sure filename dialog had time to close properly..
    QApplication::processEvents();

    QList<SerialOximeter *> loaders = GetOxiLoaders();

    bool success = false;

    oximodule = nullptr;
    Q_FOREACH(SerialOximeter * loader, loaders) {
        if (loader->Open(filename)) {
            success = true;
            oximodule = loader;
            break;
        }
    }
    if (!success) {
        QMessageBox::warning(this, STR_MessageBox_Warning, tr("No Oximetery module could parse the given file:")+QString("\n\n%1").arg(filename), QMessageBox::Ok);
        return;
    }
    ui->informationButton->setVisible(false);
    importMode = IM_FILE;


    if (oximodule->oxisessions.size() > 1) {
        chooseSession();
    } else {
        on_syncButton_clicked();
    }
}

void OximeterImport::on_liveImportButton_clicked()
{
    ui->informationButton->setVisible(false);

    ui->stackedWidget->setCurrentWidget(ui->liveImportPage);
    ui->liveImportPage->layout()->addWidget(ui->progressBar);
    QApplication::processEvents();

    liveView->setEmptyText(tr("Oximeter not detected"));
    liveView->setVisible(true);
    QApplication::processEvents();

    SerialOximeter * oximodule = detectOximeter();

    if (!oximodule) {
        updateStatus(tr("Couldn't access oximeter"));
        ui->retryButton->setVisible(true);
        ui->progressBar->setValue(0);

        return;
    }

    MachineInfo info = oximodule->newInfo();
    Machine *mach = oximodule->CreateMachine(info);

    connect(oximodule, SIGNAL(updatePlethy(QByteArray)), this, SLOT(on_updatePlethy(QByteArray)));
    ui->liveConnectLabel->setText(tr("Live Oximetery Mode"));


    liveView->setEmptyText(tr("Starting up..."));
    updateStatus(tr("If you can still read this after a few seconds, cancel and try again"));
    ui->progressBar->hide();
    liveView->update();
    oximodule->Open("live");
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

    importMode = IM_LIVE;

}

void OximeterImport::finishedRecording()
{
    updateTimer.stop();
    oximodule->closeDevice();
    disconnect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateLiveDisplay()));
    disconnect(ui->stopButton, SIGNAL(clicked()), this, SLOT(finishedRecording()));

    ui->stopButton->setVisible(false);
    ui->liveConnectLabel->setText(tr("Live Import Stopped"));
    liveView->setEmptyText(tr("Live Oximetery Stopped"));
    updateStatus(tr("Live Oximetery import has been stopped"));

    disconnect(oximodule, SIGNAL(updatePlethy(QByteArray)), this, SLOT(on_updatePlethy(QByteArray)));

    ui->syncButton->setVisible(true);

    plethyGraph->SetMinX(start_ti);
    liveView->SetXBounds(start_ti, ti, 0, true);

    plethyGraph->setBlockZoom(false);
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
    if (ui->radioSyncCPAP->isChecked()) {
        Day * day = p_profile->GetGoodDay(date, MT_CPAP);

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
            ui->sessbarLabel->setText(tr("%1 session(s) on %2, starting at %3").arg(day->size()).arg(time.date().toString(Qt::SystemLocaleLongDate)).arg(time.time().toString("hh:mm:ssap")));
            sessbar->setSelected(0);
            ui->dateTimeEdit->setDateTime(time);
        } else {
            ui->sessbarLabel->setText(tr("No CPAP data available on %1").arg(date.toString(Qt::SystemLocaleLongDate)));
            ui->dateTimeEdit->setDateTime(QDateTime(date,oximodule->startTime().time()));
        }

        sessbar->update();
    } else if (ui->radioSyncOximeter) {
        ui->sessbarLabel->setText(tr("%1").arg(date.toString(Qt::SystemLocaleLongDate)));
        ui->dateTimeEdit->setDateTime(QDateTime(date, ui->dateTimeEdit->dateTime().time()));
    }
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
    on_calendarWidget_clicked(oximodule->startTime().date());

    ui->syncCPAPGroup->setVisible(true);

}

void OximeterImport::on_radioSyncOximeter_clicked()
{
    ui->syncCPAPGroup->setVisible(false);
    if (oximodule && oximodule->isStartTimeValid()) {
        ui->calendarWidget->setSelectedDate(oximodule->startTime().date());
        ui->dateTimeEdit->setDateTime(oximodule->startTime());
    }
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

    int size = oximodule->oxirec->size();

    if (size > 0) {
        int i = oximodule->startTime().secsTo(QDateTime::currentDateTime());

        int seconds = i % 60;
        int minutes = (i / 60) % 60;
        int hours = i / 3600;

        size--;

        bool datagood = (*(oximodule->oxirec))[size].pulse > 0;


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
        pulse = (*(oximodule->oxirec))[size].pulse;
        spo2 = (*(oximodule->oxirec))[size].spo2;
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
        oximodule->trashRecords();
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

void OximeterImport::on_skipWelcomeCheckbox_clicked(bool checked)
{
    p_profile->oxi->setSkipOxiIntroScreen(checked);
}

void OximeterImport::on_informationButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->welcomePage);
    ui->nextButton->setVisible(true);
    ui->informationButton->setVisible(false);
}

void OximeterImport::on_syncButton_clicked()
{
    Q_ASSERT(oximodule != nullptr);

    ui->stackedWidget->setCurrentWidget(ui->syncPage);

    ui->syncButton->setVisible(false);
    ui->saveButton->setVisible(true);

    QDate first = p_profile->FirstDay();
    QDate last = p_profile->LastDay();

    QDate oxidate = oximodule->startTime().date();


    if ((oxidate >= first) && (oxidate <= last)) {
        // TODO: think this through better.. do I need to pick the day before?
        ui->calendarWidget->setMinimumDate(first);
        ui->calendarWidget->setMaximumDate(last);
        ui->calendarWidget->setCurrentPage(oxidate.year(), oxidate.month());
        ui->calendarWidget->setSelectedDate(oxidate);
        on_calendarWidget_clicked(oximodule->startTime().date());
    } else {
        // No CPAP sessions to sync it to.. kill the CPAP sync option
        on_calendarWidget_clicked(last);
        ui->calendarWidget->setCurrentPage(oxidate.year(), oxidate.month());
        ui->calendarWidget->setSelectedDate(oxidate);
    }

    ui->radioSyncOximeter->setChecked(true);
    on_radioSyncOximeter_clicked();

    if (importMode == IM_LIVE) {
        // Live Recording
        ui->labelSyncOximeter->setText(tr("I want to use the time my computer recorded for this live oximetry session."));
    } else if (!oximodule->isStartTimeValid()) {
        // Oximeter doesn't provide a clock
        ui->labelSyncOximeter->setText(tr("I need to set the time manually, because my oximeter doesn't have an internal clock."));
    }
}

void OximeterImport::on_saveButton_clicked()
{
    if (!oximodule) return;

    QVector<OxiRecord> * oxirec = nullptr;

    if (!oximodule->oxisessions.contains(oximodule->startTime())) {
        QMessageBox::warning(this, STR_MessageBox_Error, tr("Something went wrong getting session data"), QMessageBox::Ok);
        reject();
        return;
    }
    oxirec = oximodule->oxisessions[oximodule->startTime()];
    if (oxirec->size() < 10)
        return;


    // this can move to SerialOximeter class process function...
    MachineInfo info = oximodule->newInfo();
    Machine * mach = oximodule->CreateMachine(info);
    SessionID sid = ui->dateTimeEdit->dateTime().toUTC().toTime_t();
    quint64 start = quint64(sid) * 1000L;


    if (!session) {
        session = new Session(mach, sid);
        session->really_set_first(start);
    } else {
        // Live recording...
        if (dummyday) {
            dummyday->removeSession(session);
            delete dummyday;
            dummyday = nullptr;
        }
        session->SetSessionID(sid);
        session->really_set_first(start);
        ELplethy->setFirst(start);
        session->setFirst(OXI_Plethy, start);
        quint64 duration = start + (ELplethy->count() * ELplethy->rate());
        quint64 end = start + duration;
        session->setLast(OXI_Plethy, end);

        session->count(OXI_Plethy);
        session->Min(OXI_Plethy);
        session->Max(OXI_Plethy);
    }
    EventList * ELpulse = nullptr;
    EventList * ELspo2 = nullptr;

    quint16 lastpulse = 0;
    quint16 lastspo2 = 0;
    quint16 lastgoodpulse = 0;
    quint16 lastgoodspo2 = 0;

    quint64 ti = start;


    qint64 step = (importMode == IM_LIVE) ? oximodule->liveResolution() : oximodule->importResolution();
    int size = oxirec->size();

    for (int i=1; i < size; ++i) {
        OxiRecord * rec = &(*oxirec)[i];

        if (rec->pulse > 0) {
            if (lastpulse == 0) {
                ELpulse = session->AddEventList(OXI_Pulse, EVL_Event);
            }
            if (lastpulse != rec->pulse) {
                if (lastpulse > 0) {
                    ELpulse->AddEvent(ti, lastpulse);
                }
                ELpulse->AddEvent(ti, rec->pulse);
            }
            lastgoodpulse = rec->pulse;
        } else {
            // end section properly
            if (lastgoodpulse > 0) {
                ELpulse->AddEvent(ti, lastpulse);
                session->setLast(OXI_Pulse, ti);
                lastgoodpulse = 0;
            }
        }

        lastpulse = rec->pulse;

        if (rec->spo2 > 0) {
            if (lastspo2 == 0) {
                ELspo2 = session->AddEventList(OXI_SPO2, EVL_Event);
            }
            if (lastspo2 != rec->spo2) {
                if (lastspo2 > 0) {
                    ELspo2->AddEvent(ti, lastspo2);
                }
                ELspo2->AddEvent(ti, rec->spo2);
            }
            lastgoodspo2 = rec->spo2;
        } else {
            // end section properly
            if (lastgoodspo2 > 0) {
                ELspo2->AddEvent(ti, lastspo2);
                session->setLast(OXI_SPO2, ti);
                lastgoodspo2 = 0;
            }
        }
        lastspo2 = rec->spo2;

        ti += step;
    }
    ti -= step;
    if (lastpulse > 0) {
        ELpulse->AddEvent(ti, lastpulse);
        session->setLast(OXI_Pulse, ti);
    }

    if (lastspo2 > 0) {
        ELspo2->AddEvent(ti, lastspo2);
        session->setLast(OXI_SPO2, ti);
    }


    calcSPO2Drop(session);
    calcPulseChange(session);

    session->first(OXI_Pulse);
    session->first(OXI_SPO2);
    session->last(OXI_Pulse);
    session->last(OXI_SPO2);

    session->first(OXI_PulseChange);
    session->first(OXI_SPO2Drop);
    session->last(OXI_PulseChange);
    session->last(OXI_SPO2Drop);

    session->cph(OXI_PulseChange);
    session->sph(OXI_PulseChange);

    session->cph(OXI_SPO2Drop);
    session->sph(OXI_SPO2Drop);

    session->count(OXI_Pulse);
    session->count(OXI_SPO2);
    session->count(OXI_PulseChange);
    session->count(OXI_SPO2Drop);
    session->Min(OXI_Pulse);
    session->Max(OXI_Pulse);
    session->Min(OXI_SPO2);
    session->Max(OXI_SPO2);

    session->really_set_last(ti);
    session->SetChanged(true);

    mach->AddSession(session);
    mach->Save();

    mainwin->getDaily()->LoadDate(mainwin->getDaily()->getDate());
    mainwin->getOverview()->ReloadGraphs();

    ELplethy = nullptr;
    session = nullptr;

    oximodule->trashRecords();
    accept();
}

void OximeterImport::chooseSession()
{
    ui->stackedWidget->setCurrentWidget(ui->chooseSessionPage);
    ui->syncButton->setVisible(false);
    ui->chooseSessionButton->setVisible(true);
    QMap<QDateTime, QVector<OxiRecord> *>::iterator it;

    ui->tableOxiSessions->clearContents();
    int row = 0;
    QTableWidgetItem * item;
    QVector<OxiRecord> * oxirec;

    ui->tableOxiSessions->setRowCount(oximodule->oxisessions.size());
    ui->tableOxiSessions->setSelectionBehavior(QAbstractItemView::SelectRows);

    for (it = oximodule->oxisessions.begin(); it != oximodule->oxisessions.end(); ++it) {
        const QDateTime & key = it.key();
        oxirec = it.value();
        item = new QTableWidgetItem(key.toString(Qt::ISODate));
        ui->tableOxiSessions->setItem(row, 0, item);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        item = new QTableWidgetItem(QString(). sprintf("%lli", oxirec->size() * oximodule->importResolution() / 1000L));
        ui->tableOxiSessions->setItem(row, 1, item);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        item = new QTableWidgetItem(tr("CMS50 Session %1").arg(row+1, 0));
        ui->tableOxiSessions->setItem(row, 2, item);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);

        row++;
    }

    ui->tableOxiSessions->selectRow(0);
}

void OximeterImport::on_chooseSessionButton_clicked()
{
    ui->chooseSessionButton->setVisible(false);

    QTableWidgetItem * item = ui->tableOxiSessions->item(ui->tableOxiSessions->currentRow(),0);

    QDateTime datetime = QDateTime::fromString(item->text(), Qt::ISODate);
    oximodule->setStartTime(datetime);

    on_syncButton_clicked();
}

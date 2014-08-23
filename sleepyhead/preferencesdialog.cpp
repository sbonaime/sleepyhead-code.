/* SleepyHead Preferences Dialog Implementation
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#include <QLabel>
#include <QColorDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QProcess>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTextStream>
#include <QCalendarWidget>
#include <QMenuBar>

#include "preferencesdialog.h"
#include "common_gui.h"

#include <Graphs/gGraphView.h>
#include <mainwindow.h>
#include "ui_preferencesdialog.h"
#include "SleepLib/machine_common.h"

extern QFont *defaultfont;
extern QFont *mediumfont;
extern QFont *bigfont;
extern MainWindow *mainwin;

typedef QMessageBox::StandardButton StandardButton;
typedef QMessageBox::StandardButtons StandardButtons;

MaskProfile masks[] = {
    {Mask_Unknown, QObject::tr("Unspecified"), {{4, 25}, {8, 25}, {12, 25}, {16, 25}, {20, 25}}},
    {Mask_NasalPillows, QObject::tr("Nasal Pillows"), {{4, 20}, {8, 29}, {12, 37}, {16, 43}, {20, 49}}},
    {Mask_Hybrid, QObject::tr("Hybrid F/F Mask"), {{4, 20}, {8, 29}, {12, 37}, {16, 43}, {20, 49}}},
    {Mask_StandardNasal, QObject::tr("Nasal Interface"), {{4, 20}, {8, 29}, {12, 37}, {16, 43}, {20, 49}}},
    {Mask_FullFace, QObject::tr("Full-Face Mask"), {{4, 20}, {8, 29}, {12, 37}, {16, 43}, {20, 49}}},
};
const int num_masks = sizeof(masks) / sizeof(MaskProfile);

PreferencesDialog::PreferencesDialog(QWidget *parent, Profile *_profile) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    profile(_profile)
{
    ui->setupUi(this);
    ui->leakProfile->setRowCount(5);
    ui->leakProfile->setColumnCount(2);
    ui->leakProfile->horizontalHeader()->setStretchLastSection(true);
    ui->leakProfile->setColumnWidth(0, 100);
    ui->maskTypeCombo->clear();

    //ui->customEventGroupbox->setEnabled(false);

    QString masktype = tr("Nasal Pillows");

    //masktype=PROFILEMaskType
    for (int i = 0; i < num_masks; i++) {
        ui->maskTypeCombo->addItem(masks[i].name);

        /*if (masktype==masks[i].name) {
            ui->maskTypeCombo->setCurrentIndex(i);
            on_maskTypeCombo_activated(i);
        }*/
    }

//#ifdef LOCK_RESMED_SESSIONS
//    QList<Machine *> machines = p_profile->GetMachines(MT_CPAP);
//    for (QList<Machine *>::iterator it = machines.begin(); it != machines.end(); ++it) {
//        const QString & mclass=(*it)->loaderName();
//        if (mclass == STR_MACH_ResMed) {
//            ui->combineSlider->setEnabled(false);
//            ui->IgnoreSlider->setEnabled(false);
//            ui->timeEdit->setEnabled(false);
//            break;
//        }
//    }
//#endif

    QLocale locale = QLocale::system();
    QString shortformat = locale.dateFormat(QLocale::ShortFormat);

    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy", "yyyy");
    }

    ui->startedUsingMask->setDisplayFormat(shortformat);
    Qt::DayOfWeek dow = firstDayOfWeekFromLocale();

    ui->startedUsingMask->calendarWidget()->setFirstDayOfWeek(dow);

    //ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->colourTab));

    // Stop both calendar drop downs highlighting weekends in red
    QTextCharFormat format = ui->startedUsingMask->calendarWidget()->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->startedUsingMask->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->startedUsingMask->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);

    //ui->leakProfile->setColumnWidth(1,ui->leakProfile->width()/2);

    Q_ASSERT(profile != nullptr);
    ui->tabWidget->setCurrentIndex(0);

    //i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    //ui->timeZoneCombo->setCurrentIndex(i);

    ui->showLeakRedline->setChecked(profile->cpap->showLeakRedline());
    ui->leakRedlineSpinbox->setValue(profile->cpap->leakRedline());

    ui->oxiDesaturationThreshold->setValue(schema::channel[OXI_SPO2].lowerThreshold());
    ui->flagPulseAbove->setValue(schema::channel[OXI_Pulse].upperThreshold());
    ui->flagPulseBelow->setValue(schema::channel[OXI_Pulse].lowerThreshold());

    ui->spo2Drop->setValue(profile->oxi->spO2DropPercentage());
    ui->spo2DropTime->setValue(profile->oxi->spO2DropDuration());
    ui->pulseChange->setValue(profile->oxi->pulseChangeBPM());
    ui->pulseChangeTime->setValue(profile->oxi->pulseChangeDuration());
    ui->oxiDiscardThreshold->setValue(profile->oxi->oxiDiscardThreshold());
    ui->AddRERAtoAHI->setChecked(profile->general->calculateRDI());
    ui->automaticImport->setChecked(profile->cpap->autoImport());

    ui->timeEdit->setTime(profile->session->daySplitTime());
    int val = profile->session->combineCloseSessions();
    ui->combineSlider->setValue(val);

    if (val > 0) {
        ui->combineLCD->display(val);
    } else { ui->combineLCD->display(STR_GEN_Off); }

    val = profile->session->ignoreShortSessions();
    ui->IgnoreSlider->setValue(val);

    if (val > 0) {
        ui->IgnoreLCD->display(val);
    } else { ui->IgnoreLCD->display(STR_GEN_Off); }

    ui->LockSummarySessionSplitting->setChecked(profile->session->lockSummarySessions());

    ui->applicationFont->setCurrentFont(QApplication::font());
    //ui->applicationFont->setFont(QApplication::font());
    ui->applicationFontSize->setValue(QApplication::font().pointSize());
    ui->applicationFontBold->setChecked(QApplication::font().weight() == QFont::Bold);
    ui->applicationFontItalic->setChecked(QApplication::font().italic());

    ui->graphFont->setCurrentFont(*defaultfont);
    //ui->graphFont->setFont(*defaultfont);
    ui->graphFontSize->setValue(defaultfont->pointSize());
    ui->graphFontBold->setChecked(defaultfont->weight() == QFont::Bold);
    ui->graphFontItalic->setChecked(defaultfont->italic());

    ui->titleFont->setCurrentFont(*mediumfont);
    //ui->titleFont->setFont(*mediumfont);
    ui->titleFontSize->setValue(mediumfont->pointSize());
    ui->titleFontBold->setChecked(mediumfont->weight() == QFont::Bold);
    ui->titleFontItalic->setChecked(mediumfont->italic());

    ui->bigFont->setCurrentFont(*bigfont);
    //ui->bigFont->setFont(*bigfont);
    ui->bigFontSize->setValue(bigfont->pointSize());
    ui->bigFontBold->setChecked(bigfont->weight() == QFont::Bold);
    ui->bigFontItalic->setChecked(bigfont->italic());

    ui->lineThicknessSlider->setValue(profile->appearance->lineThickness()*2);

    ui->startedUsingMask->setDate(profile->cpap->maskStartDate());

    ui->leakModeCombo->setCurrentIndex(profile->cpap->leakMode());

    int mt = (int)profile->cpap->maskType();
    ui->maskTypeCombo->setCurrentIndex(mt);
    on_maskTypeCombo_activated(mt);

    ui->resyncMachineDetectedEvents->setChecked(profile->cpap->resyncFromUserFlagging());

    ui->maskDescription->setText(profile->cpap->maskDescription());
    ui->useAntiAliasing->setChecked(profile->appearance->antiAliasing());
    ui->usePixmapCaching->setChecked(profile->appearance->usePixmapCaching());
    ui->useSquareWavePlots->setChecked(profile->appearance->squareWavePlots());
    ui->enableGraphSnapshots->setChecked(profile->appearance->graphSnapshots());
    ui->graphTooltips->setChecked(profile->appearance->graphTooltips());
    ui->allowYAxisScaling->setChecked(profile->appearance->allowYAxisScaling());

    ui->skipLoginScreen->setChecked(PREF[STR_GEN_SkipLogin].toBool());
    ui->allowEarlyUpdates->setChecked(PREF[STR_PREF_AllowEarlyUpdates].toBool());

    ui->clockDrift->setValue(profile->cpap->clockDrift());

    ui->skipEmptyDays->setChecked(profile->general->skipEmptyDays());
    ui->showUnknownFlags->setChecked(profile->general->showUnknownFlags());
    ui->enableMultithreading->setChecked(profile->session->multithreading());
    ui->cacheSessionData->setChecked(profile->session->cacheSessions());
    ui->animationsAndTransitionsCheckbox->setChecked(profile->appearance->animations());
    ui->complianceGroupbox->setChecked(profile->cpap->showComplianceInfo());
    ui->complianceHours->setValue(profile->cpap->complianceHours());

    ui->prefCalcMiddle->setCurrentIndex(profile->general->prefCalcMiddle());
    ui->prefCalcMax->setCurrentIndex(profile->general->prefCalcMax());
    float f = profile->general->prefCalcPercentile();
    ui->prefCalcPercentile->setValue(f);

    ui->tooltipTimeoutSlider->setValue(profile->general->tooltipTimeout() / 50);
    ui->tooltipMS->display(profile->general->tooltipTimeout());

    ui->scrollDampeningSlider->setValue(profile->general->scrollDampening() / 10);

    if (profile->general->scrollDampening() > 0) {
        ui->scrollDampDisplay->display(profile->general->scrollDampening());
    } else { ui->scrollDampDisplay->display(STR_TR_Off); }

    bool bcd = profile->session->backupCardData();
    ui->createSDBackups->setChecked(bcd);
    ui->compressSDBackups->setEnabled(bcd);
    ui->compressSDBackups->setChecked(profile->session->compressBackupData());
    ui->compressSessionData->setChecked(profile->session->compressSessionData());
    ui->ignoreOlderSessionsCheck->setChecked(profile->session->ignoreOlderSessions());
    ui->ignoreOlderSessionsDate->setDate(profile->session->ignoreOlderSessionsDate().date());

    ui->graphHeight->setValue(profile->appearance->graphHeight());

    ui->automaticallyCheckUpdates->setChecked(PREF[STR_GEN_UpdatesAutoCheck].toBool());

    ui->updateCheckEvery->setValue(PREF[STR_GEN_UpdateCheckFrequency].toInt());

    if (PREF.contains(STR_GEN_UpdatesLastChecked)) {
        RefreshLastChecked();
    } else { ui->updateLastChecked->setText("Never"); }


    ui->overlayFlagsCombo->setCurrentIndex(profile->appearance->overlayType());
    ui->overviewLinecharts->setCurrentIndex(profile->appearance->overviewLinechartMode());

    ui->oximetrySync->setChecked(profile->oxi->syncOximetry());
    ui->oximetrySync->setVisible(false);
    int ot = ui->oximetryType->findText(profile->oxi->oximeterType(), Qt::MatchExactly);

    if (ot < 0) { ot = 0; }

    ui->oximetryType->setCurrentIndex(ot);

    ui->ahiGraphWindowSize->setEnabled(false);
    ui->ahiGraphWindowSize->setValue(profile->cpap->AHIWindow());
    ui->ahiGraphZeroReset->setChecked(profile->cpap->AHIReset());

    ui->customEventGroupbox->setChecked(profile->cpap->userEventFlagging());
    ui->apneaDuration->setValue(profile->cpap->userEventDuration());
    ui->apneaFlowRestriction->setValue(profile->cpap->userFlowRestriction());
    ui->apneaDuration2->setValue(profile->cpap->userEventDuration2());
    ui->apneaFlowRestriction2->setValue(profile->cpap->userFlowRestriction2());
    ui->userEventDuplicates->setChecked(profile->cpap->userEventDuplicates());
    ui->userEventDuplicates->setVisible(false);

    ui->showUserFlagsInPie->setChecked(profile->cpap->userEventPieChart());

    /*    QLocale locale=QLocale::system();
        QString shortformat=locale.dateFormat(QLocale::ShortFormat);
        if (!shortformat.toLower().contains("yyyy")) {
            shortformat.replace("yy","yyyy");
        }*/

    graphFilterModel = new MySortFilterProxyModel(this);
    graphModel = new QStandardItemModel(this);
    graphFilterModel->setSourceModel(graphModel);
    graphFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    graphFilterModel->setFilterKeyColumn(0);
    ui->graphView->setModel(graphFilterModel);

    resetGraphModel();

    chanFilterModel = new MySortFilterProxyModel(this);
    chanModel = new QStandardItemModel(this);
    chanFilterModel->setSourceModel(chanModel);
    chanFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    chanFilterModel->setFilterKeyColumn(0);
    ui->chanView->setModel(chanFilterModel);

    InitChanInfo();

}

void PreferencesDialog::InitChanInfo()
{
    QHash<schema::ChanType, int> toprows;

    chanModel->clear();
    toplevel.clear();
    toprows.clear();

    QStandardItem *hdr = nullptr;

    toplevel[schema::SPAN] = hdr = new QStandardItem(tr("Span Events"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::FLAG] = hdr = new QStandardItem(tr("Flags"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::MINOR_FLAG] = hdr = new QStandardItem(tr("Minor Flags"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::WAVEFORM] = hdr = new QStandardItem(tr("Waveforms"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::DATA] = hdr = new QStandardItem(tr("Data Channels"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::SETTING] = hdr = new QStandardItem(tr("Settings Channels"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    toplevel[schema::UNKNOWN] = hdr = new QStandardItem(tr("Unknown Channels"));
    hdr->setEditable(false);
    chanModel->appendRow(hdr);

    ui->chanView->setAlternatingRowColors(true);

    // ui->graphView->setFirstColumnSpanned(0,daily->index(),true); // Crashes on windows.. Why do I need this again?
    chanModel->setColumnCount(4);
    QStringList headers;
    headers.append(tr("Name"));
    headers.append(tr("Color"));
    headers.append(tr("Label"));
    headers.append(tr("Details"));
//    headers.append(tr("ID"));
    chanModel->setHorizontalHeaderLabels(headers);
    ui->chanView->setColumnWidth(0, 200);
    ui->chanView->setColumnWidth(1, 50);
    ui->chanView->setColumnWidth(2, 100);
    ui->chanView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->chanView->setSelectionBehavior(QAbstractItemView::SelectItems);

    QHash<QString, schema::Channel *>::iterator ci;

    int row = 0;
    for (ci = schema::channel.names.begin(); ci != schema::channel.names.end(); ci++) {
        schema::Channel * chan = ci.value();

        QList<QStandardItem *> items;
        QStandardItem *it = new QStandardItem(chan->fullname());
        it->setCheckable(true);
        it->setCheckState(chan->enabled() ? Qt::Checked : Qt::Unchecked);
        it->setEditable(true);
        it->setData(chan->id(), Qt::UserRole);
        items.push_back(it);


        it = new QStandardItem();
        it->setBackground(QBrush(chan->defaultColor()));
        it->setEditable(false);
        it->setData(chan->defaultColor().rgba(), Qt::UserRole);
        it->setSelectable(false);
        items.push_back(it);

        it = new QStandardItem(chan->label());
        it->setEditable(true);
        items.push_back(it);

        it = new QStandardItem(chan->description());
        it->setEditable(true);
        items.push_back(it);

//        it = new QStandardItem(QString().number(chan->id(),16));
//        it->setEditable(false);
//        items.push_back(it);

        row = toprows[chan->type()]++;
        toplevel[chan->type()]->insertRow(row, items);
    }
    ui->chanView->expandAll();
}

PreferencesDialog::~PreferencesDialog()
{
    disconnect(graphModel, SIGNAL(itemChanged(QStandardItem *)), this,
               SLOT(graphModel_changed(QStandardItem *)));
    delete ui;
}

bool PreferencesDialog::Save()
{
    bool recalc_events = false;
    bool needs_restart = false;

    if (ui->ahiGraphZeroReset->isChecked() != profile->cpap->AHIReset()) { recalc_events = true; }

    if (ui->useSquareWavePlots->isChecked() != profile->appearance->squareWavePlots()) {
        needs_restart = true;
    }

    if ((profile->session->daySplitTime() != ui->timeEdit->time()) ||
            (profile->session->combineCloseSessions() != ui->combineSlider->value()) ||
            (profile->session->ignoreShortSessions() != ui->IgnoreSlider->value())) {
        needs_restart = true;
    }

    if (profile->session->lockSummarySessions() != ui->LockSummarySessionSplitting->isChecked()) {
        needs_restart = true;
    }

    if (profile->cpap->userEventPieChart() != ui->showUserFlagsInPie->isChecked()) {
        // lazy.. fix me
        needs_restart = true;
    }

    if (profile->general->calculateRDI() != ui->AddRERAtoAHI->isChecked()) {
        //recalc_events=true;
        needs_restart = true;
    }

    if ((profile->general->prefCalcMiddle() != ui->prefCalcMiddle->currentIndex())
            || (profile->general->prefCalcMax() != ui->prefCalcMax->currentIndex())
            || (profile->general->prefCalcPercentile() != ui->prefCalcPercentile->value())) {
        needs_restart = true;
    }

    if (profile->cpap->leakRedline() != ui->leakRedlineSpinbox->value()) {
        recalc_events = true;
    }


    if (profile->cpap->userEventFlagging() &&
            (profile->cpap->userEventDuration() != ui->apneaDuration->value() ||
             profile->cpap->userEventDuration2() != ui->apneaDuration2->value() ||
             profile->cpap->userEventDuplicates() != ui->userEventDuplicates->isChecked() ||
             profile->cpap->userFlowRestriction2() != ui->apneaFlowRestriction2->value() ||
             profile->cpap->userFlowRestriction() != ui->apneaFlowRestriction->value())) {
        recalc_events = true;
    }

    // Restart if turning user event flagging on/off
    if (profile->cpap->userEventFlagging() != ui->customEventGroupbox->isChecked()) {
        // if (profile->cpap->userEventFlagging()) {
        // Don't bother recalculating, just switch off
        needs_restart = true;
        //} else
        recalc_events = true;
    }

    if (profile->session->compressSessionData() != ui->compressSessionData->isChecked()) {
        recalc_events = true;
    }

    if (recalc_events) {
        if (p_profile->countDays(MT_CPAP, p_profile->FirstDay(), p_profile->LastDay()) > 0) {
            if (QMessageBox::question(this, tr("Data Reindex Required"),
                                      tr("A data reindexing proceedure is required to apply these changes. This operation may take a couple of minutes to complete.\n\nAre you sure you want to make these changes?"),
                                      QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                return false;
            }
        } else { recalc_events = false; }
    } else if (needs_restart) {
        if (QMessageBox::question(this, tr("Restart Required"),
                                  tr("One or more of the changes you have made will require this application to be restarted,\nin order for these changes to come into effect.\n\nWould you like do this now?"),
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            return false;
        }
    }

    schema::channel[OXI_SPO2].setLowerThreshold(ui->oxiDesaturationThreshold->value());
    schema::channel[OXI_Pulse].setLowerThreshold(ui->flagPulseBelow->value());
    schema::channel[OXI_Pulse].setUpperThreshold(ui->flagPulseAbove->value());


    profile->cpap->setUserEventPieChart(ui->showUserFlagsInPie->isChecked());
    profile->session->setLockSummarySessions(ui->LockSummarySessionSplitting->isChecked());

    profile->appearance->setAllowYAxisScaling(ui->allowYAxisScaling->isChecked());
    profile->appearance->setGraphTooltips(ui->graphTooltips->isChecked());

    profile->appearance->setAntiAliasing(ui->useAntiAliasing->isChecked());
    profile->appearance->setUsePixmapCaching(ui->usePixmapCaching->isChecked());
    profile->appearance->setSquareWavePlots(ui->useSquareWavePlots->isChecked());
    profile->appearance->setGraphSnapshots(ui->enableGraphSnapshots->isChecked());
    profile->appearance->setLineThickness(float(ui->lineThicknessSlider->value()) / 2.0);

    profile->general->setSkipEmptyDays(ui->skipEmptyDays->isChecked());

    profile->general->setTooltipTimeout(ui->tooltipTimeoutSlider->value() * 50);
    profile->general->setScrollDampening(ui->scrollDampeningSlider->value() * 10);

    profile->general->setShowUnknownFlags(ui->showUnknownFlags->isChecked());
    profile->session->setMultithreading(ui->enableMultithreading->isChecked());
    profile->session->setCacheSessions(ui->cacheSessionData->isChecked());
    profile->cpap->setMaskDescription(ui->maskDescription->text());
    profile->appearance->setAnimations(ui->animationsAndTransitionsCheckbox->isChecked());

    profile->cpap->setShowLeakRedline(ui->showLeakRedline->isChecked());
    profile->cpap->setLeakRedline(ui->leakRedlineSpinbox->value());

    profile->cpap->setShowComplianceInfo(ui->complianceGroupbox->isChecked());
    profile->cpap->setComplianceHours(ui->complianceHours->value());

    profile->cpap->setMaskStartDate(ui->startedUsingMask->date());
    profile->appearance->setGraphHeight(ui->graphHeight->value());

    profile->general->setPrefCalcMiddle(ui->prefCalcMiddle->currentIndex());
    profile->general->setPrefCalcMax(ui->prefCalcMax->currentIndex());
    profile->general->setPrefCalcPercentile(ui->prefCalcPercentile->value());

    profile->general->setCalculateRDI(ui->AddRERAtoAHI->isChecked());
    profile->session->setBackupCardData(ui->createSDBackups->isChecked());
    profile->session->setCompressBackupData(ui->compressSDBackups->isChecked());
    profile->session->setCompressSessionData(ui->compressSessionData->isChecked());

    profile->session->setCombineCloseSessions(ui->combineSlider->value());
    profile->session->setIgnoreShortSessions(ui->IgnoreSlider->value());
    profile->session->setDaySplitTime(ui->timeEdit->time());
    profile->session->setIgnoreOlderSessions(ui->ignoreOlderSessionsCheck->isChecked());
    profile->session->setIgnoreOlderSessionsDate(ui->ignoreOlderSessionsDate->date());

    profile->cpap->setClockDrift(ui->clockDrift->value());

    profile->appearance->setOverlayType((OverlayDisplayType)ui->overlayFlagsCombo->currentIndex());
    profile->appearance->setOverviewLinechartMode((OverviewLinechartModes)
            ui->overviewLinecharts->currentIndex());

    profile->cpap->setLeakMode(ui->leakModeCombo->currentIndex());
    profile->cpap->setMaskType((MaskType)ui->maskTypeCombo->currentIndex());

    profile->oxi->setSyncOximetry(ui->oximetrySync->isChecked());
    int oxigrp = ui->oximetrySync->isChecked() ? 0 : 1;
    gGraphView *gv = mainwin->getDaily()->graphView();
    gGraph *g = gv->findGraph(schema::channel[OXI_Pulse].code());

    if (g) {
        g->setGroup(oxigrp);
    }

    g = gv->findGraph(schema::channel[OXI_SPO2].code());

    if (g) {
        g->setGroup(oxigrp);
    }

    g = gv->findGraph(schema::channel[OXI_Plethy].code());

    if (g) {
        g->setGroup(oxigrp);
    }

    profile->oxi->setOximeterType(ui->oximetryType->currentText());

    profile->oxi->setSpO2DropPercentage(ui->spo2Drop->value());
    profile->oxi->setSpO2DropDuration(ui->spo2DropTime->value());
    profile->oxi->setPulseChangeBPM(ui->pulseChange->value());
    profile->oxi->setPulseChangeDuration(ui->pulseChangeTime->value());
    profile->oxi->setOxiDiscardThreshold(ui->oxiDiscardThreshold->value());

    profile->cpap->setAHIWindow(ui->ahiGraphWindowSize->value());
    profile->cpap->setAHIReset(ui->ahiGraphZeroReset->isChecked());

    profile->cpap->setAutoImport(ui->automaticImport->isChecked());

    profile->cpap->setUserEventFlagging(ui->customEventGroupbox->isChecked());

    profile->cpap->setResyncFromUserFlagging(ui->resyncMachineDetectedEvents->isChecked());
    profile->cpap->setUserEventDuration(ui->apneaDuration->value());
    profile->cpap->setUserFlowRestriction(ui->apneaFlowRestriction->value());
    profile->cpap->setUserEventDuration2(ui->apneaDuration2->value());
    profile->cpap->setUserFlowRestriction2(ui->apneaFlowRestriction2->value());

    profile->cpap->setUserEventDuplicates(ui->userEventDuplicates->isChecked());

    PREF[STR_GEN_SkipLogin] = ui->skipLoginScreen->isChecked();

    PREF[STR_GEN_UpdatesAutoCheck] = ui->automaticallyCheckUpdates->isChecked();
    PREF[STR_GEN_UpdateCheckFrequency] = ui->updateCheckEvery->value();
    PREF[STR_PREF_AllowEarlyUpdates] = ui->allowEarlyUpdates->isChecked();

    PREF["Fonts_Application_Name"] = ui->applicationFont->currentText();
    PREF["Fonts_Application_Size"] = ui->applicationFontSize->value();
    PREF["Fonts_Application_Bold"] = ui->applicationFontBold->isChecked();
    PREF["Fonts_Application_Italic"] = ui->applicationFontItalic->isChecked();


    PREF["Fonts_Graph_Name"] = ui->graphFont->currentText();
    PREF["Fonts_Graph_Size"] = ui->graphFontSize->value();
    PREF["Fonts_Graph_Bold"] = ui->graphFontBold->isChecked();
    PREF["Fonts_Graph_Italic"] = ui->graphFontItalic->isChecked();

    PREF["Fonts_Title_Name"] = ui->titleFont->currentText();
    PREF["Fonts_Title_Size"] = ui->titleFontSize->value();
    PREF["Fonts_Title_Bold"] = ui->titleFontBold->isChecked();
    PREF["Fonts_Title_Italic"] = ui->titleFontItalic->isChecked();

    PREF["Fonts_Big_Name"] = ui->bigFont->currentText();
    PREF["Fonts_Big_Size"] = ui->bigFontSize->value();
    PREF["Fonts_Big_Bold"] = ui->bigFontBold->isChecked();
    PREF["Fonts_Big_Italic"] = ui->bigFontItalic->isChecked();

    QFont font = ui->applicationFont->currentFont();
    font.setPointSize(ui->applicationFontSize->value());
    font.setWeight(ui->applicationFontBold->isChecked() ? QFont::Bold : QFont::Normal);
    font.setItalic(ui->applicationFontItalic->isChecked());
    QApplication::setFont(font);
    mainwin->menuBar()->setFont(font);

    *defaultfont = ui->graphFont->currentFont();
    defaultfont->setPointSize(ui->graphFontSize->value());
    defaultfont->setWeight(ui->graphFontBold->isChecked() ? QFont::Bold : QFont::Normal);
    defaultfont->setItalic(ui->graphFontItalic->isChecked());

    *mediumfont = ui->titleFont->currentFont();
    mediumfont->setPointSize(ui->titleFontSize->value());
    mediumfont->setWeight(ui->titleFontBold->isChecked() ? QFont::Bold : QFont::Normal);
    mediumfont->setItalic(ui->titleFontItalic->isChecked());

    *bigfont = ui->bigFont->currentFont();
    bigfont->setPointSize(ui->bigFontSize->value());
    bigfont->setWeight(ui->bigFontBold->isChecked() ? QFont::Bold : QFont::Normal);
    bigfont->setItalic(ui->bigFontItalic->isChecked());


    int toprows = chanModel->rowCount();

    bool ok;
    for (int i=0; i < toprows; i++) {
        QStandardItem * topitem = chanModel->item(i,0);

        if (!topitem) continue;
        int rows = topitem->rowCount();
        for (int j=0; j< rows; ++j) {
            QStandardItem * item = topitem->child(j, 0);
            if (!item) continue;

            ChannelID id = item->data(Qt::UserRole).toUInt(&ok);
            schema::Channel & chan = schema::channel[id];
            if (chan.isNull()) continue;
            chan.setEnabled(item->checkState() == Qt::Checked ? true : false);
            chan.setFullname(item->text());
            chan.setDefaultColor(QColor(topitem->child(j,1)->data(Qt::UserRole).toUInt()));
            chan.setLabel(topitem->child(j,2)->text());
            chan.setDescription(topitem->child(j,3)->text());
        }
    }

    //qDebug() << "TODO: Save channels.xml to update channel data";

    PREF.Save();
    p_profile->Save();

    if (recalc_events) {
        // send a signal instead?
        mainwin->reprocessEvents(needs_restart);
    } else if (needs_restart) {
        p_profile->removeLock();
        mainwin->RestartApplication();
    } else {
        mainwin->getDaily()->LoadDate(mainwin->getDaily()->getDate());
        // Save early.. just in case..
        mainwin->getDaily()->graphView()->SaveSettings("Daily");
        mainwin->getOverview()->graphView()->SaveSettings("Overview");
    }

    return true;
}

void PreferencesDialog::on_combineSlider_valueChanged(int position)
{
    if (position > 0) {
        ui->combineLCD->display(position);
    } else { ui->combineLCD->display(STR_GEN_Off); }
}

void PreferencesDialog::on_IgnoreSlider_valueChanged(int position)
{
    if (position > 0) {
        ui->IgnoreLCD->display(position);
    } else { ui->IgnoreLCD->display(STR_GEN_Off); }
}

#include "mainwindow.h"
extern MainWindow *mainwin;
void PreferencesDialog::RefreshLastChecked()
{
    ui->updateLastChecked->setText(PREF[STR_GEN_UpdatesLastChecked].toDateTime().toString(
                                       Qt::SystemLocaleLongDate));
}

void PreferencesDialog::on_checkForUpdatesButton_clicked()
{
    mainwin->CheckForUpdates();
}

void PreferencesDialog::on_graphView_activated(const QModelIndex &index)
{
    QString a = index.data().toString();
    qDebug() << "Could do something here with" << a;
}

void PreferencesDialog::on_graphFilter_textChanged(const QString &arg1)
{
    graphFilterModel->setFilterFixedString(arg1);
}


MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{

}

bool MySortFilterProxyModel::filterAcceptsRow(int source_row,
        const QModelIndex &source_parent) const
{
    if (source_parent == qobject_cast<QStandardItemModel *>
            (sourceModel())->invisibleRootItem()->index()) {
        // always accept children of rootitem, since we want to filter their children
        return true;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

void PreferencesDialog::graphModel_changed(QStandardItem *item)
{
    QModelIndex index = item->index();

    gGraphView *gv = nullptr;
    bool ok;

    const QModelIndex &row = index.sibling(index.row(), 0);
    bool checked = row.data(Qt::CheckStateRole) != 0;
    //QString name=row.data().toString();

    int group = row.data(Qt::UserRole + 1).toInt();
    int id = row.data(Qt::UserRole + 2).toInt();

    switch (group) {
    case 0:
        gv = mainwin->getDaily()->graphView();
        break;

    case 1:
        gv = mainwin->getOverview()->graphView();
        break;

    default:
        ;
    }

    if (!gv) {
        return;
    }

    gGraph *graph = (*gv)[id];

    if (!graph) {
        return;
    }

    if (graph->visible() != checked) {
        graph->setVisible(checked);
    }

    EventDataType val;

    if (index.column() == 1) {
        val = index.data().toDouble(&ok);

        if (!ok) {
            graphModel->setData(index, QString::number(graph->rec_miny, 'f', 1));
            ui->graphView->update();
        }  else {
            //if ((val < graph->rec_maxy) || (val==0)) {
            graph->setRecMinY(val);
            /*} else {
                graphModel->setData(index,QString::number(graph->rec_miny,'f',1));
                ui->graphView->update();
             } */
        }
    } else if (index.column() == 2) {
        val = index.data().toDouble(&ok);

        if (!ok) {
            graphModel->setData(index, QString::number(graph->rec_maxy, 'f', 1));
            ui->graphView->update();
        }  else {
            //if ((val > graph->rec_miny) || (val==0)) {
            graph->setRecMaxY(val);
            /*} else {
                graphModel->setData(index,QString::number(graph->rec_maxy,'f',1));
                ui->graphView->update();
            }*/
        }

    }

    gv->updateScale();
    //    qDebug() << name << checked;
}

void PreferencesDialog::resetGraphModel()
{

    graphModel->clear();
    QStandardItem *daily = new QStandardItem(tr("Daily Graphs"));
    QStandardItem *overview = new QStandardItem(tr("Overview Graphs"));
    daily->setEditable(false);
    overview->setEditable(false);

    graphModel->appendRow(daily);
    graphModel->appendRow(overview);

    ui->graphView->setAlternatingRowColors(true);

    // ui->graphView->setFirstColumnSpanned(0,daily->index(),true); // Crashes on windows.. Why do I need this again?
    graphModel->setColumnCount(3);
    QStringList headers;
    headers.append(tr("Graph"));
    headers.append(STR_TR_Min);
    headers.append(STR_TR_Max);
    graphModel->setHorizontalHeaderLabels(headers);
    ui->graphView->setColumnWidth(0, 250);
    ui->graphView->setColumnWidth(1, 50);
    ui->graphView->setColumnWidth(2, 50);

    gGraphView *gv = mainwin->getDaily()->graphView();

    for (int i = 0; i < gv->size(); i++) {
        QList<QStandardItem *> items;
        QString title = (*gv)[i]->title();
        QStandardItem *it = new QStandardItem(title);
        it->setCheckable(true);
        it->setCheckState((*gv)[i]->visible() ? Qt::Checked : Qt::Unchecked);
        it->setEditable(false);
        it->setData(0, Qt::UserRole + 1);
        it->setData(i, Qt::UserRole + 2);
        items.push_back(it);

        if (title != STR_TR_EventFlags) {

            it = new QStandardItem(QString::number((*gv)[i]->rec_miny, 'f', 1));
            it->setEditable(true);
            items.push_back(it);

            it = new QStandardItem(QString::number((*gv)[i]->rec_maxy, 'f', 1));
            it->setEditable(true);
            items.push_back(it);
        } else {
            it = new QStandardItem(tr("N/A")); // not applicable.
            it->setEditable(false);
            items.push_back(it);
            items.push_back(it->clone());
        }

        daily->insertRow(i, items);
    }

    gv = mainwin->getOverview()->graphView();

    for (int i = 0; i < gv->size(); i++) {
        QList<QStandardItem *> items;
        QStandardItem *it = new QStandardItem((*gv)[i]->title());
        it->setCheckable(true);
        it->setCheckState((*gv)[i]->visible() ? Qt::Checked : Qt::Unchecked);
        it->setEditable(false);
        items.push_back(it);
        it->setData(1, Qt::UserRole + 1);
        it->setData(i, Qt::UserRole + 2);

        it = new QStandardItem(QString::number((*gv)[i]->rec_miny, 'f', 1));
        it->setEditable(true);
        items.push_back(it);

        it = new QStandardItem(QString::number((*gv)[i]->rec_maxy, 'f', 1));
        it->setEditable(true);
        items.push_back(it);

        overview->insertRow(i, items);
    }

    connect(graphModel, SIGNAL(itemChanged(QStandardItem *)), this,
            SLOT(graphModel_changed(QStandardItem *)));

    ui->graphView->expandAll();

}

void PreferencesDialog::on_resetGraphButton_clicked()
{
    QString title = tr("Confirmation");
    QString text  = tr("Are you sure you want to reset your graph preferences to the defaults?");
    StandardButtons buttons = QMessageBox::Yes | QMessageBox::No;
    StandardButton  defaultButton = QMessageBox::No;

    // Display confirmation dialog.
    StandardButton choice = QMessageBox::question(this, title, text, buttons, defaultButton);

    if (choice == QMessageBox::No) {
        return;
    }

    gGraphView *views[3] = {0};
    views[0] = mainwin->getDaily()->graphView();
    views[1] = mainwin->getOverview()->graphView();

    // Iterate over all graph containers.
    for (unsigned j = 0; j < 3; j++) {
        gGraphView *view = views[j];

        if (!view) {
            continue;
        }

        // Iterate over all contained graphs.
        for (int i = 0; i < view->size(); i++) {
            gGraph *g = (*view)[i];
            g->setRecMaxY(0); // FIXME: should be g->reset(), but need other patches to land.
            g->setRecMinY(0);
            g->setVisible(true);
        }

        view->updateScale();
    }

    resetGraphModel();
    ui->graphView->update();
}

/*void PreferencesDialog::on_genOpWidget_itemActivated(QListWidgetItem *item)
{
    item->setCheckState(item->checkState()==Qt::Checked ? Qt::Unchecked : Qt::Checked);
}  */

void PreferencesDialog::on_maskTypeCombo_activated(int index)
{
    if (index < num_masks) {
        QTableWidgetItem *item;

        for (int i = 0; i < 5; i++) {
            MaskProfile &mp = masks[index];

            item = ui->leakProfile->item(i, 0);
            QString val = QString::number(mp.pflow[i][0], 'f', 2);

            if (!item) {
                item = new QTableWidgetItem(val);
                ui->leakProfile->setItem(i, 0, item);
            } else { item->setText(val); }

            val = QString::number(mp.pflow[i][1], 'f', 2);
            item = ui->leakProfile->item(i, 1);

            if (!item) {
                item = new QTableWidgetItem(val);
                ui->leakProfile->setItem(i, 1, item);
            } else { item->setText(val); }
        }
    }
}

void PreferencesDialog::on_createSDBackups_toggled(bool checked)
{
    if (profile->session->backupCardData() && !checked) {
        QList<Machine *> mach = p_profile->GetMachines(MT_CPAP);
        bool haveS9 = false;

        for (int i = 0; i < mach.size(); i++) {
            if (mach[i]->loaderName() == STR_MACH_ResMed) {
                haveS9 = true;
                break;
            }
        }

        if (haveS9
                && QMessageBox::question(this, tr("This may not be a good idea"),
                                         tr("ResMed S9 machines routinely delete certain data from your SD card older than 7 and 30 days (depending on resolution).")
                                         + " " + tr("If you ever need to reimport this data again (whether in SleepyHead or ResScan) this data won't come back.")
                                         + " " + tr("If you need to conserve disk space, please remember to carry out manual backups.") +
                                         " " + tr("Are you sure you want to disable these backups?"), QMessageBox::Yes,
                                         QMessageBox::No) == QMessageBox::No) {
            ui->createSDBackups->setChecked(true);
            return;
        }
    }

    if (!checked) { ui->compressSDBackups->setChecked(false); }

    ui->compressSDBackups->setEnabled(checked);
}

void PreferencesDialog::on_okButton_clicked()
{
    if (Save()) {
        accept();
    }
}

void PreferencesDialog::on_scrollDampeningSlider_valueChanged(int value)
{
    if (value > 0) {
        ui->scrollDampDisplay->display(value * 10);
    } else { ui->scrollDampDisplay->display(STR_TR_Off); }
}

void PreferencesDialog::on_tooltipTimeoutSlider_valueChanged(int value)
{
    ui->tooltipMS->display(value * 50);
}

void PreferencesDialog::on_resetChannelDefaults_clicked()
{
    if (QMessageBox::question(this, STR_MessageBox_Warning, QObject::tr("Are you sure you want to reset all your channel colors and settings to defaults?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        schema::resetChannels();
        InitChanInfo();
    }
}

void PreferencesDialog::on_createSDBackups_clicked(bool checked)
{
    if (!checked && p_profile->session->backupCardData()) {
        if (QMessageBox::question(this, STR_MessageBox_Warning, tr("Switching off automatic backups is not a good idea, because SleepyHead needs these to rebuild the database if errors are found.")+"\n\n"+
                              tr("Are you really sure you want to do this?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        } else {
            ui->createSDBackups->setChecked(true);
        }
    }
}

void PreferencesDialog::on_channelSearch_textChanged(const QString &arg1)
{
    chanFilterModel->setFilterFixedString(arg1);
}

void PreferencesDialog::on_chanView_doubleClicked(const QModelIndex &index)
{
    if (index.column() == 1) {
        QColorDialog a;

        quint32 color = index.data(Qt::UserRole).toUInt();

        a.setCurrentColor(QColor((QRgb)color));

        if (a.exec() == QColorDialog::Accepted) {
            quint32 cv = a.currentColor().rgba();

            chanFilterModel->setData(index, cv, Qt::UserRole);
            chanFilterModel->setData(index, a.currentColor(), Qt::BackgroundRole);

        }

    }
}

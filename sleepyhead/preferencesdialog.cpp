/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * Preferences Dialog
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

    QLocale locale = QLocale::system();
    QString shortformat = locale.dateFormat(QLocale::ShortFormat);

    if (!shortformat.toLower().contains("yyyy")) {
        shortformat.replace("yy", "yyyy");
    }

    ui->startedUsingMask->setDisplayFormat(shortformat);
    Qt::DayOfWeek dow = firstDayOfWeekFromLocale();

    ui->startedUsingMask->calendarWidget()->setFirstDayOfWeek(dow);

    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->colourTab));

    // Stop both calendar drop downs highlighting weekends in red
    QTextCharFormat format = ui->startedUsingMask->calendarWidget()->weekdayTextFormat(Qt::Saturday);
    format.setForeground(QBrush(Qt::black, Qt::SolidPattern));
    ui->startedUsingMask->calendarWidget()->setWeekdayTextFormat(Qt::Saturday, format);
    ui->startedUsingMask->calendarWidget()->setWeekdayTextFormat(Qt::Sunday, format);

    //ui->leakProfile->setColumnWidth(1,ui->leakProfile->width()/2);

    {
        QString filename = PROFILE.Get("{DataFolder}/ImportLocations.txt");
        QFile file(filename);
        file.open(QFile::ReadOnly);
        QTextStream textStream(&file);

        while (1) {
            QString line = textStream.readLine();

            if (line.isNull()) {
                break;
            } else if (line.isEmpty()) {
                continue;
            } else {
                importLocations.append(line);
            }
        };

        file.close();
    }
    importModel = new QStringListModel(importLocations, this);
    ui->importListWidget->setModel(importModel);
    //ui->tabWidget->removeTab(3);

    Q_ASSERT(profile != nullptr);
    ui->tabWidget->setCurrentIndex(0);

    //i=ui->timeZoneCombo->findText((*profile)["TimeZone"].toString());
    //ui->timeZoneCombo->setCurrentIndex(i);

    ui->spo2Drop->setValue(profile->oxi->spO2DropPercentage());
    ui->spo2DropTime->setValue(profile->oxi->spO2DropDuration());
    ui->pulseChange->setValue(profile->oxi->pulseChangeBPM());
    ui->pulseChangeTime->setValue(profile->oxi->pulseChangeDuration());
    ui->oxiDiscardThreshold->setValue(profile->oxi->oxiDiscardThreshold());
    ui->AddRERAtoAHI->setChecked(profile->general->calculateRDI());

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

    ui->startedUsingMask->setDate(profile->cpap->maskStartDate());

    ui->leakModeCombo->setCurrentIndex(profile->cpap->leakMode());

    int mt = (int)profile->cpap->maskType();
    ui->maskTypeCombo->setCurrentIndex(mt);
    on_maskTypeCombo_activated(mt);


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

    ui->graphHeight->setValue(profile->appearance->graphHeight());

    ui->automaticallyCheckUpdates->setChecked(PREF[STR_GEN_UpdatesAutoCheck].toBool());

    ui->updateCheckEvery->setValue(PREF[STR_GEN_UpdateCheckFrequency].toInt());

    if (PREF.contains(STR_GEN_UpdatesLastChecked)) {
        RefreshLastChecked();
    } else { ui->updateLastChecked->setText("Never"); }


    ui->overlayFlagsCombo->setCurrentIndex(profile->appearance->overlayType());
    ui->overviewLinecharts->setCurrentIndex(profile->appearance->overviewLinechartMode());

    ui->oximetryGroupBox->setChecked(profile->oxi->oximetryEnabled());
    ui->oximetrySync->setChecked(profile->oxi->syncOximetry());
    int ot = ui->oximetryType->findText(profile->oxi->oximeterType(), Qt::MatchExactly);

    if (ot < 0) { ot = 0; }

    ui->oximetryType->setCurrentIndex(ot);

    ui->ahiGraphWindowSize->setEnabled(false);
    ui->ahiGraphWindowSize->setValue(profile->cpap->AHIWindow());
    ui->ahiGraphZeroReset->setChecked(profile->cpap->AHIReset());

    ui->customEventGroupbox->setChecked(profile->cpap->userEventFlagging());
    ui->apneaDuration->setValue(profile->cpap->userEventDuration());
    ui->apneaFlowRestriction->setValue(profile->cpap->userFlowRestriction());
    ui->userEventDuplicates->setChecked(profile->cpap->userEventDuplicates());
    ui->userEventDuplicates->setVisible(false);

    ui->eventTable->setColumnWidth(0, 40);
    ui->eventTable->setColumnWidth(1, 55);
    ui->eventTable->setColumnHidden(3, true);
    int row = 0;
    QTableWidgetItem *item;
    QHash<QString, schema::Channel *>::iterator ci;

    for (ci = schema::channel.names.begin(); ci != schema::channel.names.end(); ci++) {
        if (ci.value()->type() == schema::DATA) {
            ui->eventTable->insertRow(row);
            int id = ci.value()->id();
            ui->eventTable->setItem(row, 3, new QTableWidgetItem(QString::number(id)));
            item = new QTableWidgetItem(ci.value()->description());
            ui->eventTable->setItem(row, 2, item);
            QCheckBox *c = new QCheckBox(ui->eventTable);
            c->setChecked(true);
            QLabel *pb = new QLabel(ui->eventTable);
            pb->setText("foo");
            ui->eventTable->setCellWidget(row, 0, c);
            ui->eventTable->setCellWidget(row, 1, pb);


            QColor a = ci.value()->defaultColor(); //(rand() % 255, rand() % 255, rand() % 255, 255);
            QPalette p(a, a, a, a, a, a, a);

            pb->setPalette(p);
            pb->setAutoFillBackground(true);
            pb->setBackgroundRole(QPalette::Background);
            row++;
        }
    }

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
    //    tree->sortByColumn(0,Qt::AscendingOrder);
}


PreferencesDialog::~PreferencesDialog()
{
    disconnect(graphModel, SIGNAL(itemChanged(QStandardItem *)), this,
               SLOT(graphModel_changed(QStandardItem *)));
    delete ui;
}

void PreferencesDialog::on_eventTable_doubleClicked(const QModelIndex &index)
{
    int row = index.row();
    int col = index.column();
    bool ok;
    int id = ui->eventTable->item(row, 3)->text().toInt(&ok);

    if (col == 1) {
        QWidget *w = ui->eventTable->cellWidget(row, col);
        QColorDialog a;
        QColor color = w->palette().background().color();
        a.setCurrentColor(color);

        if (a.exec() == QColorDialog::Accepted) {
            QColor c = a.currentColor();
            QPalette p(c, c, c, c, c, c, c);
            w->setPalette(p);
            m_new_colors[id] = c;
            //qDebug() << "Color accepted" << col << id;
        }
    }
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

    if (profile->general->calculateRDI() != ui->AddRERAtoAHI->isChecked()) {
        //recalc_events=true;
        needs_restart = true;
    }

    if ((profile->general->prefCalcMiddle() != ui->prefCalcMiddle->currentIndex())
            || (profile->general->prefCalcMax() != ui->prefCalcMax->currentIndex())
            || (profile->general->prefCalcPercentile() != ui->prefCalcPercentile->value())) {
        needs_restart = true;
    }

    if (profile->cpap->userEventFlagging() &&
            (profile->cpap->userEventDuration() != ui->apneaDuration->value() ||
             profile->cpap->userEventDuplicates() != ui->userEventDuplicates->isChecked() ||
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
        if (PROFILE.countDays(MT_CPAP, PROFILE.FirstDay(), PROFILE.LastDay()) > 0) {
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

    profile->appearance->setAllowYAxisScaling(ui->allowYAxisScaling->isChecked());
    profile->appearance->setGraphTooltips(ui->graphTooltips->isChecked());

    profile->appearance->setAntiAliasing(ui->useAntiAliasing->isChecked());
    profile->appearance->setUsePixmapCaching(ui->usePixmapCaching->isChecked());
    profile->appearance->setSquareWavePlots(ui->useSquareWavePlots->isChecked());
    profile->appearance->setGraphSnapshots(ui->enableGraphSnapshots->isChecked());
    profile->general->setSkipEmptyDays(ui->skipEmptyDays->isChecked());

    profile->general->setTooltipTimeout(ui->tooltipTimeoutSlider->value() * 50);
    profile->general->setScrollDampening(ui->scrollDampeningSlider->value() * 10);

    profile->session->setMultithreading(ui->enableMultithreading->isChecked());
    profile->session->setCacheSessions(ui->cacheSessionData->isChecked());
    profile->cpap->setMaskDescription(ui->maskDescription->text());
    profile->appearance->setAnimations(ui->animationsAndTransitionsCheckbox->isChecked());

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

    profile->cpap->setClockDrift(ui->clockDrift->value());

    profile->appearance->setOverlayType((OverlayDisplayType)ui->overlayFlagsCombo->currentIndex());
    profile->appearance->setOverviewLinechartMode((OverviewLinechartModes)
            ui->overviewLinecharts->currentIndex());

    profile->cpap->setLeakMode(ui->leakModeCombo->currentIndex());
    profile->cpap->setMaskType((MaskType)ui->maskTypeCombo->currentIndex());

    profile->oxi->setOximetryEnabled(ui->oximetryGroupBox->isChecked());
    profile->oxi->setSyncOximetry(ui->oximetrySync->isChecked());
    int oxigrp = ui->oximetrySync->isChecked() ? 0 : 1;
    gGraphView *gv = mainwin->getDaily()->graphView();
    gGraph *g = gv->findGraph(STR_TR_PulseRate);

    if (g) {
        g->setGroup(oxigrp);
    }

    g = gv->findGraph(STR_TR_SpO2);

    if (g) {
        g->setGroup(oxigrp);
    }

    g = gv->findGraph(STR_TR_Plethy);

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


    profile->cpap->setUserEventFlagging(ui->customEventGroupbox->isChecked());

    profile->cpap->setUserEventDuration(ui->apneaDuration->value());
    profile->cpap->setUserFlowRestriction(ui->apneaFlowRestriction->value());
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

    // Process color changes
    for (QHash<int, QColor>::iterator i = m_new_colors.begin(); i != m_new_colors.end(); i++) {
        schema::Channel &chan = schema::channel[i.key()];

        if (!chan.isNull()) {
            qDebug() << "TODO: Change" << chan.code() << "color to" << i.value();
            chan.setDefaultColor(i.value());
        }
    }

    //qDebug() << "TODO: Save channels.xml to update channel data";

    {
        QString filename = PROFILE.Get("{DataFolder}/ImportLocations.txt");
        QFile file(filename);
        file.open(QFile::WriteOnly);
        QTextStream ts(&file);

        for (int i = 0; i < importLocations.size(); i++) {
            ts << importLocations[i] << endl;
            //file.write(importLocations[i].toUtf8());
        }

        file.close();
    }

    //PROFILE.Save();
    PREF.Save();

    if (recalc_events) {
        // send a signal instead?
        mainwin->reprocessEvents(needs_restart);
    } else if (needs_restart) {
        mainwin->RestartApplication();
    } else {
        mainwin->getDaily()->LoadDate(mainwin->getDaily()->getDate());
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

void PreferencesDialog::on_addImportLocation_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Add this Location to the Import List"),
                  "", QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        if (!importLocations.contains(dir)) {
            importLocations.append(dir);
            importModel->setStringList(importLocations);
        }
    }
}

void PreferencesDialog::on_removeImportLocation_clicked()
{
    if (ui->importListWidget->currentIndex().isValid()) {
        QString dir = ui->importListWidget->currentIndex().data().toString();
        importModel->removeRow(ui->importListWidget->currentIndex().row());
        importLocations.removeAll(dir);
    }
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

    case 2:
        gv = mainwin->getOximetry()->graphView();
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

    if (mainwin->getOximetry()) {
        QStandardItem *oximetry = new QStandardItem(tr("Oximetry Graphs"));
        graphModel->appendRow(oximetry);
        oximetry->setEditable(false);
        gv = mainwin->getOximetry()->graphView();

        for (int i = 0; i < gv->size(); i++) {
            QList<QStandardItem *> items;
            QStandardItem *it = new QStandardItem((*gv)[i]->title());
            it->setCheckable(true);
            it->setCheckState((*gv)[i]->visible() ? Qt::Checked : Qt::Unchecked);
            it->setEditable(false);
            it->setData(2, Qt::UserRole + 1);
            it->setData(i, Qt::UserRole + 2);
            items.push_back(it);

            it = new QStandardItem(QString::number((*gv)[i]->rec_miny, 'f', 1));
            it->setEditable(true);
            items.push_back(it);

            it = new QStandardItem(QString::number((*gv)[i]->rec_maxy, 'f', 1));
            it->setEditable(true);
            items.push_back(it);

            oximetry->insertRow(i, items);
        }
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

    gGraphView *views[3];
    views[0] = mainwin->getDaily()->graphView();
    views[1] = mainwin->getOverview()->graphView();
    views[2] = mainwin->getOximetry()->graphView();

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
        QList<Machine *> mach = PROFILE.GetMachines(MT_CPAP);
        bool haveS9 = false;

        for (int i = 0; i < mach.size(); i++) {
            if (mach[i]->GetClass() == STR_MACH_ResMed) {
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
